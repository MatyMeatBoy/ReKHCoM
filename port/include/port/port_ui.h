#ifndef GUARD_PORT_UI_H
#define GUARD_PORT_UI_H

#include <stdint.h>

void PortUiInit(void);
/* Draw overlays (options menu, notices) on top of the finished frame. */
void PortUiDraw(uint32_t* frame, int width, int height);
int PortUiDrawNotice(uint32_t* frame, int width, int height);
/* The FPS readout (show_fps), drawn on the notice layer while no notice is up. */
void PortUiSetFps(float fps, float logic);
/* Returns non-zero when the event was consumed by the UI. */
int PortUiHandleEvent(const void* sdlEvent);
/* Non-zero while an overlay owns the input. */
int PortUiActive(void);
/* Switch canvas/presentation only after the scene has faded completely black. */
int PortUiMenuCanvas(void);
/* Full-output scene fade, queried before PortUiDraw advances its clocks (0..255). */
int PortUiSceneFade(void);
void PortUiOpenOptions(void);
/* Test harness (KHCOM_OPTIONS_TOGGLE): open, or close as the player would. */
void PortUiHarnessToggle(void);
/* Called by the title screen while the options hint should be shown. */
void PortUiTitleHint(void);
/* Short message in the corner of the screen (UTF-8). */
void PortUiNotice(const char* text);
void PortUiLinkWaitingNotice(int seconds, int canGiveUp); /* after 10 s: "hold B: leave" */
void PortUiLinkNotice(const char* en, const char* es, const char* name);
void PortUiZoomStep(int dir); /* the main zoom, one step (dir < 0: closer), with a notice */
/* Transparent single-line text using the cartridge font. Caller owns pixels. */
int PortUiRenderTextLine(uint32_t* pixels, int width, int height, const char* text, uint32_t color);
int PortUiTextWidth(const char* text);
/* Opening movie: show or hide the "A again skips" prompt. */
void PortUiSkipPrompt(int visible);
int PortUiIntroSkip(int pressed);
void PortUiOpenMoviePause(void);
void PortUiIntroSkipReset(void);
/* Movies shown cropped and enlarged: the presenter draws the prompt itself
   (PortUiDrawSkipLayer) at the picture's normal size and place, so it is
   left out of the frame while on is set. */
void PortUiSkipOwnLayer(int on);
/* Draws the prompt into frame (a copy of the picture) and returns 1 with its
   rectangle in frame pixels, or 0 when it is not showing. */
int PortUiDrawSkipLayer(uint32_t* frame, int width, int height, int rect[4]);

#endif
