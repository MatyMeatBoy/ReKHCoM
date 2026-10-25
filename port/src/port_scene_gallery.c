/* The scene gallery (Options > Scenes, user 25/10): the story scenes already seen, to watch again in their cartridge
 * version or their Re:CoM HD one. What was seen is the whole ReKHCoM install's (any save, any bank): the furthest any
 * game got. scenes_seen.dat holds one bit per event id. Cheats > Unlock all scenes fills it. */
#include <stdio.h>
#include <string.h>

#include "port/port.h"

#define SCENE_BITS 1024

static unsigned char sSeen[SCENE_BITS / 8];
static int sLoaded;

static void Load(void) {
    FILE* f;
    if (sLoaded)
        return;
    sLoaded = 1;
    f = fopen(PortConfigPath("scenes_seen.dat"), "rb");
    if (f) {
        if (fread(sSeen, 1, sizeof(sSeen), f) != sizeof(sSeen))
            memset(sSeen, 0, sizeof(sSeen)); /* an older or broken file: start again */
        fclose(f);
    }
}

static void Save(void) {
    FILE* f = fopen(PortConfigPath("scenes_seen.dat"), "wb");
    if (!f)
        return;
    fwrite(sSeen, 1, sizeof(sSeen), f);
    fclose(f);
}

void PortSceneSeen(int eventId) {
    eventId &= 0x7FFF;
    if (eventId < 0 || eventId >= SCENE_BITS)
        return;
    Load();
    if (sSeen[eventId >> 3] & (1 << (eventId & 7)))
        return;
    sSeen[eventId >> 3] |= (unsigned char)(1 << (eventId & 7));
    Save();
}

int PortSceneIsSeen(int eventId) {
    eventId &= 0x7FFF;
    if (eventId < 0 || eventId >= SCENE_BITS)
        return 0;
    Load();
    return (sSeen[eventId >> 3] >> (eventId & 7)) & 1;
}

void PortScenesUnlockAll(void) {
    Load();
    memset(sSeen, 0xFF, sizeof(sSeen));
    Save();
}
