#include <SDL3/SDL.h>
#include <string.h>
#include "port/port.h"
#include "port/port_ui.h"
#include "../achievements/toast_layout.h"
#include "../decompui/third_party/stb_image.h"

typedef struct AchievementToast { char title[128], detail[256], label[64]; int badge; } AchievementToast;
static AchievementToast sQueue[16];
static unsigned sHead, sCount;
static Uint64 sStart;
static SDL_Texture* sTexture;
static SDL_Renderer* sRenderer;
static int sWidth;
static int sVisible;
static int sOutputWidth;
/* The set's badges (assets/achievements/badges.png, 10 per row, 64x64), local: no network. */
static unsigned char* sAtlas;
static int sAtlasRows, sAtlasTried;
static Uint64 sPreviewClock;

void PortAchievementToast(const char* title, const char* detail, const char* label, int badgeIndex) {
    AchievementToast* toast;
    if (sCount == 16) return;
    toast = &sQueue[(sHead + sCount) % 16];
    SDL_strlcpy(toast->title, title, sizeof(toast->title));
    SDL_strlcpy(toast->detail, detail, sizeof(toast->detail));
    SDL_strlcpy(toast->label, label, sizeof(toast->label));
    toast->badge = badgeIndex;
    ++sCount;
}

static void Fill(uint32_t* pixels, int x, int y, int w, int h, uint32_t color) {
    int i, j;
    for (j = y; j < y + h; ++j) for (i = x; i < x + w; ++i) pixels[j * 256 + i] = color;
}

static void Line(uint32_t* pixels, int y, const char* text, uint32_t color) {
    uint32_t line[256 * 14];
    int x, row, width = sWidth - 50;
    PortUiRenderTextLine(line, width, 14, text, color);
    for (row = 0; row < 14 && row + y < 48; ++row)
        for (x = 0; x < width; ++x) if (line[row * width + x] >> 24)
            pixels[(row + y) * 256 + x + 44] = line[row * width + x];
}

/* 32x32 box-filtered copy of the badge; returns 0 when the atlas or index is missing. */
static int Badge(uint32_t* pixels, int index) {
    int px, py, sx, sy;
    if (!sAtlasTried) {
        int w, h, channels;
        sAtlasTried = 1;
        size_t size;
        void* file = PortLoadAsset("assets/achievements/badges.png", &size);
        sAtlas = file ? stbi_load_from_memory(file, (int)size, &w, &h, &channels, 4) : NULL;
        SDL_free(file);
        if (sAtlas && (w != 640 || h % 64)) { stbi_image_free(sAtlas); sAtlas = NULL; }
        sAtlasRows = sAtlas ? h / 64 : 0;
    }
    if (!sAtlas || index < 0 || index / 10 >= sAtlasRows) return 0;
    Fill(pixels, 5, 7, 34, 34, 0xFF080808);
    for (py = 0; py < 32; ++py) for (px = 0; px < 32; ++px) {
        unsigned r = 0, g = 0, b = 0;
        for (sy = 0; sy < 2; ++sy) for (sx = 0; sx < 2; ++sx) {
            const unsigned char* p = sAtlas + (((index / 10) * 64 + py * 2 + sy) * 640 + (index % 10) * 64 + px * 2 + sx) * 4;
            r += p[0]; g += p[1]; b += p[2];
        }
        pixels[(8 + py) * 256 + 6 + px] = 0xFF000000u | (r / 4) << 16 | (g / 4) << 8 | (b / 4);
    }
    return 1;
}

