/* HD movies for the 64-bit Linux host (linux64_host.c): the game image asks through the hd_movie bridge entry with
 * the file (and, after an @, the second to start at), its two subtitle files and the font; the system's ffmpeg decodes
 * video and audio through pipes, as the 32-bit build does (port/src/port_hdmovie.c), and the subtitles are drawn with
 * stb_truetype. Without ffmpeg the cartridge's movie plays. A, B or START skip it. */
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

static void Quote(const char* s, char* out, size_t cap) {
    size_t n = 0;
    out[n++] = '\'';
    for (; *s && n + 5 < cap; s++) {
        if (*s == '\'') { memcpy(out + n, "'\\''", 4); n += 4; }
        else out[n++] = *s;
    }
    out[n++] = '\'';
    out[n] = 0;
}

static int H_hd_movie(const char* movie, const char* subs1, const char* subs2, const char* fontPath) {
    char path[1024], quoted[2200], cmd[2600], line[256];
    int start = 0, w = 0, h = 0, played = 0, skip = 0;
    double fps = 30;
    snprintf(path, sizeof(path), "%s", movie);
    char* at = strrchr(path, '@');
    if (at && strchr(at, '/') == NULL) { start = atoi(at + 1); *at = 0; }
    if (access(path, R_OK) != 0 || system("command -v ffmpeg >/dev/null 2>&1 && command -v ffprobe >/dev/null 2>&1"))
        return 0; /* nothing to play, or no ffmpeg: the cartridge's movie */
    Quote(path, quoted, sizeof(quoted));
    snprintf(cmd, sizeof(cmd), "ffprobe -v error -select_streams v:0 -show_entries stream=width,height,r_frame_rate "
             "-of csv=p=0 %s", quoted);
    FILE* probe = popen(cmd, "r");
    if (probe) {
        unsigned num = 30, den = 1;
        if (fgets(line, sizeof(line), probe) && sscanf(line, "%d,%d,%u/%u", &w, &h, &num, &den) >= 2 && den)
            fps = (double)num / den;
        pclose(probe);
    }
    if (w <= 0 || h <= 0)
        return 0;
    snprintf(cmd, sizeof(cmd), "ffmpeg -v error -nostdin -ss %d -i %s -an -sn -f rawvideo -pix_fmt rgba - 2>/dev/null",
             start, quoted);
    FILE* video = popen(cmd, "r");
    snprintf(cmd, sizeof(cmd), "ffmpeg -v error -nostdin -ss %d -i %s -vn -sn -ac 2 -ar 48000 -f s16le - 2>/dev/null",
             start, quoted);
    FILE* sound = popen(cmd, "r");
    SDL_Texture* tex = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, w, h);
    size_t frameBytes = (size_t)w * h * 4;
    uint8_t* frame = malloc(frameBytes);
    SDL_AudioSpec spec = { SDL_AUDIO_S16, 2, 48000 };
    SDL_AudioStream* audio = sound ? SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL) : NULL;
    stbtt_fontinfo font;
    unsigned char* fontData = NULL;
    int haveFont = 0;
    if (fontPath && (subs1 || subs2)) {
        size_t size = 0;
        fontData = SDL_LoadFile(fontPath, &size);
        haveFont = fontData && stbtt_InitFont(&font, fontData, stbtt_GetFontOffsetForIndex(fontData, 0));
    }
    LoadMovieSubs(subs1, subs2);
    if (audio)
        SDL_ResumeAudioStreamDevice(audio);
    if (sAudio)
        SDL_PauseAudioStreamDevice(sAudio); /* the game's own sound waits */
    Uint64 began = SDL_GetTicksNS();
    long long frames = 0, audioBytes = 0;
    int shown = -1, subW = 0, subH = 0;
    SDL_Texture* subTex = NULL;
    static uint8_t abuf[19200];
    while (video && tex && frame && !sQuit && !skip) {
        /* audio: about half a second queued ahead */
        while (audio && SDL_GetAudioStreamQueued(audio) < 48000 * 4 / 2) {
            size_t n = fread(abuf, 1, sizeof(abuf), sound);
            if (!n) break;
            SDL_PutAudioStreamData(audio, abuf, (int)n);
            audioBytes += (long long)n;
        }
        if (fread(frame, 1, frameBytes, video) != frameBytes)
            break;
        played = 1;
        SDL_UpdateTexture(tex, NULL, frame, w * 4);
        /* the clock: the audio played, or the wall clock */
        double clock = audio ? (audioBytes - SDL_GetAudioStreamQueued(audio)) / (48000.0 * 4)
                             : (SDL_GetTicksNS() - began) / 1e9;
        double due = frames / fps;
        frames++;
        if (due > clock)
            SDL_DelayNS((Uint64)((due - clock) * 1e9));
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
        SDL_RenderPresent(sRenderer);
        /* A, B or START skip it */
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) sQuit = 1;
            if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat &&
                (e.key.scancode == SDL_SCANCODE_X || e.key.scancode == SDL_SCANCODE_Z || e.key.scancode == SDL_SCANCODE_RETURN))
                skip = 1;
            if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN &&
                (e.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH || e.gbutton.button == SDL_GAMEPAD_BUTTON_EAST ||
                 e.gbutton.button == SDL_GAMEPAD_BUTTON_START))
                skip = 1;
            if (e.type == SDL_EVENT_GAMEPAD_ADDED && !sPad) sPad = SDL_OpenGamepad(e.gdevice.which);
        }
        {
            const char* limit = getenv("KHCOM_HDMOVIE_SECONDS"); /* test harness: stop after that many seconds */
            if (limit && due >= atof(limit)) break;
        }
    }
    if (subTex) SDL_DestroyTexture(subTex);
    if (tex) SDL_DestroyTexture(tex);
    if (audio) SDL_DestroyAudioStream(audio);
    if (video) pclose(video);
    if (sound) pclose(sound);
    free(frame);
    SDL_free(fontData);
    if (sAudio) {
        SDL_ClearAudioStream(sAudio);
        SDL_ResumeAudioStreamDevice(sAudio);
    }
    Log("hd movie: %s from %d s, %lld frames%s", path, start, frames, skip ? " (skipped)" : "");
    return played;
}
