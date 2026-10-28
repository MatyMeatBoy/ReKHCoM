/* HD movies for the 64-bit Linux host (linux64_host.c): the game image asks through the hd_movie bridge entry with
 * the file (and, after an @, the second to start at), its two subtitle files and the font; the system's ffmpeg decodes
 * video and audio through pipes, as the 32-bit build does (port/src/port_hdmovie.c), and the subtitles are drawn with
 * stb_truetype. Without ffmpeg the cartridge's movie plays. START pauses with an explicit skip confirmation. */
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define MSUB_MAX 400
typedef struct { int first, last; char text[300]; } MovieSub;
static MovieSub sMSubs[MSUB_MAX];
static int sMSubCount;

static void LoadMovieSubs(const char* a, const char* b) {
    const char* paths[2] = { a, b };
    char line[1024];
    FILE* f = NULL;
    sMSubCount = 0;
    for (int i = 0; i < 2 && !f; i++)
        if (paths[i] && paths[i][0])
            f = fopen(paths[i], "rb");
    if (!f)
        return;
    while (sMSubCount < MSUB_MAX && fgets(line, sizeof(line), f)) {
        unsigned first, last;
        int used = 0;
        if (sscanf(line, "%u %u %n", &first, &last, &used) < 2 || !used)
            continue;
        MovieSub* m = &sMSubs[sMSubCount++];
        char* w = m->text;
        m->first = (int)first;
        m->last = (int)last;
        for (const char* t = line + used; *t && w < m->text + sizeof(m->text) - 1; t++) {
            if (t[0] == '\\' && t[1] == 'n') { *w++ = '\n'; t++; }
            else if (*t != '\r' && *t != '\n') *w++ = *t;
        }
        *w = 0;
    }
    fclose(f);
}

/* one subtitle as a texture: white letters with a dark edge, centred lines, at the output's scale */
static SDL_Texture* SubTexture(stbtt_fontinfo* font, const char* text, float px, int* tw, int* th) {
    int lines = 1, w = 0, lw = 0, h, x, y, base, ascent, descent, gap;
    float scale = stbtt_ScaleForPixelHeight(font, px);
    stbtt_GetFontVMetrics(font, &ascent, &descent, &gap);
    base = (int)(ascent * scale);
    int lineH = (int)((ascent - descent + gap) * scale);
    /* measure */
    for (const unsigned char* p = (const unsigned char*)text; *p;) {
        unsigned c = *p++;
        if (c >= 0xC0) { int n = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : 1; c &= 0x3F >> n; while (n-- && *p) c = c << 6 | (*p++ & 0x3F); }
        if (c == '\n') { lines++; if (lw > w) w = lw; lw = 0; continue; }
        int adv, lsb;
        stbtt_GetCodepointHMetrics(font, (int)c, &adv, &lsb);
        lw += (int)(adv * scale);
    }
    if (lw > w) w = lw;
    w += 8;
    h = lines * lineH + 8;
    unsigned char* mask = calloc((size_t)w * h, 1);
    uint32_t* rgba = calloc((size_t)w * h, 4);
    if (!mask || !rgba) { free(mask); free(rgba); return NULL; }
    /* draw each line centred */
    const char* p = text;
    for (int line = 0; line < lines; line++) {
        const char* end = strchr(p, '\n');
        if (!end) end = p + strlen(p);
        int width = 0;
        for (const unsigned char* q = (const unsigned char*)p; q < (const unsigned char*)end;) {
            unsigned c = *q++;
            if (c >= 0xC0) { int n = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : 1; c &= 0x3F >> n; while (n-- && q < (const unsigned char*)end) c = c << 6 | (*q++ & 0x3F); }
            int adv, lsb;
            stbtt_GetCodepointHMetrics(font, (int)c, &adv, &lsb);
            width += (int)(adv * scale);
        }
        x = 4 + (w - 8 - width) / 2;
        y = 4 + line * lineH + base;
        for (const unsigned char* q = (const unsigned char*)p; q < (const unsigned char*)end;) {
            unsigned c = *q++;
            if (c >= 0xC0) { int n = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : 1; c &= 0x3F >> n; while (n-- && q < (const unsigned char*)end) c = c << 6 | (*q++ & 0x3F); }
            int adv, lsb, x0, y0, x1, y1;
            stbtt_GetCodepointHMetrics(font, (int)c, &adv, &lsb);
            stbtt_GetCodepointBitmapBox(font, (int)c, scale, scale, &x0, &y0, &x1, &y1);
            if (x + x0 >= 0 && y + y0 >= 0 && x + x1 < w && y + y1 < h)
                stbtt_MakeCodepointBitmap(font, mask + (size_t)(y + y0) * w + x + x0, x1 - x0, y1 - y0, w, scale, scale, (int)c);
            x += (int)(adv * scale);
        }
        p = *end ? end + 1 : end;
    }
    /* the letters white, a 2-pixel dark edge around them */
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            int edge = 0;
            for (int dy = -2; dy <= 2 && !edge; dy++)
                for (int dx = -2; dx <= 2; dx++) {
                    int xx = x + dx, yy = y + dy;
                    if (xx >= 0 && yy >= 0 && xx < w && yy < h && mask[yy * w + xx] > 96) { edge = 1; break; }
                }
            unsigned a = mask[y * w + x];
            uint32_t c = edge ? 0xE0182030u : 0;
            if (a) {
                unsigned ea = edge ? 0xE0 : 0;
                unsigned oa = a + ea * (255 - a) / 255;
                unsigned v = oa ? (255 * a + 0x20 * ea * (255 - a) / 255) / oa : 0;
                c = (oa << 24) | (v << 16) | (v << 8) | v;
            }
            rgba[y * w + x] = c;
        }
    SDL_Texture* t = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, w, h);
    if (t) {
        SDL_UpdateTexture(t, NULL, rgba, w * 4);
        SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    }
    free(mask);
    free(rgba);
    *tw = w;
    *th = h;
    return t;
}