static int Prepare(SDL_Renderer* renderer, int outputWidth) {
    uint32_t pixels[256 * 48];
    const AchievementToast* toast = &sQueue[sHead];
    sWidth = SDL_max(PortUiTextWidth(toast->title), PortUiTextWidth(toast->detail)) + 54;
    sWidth = SDL_clamp(sWidth, 196, 256);
    if (sWidth > outputWidth - 16) sWidth = outputWidth - 16;
    memset(pixels, 0, sizeof(pixels));
    Fill(pixels, 0, 0, sWidth, 48, 0xFFB5ADAD);
    Fill(pixels, 1, 1, sWidth - 2, 46, 0xFFFCFBFB);
    Fill(pixels, 1, 1, 2, 46, 0xFFFF1000);
    if (!Badge(pixels, toast->badge)) {
        /* No badge (preview, missing atlas): the cursor-square emblem of the options and launcher. */
        Fill(pixels, 10, 16, 21, 21, 0xFF080808);
        Fill(pixels, 15, 21, 11, 11, 0xFFFF1000);
        Fill(pixels, 15, 21, 1, 11, 0xFF9C1000);
    }
    if (toast->label[0]) {
        Line(pixels, 5, toast->label, 0xFF6B6565);
        Line(pixels, 19, toast->title, 0xFF080808);
        Line(pixels, 33, toast->detail, 0xFF6B6565);
    } else { /* no label line: the name and the start of the description sit centred */
        Line(pixels, 11, toast->title, 0xFF080808);
        Line(pixels, 26, toast->detail, 0xFF6B6565);
    }
    if (!sTexture || sRenderer != renderer) {
        if (sTexture) SDL_DestroyTexture(sTexture);
        sTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 256, 48);
        sRenderer = renderer;
        if (!sTexture) return 0;
        SDL_SetTextureScaleMode(sTexture, SDL_SCALEMODE_NEAREST);
        SDL_SetTextureBlendMode(sTexture, SDL_BLENDMODE_BLEND);
    }
    return SDL_UpdateTexture(sTexture, NULL, pixels, 256 * 4);
}

void PortAchievementToastDraw(void* opaqueRenderer, int width, int height) {
    SDL_Renderer* renderer = (SDL_Renderer*)opaqueRenderer;
    Uint64 now = SDL_GetTicks();
    KhToastLayout box;
    SDL_FRect dst, src, shadow;
    unsigned elapsed;
    if (!sCount || width < 96 || height < 64) return;
    /* Only the explicitly labelled preview harness may use deterministic
     * frame time, so video encoding speed cannot change its captured motion. */
    const char* preview;
    const char* frameClock;
    PORT_ENV_ONCE(preview, SDL_getenv, "KHCOM_ACHIEVEMENT_PREVIEW");
    PORT_ENV_ONCE(frameClock, SDL_getenv, "KHCOM_TOAST_FRAMECLOCK");
    if (preview && frameClock)
        now = (++sPreviewClock * 1000) / 60;
    if (sVisible && sOutputWidth != width) {
        if (!Prepare(renderer, width)) return;
    }
    sOutputWidth = width;
    if (!sVisible) {
        if (!Prepare(renderer, width)) return;
        sStart = now; sVisible = 1;
    }
    elapsed = (unsigned)(now - sStart);
    if (elapsed >= 4500) { --sCount; sHead = (sHead + 1) % 16; sVisible = 0; return; }
    box = KhAchievementToastLayout(elapsed, width, height, sWidth);
    dst.x = box.x; dst.y = box.y; dst.w = box.width; dst.h = box.height;
    shadow = dst; shadow.x += box.scale; shadow.y += box.scale * 2;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, (Uint8)(55 * box.alpha));
    SDL_RenderFillRect(renderer, &shadow);
    SDL_SetTextureAlphaMod(sTexture, (Uint8)(255 * box.alpha));
    src.x = src.y = 0; src.w = (float)sWidth; src.h = 48;
    SDL_RenderTexture(renderer, sTexture, &src, &dst);
}

void PortAchievementToastShutdown(void) {
    if (sTexture) SDL_DestroyTexture(sTexture);
    sTexture = NULL; sRenderer = NULL; sCount = sHead = 0; sStart = 0; sVisible = 0;
    sPreviewClock = 0;
    if (sAtlas) stbi_image_free(sAtlas);
    sAtlas = NULL; sAtlasRows = sAtlasTried = 0;
}
