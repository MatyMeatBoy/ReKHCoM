/**
 * key_state.c
 * Key State and Song Utilities
 */

#include "key_state.h"
#include "malloc.h"
#include "m4a.h"
#include "gba/keys.h"
#include <stddef.h>
#include "types.h"

static const u8 sSioKeyHeapName[8] = "SIOKEY";

static KeyState* sSioKeyStateA;
static KeyState* sSioKeyStateB;
static u16 sUnk_02034084;

u16 KeyGetHeld(KeyState* state) {
    return state->held;
}

u16 KeyGetPressed(KeyState* state) {
    return state->trg;
}

u16 KeyGetRepeat(KeyState* state) {
    return state->rep;
}

void KeyStateClear(KeyState* state) {
    state->held = 0;
    state->trg = 0;
    state->rep = 0;
    state->chordLatch = 0;
    state->on[0] = 0;
    state->on[1] = 0;
    state->on[2] = 0;
    state->on[3] = 0;
    state->on[6] = 0;
    state->on[7] = 0;
    state->on[4] = 0;
    state->on[5] = 0;
    state->on[8] = 0;
    state->on[9] = 0;
    state->off[0] = -1;
    state->off[1] = -1;
    state->off[2] = -1;
    state->off[3] = -1;
    state->off[6] = -1;
    state->off[7] = -1;
    state->off[4] = -1;
    state->off[5] = -1;
    state->off[8] = -1;
    state->off[9] = -1;
}

u8 KeyGetHoldFrames(KeyState* state, u16 key) {
    switch (key) {
    case DPAD_LEFT:
        return state->on[0];
    case DPAD_RIGHT:
        return state->on[1];
    case DPAD_UP:
        return state->on[2];
    case DPAD_DOWN:
        return state->on[3];
    case L_BUTTON:
        return state->on[6];
    case R_BUTTON:
        return state->on[7];
    case A_BUTTON:
        return state->on[4];
    case B_BUTTON:
        return state->on[5];
    case START_BUTTON:
        return state->on[8];
    case SELECT_BUTTON:
        return state->on[9];
    }
}

u8 KeyGetOffFrames(KeyState* state, u16 key) {
    switch (key) {
    case DPAD_LEFT:
        return state->off[0];
    case DPAD_RIGHT:
        return state->off[1];
    case DPAD_UP:
        return state->off[2];
    case DPAD_DOWN:
        return state->off[3];
    case L_BUTTON:
        return state->off[6];
    case R_BUTTON:
        return state->off[7];
    case A_BUTTON:
        return state->off[4];
    case B_BUTTON:
        return state->off[5];
    case START_BUTTON:
        return state->off[8];
    case SELECT_BUTTON:
        return state->off[9];
    }
}

u16 KeyReadChord(KeyState* state, u16 key1, u16 key2) {
    u16 keys = 0;
    u8 release1 = KeyGetOffFrames(state, key1);
    u8 release2 = KeyGetOffFrames(state, key2);

    if (release1 == 2) {
        state->chordLatch &= ~key1;
    }

    if (release2 == 2) {
        state->chordLatch &= ~key2;
    }

    if (((KeyGetPressed(state) & key1) && (KeyGetHeld(state) & key2)) ||
        ((KeyGetPressed(state) & key2) && (KeyGetHeld(state) & key1))) {
        state->chordLatch |= key1 | key2;
        keys = key1 | key2;
    }

    if (!(state->chordLatch & key1)) {
        if (KeyGetHoldFrames(state, key1) == 5 || release1 == 1) {
            state->chordLatch |= key1;
            keys = key1;
        }
    }

    if (!(state->chordLatch & key2)) {
        if (KeyGetHoldFrames(state, key2) == 5 || release2 == 1) {
            state->chordLatch |= key2;
            keys = key2;
        }
    }

    return keys;
}

