/*
 * HD movies on Android (port/progress/HD_MOD.md): the player behind port_hdmovie.c on 32-bit Android and behind the
 * hd_movie bridge entry of the 64-bit host (port/android64/host/android64_host.c). Same behaviour as the Windows
 * player: the movie plays to the end on the game's renderer, its audio on a stream of its own is the clock, late
 * frames are not uploaded, Start/Escape/Back skip it, and Re:CoM's subtitles are drawn under the
 * picture (white KHMenu letters with a dark outline).
 *
 * Decoding: the NDK's MediaExtractor + MediaCodec (format keys as plain strings: the AMEDIAFORMAT_KEY_ variables
 * live in libmediandk too), the video decoded into an AImageReader (YUV 4:2:0 planes with
 * their own strides: what every decoder can give, hardware ones included) and uploaded as NV12 or planar YUV. Those
 * are loaded with dlopen: AImageReader needs Android 7 (API 24) and the app starts on Android 5; on older systems the
 * cartridge's movie plays.
 */
#include <SDL3/SDL.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <android/native_window.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaExtractor.h>
#include <media/NdkMediaFormat.h>
#include <ft2build.h>
#include FT_FREETYPE_H

void HdMovieLog(const char* fmt, ...); /* the caller's log */

/* ---- the NDK media functions (libmediandk.so) -------------------------------------------------------------------- */

typedef struct AImageReader AImageReader;
typedef struct AImage AImage;
typedef struct { int32_t left, top, right, bottom; } HdCrop; /* AImageCropRect */
enum { HD_YUV_420_888 = 0x23 };                              /* AIMAGE_FORMAT_YUV_420_888 */

static struct {
    AMediaExtractor* (*extractorNew)(void);
    media_status_t (*extractorDelete)(AMediaExtractor*);
    media_status_t (*extractorSetFd)(AMediaExtractor*, int, off64_t, off64_t);
    size_t (*extractorTracks)(AMediaExtractor*);
    AMediaFormat* (*extractorFormat)(AMediaExtractor*, size_t);
    media_status_t (*extractorSelect)(AMediaExtractor*, size_t);
    ssize_t (*extractorRead)(AMediaExtractor*, uint8_t*, size_t);
    int64_t (*extractorTime)(AMediaExtractor*);
    bool (*extractorAdvance)(AMediaExtractor*);
    bool (*formatString)(AMediaFormat*, const char*, const char**);
    bool (*formatInt)(AMediaFormat*, const char*, int32_t*);
    bool (*formatInt64)(AMediaFormat*, const char*, int64_t*);
    media_status_t (*formatDelete)(AMediaFormat*);
    AMediaCodec* (*codecNew)(const char*);
    AMediaCodec* (*codecByName)(const char*);
    media_status_t (*codecConfigure)(AMediaCodec*, const AMediaFormat*, ANativeWindow*, AMediaCrypto*, uint32_t);
    media_status_t (*codecStart)(AMediaCodec*);
    media_status_t (*codecStop)(AMediaCodec*);
    media_status_t (*codecDelete)(AMediaCodec*);
    ssize_t (*codecInput)(AMediaCodec*, int64_t);
    uint8_t* (*codecInputBuffer)(AMediaCodec*, size_t, size_t*);
    media_status_t (*codecQueue)(AMediaCodec*, size_t, off_t, size_t, uint64_t, uint32_t);
    ssize_t (*codecOutput)(AMediaCodec*, AMediaCodecBufferInfo*, int64_t);
    uint8_t* (*codecOutputBuffer)(AMediaCodec*, size_t, size_t*);
    media_status_t (*codecRelease)(AMediaCodec*, size_t, bool);
    AMediaFormat* (*codecOutputFormat)(AMediaCodec*);
    media_status_t (*readerNew)(int32_t, int32_t, int32_t, int32_t, AImageReader**);
    void (*readerDelete)(AImageReader*);
    media_status_t (*readerWindow)(AImageReader*, ANativeWindow**);
    media_status_t (*readerLatest)(AImageReader*, AImage**);
    void (*imageDelete)(AImage*);
    media_status_t (*imageCrop)(const AImage*, HdCrop*);
    media_status_t (*imagePlane)(const AImage*, int, uint8_t**, int*);
    media_status_t (*imageRowStride)(const AImage*, int, int32_t*);
    media_status_t (*imagePixelStride)(const AImage*, int, int32_t*);
} M;

