/*
 * Direct framebuffer output for handhelds (Anbernic and similar).
 *
 * Their GPU drivers are 64-bit only, so a 32-bit process cannot use SDL's
 * KMSDRM backend (it needs EGL/GBM). With KHCOM_FBDEV=/dev/fb0 the game
 * renders with SDL's offscreen driver and software renderer, and each frame
 * is copied to the Linux framebuffer here. Audio (ALSA) and the built-in
 * controls (evdev) still go through SDL.
 */
#include <SDL3/SDL.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/fb.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#include "port/port.h"

static uint8_t* sFb;
static SDL_Surface* sRenderSurface;
static size_t sFbOffset;
static struct fb_var_screeninfo sVar;
static struct fb_fix_screeninfo sFix;
static bool sProfileFb;
static bool sAsyncFbRequested, sAsyncFbPending, sAsyncFbStopping;
static SDL_Thread* sAsyncFbThread;
static SDL_Semaphore *sAsyncFbGo, *sAsyncFbDone;
static struct {
    uint8_t* destination;
    const SDL_Surface* source;
    int height;
    size_t rowBytes;
} sAsyncFbJob;

void PortFbWaitCopy(void) {
    if (sAsyncFbPending) {
        SDL_WaitSemaphore(sAsyncFbDone);
        sAsyncFbPending = false;
    }
}

void PortFbStopCopy(void) {
    if (!sAsyncFbThread)
        return;
    PortFbWaitCopy();
    sAsyncFbStopping = true;
    SDL_SignalSemaphore(sAsyncFbGo);
    SDL_WaitThread(sAsyncFbThread, NULL);
    SDL_DestroySemaphore(sAsyncFbGo);
    SDL_DestroySemaphore(sAsyncFbDone);
    sAsyncFbThread = NULL;
    sAsyncFbGo = sAsyncFbDone = NULL;
}

static uint64_t sDrawTicks, sCopyTicks;

void PortFbStageTicks(uint64_t* draw, uint64_t* copy) {
    *draw = sDrawTicks;
    *copy = sCopyTicks;
}

