#ifndef PORT_SCENE_FRAMING_H
#define PORT_SCENE_FRAMING_H

enum { PORT_SCENE_ADJUST = 0, PORT_SCENE_EXTENDED = 1, PORT_SCENE_ORIGINAL_SHOT = 2 };

#include "scene_framing_grants.h"

/* Generated from version-bound approval records and actual renderer/art inputs.
   A changed revision or recipe loses admission until explicitly reviewed. */
static inline int PortSceneFramingHonors(int event) {
    switch (event) {
        PORT_SCENE_HONORS_CASES
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
    return activeEvent && event >= 0 && (mode == PORT_SCENE_ORIGINAL_SHOT ||
        (mode == PORT_SCENE_ADJUST && !PortSceneFramingHonors(event)));
}

static inline float PortSceneFramingNarrowScale(int adjusted, int width, int height, float normalScale) {
    return adjusted ? (float)PortSceneFramingScale(width, height) : normalScale;
}

#endif