static int LoadMedia(void) {
    static int state; /* 0 not tried, 1 ready, -1 missing */
    void* lib;
    if (state)
        return state > 0;
    state = -1;
    lib = dlopen("libmediandk.so", RTLD_NOW);
    if (!lib)
        return 0;
#define SYM(field, name) \
    if (!(*(void**)&M.field = dlsym(lib, name))) { \
        HdMovieLog("hd movie: no %s in libmediandk", name); \
        return 0; \
    }
    SYM(extractorNew, "AMediaExtractor_new") SYM(extractorDelete, "AMediaExtractor_delete")
    SYM(extractorSetFd, "AMediaExtractor_setDataSourceFd") SYM(extractorTracks, "AMediaExtractor_getTrackCount")
    SYM(extractorFormat, "AMediaExtractor_getTrackFormat") SYM(extractorSelect, "AMediaExtractor_selectTrack")
    SYM(extractorRead, "AMediaExtractor_readSampleData") SYM(extractorTime, "AMediaExtractor_getSampleTime")
    SYM(extractorAdvance, "AMediaExtractor_advance") SYM(formatString, "AMediaFormat_getString")
    SYM(formatInt, "AMediaFormat_getInt32") SYM(formatInt64, "AMediaFormat_getInt64") SYM(formatDelete, "AMediaFormat_delete")
    SYM(codecNew, "AMediaCodec_createDecoderByType") SYM(codecByName, "AMediaCodec_createCodecByName") SYM(codecConfigure, "AMediaCodec_configure")
    SYM(codecStart, "AMediaCodec_start") SYM(codecStop, "AMediaCodec_stop") SYM(codecDelete, "AMediaCodec_delete")
    SYM(codecInput, "AMediaCodec_dequeueInputBuffer") SYM(codecInputBuffer, "AMediaCodec_getInputBuffer")
    SYM(codecQueue, "AMediaCodec_queueInputBuffer") SYM(codecOutput, "AMediaCodec_dequeueOutputBuffer")
    SYM(codecOutputBuffer, "AMediaCodec_getOutputBuffer") SYM(codecRelease, "AMediaCodec_releaseOutputBuffer")
    SYM(codecOutputFormat, "AMediaCodec_getOutputFormat") SYM(readerNew, "AImageReader_new")
    SYM(readerDelete, "AImageReader_delete") SYM(readerWindow, "AImageReader_getWindow")
    SYM(readerLatest, "AImageReader_acquireLatestImage") SYM(imageDelete, "AImage_delete")
    SYM(imageCrop, "AImage_getCropRect") SYM(imagePlane, "AImage_getPlaneData")
    SYM(imageRowStride, "AImage_getPlaneRowStride") SYM(imagePixelStride, "AImage_getPlanePixelStride")
#undef SYM
    state = 1;
    return 1;
}

/* ---- subtitles ------------------------------------------------------------------------------------------------ */

#define SUB_MAX 256
typedef struct {
    int count, shown; /* shown: the line its texture holds, -1 none */
    unsigned short first[SUB_MAX], last[SUB_MAX];
    char* text[SUB_MAX]; /* UTF-8, '\n' between lines */
    SDL_Texture* texture;
    int w, h, height; /* texture size; the output height it was made for */
    FT_Library ft;
    FT_Face face;
    void* font;
} Subs;

/* hd/subs/<movie>.<lang>.txt: "<first frame> <last frame> <text>" lines, "\n" breaking the text */
static int LoadSubs(Subs* s, const char* first, const char* second, const char* fontPath) {
    char line[1024];
    size_t fontSize = 0;
    const char* path = first;
    FILE* f = first ? fopen(first, "rb") : NULL;
    if (!f && second)
        f = fopen(path = second, "rb");
    if (!f)
        return 0;
    while (s->count < SUB_MAX && fgets(line, sizeof(line), f)) {
        unsigned first, last;
        int used = 0;
        char *text, *w;
        if (sscanf(line, "%u %u %n", &first, &last, &used) < 2 || used == 0)
            continue;
        for (text = w = line + used; *text; text++) {
            if (text[0] == '\\' && text[1] == 'n') {
                *w++ = '\n';
                text++;
            } else if (*text != '\r' && *text != '\n') {
                *w++ = *text;
            }
        }
        *w = 0;
        if (!(s->text[s->count] = SDL_strdup(line + used)))
            break;
        s->first[s->count] = (unsigned short)first;
        s->last[s->count] = (unsigned short)last;
        s->count++;
    }
    fclose(f);
    /* the KHMenu face the HD text uses: the data folder's copy, else the APK's */
    s->font = fontPath ? SDL_LoadFile(fontPath, &fontSize) : NULL;
    if (!s->font)
        s->font = SDL_LoadFile("fonts/KHMenu.otf", &fontSize);
    if (!s->font || FT_Init_FreeType(&s->ft) || FT_New_Memory_Face(s->ft, s->font, (FT_Long)fontSize, 0, &s->face)) {
        HdMovieLog("hd movie: no subtitle font");
        s->face = NULL;
    }
    HdMovieLog("hd movie: %d subtitles (%s)", s->count, path);
    return s->count;
}

