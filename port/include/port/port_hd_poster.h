#ifndef PORT_HD_POSTER_API_H
#define PORT_HD_POSTER_API_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PORT_HD_POSTER_W 640
#define PORT_HD_POSTER_H 360

/* Versioned, offset-specific sidecar beside the imported MP4. */
static inline int PortHdPosterPath(const char* movie, int second, char* out, size_t size) {
    const char *dot, *slash, *back;
    int prefix, written;
    if (!movie || !*movie || !out || !size || second < 0) return 0;
    dot = strrchr(movie, '.'); slash = strrchr(movie, '/'); back = strrchr(movie, '\\');
    if (!slash || (back && back > slash)) slash = back;
    prefix = dot && (!slash || dot > slash) ? (int)(dot - movie) : (int)strlen(movie);
    written = snprintf(out, size, "%.*s.poster-v1-t%d-640x360.png", prefix, movie, second);
    return written >= 0 && (size_t)written < size;
}

/* Old MF poster writers could cache a black seek keyframe as a successful still.
 * Check decoded pixels, not PNG validity alone, so those caches can be rebuilt. */
static inline int PortHdPosterHasPicture(const void* pixels, int width, int height, int pitch) {
    int x, y, lit = 0;
    if (!pixels || width <= 0 || height <= 0) return 0;
    for (y = 0; y < height; y++) {
        const uint32_t* row = (const uint32_t*)((const unsigned char*)pixels + y * pitch);
        for (x = 0; x < width; x++) {
            uint32_t rgb = row[x];
            if (((rgb >> 16) & 255) > 8 || ((rgb >> 8) & 255) > 8 || (rgb & 255) > 8)
                if (++lit >= 32) return 1;
        }
    }
    return 0;
}

#if defined(_WIN32) || defined(PORT_LINUX)
#ifdef __cplusplus
extern "C" {
#endif
/* Resolve on the main thread; the copied path is then owned by the job. */
int PortHdScenePreviewPath(const char* name, char* out, size_t size);
/* Worker-safe: no renderer calls and no PortConfigPath/shared temporary path. */
int PortHdScenePreviewFile(const char* path, uint32_t* out, int width, int height, int second);
#ifdef __cplusplus
}
#endif
#endif /* decoder APIs: Windows/Linux; PNG sidecar format is portable */
#endif
