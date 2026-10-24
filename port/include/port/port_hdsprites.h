#ifndef PORT_HDSPRITES_H
#define PORT_HDSPRITES_H

#include <stdint.h>

/* HD sprites (port_hdsprites.c): cartridge sprites drawn with imported Re:CoM art. */

/* 1-based id of the HD picture for these ROM tiles and frame, 0: none (option off or file missing). */
int PortHdSpriteFind(const void* tiles, const void* frame);
/* 1 when some frame of these ROM tiles has its HD picture (the option on, the file there). */
int PortHdSpriteAny(const void* tiles);
/* 1 when this frame's HD drawing is to be logged (KHCOM_HDDRAWLOG, test_hd_menus.py). */
int PortHdDrawLogged(void);
/* Show this HD picture this frame at (x, y) of the GBA screen, though no OAM entry draws it (a menu title the menu
   keeps as a BG copy, its tiles released), in a fade (dark: to black, white: to white, 0..255). */
void PortHdStillSprite(const void* tiles, const void* frame, int x, int y, int dark, int white);
/* Its group (1 cards, 2 title, 3 battle HUD, 4 worlds, 5 menu titles). */
int PortHdSpriteGroup(int id);
/* The sprite's box in its frame's coordinates (GBA pixels). */
void PortHdSpriteBox(int id, int* x0, int* y0, int* w, int* h);
/* A new picture of a layer (0 the game's picture, 1 the HUD canvas, 2 the screen layer): forget the parts queued
   for the last one. */
void PortHdSpritesBegin(int layer);
/* Queue one OAM part: its four corners on the layer's canvas (top-left, top-right, bottom-right, bottom-left of the
   part's texture), the part's box in the frame's coordinates, its opacity and the fade its palette is in (dark: to
   black, white: to white, 0..255; PortPaletteFade). */
int PortHdSpriteQueue(int layer, int id, const float* x, const float* y, int px, int py, int pw, int ph, int alpha,
                      int dark, int white);
/* How far a palette's current colours have faded from its original ones, to black or to white (0..255 each; both 0
   when they are not a fade of it, e.g. another palette). */
void PortPaletteFade(const uint16_t* original, const volatile uint16_t* current, int count, int* dark, int* white);
#ifdef SDL_h_
/* Draw a layer's queued parts, back to front, over its canvas placed at (ox, oy) with this scale; darken/brighten
   0..16 follow the game's fade. */
void PortHdSpritesDraw(SDL_Renderer* renderer, int layer, float ox, float oy, float scale, int darken, int brighten);
#endif
void PortHdSpritesShutdown(void);
/* The HP ring (Re:CoM's player gauge) around this HD face, filled to ratio/256 this frame: 1 when it is drawn (HD
   cards on, the face's HD picture there), so the cartridge's gauge is left out; 0 keeps it. */
int PortHdHpRing(const void* faceTiles, const void* faceFrame, int ratio);

#endif
