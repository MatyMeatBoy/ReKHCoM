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

#include "port/port.h"

static uint8_t* sFb;
static size_t sFbOffset;
static struct fb_var_screeninfo sVar;
static struct fb_fix_screeninfo sFix;

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
    PortLog("fbdev: %s %ux%u, %u bpp (red at bit %u)", path, sVar.xres, sVar.yres, sVar.bits_per_pixel,
            sVar.red.offset);
    return 1;
}

int PortFbActive(void) {
    return sFb != NULL;
}

static void CopyRows(uint8_t* dst, const SDL_Surface* src, int h, size_t rowBytes) {
    for (int y = 0; y < h; y++)
        memcpy(dst + (size_t)y * sFix.line_length, (const uint8_t*)src->pixels + (size_t)y * src->pitch, rowBytes);
}

static void WriteFrame(SDL_Surface* frame) {
    SDL_PixelFormat format;
    if (sVar.bits_per_pixel == 16)
        format = SDL_PIXELFORMAT_RGB565;
    else
        format = sVar.red.offset == 0 ? SDL_PIXELFORMAT_XBGR8888 : SDL_PIXELFORMAT_XRGB8888;
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
    if (!sFb)
        return;
    SDL_Surface* frame = SDL_RenderReadPixels(renderer, NULL);
    if (frame) {
        WriteFrame(frame);
        SDL_DestroySurface(frame);
    }
}
