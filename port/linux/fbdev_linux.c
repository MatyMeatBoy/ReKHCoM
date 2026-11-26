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
#include "fb_fit_row.h"
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
    {
        /* the panel's refresh from its timing (frame pacing: the game runs at 59.7275 Hz) */
        const uint64_t htotal = (uint64_t)sVar.xres + sVar.left_margin + sVar.right_margin + sVar.hsync_len;
        const uint64_t vtotal = (uint64_t)sVar.yres + sVar.upper_margin + sVar.lower_margin + sVar.vsync_len;
        if (sVar.pixclock && htotal && vtotal)
            PortLog("fbdev: panel %.3f Hz (pixclock %u ps, %llux%llu total)",
                    1e12 / ((double)sVar.pixclock * (double)htotal * (double)vtotal), sVar.pixclock,
                    (unsigned long long)htotal, (unsigned long long)vtotal);
        else
            PortLog("fbdev: panel refresh unknown (no timing in the screen info)");
    }
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

bool PortFbBeginGlyphBatch(SDL_Renderer* renderer);

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

/* KHCOM_FBDEV_PARALLEL_PRESENT: the per-row presentation kernels are split
 * by source row among this thread and the PPU workers, idle at this point.
 * Each band writes only its own output rows and reads the finished frame,
 * so the result is the same as the single-thread loop. */
int PortPpuParallelRows(void (*fn)(void* ctx, int begin, int end), void* ctx, int count, int chunk);

static bool ParallelPresent(void) {
    static int enabled = -1;
    if (enabled < 0) {
        enabled = SDL_getenv("KHCOM_FBDEV_PARALLEL_PRESENT") != NULL;
        if (enabled) PortLog("fbdev: presentation rows on the PPU workers");
    }
    return enabled;
}

/* First output row that samples source row s or a later one, for the 16.16
 * pixel-centre walk pos(y) = first + y * step. */
static int FirstOutputRow(unsigned s, uint64_t first, uint64_t step, int outH) {
    const uint64_t target = (uint64_t)s << 16;
    if (target <= first)
        return 0;
    const uint64_t y = (target - first + step - 1) / step;
    return y < (uint64_t)outH ? (int)y : outH;
}

#ifdef __ARM_NEON
typedef struct {
    uint8_t* pixels;
    int pitch, outH, stride;
    unsigned stepY;
    const uint32_t* src;
    const uint8_t* hiddenRows;
    bool rowCopy;
} FitJob;

/* Same grouping as the serial fit loop, started at a source-row boundary. */
static void FitRowsBand(void* ctx, int begin, int end) {
    const FitJob* job = ctx;
    int y = FirstOutputRow((unsigned)begin, job->stepY / 2, job->stepY, job->outH);
    unsigned posY = job->stepY / 2 + (unsigned)y * job->stepY;
    while (y < job->outH) {
        const unsigned row = posY >> 16;
        if ((int)row >= end)
            break;
        uint32_t* rows[3];
        int count = 0;
        do {
            rows[count++] = (uint32_t*)(job->pixels + (size_t)y * job->pitch);
            y++;
            posY += job->stepY;
        } while (count < 3 && y < job->outH && (posY >> 16) == row);
        if (job->hiddenRows && job->hiddenRows[row])
            continue;
        if (job->rowCopy)
            FbFitRows240To640Copy(rows, count, job->src + (size_t)row * job->stride);
        else
            FbFitRow240To640(rows, count, job->src + (size_t)row * job->stride);
    }
}

typedef struct {
    uint8_t* row;
    int pitch, stride;
    const uint32_t* src;
} Allmap2xJob;

static void Allmap2xBand(void* ctx, int begin, int end) {
    const Allmap2xJob* job = ctx;
    for (int y = begin; y < end; y++) {
        uint8_t* row = job->row + (size_t)(2 * y) * job->pitch;
        FbDoubleRow((uint32_t*)row, job->src + (size_t)y * job->stride, 240);
        memcpy(row + job->pitch, row, 480 * sizeof(uint32_t));
    }
}
#endif

/* The handheld's opaque nearest 2x case can write the software renderer's
 * surface directly. Other geometry and renderer paths stay with SDL. */
