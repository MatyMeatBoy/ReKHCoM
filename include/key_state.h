#ifndef GUARD_KEY_STATE_H
#define GUARD_KEY_STATE_H

#include "types.h"

typedef struct KeyState {
    u16 held;
    u16 trg;
    u16 rep;
    u16 chordLatch;
    u8 on[10];
    u8 off[10];
} KeyState;

u16 KeyGetHeld(KeyState* state);
u16 KeyGetPressed(KeyState* state);
u16 KeyGetRepeat(KeyState* state);
void KeyStateClear(KeyState* state);
u8 KeyGetHoldFrames(KeyState* state, u16 key);
u8 KeyGetOffFrames(KeyState* state, u16 key);
u16 KeyReadChord(KeyState* state, u16 key1, u16 key2);
void KeyStateUpdate(KeyState* state, u16 keys);
void SioKeyInit();
void SioKeyFree();
u16 SioKeyGetHeldA();
u16 SioKeyGetHeldB();
u16 SioKeyGetPressedA();
u16 SioKeyGetPressedB();
u16 SioKeyGetRepeatA();
u16 SioKeyGetRepeatB();
u16 SioKeyReadChordA(u16 key1, u16 key2);
u16 SioKeyReadChordB(u16 key1, u16 key2);
void StopSong(u16 songNum);

void SioKeyStateUpdateA(u16 keys);
void SioKeyStateUpdateB(u16 keys);
u8 IsSongPlaying(u16 songNum);

#endif
