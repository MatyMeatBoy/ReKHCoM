#ifndef GUARD_PORT_BATTLE_BACKDROP_H
#define GUARD_PORT_BATTLE_BACKDROP_H

#include <stdint.h>

/* Presentation-only continuation of Guard Armor's authored patio.
 * Called only outside the hardware picture. No game/collision map changes. */
int PortGuardArmorBackdropMatches(const void* const* maps);
int PortGuardArmorBackdropPixel(int x, int y, uint16_t* color);

/* Darkside's beach (port_darkside.c): 1 the beach layer, 2 Darkside's body, 0 neither. */
int PortDarksideLayer(const void* const* maps);
/* cx, cy: the map tile at the middle of the original picture (the body's single copy is the one nearest it) */
int PortDarksideTileSource(int layer, int* tx, int* ty, int cx, int cy);
int PortDarksideShorePixel(int x, int y, uint16_t* color);
/* once a frame for the body layer: where its tiles are in VRAM (the frame's own pixels beside the drawing) */
void PortDarksideBeginFrame(int bg, const void* const* maps);
int PortDarksideBodyPixel(const void* const* maps, int x, int y, int cx, int cy, uint16_t* color);

#endif
