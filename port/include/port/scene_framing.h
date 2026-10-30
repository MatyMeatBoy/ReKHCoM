#ifndef PORT_SCENE_FRAMING_H
#define PORT_SCENE_FRAMING_H

enum { PORT_SCENE_ADJUST = 0, PORT_SCENE_EXTENDED = 1 };

/* Exact events admitted from current build_version honors records.
   See port/progress/SCENE_FRAMING_HONORS.json. Shared-map variants and later
   revisions do not inherit admission; review the manifest when replacing art. */
static inline int PortSceneFramingHonors(int event) {
    switch (event) {
        case 4: case 8: case 53: case 54: case 55:
        case 74: case 102: case 109: case 121: case 122:
            return 1;
        default: return 0;
    }
}

/* Largest whole scale containing the complete native picture. SDL's minimum
   window is240x160; retain a safe1 for invalid/transient sizes. */
static inline int PortSceneFramingScale(int width, int height) {
    int x = width / 240, y = height / 160;
    int scale = x < y ? x : y;
    return scale > 0 ? scale : 1;
}

static inline int PortSceneFramingUse(int mode, int activeEvent, int event) {
    return mode == PORT_SCENE_ADJUST && activeEvent && event >= 0 && !PortSceneFramingHonors(event);
}

static inline float PortSceneFramingNarrowScale(int adjusted, int width, int height, float normalScale) {
    return adjusted ? (float)PortSceneFramingScale(width, height) : normalScale;
}

#endif
