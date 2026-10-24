#ifndef PORT_HDLAYOUT_H
#define PORT_HDLAYOUT_H

/* Re:CoM 1.5's 2D layouts and sequences (port_hdlayout.c), drawn as its engine does. */

enum { HD_LAYOUT_GAUGE, HD_LAYOUT_COCKPIT, HD_LAYOUT_COUNT };
enum { HD_LAYOUT_TEX_GAUGE_FACES, HD_LAYOUT_TEX_GAUGE, HD_LAYOUT_TEX_RIKU_FACES, HD_LAYOUT_TEX_COCKPIT,
       HD_LAYOUT_TEXTURE_COUNT };

/* The layout and the texture are imported (hd/sprites/gHud*). */
int PortHdLayoutReady(int layout, int texture);
/* The last frame of an animation group (0: none). */
int PortHdLayoutGroupFrames(int layout, int sequence, int group);
/* A frame the caller holds, not played (a gauge's length): drawn as it is, never wrapped by the group's loop. */
#define HD_LAYOUT_HOLD(frame) ((frame) | 0x100000)
#ifdef SDL_h_
/* Draw an animation group of a layout's sequence at a frame, its origin at (x, y) on the screen, scale screen pixels a
   layout unit, with an opacity and a colour (RGBA, NULL: white). 0 when it is not there. */
int PortHdLayoutDraw(SDL_Renderer* renderer, int layout, int texture, int sequence, int group, int frame, float x,
                     float y, float scale, float alpha, const float* tint);
#endif
/* Re:CoM's HP gauge for this frame's HP (the game's own values): 1 when it is drawn (HD cards on, its layout and
   textures imported), so the cartridge's gauge and face are left out. */
int PortHdGauge(int hp, int maxHp, int riku);
#ifdef SDL_h_
/* Draw it on the HUD canvas placed at (ox, oy), scale screen pixels a GBA pixel, darkened to shade (1: none). */
void PortHdGaugeDraw(SDL_Renderer* renderer, float ox, float oy, float scale, float shade);
/* The enemy HP gauge as Re:CoM 1.5 draws it (CEnemyGauge): 1 when it takes the cartridge's place this frame. */
int PortHdEnemyGauge(int visible, int hp, int maxHp, const void* who);
void PortHdEnemyGaugeDraw(SDL_Renderer* renderer, float ox, float oy, float scale, float shade);
#endif
void PortHdLayoutShutdown(void);

#endif