static int DrawNearest2x(SDL_Renderer* renderer, const uint32_t* src, int w, int h, int stride, int outY, int outH, const SDL_Rect* cover, const uint8_t* hiddenRows) {
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
        static int fitNeon = -1;
        if (fitNeon < 0) {
            fitNeon = SDL_getenv("KHCOM_FBDEV_FIT_NEON") != NULL;
            if (fitNeon) PortLog("fbdev: NEON fit rows active");
        }
        if (fitNeon) {
            static int rowCopy = -1;
            if (rowCopy < 0) {
                rowCopy = SDL_getenv("KHCOM_FBDEV_FIT_ROWCOPY") != NULL;
                if (rowCopy) PortLog("fbdev: fit rows copied in sequence");
            }
            if (ParallelPresent()) {
                FitJob job = { pixels, pitch, outH, stride, stepY, src, hiddenRows, rowCopy != 0 };
                if (PortPpuParallelRows(FitRowsBand, &job, 160, 16))
                    return 1;
            }
            /* Each source row feeds two or three output rows: store them
             * together instead of reading the first one back. */
            for (int y = 0; y < outH;) {
                const unsigned row = posY >> 16;
                uint32_t* rows[3];
                int count = 0;
                do {
                    rows[count++] = (uint32_t*)(pixels + (size_t)y * pitch);
                    y++;
                    posY += stepY;
                } while (count < 3 && y < outH && (posY >> 16) == row);
                if (hiddenRows && hiddenRows[row])
                    continue;
                if (rowCopy)
                    FbFitRows240To640Copy(rows, count, src + (size_t)row * stride);
                else
                    FbFitRow240To640(rows, count, src + (size_t)row * stride);
            }
            return 1;
        }
        for (int y = 0; y < outH; y++, posY += stepY) {
            const unsigned row = posY >> 16;
            uint32_t* dest = (uint32_t*)(pixels + (size_t)y * pitch);
            if (hiddenRows && hiddenRows[row])
                continue;
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
    static int rowCopy2x = -1;
    if (rowCopy2x < 0) {
        rowCopy2x = SDL_getenv("KHCOM_FBDEV_2X_ROWCOPY") != NULL;
        if (rowCopy2x) PortLog("fbdev: 2x rows copied in sequence");
    }
    if (rowCopy2x) {
        for (int y = 0; y < h; y++, src += stride) {
            uint32_t* upper = (uint32_t*)(pixels + (size_t)(2 * y) * pitch);
            FbDoubleRow(upper, src, w);
            memcpy((uint8_t*)upper + pitch, upper, (size_t)w * 2 * sizeof(*upper));
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
    (void)renderer; (void)src; (void)w; (void)h; (void)stride; (void)outY; (void)outH; (void)cover; (void)hiddenRows;
    return 0;
#endif
}

int PortFbDrawNearest2x(SDL_Renderer* renderer, const uint32_t* src, int w, int h, int outH, const SDL_Rect* cover) {
    return sRenderSurface && sRenderSurface->h == outH && DrawNearest2x(renderer, src, w, h, w, 0, outH, cover, NULL);
}

int PortFbDrawFit2x(SDL_Renderer* renderer, const uint32_t* src, int w, int h, const SDL_FRect* dest) {
    return sRenderSurface && sRenderSurface->w == 640 && sRenderSurface->h == 480 &&
           dest->x == 0 && dest->w == 640 &&
           DrawNearest2x(renderer, src, w, h, w, (int)dest->y, (int)dest->h, NULL, NULL);
}

int PortFbDrawWorldInspectFit(SDL_Renderer* renderer, const uint32_t* src, int stride) {
    int drawn = sRenderSurface && sRenderSurface->w == 640 && sRenderSurface->h == 480 &&
                DrawNearest2x(renderer, src, 240, 160, stride, 26, 426, NULL, NULL);
    if (drawn) {
        static bool logged;
        if (!logged) { PortLog("fbdev: world inspect fit active"); logged = true; }
    }
    return drawn;
}

/* The 4:3 world map at exactly 2x, centred: 480x320 at (80, 80). */
int PortFbDrawAllmap2x(SDL_Renderer* renderer, const uint32_t* src, int stride) {
#ifdef __ARM_NEON
    if (!PortFbBeginGlyphBatch(renderer) || sRenderSurface->format != SDL_PIXELFORMAT_XRGB8888)
        return 0;
    const int pitch = sRenderSurface->pitch;
    uint8_t* row = (uint8_t*)sRenderSurface->pixels + 80 * pitch + 80 * sizeof(uint32_t);
    Allmap2xJob job = { row, pitch, stride, src };
    if (!ParallelPresent() || !PortPpuParallelRows(Allmap2xBand, &job, 160, 16))
        Allmap2xBand(&job, 0, 160);
    static bool logged;
    if (!logged) { PortLog("fbdev: allmap direct 2x active"); logged = true; }
    return 1;
#else
    (void)renderer; (void)src; (void)stride;
    return 0;
#endif
}

/* Any GBA-size menu picture fitted to the 640x480 panel's width. */
/* Rows of the 640x480 panel moved, repeated or filled in place (the world
   map's header and footer, after the menu fit drew its picture). Call after
   PortFbBeginGlyphBatch. */
void PortFbMoveRows(int dstY, int srcY, int count) {
    const int pitch = sRenderSurface->pitch;
    uint8_t* base = sRenderSurface->pixels;
    if (count <= 0 || dstY == srcY || dstY < 0 || srcY < 0 || dstY + count > 480 || srcY + count > 480)
        return;
    if (dstY > srcY)
        for (int i = count - 1; i >= 0; i--) memcpy(base + (size_t)(dstY + i) * pitch, base + (size_t)(srcY + i) * pitch, 640 * 4);
    else
        for (int i = 0; i < count; i++) memcpy(base + (size_t)(dstY + i) * pitch, base + (size_t)(srcY + i) * pitch, 640 * 4);
}

void PortFbRepeatRow(int srcY, int dstY, int count) {
    const int pitch = sRenderSurface->pitch;
    uint8_t* base = sRenderSurface->pixels;
    for (int i = 0; i < count && srcY >= 0 && srcY < 480; i++)
        if (dstY + i >= 0 && dstY + i < 480 && dstY + i != srcY)
            memcpy(base + (size_t)(dstY + i) * pitch, base + (size_t)srcY * pitch, 640 * 4);
}

void PortFbFillRect(int x, int y, int w, int h, uint32_t color) {
    const int left = SDL_max(x, 0), right = SDL_min(x + w, 640), top = SDL_max(y, 0), bottom = SDL_min(y + h, 480);
    for (int row = top; row < bottom; row++) {
        uint32_t* dst = (uint32_t*)((uint8_t*)sRenderSurface->pixels + (size_t)row * sRenderSurface->pitch);
        for (int col = left; col < right; col++) dst[col] = color;
    }
}

int PortFbDrawMenuFit(SDL_Renderer* renderer, const uint32_t* src, int stride, const uint8_t* hiddenRows) {
    int drawn = sRenderSurface && sRenderSurface->w == 640 && sRenderSurface->h == 480 &&
                DrawNearest2x(renderer, src, 240, 160, stride, 26, 426, NULL, hiddenRows);
    if (drawn) {
        static bool logged;
        if (!logged) { PortLog("fbdev: menu fit active"); logged = true; }
    }
    return drawn;
}

int PortFbDrawJournalFit(SDL_Renderer* renderer, const uint32_t* src, int stride) {
    return sRenderSurface && sRenderSurface->w == 640 && sRenderSurface->h == 480 &&
           DrawNearest2x(renderer, src, 240, 160, stride, 8, 426, NULL, NULL);
}

#ifdef __ARM_NEON
static uint8x8_t BlendOverlayChannels(uint8x8_t source, uint8x8_t dest, uint8x8_t srcAlpha, uint8x8_t dstAlpha) {
    uint16x8_t sum = vmlal_u8(vmull_u8(source, srcAlpha), dest, dstAlpha);
    /* Exact floor(sum / 255), including the range endpoints. */
    sum = vaddq_u16(sum, vdupq_n_u16(1));
    return vshrn_n_u16(vaddq_u16(sum, vshrq_n_u16(sum, 8)), 8);
}
#endif

static uint32_t ScaleGlyphRgb(uint32_t rgb, unsigned factor);

/* Output rows that sample the same source row are blended together: one
 * alpha test per source pixel for the group, transparent quads skipped at once
 * and opaque spans expanded once and copied. Same arithmetic as the row loops. */
typedef struct {
    const uint32_t* source;
    const uint16_t* first;
    int width, outH, pitch;
    uint8_t* base;
    uint64_t dy;
} OverlayGroupsJob;

static void OverlayGroupsBand(void* ctx, int begin, int end);

static void BlendOverlayRowGroups(const uint32_t* source, int width, int height, const SDL_Rect* dest) {
    uint16_t first[241]; /* first output column of each source column */
    const uint64_t dx = ((uint64_t)width << 16) / dest->w, dy = ((uint64_t)height << 16) / dest->h;
    uint64_t pos = dx / 2;
    for (int sx = 0, x = 0; sx <= width; sx++) {
        while (x < dest->w && (int)(pos >> 16) < sx) { x++; pos += dx; }
        first[sx] = (uint16_t)x;
    }
    static bool logged;
    if (!logged) { PortLog("fbdev: overlay row groups active"); logged = true; }
    OverlayGroupsJob job = { source, first, width, dest->h, sRenderSurface->pitch,
        (uint8_t*)sRenderSurface->pixels + dest->y * sRenderSurface->pitch + 4 * dest->x, dy };
    if (!ParallelPresent() || !PortPpuParallelRows(OverlayGroupsBand, &job, height, 8))
        OverlayGroupsBand(&job, 0, height);
}

static void OverlayGroupsBand(void* ctx, int begin, int end) {
    const OverlayGroupsJob* job = ctx;
    const uint32_t* source = job->source;
    const uint16_t* first = job->first;
    const int width = job->width, pitch = job->pitch;
    const uint64_t dy = job->dy;
    uint8_t* base = job->base;
    int y = FirstOutputRow((unsigned)begin, dy / 2, dy, job->outH);
    uint64_t py = dy / 2 + (uint64_t)y * dy;
    while (y < job->outH) {
        const int sy = (int)(py >> 16);
        if (sy >= end)
            break;
        uint32_t* rows[4];
        int count = 0;
        do {
            rows[count++] = (uint32_t*)(base + (size_t)y * pitch);
            y++;
            py += dy;
        } while (count < 4 && y < job->outH && (int)(py >> 16) == sy);
        const uint32_t* src = source + sy * width;
        for (int sx = 0; sx < width;) {
            if (sx + 4 <= width && !((src[sx] | src[sx + 1] | src[sx + 2] | src[sx + 3]) >> 24)) {
                sx += 4;
                continue;
            }
            const uint32_t pixel = src[sx];
            const unsigned alpha = pixel >> 24;
            if (alpha == 255) {
                const int start = first[sx];
                int x = start;
                do {
                    const uint32_t color = src[sx];
                    while (x < first[sx + 1]) rows[0][x++] = color;
                    sx++;
                } while (sx < width && (src[sx] >> 24) == 255);
                for (int r = 1; r < count; r++)
                    memcpy(rows[r] + start, rows[0] + start, (size_t)(x - start) * sizeof(**rows));
                continue;
            }
            if (alpha) {
                const uint32_t fg = ScaleGlyphRgb(pixel, alpha);
                for (int r = 0; r < count; r++)
                    for (int x = first[sx]; x < first[sx + 1]; x++) {
                        const uint32_t under = rows[r][x];
                        rows[r][x] = (under & 0x00FFFFFFu) ? fg + ScaleGlyphRgb(under, 255 - alpha) : fg;
                    }
            }
            sx++;
        }
    }
}

static bool UseOverlayRowGroups(int width, const SDL_Rect* dest) {
    static int enabled = -1;
    if (enabled < 0) enabled = SDL_getenv("KHCOM_FBDEV_OVERLAY_ROW_GROUPS") != NULL;
    return enabled && width == 240 && dest->w == 640;
}

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

/* A premultiplied ARGB picture's covered spans (first/last column per row, -1 none) over the framebuffer at (x, y):
   c = s + d (1 - a). The HD gauge's cache: SDL's premultiplied copies of its row bands cost more than the blend. */
bool PortFbBlendPremultipliedSpans(SDL_Renderer* renderer, const uint32_t* src, int pitch, const int16_t* spans,
                                   int x, int y, int w, int h) {
    if (!src || !spans || !PortFbBeginGlyphBatch(renderer) || sRenderSurface->format != SDL_PIXELFORMAT_XRGB8888)
        return false;
    for (int r = 0; r < h; r++) {
        const int sy = y + r;
        int first = spans[r * 2], last = spans[r * 2 + 1];
        if (first < 0 || sy < 0 || sy >= sRenderSurface->h) continue;
        if (x + first < 0) first = -x;
        if (x + last >= sRenderSurface->w) last = sRenderSurface->w - 1 - x;
        if (last >= w) last = w - 1;
        const uint32_t* in = src + (size_t)r * pitch;
        uint32_t* out = (uint32_t*)((uint8_t*)sRenderSurface->pixels + (size_t)sy * sRenderSurface->pitch) + x;
        for (int c = first; c <= last; c++) {
            const uint32_t p = in[c];
            const unsigned a = p >> 24;
            if (!p) continue;
            if (a == 255) { out[c] = p; continue; }
            const uint32_t d = out[c];
            const unsigned inv = 255 - a;
            unsigned rr = ((d >> 16) & 255) * inv, gg = ((d >> 8) & 255) * inv, bb = (d & 255) * inv;
            rr = ((rr + 1 + (rr >> 8)) >> 8) + ((p >> 16) & 255);
            gg = ((gg + 1 + (gg >> 8)) >> 8) + ((p >> 8) & 255);
            bb = ((bb + 1 + (bb >> 8)) >> 8) + (p & 255);
            out[c] = 0xFF000000u | (SDL_min(rr, 255u) << 16) | (SDL_min(gg, 255u) << 8) | SDL_min(bb, 255u);
        }
    }
    return true;
}

/* A sparse ARGB layer (straight alpha) scaled by nearest pixel to a box that may reach past the 640x480 framebuffer
   and need not sit on whole pixels (the HD title: the 320x240 canvas at 2.67x, from -107,-81): only the box's part on
   the screen, rows and columns of the layer that hold nothing skipped. SDL scaled the whole box, about 11 million
   instructions a frame on GA36 (QEMU). */
bool PortFbBlendOverlayNearestClipped(SDL_Renderer* renderer, const uint32_t* src, int w, int h, float bx, float by,
                                      float bw, float bh) {
    static int16_t first[1024], last[1024];
    static int16_t colOf[640];
    if (!src || w <= 0 || h <= 0 || w > 1024 || h > 1024 || bw <= 0 || bh <= 0 || !PortFbBeginGlyphBatch(renderer) ||
        sRenderSurface->format != SDL_PIXELFORMAT_XRGB8888)
        return false;
    int any = 0;
    for (int r = 0; r < h; r++) {
        const uint32_t* row = src + (size_t)r * w;
        int a = 0, b = w - 1;
        while (a < w && !(row[a] >> 24)) a++;
        if (a == w) { first[r] = -1; continue; }
        while (!(row[b] >> 24)) b--;
        first[r] = (int16_t)a; last[r] = (int16_t)b; any = 1;
    }
    if (!any) return true;
    const float sx = w / bw, sy = h / bh;
    const int W = sRenderSurface->w, H = sRenderSurface->h;
    /* a pixel takes the layer pixel under its centre, as SDL's nearest copy does */
    for (int x = 0; x < W && x < 640; x++) {
        const float u = (x + 0.5f - bx) * sx;
        colOf[x] = (int16_t)(u < 0 ? -1 : u >= w ? -1 : (int)u);
    }
    for (int y = 0; y < H; y++) {
        const float v = (y + 0.5f - by) * sy;
        if (v < 0 || v >= h) continue;
        const int r = (int)v;
        if (first[r] < 0) continue;
        /* the output columns of the row's covered span */
        int x0 = (int)SDL_floorf(bx + first[r] / sx), x1 = (int)SDL_ceilf(bx + (last[r] + 1) / sx);
        x0 = SDL_max(x0 - 1, 0); x1 = SDL_min(x1 + 1, SDL_min(W, 640));
        const uint32_t* in = src + (size_t)r * w;
        uint32_t* out = (uint32_t*)((uint8_t*)sRenderSurface->pixels + (size_t)y * sRenderSurface->pitch);
        for (int x = x0; x < x1; x++) {
            const int c = colOf[x];
            if (c < 0) continue;
            const uint32_t p = in[c];
            const unsigned a = p >> 24;
            if (!a) continue;
            if (a == 255) { out[x] = p | 0xFF000000u; continue; }
            const uint32_t d = out[x];
            const unsigned inv = 255 - a;
            unsigned rr = ((p >> 16) & 255) * a + ((d >> 16) & 255) * inv, gg = ((p >> 8) & 255) * a + ((d >> 8) & 255) * inv,
                     bb = (p & 255) * a + (d & 255) * inv;
            rr = (rr + 1 + (rr >> 8)) >> 8; gg = (gg + 1 + (gg >> 8)) >> 8; bb = (bb + 1 + (bb >> 8)) >> 8;
            out[x] = 0xFF000000u | (rr << 16) | (gg << 8) | bb;
        }
    }
    return true;
}

/* One row of XRGB colours copied into framebuffer rows y0..y1-1 from column x (the HD title's smooth bar). */
bool PortFbFillRows(SDL_Renderer* renderer, const uint32_t* row, int x, int w, int y0, int y1) {
    if (!row || !PortFbBeginGlyphBatch(renderer) || sRenderSurface->format != SDL_PIXELFORMAT_XRGB8888)
        return false;
    if (x < 0) { row -= x; w += x; x = 0; }
    if (x + w > sRenderSurface->w) w = sRenderSurface->w - x;
    y0 = SDL_max(y0, 0);
    y1 = SDL_min(y1, sRenderSurface->h);
    for (int y = y0; y < y1 && w > 0; y++)
        memcpy((uint32_t*)((uint8_t*)sRenderSurface->pixels + (size_t)y * sRenderSurface->pitch) + x, row, (size_t)w * 4);
    return true;
}

/* An upright RGBA picture (straight alpha) copied 1:1 into the framebuffer: w x h from (sx, sy) of a picture pitch
   pixels wide, to (x, y), with SDL's 8-bit colour/alpha modulation (white: its colour taken as white). The HD title's
   poster and words at the size they are drawn (port_hdsprites.c CpuFit). */
bool PortFbBlendRgba(SDL_Renderer* renderer, const unsigned char* src, int pitch, int sx, int sy, int w, int h, int x,
                     int y, unsigned cr, unsigned cg, unsigned cb, unsigned ca, int white) {
    if (!src || !PortFbBeginGlyphBatch(renderer) || sRenderSurface->format != SDL_PIXELFORMAT_XRGB8888)
        return false;
    if (x < 0) { sx -= x; w += x; x = 0; }
    if (y < 0) { sy -= y; h += y; y = 0; }
    if (x + w > sRenderSurface->w) w = sRenderSurface->w - x;
    if (y + h > sRenderSurface->h) h = sRenderSurface->h - y;
    if (w <= 0 || h <= 0) return true;
    const int plain = !white && cr == 255 && cg == 255 && cb == 255 && ca == 255;
    for (int r = 0; r < h; r++) {
        const unsigned char* in = src + ((size_t)(sy + r) * pitch + sx) * 4;
        uint32_t* out = (uint32_t*)((uint8_t*)sRenderSurface->pixels + (size_t)(y + r) * sRenderSurface->pitch) + x;
        if (plain) {
            for (int c = 0; c < w; c++, in += 4) {
                const unsigned a = in[3];
                if (!a) continue;
                if (a == 255) { out[c] = 0xFF000000u | ((uint32_t)in[0] << 16) | ((uint32_t)in[1] << 8) | in[2]; continue; }
                const uint32_t d = out[c];
                const unsigned inv = 255 - a;
                unsigned rr = in[0] * a + ((d >> 16) & 255) * inv, gg = in[1] * a + ((d >> 8) & 255) * inv,
                         bb = in[2] * a + (d & 255) * inv;
                rr = (rr + 1 + (rr >> 8)) >> 8; gg = (gg + 1 + (gg >> 8)) >> 8; bb = (bb + 1 + (bb >> 8)) >> 8;
                out[c] = 0xFF000000u | (rr << 16) | (gg << 8) | bb;
            }
            continue;
        }
        for (int c = 0; c < w; c++, in += 4) {
            if (!in[3]) continue;
            unsigned sa = in[3] * ca, sr = (white ? 255u : in[0]) * cr, sg = (white ? 255u : in[1]) * cg,
                     sb = (white ? 255u : in[2]) * cb;
            sa = (sa + 1 + (sa >> 8)) >> 8;
            if (!sa) continue;
            sr = (sr + 1 + (sr >> 8)) >> 8; sg = (sg + 1 + (sg >> 8)) >> 8; sb = (sb + 1 + (sb >> 8)) >> 8;
            const uint32_t d = out[c];
            if (sa == 255) { out[c] = 0xFF000000u | (sr << 16) | (sg << 8) | sb; continue; }
            const unsigned inv = 255 - sa;
            unsigned rr = sr * sa + ((d >> 16) & 255) * inv, gg = sg * sa + ((d >> 8) & 255) * inv, bb = sb * sa + (d & 255) * inv;
            rr = (rr + 1 + (rr >> 8)) >> 8; gg = (gg + 1 + (gg >> 8)) >> 8; bb = (bb + 1 + (bb >> 8)) >> 8;
            out[c] = 0xFF000000u | (rr << 16) | (gg << 8) | bb;
        }
    }
    return true;
}

/* A turned HD sprite (a card spinning as it is played) drawn straight into the framebuffer: a parallelogram (corners
   top-left, top-right, bottom-right, bottom-left), nearest texel, SDL's 8-bit blend over the opaque picture. SDL's
   triangle rasteriser cost about 620 instructions a pixel there. RGBA texels, one colour for the quad. */
bool PortFbRasterQuad(SDL_Renderer* renderer, const unsigned char* tex, int tw, int th, const SDL_Vertex* v,
                      unsigned cr, unsigned cg, unsigned cb, unsigned ca, int white) {
    if (!tex || tw <= 0 || th <= 0 || !PortFbBeginGlyphBatch(renderer) ||
        sRenderSurface->format != SDL_PIXELFORMAT_XRGB8888)
        return false;
    const float ex = v[1].position.x - v[0].position.x, ey = v[1].position.y - v[0].position.y;
    const float fx = v[3].position.x - v[0].position.x, fy = v[3].position.y - v[0].position.y;
    const float det = ex * fy - ey * fx;
    if (det > -0.01f && det < 0.01f)
        return true;
    if (SDL_fabsf(v[2].position.x - (v[1].position.x + fx)) > 0.5f || SDL_fabsf(v[2].position.y - (v[1].position.y + fy)) > 0.5f)
        return false; /* not a parallelogram */
    float minx = v[0].position.x, maxx = minx, miny = v[0].position.y, maxy = miny;
    for (int k = 1; k < 4; k++) {
        minx = SDL_min(minx, v[k].position.x); maxx = SDL_max(maxx, v[k].position.x);
        miny = SDL_min(miny, v[k].position.y); maxy = SDL_max(maxy, v[k].position.y);
    }
    const int x0 = SDL_max(0, (int)SDL_floorf(minx)), y0 = SDL_max(0, (int)SDL_floorf(miny));
    const int x1 = SDL_min(sRenderSurface->w, (int)SDL_ceilf(maxx)), y1 = SDL_min(sRenderSurface->h, (int)SDL_ceilf(maxy));
    const float ia = fy / det, ib = -fx / det, ic = -ey / det, id = ex / det;
    const float dua = v[1].tex_coord.x - v[0].tex_coord.x, dub = v[3].tex_coord.x - v[0].tex_coord.x;
    const float dva = v[1].tex_coord.y - v[0].tex_coord.y, dvb = v[3].tex_coord.y - v[0].tex_coord.y;
    const int32_t dA = (int32_t)(ia * 65536.0f), dB = (int32_t)(ic * 65536.0f);
    const int32_t dU = (int32_t)((dua * ia + dub * ic) * tw * 65536.0f), dV = (int32_t)((dva * ia + dvb * ic) * th * 65536.0f);
    for (int y = y0; y < y1; y++) {
        const float py = y + 0.5f - v[0].position.y;
        float a = ia * (x0 + 0.5f - v[0].position.x) + ib * py, b = ic * (x0 + 0.5f - v[0].position.x) + id * py;
        float lo = (float)x0, hi = (float)x1;
        if (ia > 1e-6f) { lo = SDL_max(lo, x0 - a / ia - 1); hi = SDL_min(hi, x0 + (1 - a) / ia + 1); }
        else if (ia < -1e-6f) { lo = SDL_max(lo, x0 + (1 - a) / ia - 1); hi = SDL_min(hi, x0 - a / ia + 1); }
        else if (a < 0 || a >= 1) continue;
        if (ic > 1e-6f) { lo = SDL_max(lo, x0 - b / ic - 1); hi = SDL_min(hi, x0 + (1 - b) / ic + 1); }
        else if (ic < -1e-6f) { lo = SDL_max(lo, x0 + (1 - b) / ic - 1); hi = SDL_min(hi, x0 - b / ic + 1); }
        else if (b < 0 || b >= 1) continue;
        const int xs = SDL_max(x0, (int)SDL_floorf(lo)), xe = SDL_min(x1, (int)SDL_ceilf(hi));
        if (xs >= xe) continue;
        a += ia * (xs - x0);
        b += ic * (xs - x0);
        int32_t A = (int32_t)(a * 65536.0f), B = (int32_t)(b * 65536.0f);
        int32_t U = (int32_t)((v[0].tex_coord.x + a * dua + b * dub) * tw * 65536.0f);
        int32_t V = (int32_t)((v[0].tex_coord.y + a * dva + b * dvb) * th * 65536.0f);
        uint32_t* line = (uint32_t*)((uint8_t*)sRenderSurface->pixels + (size_t)y * sRenderSurface->pitch);
        for (int x = xs; x < xe; x++, A += dA, B += dB, U += dU, V += dV) {
            if ((uint32_t)A >= 65536u || (uint32_t)B >= 65536u) continue;
            const int tx = U >> 16, ty = V >> 16;
            if ((unsigned)tx >= (unsigned)tw || (unsigned)ty >= (unsigned)th) continue;
            const unsigned char* sp = tex + ((size_t)ty * tw + tx) * 4;
            if (!sp[3]) continue;
            unsigned sa = sp[3] * ca, sr = (white ? 255u : sp[0]) * cr, sg = (white ? 255u : sp[1]) * cg,
                     sb = (white ? 255u : sp[2]) * cb;
            sa = (sa + 1 + (sa >> 8)) >> 8;
            if (!sa) continue;
            sr = (sr + 1 + (sr >> 8)) >> 8; sg = (sg + 1 + (sg >> 8)) >> 8; sb = (sb + 1 + (sb >> 8)) >> 8;
            const uint32_t d = line[x];
            if (sa == 255) { line[x] = 0xFF000000u | (sr << 16) | (sg << 8) | sb; continue; }
            const unsigned inv = 255 - sa;
            unsigned r = sr * sa + ((d >> 16) & 255) * inv, g = sg * sa + ((d >> 8) & 255) * inv, bl = sb * sa + (d & 255) * inv;
            r = (r + 1 + (r >> 8)) >> 8; g = (g + 1 + (g >> 8)) >> 8; bl = (bl + 1 + (bl >> 8)) >> 8;
            line[x] = 0xFF000000u | (r << 16) | (g << 8) | bl;
        }
    }
    return true;
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
    if (UseOverlayRowGroups(width, dest)) {
        BlendOverlayRowGroups(source, width, height, dest);
        return true;
    }
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

typedef struct {
    const uint32_t* source;
    const uint16_t* xMap;
    const uint16_t* first; /* NULL: per-output-pixel loop */
    int width, outW, outH, outX;
    uint8_t* base;
    int pitch;
    uint64_t dy;
} SparseJob;

static void SparseBand(void* ctx, int bandBegin, int bandEnd);

/* Preserve SDL's pixel-centre mapping and alpha arithmetic, but avoid
 * visiting transparent ends (and entire empty rows) of sparse menu layers. */
bool PortFbBlendOverlayScaledSparse(SDL_Renderer* renderer, const uint32_t* source, int width, int height,
                                    const SDL_Rect* dest) {
    uint16_t xMap[640];
    if (!source || width <= 0 || height <= 0 || dest->x < 0 || dest->y < 0 ||
        dest->w <= 0 || dest->h <= 0 || dest->x + dest->w > 640 || dest->y + dest->h > 480 ||
        !PortFbBeginGlyphBatch(renderer)) return false;
    if (UseOverlayRowGroups(width, dest)) {
        BlendOverlayRowGroups(source, width, height, dest);
        return true;
    }
    const uint64_t dx = ((uint64_t)width << 16) / dest->w, dy = ((uint64_t)height << 16) / dest->h;
    uint64_t pos = dx / 2;
    for (int x = 0; x < dest->w; x++, pos += dx) xMap[x] = (uint16_t)(pos >> 16);
    /* Optionally walk source pixels: one alpha test and one scaled foreground
     * per output run, and no destination term over black. */
    static int grouped = -1;
    if (grouped < 0) grouped = SDL_getenv("KHCOM_FBDEV_SPARSE_RUNS") != NULL;
    uint16_t first[641]; /* first output column of each source column */
    const bool runs = grouped && width <= 640;
    if (runs) {
        static bool logged;
        if (!logged) { PortLog("fbdev: sparse overlay runs active"); logged = true; }
        for (int sx = 0, x = 0; sx <= width; sx++) {
            while (x < dest->w && xMap[x] < sx) x++;
            first[sx] = (uint16_t)x;
        }
    }
    SparseJob job = { source, xMap, runs ? first : NULL, width, dest->w, dest->h, dest->x,
        (uint8_t*)sRenderSurface->pixels + dest->y * sRenderSurface->pitch, sRenderSurface->pitch, dy };
    if (!ParallelPresent() || !PortPpuParallelRows(SparseBand, &job, height, 8))
        SparseBand(&job, 0, height);
    return true;
}

static void SparseBand(void* ctx, int bandBegin, int bandEnd) {
    const SparseJob* job = ctx;
    const uint32_t* source = job->source;
    const uint16_t* xMap = job->xMap;
    const uint16_t* first = job->first;
    const bool runs = first != NULL;
    const int width = job->width;
    const uint64_t dy = job->dy;
    int previous = -1, begin = 0, end = 0;
    int sourceLeft = 0, sourceRight = 0;
    int y = FirstOutputRow((unsigned)bandBegin, dy / 2, dy, job->outH);
    uint64_t py = dy / 2 + (uint64_t)y * dy;
    for (; y < job->outH; y++, py += dy) {
        const int sy = (int)(py >> 16);
        if (sy >= bandEnd)
            break;
        const uint32_t* src = source + sy * width;
        if (sy != previous) {
            int left = 0, right = width, lo, hi;
            while (left < right && !(src[left] >> 24)) left++;
            while (right > left && !(src[right - 1] >> 24)) right--;
            lo = 0; hi = job->outW;
            while (lo < hi) { int mid = (lo + hi) / 2; if (xMap[mid] < left) lo = mid + 1; else hi = mid; }
            begin = lo;
            lo = begin; hi = job->outW;
            while (lo < hi) { int mid = (lo + hi) / 2; if (xMap[mid] < right) lo = mid + 1; else hi = mid; }
            end = lo;
            sourceLeft = left;
            sourceRight = right;
            previous = sy;
        }
        uint32_t* dst = (uint32_t*)(job->base + (size_t)y * job->pitch) + job->outX;
        if (runs) {
            for (int sx = sourceLeft; sx < sourceRight; sx++) {
                const uint32_t pixel = src[sx];
                const unsigned alpha = pixel >> 24;
                if (!alpha) continue;
                const int stop = first[sx + 1];
                if (alpha == 255) {
                    for (int x = first[sx]; x < stop; x++) dst[x] = pixel;
                } else {
                    const uint32_t fg = ScaleGlyphRgb(pixel, alpha);
                    for (int x = first[sx]; x < stop; x++) {
                        const uint32_t under = dst[x];
                        dst[x] = (under & 0x00FFFFFFu) ? fg + ScaleGlyphRgb(under, 255 - alpha) : fg;
                    }
                }
            }
            continue;
        }
        for (int x = begin; x < end; x++) {
            const uint32_t pixel = src[xMap[x]];
            const unsigned alpha = pixel >> 24;
            if (alpha == 255) dst[x] = pixel;
            else if (alpha) dst[x] = ScaleGlyphRgb(pixel, alpha) + ScaleGlyphRgb(dst[x], 255 - alpha);
        }
    }
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
