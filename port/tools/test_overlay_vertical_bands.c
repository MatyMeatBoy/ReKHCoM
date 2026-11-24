#include <stdint.h>
#include <stdio.h>
#include "port/overlay_vertical_bands.h"
int main(void) {
    uint32_t row[240], color = 0;
    for (int i = 0; i < 240; i++) row[i] = 0xff123456u;
    if (!PortOverlaySolidEdge(row, 240, &color) || color != 0xff123456u) return 1;
    row[117] = 0x80123456u;
    if (PortOverlaySolidEdge(row, 240, &color)) return 2;
    row[117] = 0xff123457u;
    if (PortOverlaySolidEdge(row, 240, &color)) return 3;
    if (PortOverlaySolidEdge(row, 0, &color)) return 4;
    puts("PASS opaque uniform overlay edges and rejection cases");
    return 0;
}