#include "hdmovie_ui.inc"

/* Each decoder is our direct child: no shell, inherited pipe writers or pclose wait. */
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <spawn.h>
#include <sys/wait.h>
typedef struct { int fd; pid_t pid; } MoviePipe;
static MoviePipe MovieSpawn(char* const argv[]) {
    MoviePipe p = { -1, -1 };
    int fds[2];
    posix_spawn_file_actions_t actions;
    if (pipe2(fds, O_CLOEXEC) < 0) return p;
    /* Spawn actions address 0/1/2. A host with closed stdio can otherwise get
       a pipe there, then close the freshly duplicated decoder stdout. */
    for (int i = 0; i < 2; i++) {
        if (fds[i] >= 3) continue;
        int moved = fcntl(fds[i], F_DUPFD_CLOEXEC, 3);
        if (moved < 0) {
            close(fds[0]); close(fds[1]); return p;
        }
        close(fds[i]);
        fds[i] = moved;
    }
    if (posix_spawn_file_actions_init(&actions) != 0) {
        close(fds[0]); close(fds[1]); return p;
    }
    int error = posix_spawn_file_actions_adddup2(&actions, fds[1], STDOUT_FILENO);
    if (!error) error = posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    if (!error) error = posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    if (!error) error = posix_spawn_file_actions_addclose(&actions, fds[0]);
    if (!error) error = posix_spawn_file_actions_addclose(&actions, fds[1]);
    if (!error) error = posix_spawnp(&p.pid, argv[0], &actions, NULL, argv, environ);
    posix_spawn_file_actions_destroy(&actions);
    close(fds[1]);
    if (error) { close(fds[0]); p.pid = -1; return p; }
    p.fd = fds[0];
    if (fcntl(p.fd, F_SETFL, O_NONBLOCK) < 0) {
        close(p.fd); p.fd = -1;
        kill(p.pid, SIGKILL);
        while (waitpid(p.pid, NULL, 0) < 0 && errno == EINTR) {}
        p.pid = -1;
    }
    return p;
}
static void MovieClose(MoviePipe* p) {
    if (p->fd >= 0) close(p->fd);
    p->fd = -1;
    if (p->pid > 0) {
        /* Cancel even a stopped/stalled decoder. Reap only our PID, exactly once. */
        kill(p->pid, SIGKILL);
        while (waitpid(p->pid, NULL, 0) < 0 && errno == EINTR) {}
        p->pid = -1;
    }
}
/* Wake as soon as a decoder writes again. A fixed sleep per empty fragment
 * multiplies scheduler latency across a multi-megabyte raw frame. Bound the
 * wait to1ms so the outer loop still services audio and input during stalls. */