static void FreeSubs(Subs* s) {
    int i;
    for (i = 0; i < s->count; i++)
        SDL_free(s->text[i]);
    if (s->texture)
        SDL_DestroyTexture(s->texture);
    if (s->face)
        FT_Done_Face(s->face);
    if (s->ft)
        FT_Done_FreeType(s->ft);
    SDL_free(s->font);
    SDL_zerop(s);
}

static unsigned NextCodepoint(const char** p) {
    const unsigned char* s = (const unsigned char*)*p;
    unsigned c = *s++;
    int more = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : 0;
    if (more)
        c &= 0x3F >> more;
    while (more-- && (*s & 0xC0) == 0x80)
        c = c << 6 | (*s++ & 0x3F);
    *p = (const char*)s;
    return c;
}

/* the width of one line of text (up to '\n' or the end), in pixels */
static int LineWidth(FT_Face face, const char* p) {
    int w = 0;
    while (*p && *p != '\n')
        if (!FT_Load_Char(face, NextCodepoint(&p), FT_LOAD_DEFAULT))
            w += (int)(face->glyph->advance.x >> 6);
    return w;
}

/* the line as white letters with a dark outline, at a size for the output height (as the Windows player) */
static void MakeSubTexture(Subs* s, SDL_Renderer* renderer, int line, int outH) {
    int px = SDL_max(12, outH / 22), pad = SDL_max(2, px / 9), lines = 1, widest = 0, x, y, dx, dy, row, lineH, ascent;
    const char* p;
    Uint8* cover;
    Uint32* rgba;

    if (s->texture)
        SDL_DestroyTexture(s->texture);
    s->texture = NULL;
    s->shown = line;
    s->height = outH;
    if (!s->face || FT_Set_Pixel_Sizes(s->face, 0, (FT_UInt)px))
        return;
    lineH = (int)(s->face->size->metrics.height >> 6);
    ascent = (int)(s->face->size->metrics.ascender >> 6);
    for (p = s->text[line];; p++) {
        widest = SDL_max(widest, LineWidth(s->face, p));
        if (!(p = SDL_strchr(p, '\n')))
            break;
        lines++;
    }
    s->w = widest + pad * 2 + 2;
    s->h = lineH * lines + pad * 2 + 2;
    cover = SDL_calloc((size_t)s->w * s->h, 1);
    rgba = SDL_malloc((size_t)s->w * s->h * 4);
    if (!cover || !rgba)
        goto done;
    for (p = s->text[line], row = 0; *p; row++) {
        int pen = pad + 1 + (widest - LineWidth(s->face, p)) / 2, base = pad + 1 + row * lineH + ascent;
        while (*p && *p != '\n') {
            FT_GlyphSlot g;
            if (FT_Load_Char(s->face, NextCodepoint(&p), FT_LOAD_RENDER))
                continue;
            g = s->face->glyph;
            for (y = 0; y < (int)g->bitmap.rows; y++)
                for (x = 0; x < (int)g->bitmap.width; x++) {
                    int tx = pen + g->bitmap_left + x, ty = base - g->bitmap_top + y;
                    if (tx >= 0 && ty >= 0 && tx < s->w && ty < s->h)
                        cover[ty * s->w + tx] = (Uint8)SDL_max(cover[ty * s->w + tx], g->bitmap.buffer[y * g->bitmap.pitch + x]);
                }
            pen += (int)(g->advance.x >> 6);
        }
        if (*p == '\n')
            p++;
    }
    /* the outline: the letters' coverage spread over a disc of radius pad */
    for (y = 0; y < s->h; y++)
        for (x = 0; x < s->w; x++) {
            int a = cover[y * s->w + x], o = a;
            for (dy = -pad; dy <= pad && o < 255; dy++)
                for (dx = -pad; dx <= pad; dx++) {
                    int yy = y + dy, xx = x + dx;
                    if (dx * dx + dy * dy > pad * pad + pad || yy < 0 || xx < 0 || yy >= s->h || xx >= s->w)
                        continue;
                    o = SDL_max(o, cover[yy * s->w + xx]);
                }
            rgba[y * s->w + x] = (Uint32)(o * 230 / 255 > a ? o * 230 / 255 : a) << 24 | (Uint32)a << 16 | (Uint32)a << 8 | (Uint32)a;
        }
    s->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, s->w, s->h);
    if (s->texture) {
        SDL_UpdateTexture(s->texture, NULL, rgba, s->w * 4);
        SDL_SetTextureBlendMode(s->texture, SDL_BLENDMODE_BLEND);
    }