void KeyStateUpdate(KeyState* state, u16 keys) {
    state->trg = keys & ~state->held;
    state->held = keys;

    if (state->held & DPAD_LEFT) {
        state->on[0]++;
        state->off[0] = 0;

        if (state->on[0] > 32) {
            state->on[0] = 29;
        }
    } else {
        state->on[0] = 0;

        if (state->off[0] < 255) {
            state->off[0]++;
        }
    }

    if (state->held & DPAD_RIGHT) {
        state->on[1]++;
        state->off[1] = 0;

        if (state->on[1] > 32) {
            state->on[1] = 29;
        }
    } else {
        state->on[1] = 0;

        if (state->off[1] < 255) {
            state->off[1]++;
        }
    }

    if (state->held & DPAD_UP) {
        state->on[2]++;
        state->off[2] = 0;

        if (state->on[2] > 32) {
            state->on[2] = 29;
        }
    } else {
        state->on[2] = 0;

        if (state->off[2] < 255) {
            state->off[2]++;
        }
    }

    if (state->held & DPAD_DOWN) {
        state->on[3]++;
        state->off[3] = 0;

        if (state->on[3] > 32) {
            state->on[3] = 29;
        }
    } else {
        state->on[3] = 0;

        if (state->off[3] < 255) {
            state->off[3]++;
        }
    }

    if (state->held & L_BUTTON) {
        state->on[6]++;
        state->off[6] = 0;

        if (state->on[6] > 32) {
            state->on[6] = 29;
        }
    } else {
        state->on[6] = 0;

        if (state->off[6] < 255) {
            state->off[6]++;
        }
    }

    if (state->held & R_BUTTON) {
        state->on[7]++;
        state->off[7] = 0;

        if (state->on[7] > 32) {
            state->on[7] = 29;
        }
    } else {
        state->on[7] = 0;

        if (state->off[7] < 255) {
            state->off[7]++;
        }
    }

    if (state->held & A_BUTTON) {
        state->on[4]++;
        state->off[4] = 0;

        if (state->on[4] > 32) {
            state->on[4] = 29;
        }
    } else {
        state->on[4] = 0;

        if (state->off[4] < 255) {
            state->off[4]++;
        }
    }

    if (state->held & B_BUTTON) {
        state->on[5]++;
        state->off[5] = 0;

        if (state->on[5] > 32) {
            state->on[5] = 29;
        }
    } else {
        state->on[5] = 0;

        if (state->off[5] < 255) {
            state->off[5]++;
        }
    }

    if (state->held & START_BUTTON) {
        state->on[8]++;
        state->off[8] = 0;

        if (state->on[8] > 32) {
            state->on[8] = 29;
        }
    } else {
        state->on[8] = 0;

        if (state->off[8] < 255) {
            state->off[8]++;
        }
    }

    if (state->held & SELECT_BUTTON) {
        state->on[9]++;
        state->off[9] = 0;

        if (state->on[9] > 32) {
            state->on[9] = 29;
        }
    } else {
        state->on[9] = 0;

        if (state->off[9] < 255) {
            state->off[9]++;
        }
    }

    state->rep = 0;

    if (state->on[0] == 1 || state->on[0] == 32) {
        state->rep |= DPAD_LEFT;
    }

    if (state->on[1] == 1 || state->on[1] == 32) {
        state->rep |= DPAD_RIGHT;
    }

    if (state->on[2] == 1 || state->on[2] == 32) {
        state->rep |= DPAD_UP;
    }

    if (state->on[3] == 1 || state->on[3] == 32) {
        state->rep |= DPAD_DOWN;
    }

    if (state->on[6] == 1 || state->on[6] == 32) {
        state->rep |= L_BUTTON;
    }

    if (state->on[7] == 1 || state->on[7] == 32) {
        state->rep |= R_BUTTON;
    }

    if (state->on[4] == 1 || state->on[4] == 32) {
        state->rep |= A_BUTTON;
    }

    if (state->on[5] == 1 || state->on[5] == 32) {
        state->rep |= B_BUTTON;
    }

    if (state->on[8] == 1 || state->on[8] == 32) {
        state->rep |= START_BUTTON;
    }

    if (state->on[9] == 1 || state->on[9] == 32) {
        state->rep |= SELECT_BUTTON;
    }
}

void SioKeyInit() {
    SetIwramHeapName(sSioKeyHeapName);
    sSioKeyStateA = IwramAlloc(sizeof(KeyState));
    sSioKeyStateB = IwramAlloc(sizeof(KeyState));
    KeyStateClear(sSioKeyStateA);
    KeyStateClear(sSioKeyStateB);
    sUnk_02034084 = 0;
}

void SioKeyFree() {
    IwramFree(sSioKeyStateB);
    IwramFree(sSioKeyStateA);
}

u16 SioKeyGetHeldA() {
    return KeyGetHeld(sSioKeyStateA);
}

u16 SioKeyGetHeldB() {
    return KeyGetHeld(sSioKeyStateB);
}

u16 SioKeyGetPressedA() {
    return KeyGetPressed(sSioKeyStateA);
}

u16 SioKeyGetPressedB() {
    return KeyGetPressed(sSioKeyStateB);
}

u16 SioKeyGetRepeatA() {
    return KeyGetRepeat(sSioKeyStateA);
}

u16 SioKeyGetRepeatB() {
    return KeyGetRepeat(sSioKeyStateB);
}

u16 SioKeyReadChordA(u16 key1, u16 key2) {
    return KeyReadChord(sSioKeyStateA, key1, key2);
}

u16 SioKeyReadChordB(u16 key1, u16 key2) {
    return KeyReadChord(sSioKeyStateB, key1, key2);
}

void SioKeyStateUpdateA(u16 keys) {
    KeyStateUpdate(sSioKeyStateA, keys);
}

void SioKeyStateUpdateB(u16 keys) {
    KeyStateUpdate(sSioKeyStateB, keys);
}

u8 IsSongPlaying(u16 songNum) {
    u8 idx = gSongTable[songNum].ms;
    SongHeader* header = gSongTable[songNum].header;
    MusicPlayerInfo* info = gMPlayTable[idx].info;
    s32 playing = FALSE;

    if (header == info->songHeader) {
        playing = (u16)info->status != 0;
    }

    return playing;
}

void StopSong(u16 songNum) {
    u8 idx = gSongTable[songNum].ms;
    SongHeader* header = gSongTable[songNum].header;
    MusicPlayerInfo* info = gMPlayTable[idx].info;

    if (header == info->songHeader) {
        if (info->status & MUSICPLAYER_STATUS_TRACK) {
            info->status = MUSICPLAYER_STATUS_PAUSE;
            info->songHeader = NULL;
        }
    }
}