static void MovieWaitReadable(MoviePipe* pipe) {
    struct pollfd ready = { pipe->fd, POLLIN, 0 };
    (void)poll(&ready, 1, 1);
}
/* Positive: bytes; zero: temporarily empty; -1: EOF; -2: read error. */
static ssize_t MovieRead(MoviePipe* p, void* bytes, size_t size) {
    ssize_t n;
    do n = read(p->fd, bytes, size); while (n < 0 && errno == EINTR);
    if (n > 0) return n;
    if (!n) return -1;
    return errno == EAGAIN || errno == EWOULDBLOCK ? 0 : -2;
}
static double MovieClock(int audio, int eof, long long bytes, int queued, double now, double* began) {
    double pcm = (bytes - queued) / 192000.0;
    if (audio && !eof) {
        *began = now - pcm; /* retain the last PCM position when switching clocks */
        return pcm;
    }
    return now - *began;
}
static int MovieEvents(int* pause) {
    int reset = 0;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_RENDER_DEVICE_RESET) { RendererDeviceReset(); reset = 1; }
        if (e.type == SDL_EVENT_QUIT) sQuit = 1;
        if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat &&
            (e.key.key == SDLK_ESCAPE || e.key.key == sMovieStartKeys[0] ||
             (sMovieStartKeys[1] && e.key.key == sMovieStartKeys[1])))
            *pause = 1;
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN &&
            e.gbutton.button == SDL_GAMEPAD_BUTTON_START)
            *pause = 1;
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN || e.type == SDL_EVENT_FINGER_DOWN) *pause = 1;
        if (e.type == SDL_EVENT_GAMEPAD_ADDED && !sPad) sPad = SDL_OpenGamepad(e.gdevice.which);
    }
    return reset;
}