done:
    SDL_free(cover);
    SDL_free(rgba);
}

/* the subtitle due at clock (microseconds; the subtitles count the movie's frames at 30 a second) */
static void DrawSubs(Subs* s, SDL_Renderer* renderer, int64_t clock, const SDL_FRect* picture, int outH) {
    int frame = (int)(clock * 30 / 1000000), i, line = -1;
    SDL_FRect dst;
    for (i = 0; i < s->count; i++)
        if (frame >= s->first[i] && frame <= s->last[i])
            line = i;
    if (line < 0)
        return;
    if (line != s->shown || outH != s->height || !s->texture)
        MakeSubTexture(s, renderer, line, outH);
    if (!s->texture)
        return;
    dst.w = (float)s->w;
    dst.h = (float)s->h;
    dst.x = SDL_roundf(picture->x + (picture->w - dst.w) / 2);
    dst.y = SDL_roundf(SDL_min(picture->y + picture->h, (float)outH) - dst.h - outH / 18.0f);
    SDL_RenderTexture(renderer, s->texture, NULL, &dst);
}

/* ---- the player --------------------------------------------------------------------------------------------- */

/* 1 skip, 2 quit; while the app is in the background the movie waits (audio paused) */
static int SkipPressed(SDL_AudioStream* audio) {
    SDL_Event e;
    int skip = 0;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_TERMINATING: return 2;
            case SDL_EVENT_DID_ENTER_BACKGROUND:
                if (audio)
                    SDL_PauseAudioStreamDevice(audio);
                while (SDL_WaitEvent(&e) && e.type != SDL_EVENT_DID_ENTER_FOREGROUND)
                    if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_TERMINATING)
                        return 2;
                if (audio)
                    SDL_ResumeAudioStreamDevice(audio);
                break;
            /* only Back, Escape or the pad's Start: taps and A advance the dialogue, so they reached the movie at once
               and ended the HD scenes as they began (22/10) */
            case SDL_EVENT_KEY_DOWN:
                if (!e.key.repeat && (e.key.key == SDLK_ESCAPE || e.key.key == SDLK_AC_BACK))
                    skip = 1;
                break;
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                if (e.gbutton.button == SDL_GAMEPAD_BUTTON_START)
                    skip = 1;
                break;
            default: break;
        }
    }
    return skip;
}

typedef struct {
    AMediaExtractor* ex;
    AMediaCodec* codec;
    int inputDone, outputDone;
} Track;

/* the first track whose type starts with kind ("video/", "audio/"): its own extractor and a decoder for it */
static int OpenTrack(Track* t, int fd, off64_t size, const char* kind, ANativeWindow* window, AMediaFormat** formatOut) {
    size_t i, n;
    SDL_zerop(t);
    t->ex = M.extractorNew();
    if (!t->ex || M.extractorSetFd(t->ex, fd, 0, size) != AMEDIA_OK)
        return 0;
    for (i = 0, n = M.extractorTracks(t->ex); i < n; i++) {
        AMediaFormat* format = M.extractorFormat(t->ex, i);
        const char* mime = NULL;
        if (format && M.formatString(format, "mime", &mime) && mime && SDL_strncmp(mime, kind, 6) == 0) {
            M.extractorSelect(t->ex, i);
            /* test harness: KHCOM_HDMOVIE_DECODER=<codec name> for the video (e.g. the software c2.android.avc.decoder) */
            const char* forced = window ? SDL_getenv("KHCOM_HDMOVIE_DECODER") : NULL;
            t->codec = forced ? M.codecByName(forced) : M.codecNew(mime);
            if (t->codec && M.codecConfigure(t->codec, format, window, NULL, 0) == AMEDIA_OK && M.codecStart(t->codec) == AMEDIA_OK) {
                *formatOut = format;
                return 1;
            }
            HdMovieLog("hd movie: no %s decoder", mime);
            if (t->codec)
                M.codecDelete(t->codec);
            t->codec = NULL;
            M.formatDelete(format);
            return 0;
        }
        if (format)
            M.formatDelete(format);
    }
    return 0;
}

static void CloseTrack(Track* t) {
    if (t->codec) {
        M.codecStop(t->codec);
        M.codecDelete(t->codec);
    }
    if (t->ex)
        M.extractorDelete(t->ex);
    SDL_zerop(t);
}

