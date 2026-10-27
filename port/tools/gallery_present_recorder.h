/* QA-only forced include. Never included by a production build. */
#ifndef GALLERY_PRESENT_RECORDER_H
#define GALLERY_PRESENT_RECORDER_H
#include <SDL3/SDL.h>
bool GalleryQaPresent(SDL_Renderer*, const char*, int);
#ifndef GALLERY_QA_IMPLEMENTATION
#define SDL_RenderPresent(renderer) GalleryQaPresent(renderer, __FILE__, __LINE__)
#endif
#endif