static int H_hd_movie(const char* movie, const char* subs1, const char* subs2, const char* fontPath) {
    const int masterVolume = PortMovieVolumeTake(&sMovieVolume);
    if (!movie) return 0;
    char path[1024], line[256], seek[32];
    int start = 0, w = 0, h = 0, played = 0, skip = 0, pause = 0, rendererLost = 0, pauseTested = 0;
    double fps = 30;
    MovieSettings(subs1);
    snprintf(path, sizeof(path), "%s", movie);
    char* at = strrchr(path, '@');
    if (at && strchr(at, '/') == NULL) { start = atoi(at + 1); *at = 0; }
    if (access(path, R_OK) != 0) return 0;
    char* probeArgs[] = { "ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
        "stream=width,height,r_frame_rate", "-of", "csv=p=0", "-i", path, NULL };
    MoviePipe probe = MovieSpawn(probeArgs);
    size_t lineBytes = 0;
    while (probe.fd >= 0 && !sQuit && !skip && lineBytes < sizeof(line) - 1) {
        if (MovieEvents(&pause)) { MovieClose(&probe); return 0; }
        ssize_t n = MovieRead(&probe, line + lineBytes, sizeof(line) - 1 - lineBytes);
        if (n < 0) break;
        lineBytes += (size_t)n;
        if (memchr(line, '\n', lineBytes)) break;
        if (!n) SDL_Delay(10); /* pump events while metadata is unavailable */
    }
    line[lineBytes] = 0;
    MovieClose(&probe);
    unsigned num = 30, den = 1;
    if (sscanf(line, "%d,%d,%u/%u", &w, &h, &num, &den) >= 2 && num && den)
        fps = (double)num / den;
    if (sQuit || skip) return 1; /* explicit cancellation consumes the scene */
    if (w <= 0 || h <= 0 || (size_t)w > SIZE_MAX / 4 / (size_t)h) return 0;
    snprintf(seek, sizeof(seek), "%d", start);
    char* videoArgs[] = { "ffmpeg", "-v", "error", "-nostdin", "-ss", seek, "-i", path,
        "-an", "-sn", "-f", "rawvideo", "-pix_fmt", "rgba", "-", NULL };
    char* audioArgs[] = { "ffmpeg", "-v", "error", "-nostdin", "-ss", seek, "-i", path,
        "-vn", "-sn", "-ac", "2", "-ar", "48000", "-f", "s16le", "-", NULL };
    MoviePipe video = MovieSpawn(videoArgs), sound = MovieSpawn(audioArgs);
    SDL_Texture* tex = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, w, h);
    size_t frameBytes = (size_t)w * h * 4;
    uint8_t* frame = malloc(frameBytes);
    SDL_AudioSpec spec = { SDL_AUDIO_S16, 2, 48000 };
    SDL_AudioStream* audio = sound.fd >= 0 ? SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL) : NULL;
    if (audio && !SDL_SetAudioStreamGain(audio, PortMovieGain(masterVolume, 1.0f))) {
        SDL_DestroyAudioStream(audio);
        if (tex) SDL_DestroyTexture(tex);
        MovieClose(&video); MovieClose(&sound); free(frame);
        return 0;
    }
    stbtt_fontinfo font;
    unsigned char* fontData = NULL;
    int haveFont = 0;
    if (fontPath) {
        size_t size = 0;
        fontData = SDL_LoadFile(fontPath, &size);
        haveFont = fontData && stbtt_InitFont(&font, fontData, stbtt_GetFontOffsetForIndex(fontData, 0));
    }
    LoadMovieSubs(subs1, subs2);
    if (audio) SDL_ResumeAudioStreamDevice(audio);
    if (sAudio)
        SDL_PauseAudioStreamDevice(sAudio); /* the game's own sound waits */
    double began = SDL_GetTicksNS() / 1e9;
    double fpsEpoch = began, fpsValue = 0;
    int fpsFrames = 0;
    int audioEof = !audio;
    size_t frameUsed = 0;
    long long frames = 0, audioBytes = 0;
    int shown = -1, subW = 0, subH = 0;
    SDL_Texture* subTex = NULL;
    uint8_t abuf[19200];
    size_t audioUsed = 0;
    while (video.fd >= 0 && tex && frame && !sQuit && !skip) {
        if (MovieEvents(&pause)) { played = 0; rendererLost = 1; break; }
        if (sQuit || skip) break;
        /* Nonblocking producer work is bounded; video and events keep progressing without PCM. */
        for (int reads = 0; audio && !audioEof && reads < 8 &&
             SDL_GetAudioStreamQueued(audio) < 48000 * 4 / 2; reads++) {
            ssize_t n = MovieRead(&sound, abuf + audioUsed, sizeof(abuf) - audioUsed);
            if (n < 0) {
                MovieClock(1, 0, audioBytes, SDL_GetAudioStreamQueued(audio), SDL_GetTicksNS() / 1e9, &began);
                audioEof = 1;
                break;
            }
            if (!n) break;
            size_t total = audioUsed + (size_t)n;
            size_t aligned = total & ~(size_t)3; /* stereo s16: preserve a fragmented sample frame */
            if (aligned && !SDL_PutAudioStreamData(audio, abuf, (int)aligned)) {
                MovieClock(1, 0, audioBytes, SDL_GetAudioStreamQueued(audio), SDL_GetTicksNS() / 1e9, &began);
                audioEof = 1;
                break;
            }
            audioBytes += (long long)aligned;
            audioUsed = total - aligned;
            memmove(abuf, abuf + aligned, audioUsed);
        }
        if (frameUsed < frameBytes) {
            ssize_t n = MovieRead(&video, frame + frameUsed, frameBytes - frameUsed);
            if (n < 0) break;
            frameUsed += (size_t)n;
            /* A pipe fragments even an immediately available decoded image.
               Waiting for every positive fragment throttles a 1080p frame by
               hundreds of milliseconds. Yield only when the pipe has no data;
               the outer loop still pumps events and PCM after every fragment. */
            if (frameUsed < frameBytes) { if (!n) MovieWaitReadable(&video); continue; }
        }
        double clock = MovieClock(audio != NULL, audioEof, audioBytes,
            audio ? SDL_GetAudioStreamQueued(audio) : 0, SDL_GetTicksNS() / 1e9, &began);
        double due = frames / fps;
        if (due > clock) { SDL_Delay(1); continue; }
        if (!SDL_UpdateTexture(tex, NULL, frame, w * 4)) { played = 0; break; }
        played = 1;
        frameUsed = 0;
        frames++;
        int ow, oh;
        SDL_GetCurrentRenderOutputSize(sRenderer, &ow, &oh);
        float k = SDL_min(ow / (float)w, oh / (float)h);
        SDL_FRect dst = { (ow - w * k) / 2, (oh - h * k) / 2, w * k, h * k };
        SDL_SetRenderClipRect(sRenderer, NULL);
        SDL_SetRenderDrawColor(sRenderer, 0, 0, 0, 255);
        SDL_RenderClear(sRenderer);
        SDL_RenderTexture(sRenderer, tex, NULL, &dst);
        if (haveFont) {
            int at30 = (int)((start + due) * 30), want = -1;
            for (int i = 0; i < sMSubCount; i++)
                if (at30 >= sMSubs[i].first && at30 < sMSubs[i].last) { want = i; break; }
            if (want != shown) {
                if (subTex) SDL_DestroyTexture(subTex);
                subTex = want >= 0 ? SubTexture(&font, sMSubs[want].text, oh / 18.0f, &subW, &subH) : NULL;
                shown = want;
            }
            if (subTex) {
                SDL_FRect r = { (ow - subW) / 2.0f, dst.y + dst.h - subH - oh / 18.0f, (float)subW, (float)subH };
                SDL_RenderTexture(sRenderer, subTex, NULL, &r);
            }
        }
        {
            static int shot; /* test harness: KHCOM_HDMOVIE_SHOT=<seconds from the file's start> saves hdmovie.bmp */
            const char* at = getenv("KHCOM_HDMOVIE_SHOT");
            if (at && !shot && start + due >= atof(at)) {
                SDL_Surface* s = SDL_RenderReadPixels(sRenderer, NULL);
                char file[1200];
                snprintf(file, sizeof(file), "%s/hdmovie.bmp", sData);
                if (s) { SDL_SaveBMP(s, file); SDL_DestroySurface(s); }
                shot = 1;
            }
        }
        if (sMovieFps) {
            double now = SDL_GetTicksNS() / 1e9;
            fpsFrames++;
            if (now - fpsEpoch >= .5) { fpsValue = fpsFrames / (now - fpsEpoch); fpsFrames = 0; fpsEpoch = now; }
            char text[64]; snprintf(text, sizeof(text), "Video %.1f FPS", fpsValue);
            SDL_SetRenderScale(sRenderer, 2, 2);
            SDL_SetRenderDrawColor(sRenderer, 0, 0, 0, 255); SDL_RenderDebugText(sRenderer, 4, 4, text);
            SDL_SetRenderDrawColor(sRenderer, 255, 255, 255, 255); SDL_RenderDebugText(sRenderer, 3, 3, text);
            SDL_SetRenderScale(sRenderer, 1, 1);
        }
        {
            const char* at = getenv("KHCOM_HDMOVIE_PAUSETEST");
            if (at && !pauseTested && due >= atof(at)) { pause = 1; pauseTested = 1; }
        }
        if (pause) {
            double before = SDL_GetTicksNS() / 1e9;
            skip = MoviePause(&font, haveFont, audio, &rendererLost, &began);
            fpsEpoch += SDL_GetTicksNS() / 1e9 - before;
            pause = 0;
            if (rendererLost) { played = 0; break; }
        } else SDL_RenderPresent(sRenderer);
        {
            const char* limit = getenv("KHCOM_HDMOVIE_SECONDS"); /* test harness: stop after that many seconds */
            if (limit && due >= atof(limit)) break;
        }
    }
    if (subTex) SDL_DestroyTexture(subTex);
    if (tex) SDL_DestroyTexture(tex);
    if (audio) SDL_DestroyAudioStream(audio);
    MovieClose(&video);
    MovieClose(&sound);
    free(frame);
    SDL_free(fontData);
    if (sAudio) {
        H_audio_clear();
        if (!sAudioBridgeControls) SDL_ResumeAudioStreamDevice(sAudio);
    }
    Log("hd movie: %s from %d s, %lld frames%s", path, start, frames, skip ? " (skipped)" : "");
    return rendererLost ? 0 : played || skip || sQuit;
}
