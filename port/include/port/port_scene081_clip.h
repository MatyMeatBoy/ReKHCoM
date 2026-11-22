#ifndef PORT_SCENE081_CLIP_H
#define PORT_SCENE081_CLIP_H

/* In the native map the cap is finite: all three BG layers are empty in
 * this lower-left rectangle. Platform03's last OBJ part leaks below it.
 * Only its exterior pixels may be omitted, never the original GBA frame. */
static inline int PortScene081DecorBelowCap(int gx, int gy, int wx, int wy) {
    return (gx<0 || gx>=240 || gy<0 || gy>=160) &&
        wx>=0 && wx<128 && wy>=256 && wy<512;
}
#endif