/* Opens the framebuffer named by KHCOM_FBDEV; returns 0 when not in use. */
int PortFbInit(int* w, int* h) {
    const char* path = SDL_getenv("KHCOM_FBDEV");
    int fd;

    if (path == NULL || *path == '\0')
        return 0;
    fd = open(path, O_RDWR);
    if (fd < 0 || ioctl(fd, FBIOGET_VSCREENINFO, &sVar) != 0 || ioctl(fd, FBIOGET_FSCREENINFO, &sFix) != 0) {
        PortLog("fbdev: cannot open %s", path);
        if (fd >= 0)
            close(fd);
        return 0;
    }
    if (sVar.bits_per_pixel != 32 && sVar.bits_per_pixel != 16) {
        PortLog("fbdev: unsupported %u bits per pixel", sVar.bits_per_pixel);
        close(fd);
        return 0;
    }
    const unsigned bpp = sVar.bits_per_pixel / 8;
    const uint64_t widthEnd = (uint64_t)sVar.xoffset + sVar.xres;
    const uint64_t heightEnd = (uint64_t)sVar.yoffset + sVar.yres;
    const uint64_t rowEnd = widthEnd * bpp;
    /* The virtual rows describe the layout. smem_len bounds the actual aperture. */
    const uint64_t mapSize = SDL_min((uint64_t)sFix.line_length * sVar.yres_virtual, sFix.smem_len);
    if (!sVar.xres || !sVar.yres || sVar.xres > INT_MAX || sVar.yres > INT_MAX || sFix.line_length > INT_MAX ||
        widthEnd > sVar.xres_virtual || heightEnd > sVar.yres_virtual || rowEnd > sFix.line_length ||
        (heightEnd - 1) * sFix.line_length + rowEnd > mapSize) {
        PortLog("fbdev: invalid screen geometry for %s", path);
        close(fd);
        return 0;
    }
    sFbOffset = (size_t)sVar.yoffset * sFix.line_length + (size_t)sVar.xoffset * bpp;
    sFb = mmap(NULL, (size_t)mapSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (sFb == MAP_FAILED) {
        sFb = NULL;
        PortLog("fbdev: cannot map %s", path);
        return 0;
    }
    *w = (int)sVar.xres;
    *h = (int)sVar.yres;
    sProfileFb = SDL_getenv("KHCOM_FBDEV_PROFILE") != NULL;
    sAsyncFbRequested = SDL_getenv("KHCOM_FBDEV_ASYNC_COPY") != NULL;
    PortLog("fbdev: %s %ux%u, %u bpp (red at bit %u)", path, sVar.xres, sVar.yres, sVar.bits_per_pixel,
            sVar.red.offset);
    return 1;
}

int PortFbActive(void) {
    return sFb != NULL;
}

/* Rendering into our own surface avoids allocating and copying a full-size
 * SDL_RenderReadPixels surface on every frame. Keep the drawing surface in
 * 32-bit colour so SDL applies the same blending before conversion to RGB565. */
SDL_Renderer* PortFbCreateRenderer(void) {
    PortFbWaitCopy();
    if (!sFb)
        return NULL;
    sRenderSurface = SDL_CreateSurface((int)sVar.xres, (int)sVar.yres, SDL_PIXELFORMAT_XRGB8888);
    if (!sRenderSurface)
        return NULL;
    SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(sRenderSurface);
    if (!renderer) {
        SDL_DestroySurface(sRenderSurface);
        sRenderSurface = NULL;
    }
    return renderer;
}

/* The 720x480 picture is centred on the 640x480 handheld panel. */
bool PortFbDrawCropped3x(SDL_Renderer* renderer, const uint32_t* source, int stride) {
    SDL_Rect viewport;
    float scaleX, scaleY;
    int logicalW, logicalH;
    SDL_RendererLogicalPresentation logicalMode;
    if (!sRenderSurface || sRenderSurface->w != 640 || sRenderSurface->h != 480 ||
        sRenderSurface->format != SDL_PIXELFORMAT_XRGB8888 ||
        SDL_MUSTLOCK(sRenderSurface) || SDL_GetRenderTarget(renderer) || SDL_RenderClipEnabled(renderer) ||
        !SDL_GetRenderViewport(renderer, &viewport) || viewport.x || viewport.y ||
        viewport.w != 640 || viewport.h != 480 ||
        !SDL_GetRenderScale(renderer, &scaleX, &scaleY) || scaleX != 1 || scaleY != 1 ||
        !SDL_GetRenderLogicalPresentation(renderer, &logicalW, &logicalH, &logicalMode) ||
        logicalMode != SDL_LOGICAL_PRESENTATION_DISABLED || !SDL_FlushRenderer(renderer))
        return false;
    const int pitch = sRenderSurface->pitch;
    uint8_t* row = sRenderSurface->pixels;
    for (int y = 0; y < 160; y++, source += stride, row += 3 * pitch) {
        uint32_t* output = (uint32_t*)row;
        /* The 40-pixel crop cuts one pixel from the first and last triples. */
        output[0] = output[1] = source[13];
        for (int x = 0; x < 212; x++) {
            const uint32_t color = source[14 + x];
            output[2 + 3 * x] = output[3 + 3 * x] = output[4 + 3 * x] = color;
        }
        output[638] = output[639] = source[226];
        memcpy(row + pitch, row, 640 * sizeof(*output));
        memcpy(row + 2 * pitch, row, 640 * sizeof(*output));
    }
    return true;
}

/* The handheld's opaque nearest 2x case can write the software renderer's
 * surface directly. Other geometry and renderer paths stay with SDL. */
static int DrawNearest2x(SDL_Renderer* renderer, const uint32_t* src, int w, int h, int stride, int outY, int outH, const SDL_Rect* cover) {
#ifdef __ARM_NEON
    SDL_Rect viewport;
    float scaleX, scaleY;
    int logicalW, logicalH;
    SDL_RendererLogicalPresentation logicalMode;
    const bool fractionalFit = sRenderSurface && sRenderSurface->w == 640 && w == 240 && h == 160 &&
                               (outY == 26 || outY == 8) && outH == 426 && !cover;
    if (!sRenderSurface || w <= 0 || (w & 3) || h <= 0 ||
        (!fractionalFit && sRenderSurface->w / 2 != w) || outY < 0 || outH <= 0 || outH > sRenderSurface->h - outY ||
        (sRenderSurface->w & 1) ||
        SDL_MUSTLOCK(sRenderSurface) || SDL_GetRenderTarget(renderer) || SDL_RenderClipEnabled(renderer) ||
        !SDL_GetRenderViewport(renderer, &viewport) || viewport.x || viewport.y ||
        viewport.w != sRenderSurface->w || viewport.h != sRenderSurface->h ||
        !SDL_GetRenderScale(renderer, &scaleX, &scaleY) || scaleX != 1 || scaleY != 1 ||
        !SDL_GetRenderLogicalPresentation(renderer, &logicalW, &logicalH, &logicalMode) ||
        logicalMode != SDL_LOGICAL_PRESENTATION_DISABLED || !SDL_FlushRenderer(renderer))
        return 0;
    const int pitch = sRenderSurface->pitch;
    uint8_t* pixels = (uint8_t*)sRenderSurface->pixels + (size_t)outY * pitch;
    if (fractionalFit) {
        const unsigned stepX = (240u << 16) / 640u;
        const unsigned stepY = (160u << 16) / 426u;
        unsigned posY = stepY / 2;
        unsigned previous = 160;
        for (int y = 0; y < outH; y++, posY += stepY) {
            const unsigned row = posY >> 16;
            uint32_t* dest = (uint32_t*)(pixels + (size_t)y * pitch);
            if (row == previous) {
                memcpy(dest, (uint8_t*)dest - pitch, 640 * sizeof(*dest));
            } else {
                const uint32_t* input = src + row * stride;
                unsigned posX = stepX / 2;
                for (int x = 0; x < 640; x++, posX += stepX)
                    dest[x] = input[posX >> 16];
            }
            previous = row;
        }
        return 1;
    }
    if (h != outH / 2 || (outH & 1)) {
        /* SDL's nearest scaler samples the pixel centres with a 16.16 step. */
        const uint64_t step = ((uint64_t)h << 16) / outH;
        uint64_t pos = step / 2;
        int previous = -1;
        bool previousCovered = false;
        for (int y = 0; y < outH; y++, pos += step) {
            const int row = (int)(pos >> 16);
            uint32_t* dst = (uint32_t*)(pixels + (size_t)y * pitch);
            const bool covered = cover && y >= cover->y && y < cover->y + cover->h;
            const int left = covered ? cover->x / 2 : w;
            const int right = covered ? (cover->x + cover->w) / 2 : w;
            if (row == previous && covered == previousCovered) {
                memcpy(dst, (const uint8_t*)dst - pitch, (size_t)left * 2 * sizeof(*dst));
                if (right < w)
                    memcpy(dst + 2 * right, (const uint8_t*)(dst + 2 * right) - pitch,
                           (size_t)(w - right) * 2 * sizeof(*dst));
                continue;
            }
            const uint32_t* input = src + (size_t)row * stride;
            for (int x = 0; x < w; x += 4) {
                if (covered && x == left) {
                    x = right;
                    if (x == w)
                        break;
                }
                const uint32x4_t colors = vld1q_u32(input + x);
                const uint32x4x2_t doubled = vzipq_u32(colors, colors);
                vst1q_u32(dst + 2 * x, doubled.val[0]);
                vst1q_u32(dst + 2 * x + 4, doubled.val[1]);
            }
            previous = row;
            previousCovered = covered;
        }
        return 1;
    }
    for (int y = 0; y < h; y++) {
        uint32_t* upper = (uint32_t*)(pixels + (size_t)(2 * y) * pitch);
        uint32_t* lower = (uint32_t*)((uint8_t*)upper + pitch);
        for (int x = 0; x < w; x += 4) {
            uint32x4_t colors = vld1q_u32(src + x);
            uint32x4x2_t doubled = vzipq_u32(colors, colors);
            vst1q_u32(upper + 2 * x, doubled.val[0]);
            vst1q_u32(upper + 2 * x + 4, doubled.val[1]);
            vst1q_u32(lower + 2 * x, doubled.val[0]);
            vst1q_u32(lower + 2 * x + 4, doubled.val[1]);
        }
        src += stride;
    }
    return 1;
#else
    (void)renderer; (void)src; (void)w; (void)h; (void)stride; (void)outY; (void)outH; (void)cover;
    return 0;
#endif
}

int PortFbDrawNearest2x(SDL_Renderer* renderer, const uint32_t* src, int w, int h, int outH, const SDL_Rect* cover) {
    return sRenderSurface && sRenderSurface->h == outH && DrawNearest2x(renderer, src, w, h, w, 0, outH, cover);
}

int PortFbDrawFit2x(SDL_Renderer* renderer, const uint32_t* src, int w, int h, const SDL_FRect* dest) {
    return sRenderSurface && sRenderSurface->w == 640 && sRenderSurface->h == 480 &&
           dest->x == 0 && dest->w == 640 &&
           DrawNearest2x(renderer, src, w, h, w, (int)dest->y, (int)dest->h, NULL);
}

int PortFbDrawWorldInspectFit(SDL_Renderer* renderer, const uint32_t* src, int stride) {
    int drawn = sRenderSurface && sRenderSurface->w == 640 && sRenderSurface->h == 480 &&
                DrawNearest2x(renderer, src, 240, 160, stride, 26, 426, NULL);
    if (drawn) {
        static bool logged;
        if (!logged) { PortLog("fbdev: world inspect fit active"); logged = true; }
    }
    return drawn;
}

/* Any GBA-size menu picture fitted to the 640x480 panel's width. */
int PortFbDrawMenuFit(SDL_Renderer* renderer, const uint32_t* src, int stride) {
    int drawn = sRenderSurface && sRenderSurface->w == 640 && sRenderSurface->h == 480 &&
                DrawNearest2x(renderer, src, 240, 160, stride, 26, 426, NULL);
    if (drawn) {
        static bool logged;
        if (!logged) { PortLog("fbdev: menu fit active"); logged = true; }
    }
    return drawn;
}

int PortFbDrawJournalFit(SDL_Renderer* renderer, const uint32_t* src, int stride) {
    return sRenderSurface && sRenderSurface->w == 640 && sRenderSurface->h == 480 &&
           DrawNearest2x(renderer, src, 240, 160, stride, 8, 426, NULL);
}

#ifdef __ARM_NEON
static uint8x8_t BlendOverlayChannels(uint8x8_t source, uint8x8_t dest, uint8x8_t srcAlpha, uint8x8_t dstAlpha) {
    uint16x8_t sum = vmlal_u8(vmull_u8(source, srcAlpha), dest, dstAlpha);
    /* Exact floor(sum / 255), including the range endpoints. */
    sum = vaddq_u16(sum, vdupq_n_u16(1));
    return vshrn_n_u16(vaddq_u16(sum, vshrq_n_u16(sum, 8)), 8);
}
#endif

/* Four adjacent output pixels from one ARGB source pixel, over an XRGB
 * surface. The caller has already clipped to complete source pixels. */
bool PortFbBlendOverlay4x(SDL_Renderer* renderer, const uint32_t* source, int pitch, const SDL_Rect* dest) {
#ifdef __ARM_NEON
    SDL_Rect viewport;
    float scaleX, scaleY;
    int logicalW, logicalH;
    SDL_RendererLogicalPresentation logicalMode;
    if (!sRenderSurface || sRenderSurface->w != 640 || sRenderSurface->h != 480 ||
        dest->x < 0 || dest->y < 0 || dest->w <= 0 || dest->h <= 0 ||
        dest->w > 640 - dest->x || dest->h > 480 - dest->y ||
        ((dest->w | dest->h) & 3) || SDL_MUSTLOCK(sRenderSurface) ||
        SDL_GetRenderTarget(renderer) || SDL_RenderClipEnabled(renderer) ||
        !SDL_GetRenderViewport(renderer, &viewport) || viewport.x || viewport.y ||
        viewport.w != 640 || viewport.h != 480 ||
        !SDL_GetRenderScale(renderer, &scaleX, &scaleY) || scaleX != 1 || scaleY != 1 ||
        !SDL_GetRenderLogicalPresentation(renderer, &logicalW, &logicalH, &logicalMode) ||
        logicalMode != SDL_LOGICAL_PRESENTATION_DISABLED || !SDL_FlushRenderer(renderer))
        return false;

    uint8_t* const surface = sRenderSurface->pixels;
    const int targetPitch = sRenderSurface->pitch;
    for (int y = 0; y < dest->h / 4; y++) {
        uint8_t* rows = surface + (dest->y + 4 * y) * targetPitch + 4 * dest->x;
        for (int x = 0; x < dest->w / 4; x++) {
            const uint32_t pixel = source[x];
            const uint8_t alpha = (uint8_t)(pixel >> 24);
            if (!alpha)
                continue;
            const uint32x4_t repeated = vdupq_n_u32(pixel);
            for (int row = 0; row < 4; row++) {
                uint32_t* target = (uint32_t*)(rows + row * targetPitch) + 4 * x;
                if (alpha == 255) {
                    vst1q_u32(target, repeated);
                } else {
                    const uint8x16_t old = vreinterpretq_u8_u32(vld1q_u32(target));
                    const uint8x8_t color = vget_low_u8(vreinterpretq_u8_u32(repeated));
                    const uint8x8_t alphaIn = vdup_n_u8(alpha);
                    const uint8x8_t alphaOut = vdup_n_u8(255 - alpha);
                    const uint8x8_t lo = BlendOverlayChannels(color, vget_low_u8(old), alphaIn, alphaOut);
                    const uint8x8_t hi = BlendOverlayChannels(color, vget_high_u8(old), alphaIn, alphaOut);
                    vst1q_u32(target, vreinterpretq_u32_u8(vcombine_u8(lo, hi)));
                }
            }
        }
        source += pitch;
    }
    return true;
#else
    (void)renderer;
    (void)source;
    (void)pitch;
    (void)dest;
    return false;
#endif
}

bool PortFbBeginGlyphBatch(SDL_Renderer* renderer) {
    SDL_Rect viewport;
    float sx, sy;
    int logicalW, logicalH;
    SDL_RendererLogicalPresentation logicalMode;
    return sRenderSurface && sRenderSurface->w == 640 && sRenderSurface->h == 480 &&
        !SDL_MUSTLOCK(sRenderSurface) && !SDL_GetRenderTarget(renderer) && !SDL_RenderClipEnabled(renderer) &&
        SDL_GetRenderViewport(renderer, &viewport) && !viewport.x && !viewport.y &&
        viewport.w == 640 && viewport.h == 480 &&
        SDL_GetRenderScale(renderer, &sx, &sy) && sx == 1.f && sy == 1.f &&
        SDL_GetRenderLogicalPresentation(renderer, &logicalW, &logicalH, &logicalMode) &&
        logicalMode == SDL_LOGICAL_PRESENTATION_DISABLED && SDL_FlushRenderer(renderer);
}

/* The fixed 480p dialogue plane already fits the viewport at integer 2x. */
bool PortFbBlendOverlay2x(SDL_Renderer* renderer, const uint32_t* source, int stride, const SDL_Rect* dest) {
#ifdef __ARM_NEON
    if (dest->x < 0 || dest->y < 0 || dest->w <= 0 || dest->h <= 0 ||
        dest->w > 640 - dest->x || dest->h > 480 - dest->y || ((dest->w | dest->h) & 1) ||
        !PortFbBeginGlyphBatch(renderer))
        return false;

    uint8_t* pixels = sRenderSurface->pixels;
    const int pitch = sRenderSurface->pitch;
    for (int y = 0; y < dest->h / 2; y++, source += stride) {
        uint32_t* upper = (uint32_t*)(pixels + (dest->y + y * 2) * pitch) + dest->x;
        uint32_t* lower = (uint32_t*)((uint8_t*)upper + pitch);
        for (int x = 0; x < dest->w / 2; x++) {
            const uint32_t pixel = source[x];
            const unsigned alpha = pixel >> 24;
            if (!alpha)
                continue;
            const uint32x2_t color = vdup_n_u32(pixel);
            if (alpha == 255) {
                vst1_u32(upper + x * 2, color);
                vst1_u32(lower + x * 2, color);
            } else {
                const uint8x8_t rgb = vreinterpret_u8_u32(color);
                const uint8x8_t in = vdup_n_u8((uint8_t)alpha), out = vmvn_u8(in);
                const uint8x8_t a = vreinterpret_u8_u32(vld1_u32(upper + x * 2));
                const uint8x8_t b = vreinterpret_u8_u32(vld1_u32(lower + x * 2));
                vst1_u32(upper + x * 2, vreinterpret_u32_u8(BlendOverlayChannels(rgb, a, in, out)));
                vst1_u32(lower + x * 2, vreinterpret_u32_u8(BlendOverlayChannels(rgb, b, in, out)));
            }
        }
    }
    return true;
#else
    (void)renderer; (void)source; (void)stride; (void)dest;
    return false;
#endif
}

static uint32_t ScaleGlyphRgb(uint32_t rgb, unsigned factor) {
    const uint32_t rb = 0x00FF00FFu;
    uint32_t redBlue = (rgb & rb) * factor + 0x00010001u;
    uint32_t green = ((rgb >> 8) & 255u) * factor + 1;
    redBlue += (redBlue >> 8) & rb;
    green += green >> 8;
    return ((redBlue >> 8) & rb) | (green & 0xFF00u);
}

static uint32_t BlendGlyphPixel(uint32_t rgb, uint32_t dst, unsigned opacity) {
    /* The software blitter rounds each term separately when tinting.
       White glyphs take its unmodulated, combined-sum blend instead. */
    if (rgb != 0x00FFFFFFu)
        return ScaleGlyphRgb(rgb, opacity) + ScaleGlyphRgb(dst, 255 - opacity);
    const uint32_t rb = 0x00FF00FFu;
    const uint32_t inv = 255 - opacity;
    uint32_t redBlue = (rgb & rb) * opacity + (dst & rb) * inv + 0x00010001u;
    uint32_t green = ((rgb >> 8) & 255u) * opacity + ((dst >> 8) & 255u) * inv + 1;
    redBlue += (redBlue >> 8) & rb;
    green += green >> 8;
    return ((redBlue >> 8) & rb) | (green & 0xFF00u);
}

/* Candidate for the handheld's fractional menu overlay. SDL's software
   nearest blitter uses 16.16 pixel-centre sampling and separately rounded
   source/destination alpha terms. Keep the same calculation here. */
bool PortFbBlendOverlayScaled(SDL_Renderer* renderer, const uint32_t* source, int width, int height,
                              const SDL_Rect* dest) {
    uint16_t xMap[640];
    uint8_t repeats[240];
    static int grouped = -1;
    if (grouped < 0) grouped = SDL_getenv("KHCOM_FBDEV_OVERLAY_240_RUNS") != NULL;
    if (!source || width <= 0 || height <= 0 || dest->x < 0 || dest->y < 0 ||
        dest->w <= 0 || dest->h <= 0 || dest->x + dest->w > 640 || dest->y + dest->h > 480 ||
        !PortFbBeginGlyphBatch(renderer)) return false;
    static bool logged;
    if (!logged) { PortLog("fbdev: direct fractional overlay active"); logged = true; }
    const uint64_t dx = ((uint64_t)width << 16) / dest->w, dy = ((uint64_t)height << 16) / dest->h;
    uint64_t pos = dx / 2, py = dy / 2;
    for (int x = 0; x < dest->w; x++, pos += dx) xMap[x] = (uint16_t)(pos >> 16);
    bool runs = grouped && width == 240 && dest->w == 640;
    if (runs) {
        memset(repeats, 0, sizeof(repeats));
        for (int x = 0; x < dest->w; x++) repeats[xMap[x]]++;
        for (int x = 0; x < 240; x++) if (repeats[x] < 2 || repeats[x] > 3) runs = false;
        if (runs) { static bool logged; if (!logged) { PortLog("fbdev: overlay 240 runs active"); logged = true; } }
    }
    for (int y = 0; y < dest->h; y++, py += dy) {
        const uint32_t* src = source + (py >> 16) * width;
        uint32_t* dst = (uint32_t*)((uint8_t*)sRenderSurface->pixels + (dest->y + y) * sRenderSurface->pitch) + dest->x;
        if (runs) {
            int x = 0;
            for (int sx = 0; sx < 240; sx++) {
                const uint32_t pixel = src[sx];
                const unsigned alpha = pixel >> 24, count = repeats[sx];
                if (alpha == 255) {
                    dst[x] = dst[x + 1] = pixel;
                    if (count == 3) dst[x + 2] = pixel;
                } else if (alpha) {
                    const uint32_t fg = ScaleGlyphRgb(pixel, alpha);
                    for (unsigned j = 0; j < count; j++) dst[x + j] = fg + ScaleGlyphRgb(dst[x + j], 255 - alpha);
                }
                x += (int)count;
            }
            continue;
        }
        for (int x = 0; x < dest->w; x++) {
            const uint32_t pixel = src[xMap[x]];
            const unsigned alpha = pixel >> 24;
            if (alpha == 255) dst[x] = pixel;
            else if (alpha) dst[x] = ScaleGlyphRgb(pixel, alpha) + ScaleGlyphRgb(dst[x], 255 - alpha);
        }
    }
    return true;
}

/* Preserve SDL's pixel-centre mapping and alpha arithmetic, but avoid
 * visiting transparent ends (and entire empty rows) of sparse menu layers. */
bool PortFbBlendOverlayScaledSparse(SDL_Renderer* renderer, const uint32_t* source, int width, int height,
                                    const SDL_Rect* dest) {
    uint16_t xMap[640];
    if (!source || width <= 0 || height <= 0 || dest->x < 0 || dest->y < 0 ||
        dest->w <= 0 || dest->h <= 0 || dest->x + dest->w > 640 || dest->y + dest->h > 480 ||
        !PortFbBeginGlyphBatch(renderer)) return false;
    const uint64_t dx = ((uint64_t)width << 16) / dest->w, dy = ((uint64_t)height << 16) / dest->h;
    uint64_t pos = dx / 2, py = dy / 2;
    int previous = -1, begin = 0, end = 0;
    for (int x = 0; x < dest->w; x++, pos += dx) xMap[x] = (uint16_t)(pos >> 16);
    for (int y = 0; y < dest->h; y++, py += dy) {
        const int sy = (int)(py >> 16);
        const uint32_t* src = source + sy * width;
        if (sy != previous) {
            int left = 0, right = width, lo, hi;
            while (left < right && !(src[left] >> 24)) left++;
            while (right > left && !(src[right - 1] >> 24)) right--;
            lo = 0; hi = dest->w;
            while (lo < hi) { int mid = (lo + hi) / 2; if (xMap[mid] < left) lo = mid + 1; else hi = mid; }
            begin = lo;
            lo = begin; hi = dest->w;
            while (lo < hi) { int mid = (lo + hi) / 2; if (xMap[mid] < right) lo = mid + 1; else hi = mid; }
            end = lo;
            previous = sy;
        }
        uint32_t* dst = (uint32_t*)((uint8_t*)sRenderSurface->pixels + (dest->y + y) * sRenderSurface->pitch) + dest->x;
        for (int x = begin; x < end; x++) {
            const uint32_t pixel = src[xMap[x]];
            const unsigned alpha = pixel >> 24;
            if (alpha == 255) dst[x] = pixel;
            else if (alpha) dst[x] = ScaleGlyphRgb(pixel, alpha) + ScaleGlyphRgb(dst[x], 255 - alpha);
        }
    }
    return true;
}

void PortFbDrawGlyphMaskClipped(const uint8_t* alpha, int x, int y, int w, int h, uint32_t rgb, const SDL_Rect* clip) {
    const int left = SDL_max(SDL_max(x, 0), clip ? clip->x : 0);
    const int right = SDL_min(SDL_min(x + w, 640), clip ? clip->x + clip->w : 640);
    const int top = SDL_max(SDL_max(y, 0), clip ? clip->y : 0);
    const int bottom = SDL_min(SDL_min(y + h, 480), clip ? clip->y + clip->h : 480);
    if (left >= right || top >= bottom)
        return;
    static bool logged;
    if (!logged) {
        PortLog("hd-text: direct alpha glyph active");
        logged = true;
    }
    rgb &= 0x00FFFFFFu;
    for (int row = top; row < bottom; row++) {
        uint32_t* dst = (uint32_t*)((uint8_t*)sRenderSurface->pixels + row * sRenderSurface->pitch);
        const uint8_t* coverage = alpha + (row - y) * w + left - x;
        for (int col = left; col < right; col++) {
            const unsigned opacity = *coverage++;
            if (opacity)
                dst[col] = opacity == 255 ? rgb : BlendGlyphPixel(rgb, dst[col], opacity);
        }
    }
}

void PortFbDrawGlyphMask(const uint8_t* alpha, int x, int y, int w, int h, uint32_t rgb) {
    PortFbDrawGlyphMaskClipped(alpha, x, y, w, h, rgb, NULL);
}

static void CopyRows(uint8_t* dst, const SDL_Surface* src, int h, size_t rowBytes) {
    /* The visible region is contiguous only when neither surface has row padding. */
    if (rowBytes == sFix.line_length && rowBytes == (size_t)src->pitch) {
        memcpy(dst, src->pixels, (size_t)h * rowBytes);
        return;
    }
    for (int y = 0; y < h; y++)
        memcpy(dst + (size_t)y * sFix.line_length, (const uint8_t*)src->pixels + (size_t)y * src->pitch, rowBytes);
}

static int SDLCALL FbCopyWorker(void* unused) {
    (void)unused;
    for (;;) {
        SDL_WaitSemaphore(sAsyncFbGo);
        if (sAsyncFbStopping)
            return 0;
        CopyRows(sAsyncFbJob.destination, sAsyncFbJob.source, sAsyncFbJob.height, sAsyncFbJob.rowBytes);
        SDL_SignalSemaphore(sAsyncFbDone);
    }
}

static bool StartFbCopyWorker(void) {
    if (sAsyncFbThread)
        return true;
    sAsyncFbGo = SDL_CreateSemaphore(0);
    sAsyncFbDone = SDL_CreateSemaphore(0);
    if (sAsyncFbGo && sAsyncFbDone)
        sAsyncFbThread = SDL_CreateThread(FbCopyWorker, "fbdev copy", NULL);
    if (!sAsyncFbThread) {
        SDL_DestroySemaphore(sAsyncFbGo);
        SDL_DestroySemaphore(sAsyncFbDone);
        sAsyncFbGo = sAsyncFbDone = NULL;
        sAsyncFbRequested = false;
        return false;
    }
    PortLog("fbdev: asynchronous panel copy active");
    return true;
}

static SDL_PixelFormat FbFormat(void) {
    if (sVar.bits_per_pixel == 16)
        return SDL_PIXELFORMAT_RGB565;
    return sVar.red.offset == 0 ? SDL_PIXELFORMAT_XBGR8888 : SDL_PIXELFORMAT_XRGB8888;
}

/* Sample the visible aperture before the next frame overwrites it. */
bool PortFbCapturePair(const char* framebufferName, const char* rendererName) {
    PortFbWaitCopy();
    if (!sFb || !sRenderSurface)
        return false;
    const SDL_PixelFormat format = FbFormat();
    SDL_Surface* snapshot = SDL_CreateSurface((int)sVar.xres, (int)sVar.yres, format);
    if (!snapshot)
        return false;
    const unsigned rowBytes = sVar.xres * (sVar.bits_per_pixel / 8);
    for (int y = 0; y < snapshot->h; y++)
        memcpy((uint8_t*)snapshot->pixels + (size_t)y * snapshot->pitch,
               sFb + sFbOffset + (size_t)y * sFix.line_length, rowBytes);
    const bool saved = SDL_SaveBMP(snapshot, PortConfigPath(framebufferName));
    SDL_DestroySurface(snapshot);
    if (!saved || !SDL_SaveBMP(sRenderSurface, PortConfigPath(rendererName)))
        PortLog("fbdev: could not save paired capture: %s", SDL_GetError());
    return true;
}

static void WriteFrame(SDL_Surface* frame) {
    const SDL_PixelFormat format = FbFormat();
    const int w = SDL_min(frame->w, (int)sVar.xres);
    const int h = SDL_min(frame->h, (int)sVar.yres);
    const unsigned bpp = sVar.bits_per_pixel / 8;
    if (w <= 0 || h <= 0)
        return;
    uint8_t* dst = sFb + sFbOffset;
    if (frame->pixels && !SDL_MUSTLOCK(frame) && SDL_GetSurfaceColorspace(frame) == SDL_COLORSPACE_SRGB &&
        !SDL_SurfaceHasColorKey(frame)) {
        if (frame->format == format) {
            /* SDL_ConvertPixels can copy the full pitch for equal formats. */
            CopyRows(dst, frame, h, (size_t)w * bpp);
            return;
        }
        /* Common software readback formats can go straight to aligned fbdev rows. */
        const bool packed = frame->format == SDL_PIXELFORMAT_RGB565 ||
            (SDL_ISPIXELFORMAT_PACKED(frame->format) && SDL_PIXELLAYOUT(frame->format) == SDL_PACKEDLAYOUT_8888);
        if (packed && !(sFix.line_length % bpp) &&
            SDL_ConvertPixels(w, h, frame->format, frame->pixels, frame->pitch, format, dst, (int)sFix.line_length))
            return;
    }
    SDL_Surface* conv = SDL_ConvertSurface(frame, format);
    if (conv) {
        CopyRows(dst, conv, h, (size_t)w * bpp);
        SDL_DestroySurface(conv);
    }
}

/* Copies the renderer's frame to the framebuffer (call before presenting). */
void PortFbPresent(SDL_Renderer* renderer) {
    sDrawTicks = sCopyTicks = 0;
    if (!sFb)
        return;
    uint64_t start = sProfileFb ? SDL_GetPerformanceCounter() : 0;
    if (sRenderSurface) {
        PortFbWaitCopy();
        if (SDL_FlushRenderer(renderer)) {
            uint64_t ready = sProfileFb ? SDL_GetPerformanceCounter() : 0;
            if (sAsyncFbRequested && sRenderSurface->format == FbFormat() &&
                sRenderSurface->w == (int)sVar.xres && sRenderSurface->h == (int)sVar.yres &&
                !SDL_MUSTLOCK(sRenderSurface) && StartFbCopyWorker()) {
                sAsyncFbJob.destination = sFb + sFbOffset;
                sAsyncFbJob.source = sRenderSurface;
                sAsyncFbJob.height = sRenderSurface->h;
                sAsyncFbJob.rowBytes = (size_t)sRenderSurface->w * (sVar.bits_per_pixel / 8);
                sAsyncFbPending = true;
                SDL_SignalSemaphore(sAsyncFbGo);
                if (sProfileFb)
                    sDrawTicks = ready - start;
                return;
            }
            WriteFrame(sRenderSurface);
            if (sProfileFb) {
                sDrawTicks = ready - start;
                sCopyTicks = SDL_GetPerformanceCounter() - ready;
            }
        }
        return;
    }
    SDL_Surface* frame = SDL_RenderReadPixels(renderer, NULL);
    if (frame) {
        uint64_t ready = sProfileFb ? SDL_GetPerformanceCounter() : 0;
        WriteFrame(frame);
        if (sProfileFb) {
            sDrawTicks = ready - start;
            sCopyTicks = SDL_GetPerformanceCounter() - ready;
        }
        SDL_DestroySurface(frame);
    }
}