/* hands the decoder every compressed sample it has room for */
static void Feed(Track* t) {
    while (!t->inputDone) {
        ssize_t index = M.codecInput(t->codec, 0);
        size_t cap = 0;
        uint8_t* buf;
        ssize_t n;
        if (index < 0)
            return;
        buf = M.codecInputBuffer(t->codec, (size_t)index, &cap);
        n = buf ? M.extractorRead(t->ex, buf, cap) : -1;
        if (n < 0) {
            M.codecQueue(t->codec, (size_t)index, 0, 0, 0, AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
            t->inputDone = 1;
            return;
        }
        M.codecQueue(t->codec, (size_t)index, 0, (size_t)n, (uint64_t)M.extractorTime(t->ex), 0);
        M.extractorAdvance(t->ex);
    }
}

/* The frame is converted to RGB here (BT.709, studio range) and uploaded as a plain texture, the default: SDL's YUV
   textures (KHCOM_HDMOVIE_YUV=1) hang some GPUs while their shader is built (the Android 14 emulator, 21/10) */
static void YuvToRgb(SDL_Texture* texture, int w, int h, const uint8_t* y, int sy, const uint8_t* u, const uint8_t* v,
                     int su, int sv, int pu) {
    /* the same integer formula per pixel, from tables built once; chroma once per pixel pair; one 32-bit store
       (ABGR8888: bytes R, G, B, A) */
    static int tY[256], tRv[256], tGu[256], tGv[256], tBu[256], ready;
    static uint8_t clampTab[1024];
    uint8_t* out;
    int pitch;
    if (!ready) {
        for (int k = 0; k < 256; k++) {
            tY[k] = (k - 16) * 298 + 128;
            tRv[k] = 459 * (k - 128);
            tGu[k] = -55 * (k - 128);
            tGv[k] = -136 * (k - 128);
            tBu[k] = 541 * (k - 128);
        }
        for (int k = 0; k < 1024; k++)
            clampTab[k] = (uint8_t)(k < 384 ? 0 : k > 384 + 255 ? 255 : k - 384);
        ready = 1;
    }
#define CLAMP8(x) clampTab[((x) >> 8) + 384]
    if (!SDL_LockTexture(texture, NULL, (void**)&out, &pitch))
        return;
    for (int j = 0; j < h; j++) {
        const uint8_t* ly = y + (size_t)j * sy;
        const uint8_t* lu = u + (size_t)(j / 2) * su;
        const uint8_t* lv = v + (size_t)(j / 2) * sv;
        uint32_t* d = (uint32_t*)(out + (size_t)j * pitch);
        for (int i = 0; i < w; i += 2) {
            int cu = lu[(i / 2) * pu], cv = lv[(i / 2) * pu];
            int rv = tRv[cv], g = tGu[cu] + tGv[cv], bu = tBu[cu];
            int c = tY[ly[i]];
            d[i] = 0xFF000000u | (uint32_t)CLAMP8(c + bu) << 16 | (uint32_t)CLAMP8(c + g) << 8 | CLAMP8(c + rv);
            if (i + 1 < w) {
                c = tY[ly[i + 1]];
                d[i + 1] = 0xFF000000u | (uint32_t)CLAMP8(c + bu) << 16 | (uint32_t)CLAMP8(c + g) << 8 | CLAMP8(c + rv);
            }
        }
    }
#undef CLAMP8
    SDL_UnlockTexture(texture);
}

/* Plays the movie at path to the end (or until skipped). subs, subs2: the subtitle file to show and the one to try
   if it is missing (NULL: none); font: the KHMenu face to try before the APK's. 0: it could not be opened, 1 played,
   2 played and the app was closed. */
int HdMovieAndroidPlay(SDL_Renderer* renderer, const char* path, const char* subs1, const char* subs2, const char* fontPath) {
    Track video, audioTrack;
    AMediaFormat *vFormat = NULL, *aFormat = NULL;
    AImageReader* reader = NULL;
    ANativeWindow* window = NULL;
    SDL_Texture* texture = NULL;
    SDL_AudioStream* audio = NULL;
    int32_t width = 0, height = 0, rate = 0, channels = 0;
    int texW = 0, texH = 0, texNv12 = -1; /* the texture's size and kind (NV12 or planar) */
    int64_t audioPushed = 0, pendingPts = 0, start = 0; /* microseconds */
    Uint64 lastShown = 0; /* when a frame last went to the screen */
    ssize_t pending = -1;
    int fd, quit = 0, played = 0, shown = 0, dropped = 0, audioOn, rgb = SDL_getenv("KHCOM_HDMOVIE_YUV") == NULL;
    struct stat st;
    Subs subs;

    SDL_zero(video);
    SDL_zero(audioTrack);
    SDL_zero(subs);
    subs.shown = -1;
    if (!renderer || !path || stat(path, &st) != 0 || !LoadMedia())
        return 0;
    fd = open(path, O_RDONLY);
    if (fd < 0)
        return 0;
    /* the video's size first: the image reader that receives the decoded frames needs it */
    {
        Track probe;
        AMediaFormat* format = NULL;
        size_t i, n;
        SDL_zero(probe);
        probe.ex = M.extractorNew();
        if (probe.ex && M.extractorSetFd(probe.ex, fd, 0, st.st_size) == AMEDIA_OK)
            for (i = 0, n = M.extractorTracks(probe.ex); i < n && !width; i++) {
                const char* mime = NULL;
                format = M.extractorFormat(probe.ex, i);
                if (format && M.formatString(format, "mime", &mime) && mime && SDL_strncmp(mime, "video/", 6) == 0) {
                    M.formatInt(format, "width", &width);
                    M.formatInt(format, "height", &height);
                }
                if (format)
                    M.formatDelete(format);
            }
        CloseTrack(&probe);
    }
    if (width <= 0 || height <= 0 || M.readerNew(width, height, HD_YUV_420_888, 4, &reader) != AMEDIA_OK ||
        M.readerWindow(reader, &window) != AMEDIA_OK || !OpenTrack(&video, fd, st.st_size, "video/", window, &vFormat)) {
        HdMovieLog("hd movie: %s does not open (%dx%d)", path, (int)width, (int)height);
        goto done;
    }
    audioOn = OpenTrack(&audioTrack, fd, st.st_size, "audio/", NULL, &aFormat);
    if (audioOn) {
        M.formatInt(aFormat, "sample-rate", &rate);
        M.formatInt(aFormat, "channel-count", &channels);
        if (rate > 0 && channels > 0) {
            SDL_AudioSpec spec = { SDL_AUDIO_S16, (int)channels, (int)rate };
            audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
        }
        if (audio)
            SDL_ResumeAudioStreamDevice(audio);
        else {
            CloseTrack(&audioTrack);
            audioOn = 0;
        }
    }
    if (!audioOn)
        audioTrack.outputDone = 1;
    HdMovieLog("hd movie: %s (%dx%d, %d Hz)", path, (int)width, (int)height, (int)rate);
    LoadSubs(&subs, subs1, subs2, fontPath);
    played = 1;
    start = (int64_t)(SDL_GetTicksNS() / 1000);
    /* the presses that started the movie (the dialogue's A or tap) are not the movie's */
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_KEY_DOWN, SDL_EVENT_KEY_UP);
    SDL_FlushEvents(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_EVENT_MOUSE_BUTTON_UP);
    SDL_FlushEvents(SDL_EVENT_GAMEPAD_BUTTON_DOWN, SDL_EVENT_GAMEPAD_BUTTON_UP);
    SDL_FlushEvents(SDL_EVENT_FINGER_DOWN, SDL_EVENT_FINGER_UP);

    int64_t skipEnd = -1, lastClock = 0; /* skipped: the movie fades out until skipEnd, then ends (no cut, no pop) */
    while (!(video.outputDone && audioTrack.outputDone)) {
        Uint64 frameStart = SDL_GetTicksNS();
        int64_t clock;
        AImage* image = NULL;
        {
            int pressed = SkipPressed(audio);
            if (pressed == 2) {
                quit = 2;
                break;
            }
            if (pressed == 1 && skipEnd < 0)
                skipEnd = lastClock + 300000;
        }
        /* keep about half a second of audio queued: it is the clock */
        while (audio && !audioTrack.outputDone && SDL_GetAudioStreamQueued(audio) < rate * channels * 2 / 2) {
            AMediaCodecBufferInfo info;
            ssize_t index;
            Feed(&audioTrack);
            index = M.codecOutput(audioTrack.codec, &info, 2000);
            if (index == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
                AMediaFormat* f = M.codecOutputFormat(audioTrack.codec);
                int32_t r = rate, c = channels;
                if (f) {
                    M.formatInt(f, "sample-rate", &r);
                    M.formatInt(f, "channel-count", &c);
                    M.formatDelete(f);
                }
                if (r > 0 && c > 0 && (r != rate || c != channels)) {
                    SDL_AudioSpec spec = { SDL_AUDIO_S16, (int)c, (int)r };
                    SDL_SetAudioStreamFormat(audio, &spec, NULL);
                    rate = r;
                    channels = c;
                }
                continue;
            }
            if (index < 0)
                break; /* nothing decoded yet */
            if (info.size > 0) {
                size_t cap = 0;
                uint8_t* buf = M.codecOutputBuffer(audioTrack.codec, (size_t)index, &cap);
                if (buf && (size_t)(info.offset + info.size) <= cap) {
                    SDL_PutAudioStreamData(audio, buf + info.offset, info.size);
                    audioPushed += (int64_t)info.size * 1000000 / (rate * channels * 2);
                }
            }
            M.codecRelease(audioTrack.codec, (size_t)index, false);
            if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
                audioTrack.outputDone = 1;
                start = (int64_t)(SDL_GetTicksNS() / 1000) + (int64_t)SDL_GetAudioStreamQueued(audio) * 1000000 / (rate * channels * 2) - audioPushed;
            }
        }
        /* the presentation time: audio played so far, or the wall clock without audio */
        if (audio && !audioTrack.outputDone)
            clock = audioPushed - (int64_t)SDL_GetAudioStreamQueued(audio) * 1000000 / (rate * channels * 2);
        else /* start: moved when the audio ended, so the clock goes on from where the audio left it */
            clock = (int64_t)(SDL_GetTicksNS() / 1000) - start;
        lastClock = clock;
        if (skipEnd >= 0 && clock >= skipEnd) {
            quit = 1;
            break;
        }
        /* the latest video frame due by now goes to the image reader; late ones are dropped undrawn */
        while (!video.outputDone) {
            if (pending < 0) {
                AMediaCodecBufferInfo info;
                Feed(&video);
                pending = M.codecOutput(video.codec, &info, 0);
                if (pending < 0) {
                    pending = -1;
                    break;
                }
                pendingPts = info.presentationTimeUs;
                if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
                    M.codecRelease(video.codec, (size_t)pending, false);
                    pending = -1;
                    video.outputDone = 1;
                    break;
                }
            }
            if (pendingPts > clock + 10000) /* more than 10 ms ahead: later */
                break;
            /* a slow decoder still shows ten frames a second */
            if (audio && !audioTrack.outputDone && pendingPts + 33333 <= clock && SDL_GetTicksNS() - lastShown < 100000000) {
                M.codecRelease(video.codec, (size_t)pending, false);
                dropped++;
            } else {
                M.codecRelease(video.codec, (size_t)pending, true);
                lastShown = SDL_GetTicksNS();
                shown++;
            }
            pending = -1;
        }
        if (video.outputDone && !audio)
            audioTrack.outputDone = 1;
        if (M.readerLatest(reader, &image) == AMEDIA_OK && image) {
            HdCrop crop = { 0, 0, width, height };
            uint8_t *y = NULL, *u = NULL, *v = NULL;
            int ly = 0, lu = 0, lv = 0;
            int32_t sy = 0, su = 0, sv = 0, pu = 0;
            M.imageCrop(image, &crop);
            if (M.imagePlane(image, 0, &y, &ly) == AMEDIA_OK && M.imagePlane(image, 1, &u, &lu) == AMEDIA_OK &&
                M.imagePlane(image, 2, &v, &lv) == AMEDIA_OK && M.imageRowStride(image, 0, &sy) == AMEDIA_OK &&
                M.imageRowStride(image, 1, &su) == AMEDIA_OK && M.imageRowStride(image, 2, &sv) == AMEDIA_OK &&
                M.imagePixelStride(image, 1, &pu) == AMEDIA_OK) {
                /* interleaved chroma with U first is NV12; one byte per sample is planar YUV; anything else (NV21)
                   goes through NV21 */
                int w = crop.right - crop.left, h = crop.bottom - crop.top;
                int nv = rgb ? 3 : pu == 2 ? (v == u + 1 ? 1 : u == v + 1 ? 2 : 0) : 0;
                if (w > 0 && h > 0 && (pu == 1 || nv)) {
                    if (!texture || texW != w || texH != h || texNv12 != nv) {
                        SDL_PropertiesID props = SDL_CreateProperties();
                        if (texture)
                            SDL_DestroyTexture(texture);
                        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER,
                                              nv == 3 ? SDL_PIXELFORMAT_ABGR8888 : nv == 1 ? SDL_PIXELFORMAT_NV12 : nv == 2 ? SDL_PIXELFORMAT_NV21 :
                                              SDL_PIXELFORMAT_IYUV);
                        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER, SDL_TEXTUREACCESS_STREAMING);
                        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, w);
                        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, h);
                        /* HD video: BT.709, studio range */
                        if (nv != 3)
                            SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_COLORSPACE_NUMBER, SDL_COLORSPACE_BT709_LIMITED);
                        texture = SDL_CreateTextureWithProperties(renderer, props);
                        SDL_DestroyProperties(props);
                        if (texture)
                            SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
                        else
                            HdMovieLog("hd movie: no video texture (%s)", SDL_GetError());
                        texW = w;
                        texH = h;
                        texNv12 = nv;
                    }
                    if (texture) {
                        const uint8_t* yy = y + crop.top * sy + crop.left;
                        size_t c = (size_t)(crop.top / 2) * su + (size_t)(crop.left / 2) * pu;
                        if (nv == 3)
                            YuvToRgb(texture, w, h, yy, sy, u + c, v + (size_t)(crop.top / 2) * sv + (size_t)(crop.left / 2) * pu, su, sv, pu);
                        else if (nv == 1)
                            SDL_UpdateNVTexture(texture, NULL, yy, sy, u + c, su);
                        else if (nv == 2)
                            SDL_UpdateNVTexture(texture, NULL, yy, sy, v + c, sv);
                        else
                            SDL_UpdateYUVTexture(texture, NULL, yy, sy, u + c, su, v + (size_t)(crop.top / 2) * sv + crop.left / 2, sv);
                    }
                }
            }
            M.imageDelete(image);
        }
        {
            int ow = 0, oh = 0;
            SDL_FRect dst;
            float scale;
            SDL_GetCurrentRenderOutputSize(renderer, &ow, &oh);
            SDL_SetRenderViewport(renderer, NULL);
            SDL_SetRenderClipRect(renderer, NULL);
            SDL_SetRenderScale(renderer, 1.0f, 1.0f);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            if (texture) {
                scale = SDL_min((float)ow / texW, (float)oh / texH);
                dst.w = texW * scale;
                dst.h = texH * scale;
                dst.x = (ow - dst.w) / 2;
                dst.y = (oh - dst.h) / 2;
                SDL_RenderTexture(renderer, texture, NULL, &dst);
                DrawSubs(&subs, renderer, clock, &dst, oh);
                {
                    /* fades in from black and out to black (0.3 s each), as the desktop player (port_hdmovie.c) */
                    const int64_t fade = 300000;
                    int64_t duration = 0, edge;
                    if (vFormat)
                        M.formatInt64(vFormat, "durationUs", &duration);
                    if (skipEnd >= 0)
                        duration = skipEnd;
                    edge = clock < fade ? clock : (duration > fade && clock > duration - fade ? duration - clock : fade);
                    if (audio) /* the sound fades with the picture */
                        SDL_SetAudioStreamGain(audio, edge <= 0 ? 0.0f : (float)edge / fade);
                    if (edge < fade) {
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, (Uint8)(255 - (edge < 0 ? 0 : edge) * 255 / fade));
                        SDL_RenderFillRect(renderer, NULL);
                    }
                }
            }
            {
                /* test harness: KHCOM_HDMOVIE_SHOT=<seconds> saves that moment as hdmovie.bmp next to the movie's
                   hd folder (the app's files) */
                static int shot;
                const char* at = SDL_getenv("KHCOM_HDMOVIE_SHOT");
                if (at && !shot && texture && clock >= (int64_t)SDL_atoi(at) * 1000000) {
                    SDL_Surface* s = SDL_RenderReadPixels(renderer, NULL);
                    char out[1024];
                    const char* hd = SDL_strstr(path, "hd/movies/");
                    SDL_snprintf(out, sizeof(out), "%.*shdmovie.bmp", hd ? (int)(hd - path) : 0, path);
                    if (s) {
                        SDL_SaveBMP(s, out);
                        SDL_DestroySurface(s);
                        HdMovieLog("hd movie: shot %s", out);
                    }
                    shot = 1;
                }
            }
            SDL_RenderPresent(renderer);
        }
        {
            Uint64 spent = SDL_GetTicksNS() - frameStart;
            if (spent < 16000000)
                SDL_DelayNS(16000000 - spent);
        }
    }
    /* the last of the audio */
    while (!quit && audio && SDL_GetAudioStreamQueued(audio) > 0 && !(quit = SkipPressed(audio)))
        SDL_Delay(10);

done:
    HdMovieLog("hd movie: %d frames shown, %d late ones dropped%s", shown, dropped, quit == 2 ? ", app closed" : "");
    if (played)
        FreeSubs(&subs);
    if (pending >= 0)
        M.codecRelease(video.codec, (size_t)pending, false);
    CloseTrack(&video);
    CloseTrack(&audioTrack);
    if (vFormat)
        M.formatDelete(vFormat);
    if (aFormat)
        M.formatDelete(aFormat);
    if (reader)
        M.readerDelete(reader);
    if (texture)
        SDL_DestroyTexture(texture);
    if (audio)
        SDL_DestroyAudioStream(audio);
    close(fd);
    return played ? (quit == 2 ? 2 : 1) : 0;
}
