/*
 * C version of asm/m4a_1.s (MusicPlayer2000 sequencer core and mixer).
 *
 * The sequencer (MPlayMain, ply_*, TrackStop, ...) follows the assembly
 * step by step. The mixer keeps the original envelope and sample-stepping
 * behaviour but accumulates into 32-bit buffers instead of wrapping 8-bit
 * bytes, and can run at a multiple of the game's mixing rate for cleaner
 * output. The result is read by port_audio.c instead of the FIFO DMA.
 */
#include <string.h>
#include <SDL3/SDL.h>

#include "m4a.h"
#include "port/port.h"
#include "port/port_audio.h"

void FadeOutBody(MusicPlayerInfo* mplayInfo);
void TrkVolPitSet(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track);
u32 MidiKeyToFreq(WaveData* wav, u8 key, u8 fineAdjust);
void ClearChain(void* x);
void Clear64byte(void* x);

extern MusicPlayerInfo gMPlayInfo_BGM;

PortMixBuffer gPortMix;

#define CHAN_ON (SOUND_CHANNEL_SF_START | SOUND_CHANNEL_SF_STOP | SOUND_CHANNEL_SF_IEC | SOUND_CHANNEL_SF_ENV)
#define TONE_RHY 0x80
#define TONE_SPL 0x40

/* Channel lists may point a channel at itself; treat that as the end. */
static void* NextChan(void* chanPtr) {
    SoundChannel* chan = chanPtr;
    void* next = chan->nextChannelPointer;

    if (next == chan) {
        chan->nextChannelPointer = NULL;
        next = NULL;
    }
    return next;
}

u32 umul3232H32(u32 a, u32 b) {
    return (u32)(((u64)a * b) >> 32);
}

void SoundMainBTM(void* work) {
    memset(work, 0, 64);
}

void RealClearChain(void* work) {
    SoundChannel* chan = work;
    MusicPlayerTrack* track = chan->track;
    SoundChannel* next;
    SoundChannel* prev;

    if (track == NULL)
        return;
    next = chan->nextChannelPointer;
    prev = chan->prevChannelPointer;
    if (prev != NULL)
        prev->nextChannelPointer = next;
    else
        track->chan = next;
    if (next != NULL)
        next->prevChannelPointer = prev;
    chan->track = NULL;
}

void MPlayJumpTableCopy(MPlayFunc* dest) {
    int i;

    for (i = 0; i < 36; i++)
        dest[i] = gMPlayJumpTableTemplate[i];
}

static u8 ReadCmd(MusicPlayerTrack* track) {
    return *track->cmdPtr++;
}

static void PortClearModM(MusicPlayerTrack* track) {
    track->modM = 0;
    track->lfoSpeedC = 0;
    track->flags |= track->modT == 0 ? MPT_FLG_PITCHG : MPT_FLG_VOLCHG;
}

void ply_fine(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    SoundChannel* chan = track->chan;

    (void)mplayInfo;
    while (chan != NULL) {
        if (chan->statusFlags & CHAN_ON)
            chan->statusFlags |= SOUND_CHANNEL_SF_STOP;
        RealClearChain(chan);
        chan = NextChan(chan);
    }
    track->flags = 0;
}

static void GotoAddress(MusicPlayerTrack* track) {
    const u8* p = track->cmdPtr;

    track->cmdPtr = (u8*)(uintptr_t)(p[0] | (p[1] << 8) | (p[2] << 16) | ((u32)p[3] << 24));
}

void ply_goto(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    (void)mplayInfo;
    GotoAddress(track);
}

void ply_patt(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    if (track->patternLevel >= 3) {
        ply_fine(mplayInfo, track);
        return;
    }
    track->patternStack[track->patternLevel] = track->cmdPtr + 4;
    track->patternLevel++;
    GotoAddress(track);
}

void ply_pend(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    (void)mplayInfo;
    if (track->patternLevel != 0) {
        track->patternLevel--;
        track->cmdPtr = track->patternStack[track->patternLevel];
    }
}

void ply_rept(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    u8* p = track->cmdPtr;

    (void)mplayInfo;
    if (*p == 0) {
        track->cmdPtr = p + 1;
        GotoAddress(track);
        return;
    }
    track->repN++;
    track->cmdPtr = p + 1;
    if (track->repN < *p) {
        GotoAddress(track);
    } else {
        track->repN = 0;
        track->cmdPtr = p + 5;
    }
}

void ply_prio(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    (void)mplayInfo;
    track->priority = ReadCmd(track);
}

void ply_tempo(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    u32 v = ReadCmd(track) * 2;

    mplayInfo->tempoD = (u16)v;
    mplayInfo->tempoI = (u16)((v * mplayInfo->tempoU) >> 8);
}

void ply_keysh(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    (void)mplayInfo;
    track->keyShift = (s8)ReadCmd(track);
    track->flags |= MPT_FLG_PITCHG;
}

void ply_voice(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    u8 index = ReadCmd(track);

    memcpy(&track->tone, &mplayInfo->tone[index], sizeof(ToneData));
}

void ply_vol(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    (void)mplayInfo;
    track->vol = ReadCmd(track);
    track->flags |= MPT_FLG_VOLCHG;
}

void ply_pan(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    (void)mplayInfo;
    track->pan = (s8)(ReadCmd(track) - 0x40);
    track->flags |= MPT_FLG_VOLCHG;
}

void ply_bend(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    (void)mplayInfo;
    track->bend = (s8)(ReadCmd(track) - 0x40);
    track->flags |= MPT_FLG_PITCHG;
}

void ply_bendr(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    (void)mplayInfo;
    track->bendRange = ReadCmd(track);
    track->flags |= MPT_FLG_PITCHG;
}

void ply_lfodl(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    (void)mplayInfo;
    track->lfoDelay = ReadCmd(track);
}

void ply_modt(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    u8 v = ReadCmd(track);

    (void)mplayInfo;
    if (track->modT != v) {
        track->modT = v;
        track->flags |= MPT_FLG_VOLCHG | MPT_FLG_PITCHG;
    }
}

void ply_tune(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    (void)mplayInfo;
    track->tune = (s8)(ReadCmd(track) - 0x40);
    track->flags |= MPT_FLG_PITCHG;
}

void ply_port(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    u8 reg = ReadCmd(track);
    u8 value = ReadCmd(track);

    (void)mplayInfo;
    IO8(0x60 + reg) = value;
}

void ply_lfos(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    (void)mplayInfo;
    track->lfoSpeed = ReadCmd(track);
    if (track->lfoSpeed == 0)
        PortClearModM(track);
}

void ply_mod(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    (void)mplayInfo;
    track->mod = ReadCmd(track);
    if (track->mod == 0)
        PortClearModM(track);
}

void ply_endtie(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    SoundChannel* chan;
    u8 key;

    (void)mplayInfo;
    if (*track->cmdPtr < 0x80) {
        key = *track->cmdPtr++;
        track->key = key;
    } else {
        key = track->key;
    }
    for (chan = track->chan; chan != NULL; chan = NextChan(chan)) {
        if ((chan->statusFlags & 0x83) && !(chan->statusFlags & SOUND_CHANNEL_SF_STOP) && chan->midiKey == key) {
            chan->statusFlags |= SOUND_CHANNEL_SF_STOP;
            return;
        }
    }
}

static void ChnVolSet(SoundChannel* chan, MusicPlayerTrack* track) {
    s32 pan = (s8)chan->rhythmPan;
    u32 r = ((u32)(0x80 + pan) * chan->velocity * track->volMR) >> 14;
    u32 l = ((u32)(0x7F - pan) * chan->velocity * track->volML) >> 14;

    chan->rightVolume = (u8)(r > 0xFF ? 0xFF : r);
    chan->leftVolume = (u8)(l > 0xFF ? 0xFF : l);
}

void ply_note(u32 clockIndex, MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    SoundInfo* info = gSoundInfoPtr;
    const ToneData* tone;
    SoundChannel* chan = NULL;
    u8* p;
    u32 key, priority, cgbType;
    s32 rhythmPan = 0;
    s32 k;

    track->gateTime = gClockTable[clockIndex];
    p = track->cmdPtr;
    if (*p < 0x80) {
        track->key = *p++;
        if (*p < 0x80) {
            track->velocity = *p++;
            if (*p < 0x80)
                track->gateTime += *p++;
        }
        track->cmdPtr = p;
    }

    if (track->tone.type & (TONE_RHY | TONE_SPL)) {
        u32 index = track->key;
        if (track->tone.type & TONE_SPL)
            index = ((const u8*)(uintptr_t)*(u32*)&track->tone.attack)[track->key];
        tone = (const ToneData*)track->tone.wav + index;
        if (tone->type & (TONE_RHY | TONE_SPL))
            return;
        if (track->tone.type & TONE_RHY) {
            if (tone->pan_sweep & 0x80)
                rhythmPan = ((s32)tone->pan_sweep - 0xC0) * 2;
            key = tone->key;
        } else {
            key = track->key;
        }
    } else {
        tone = &track->tone;
        key = track->key;
    }

    priority = mplayInfo->priority + track->priority;
    if (priority > 0xFF)
        priority = 0xFF;
    cgbType = tone->type & 7;

    if (cgbType != 0) {
        CgbChannel* cgb = info->cgbChans;
        if (cgb == NULL)
            return;
        cgb += cgbType - 1;
        if ((cgb->statusFlags & CHAN_ON) && !(cgb->statusFlags & SOUND_CHANNEL_SF_STOP)) {
            if (cgb->priority > priority)
                return;
            if (cgb->priority == priority && (uintptr_t)cgb->track < (uintptr_t)track)
                return;
        }
        chan = (SoundChannel*)cgb;
    } else {
        u32 bestPrio = priority;
        uintptr_t bestTrack = (uintptr_t)track;
        int stopping = 0, i;

        for (i = 0; i < info->maxChans; i++) {
            SoundChannel* c = &info->chans[i];
            u32 cp;
            uintptr_t ct;

            if (!(c->statusFlags & CHAN_ON)) {
                chan = c;
                break;
            }
            if (c->statusFlags & SOUND_CHANNEL_SF_STOP) {
                if (!stopping) {
                    stopping = 1;
                    bestPrio = c->priority;
                    bestTrack = (uintptr_t)c->track;
                    chan = c;
                    continue;
                }
            } else if (stopping) {
                continue;
            }
            cp = c->priority;
            ct = (uintptr_t)c->track;
            if (cp < bestPrio) {
                bestPrio = cp;
                bestTrack = ct;
                chan = c;
            } else if (cp == bestPrio) {
                if (ct > bestTrack) {
                    bestTrack = ct;
                    chan = c;
                } else if (ct == bestTrack) {
                    chan = c;
                }
            }
        }
        if (chan == NULL)
            return;
    }

    ClearChain(chan);
    chan->prevChannelPointer = NULL;
    chan->nextChannelPointer = track->chan;
    if (track->chan != NULL)
        track->chan->prevChannelPointer = chan;
    track->chan = chan;
    chan->track = track;

    track->lfoDelayC = track->lfoDelay;
    if (track->lfoDelay != 0)
        PortClearModM(track);
    TrkVolPitSet(mplayInfo, track);

    chan->gateTime = track->gateTime;
    chan->midiKey = track->key;
    chan->velocity = track->velocity;
    chan->priority = (u8)priority;
    chan->key = (u8)key;
    chan->rhythmPan = (u8)rhythmPan;
    chan->type = tone->type;
    chan->wav = tone->wav;
    chan->attack = tone->attack;
    chan->decay = tone->decay;
    chan->sustain = tone->sustain;
    chan->release = tone->release;
    chan->pseudoEchoVolume = track->pseudoEchoVolume;
    chan->pseudoEchoLength = track->pseudoEchoLength;
    ChnVolSet(chan, track);

    k = (s32)chan->key + (s8)track->keyM;
    if (k < 0)
        k = 0;
    if (cgbType != 0) {
        CgbChannel* cgb = (CgbChannel*)chan;
        u8 sweep = tone->pan_sweep;
        cgb->length = tone->length;
        if ((sweep & 0x80) || !(sweep & 0x70))
            sweep = 8;
        cgb->sweep = sweep;
        cgb->frequency = info->MidiKeyToCgbFreq((u8)cgbType, (u8)k, track->pitM);
    } else {
        chan->frequency = MidiKeyToFreq(chan->wav, (u8)k, track->pitM);
    }
    chan->statusFlags = SOUND_CHANNEL_SF_START;
    track->flags &= 0xF0;
}

void TrackStop(MusicPlayerInfo* mplayInfo, MusicPlayerTrack* track) {
    SoundChannel* chan;

    (void)mplayInfo;
    if (!(track->flags & MPT_FLG_EXIST))
        return;
    for (chan = track->chan; chan != NULL; chan = NextChan(chan)) {
        if (chan->statusFlags != 0) {
            u8 cgbType = chan->type & 7;
            if (cgbType != 0)
                gSoundInfoPtr->CgbOscOff(cgbType);
            chan->statusFlags = 0;
        }
        chan->track = NULL;
    }
    track->chan = NULL;
}

void MPlayMain(MusicPlayerInfo* mplayInfo) {
    SoundInfo* info;
    u32 tempo;

    if (mplayInfo->ident != ID_NUMBER)
        return;
    mplayInfo->ident++;
    if (mplayInfo->MPlayMainNext != NULL)
        mplayInfo->MPlayMainNext(mplayInfo->musicPlayerNext);

    if ((s32)mplayInfo->status < 0)
        goto done;
    info = gSoundInfoPtr;
    FadeOutBody(mplayInfo);
    if ((s32)mplayInfo->status < 0)
        goto done;

    tempo = (u16)(mplayInfo->tempoC + mplayInfo->tempoI);
    while (tempo >= 150) {
        MusicPlayerTrack* track = mplayInfo->tracks;
        u32 bit = 1, active = 0;
        int n;

        mplayInfo->tempoC = (u16)tempo;
        for (n = mplayInfo->trackCount; n > 0; n--, track++, bit <<= 1) {
            SoundChannel* chan;

            if (!(track->flags & MPT_FLG_EXIST))
                continue;
            active |= bit;

            for (chan = track->chan; chan != NULL; chan = NextChan(chan)) {
                if (chan->statusFlags & CHAN_ON) {
                    if (chan->gateTime != 0 && --chan->gateTime == 0)
                        chan->statusFlags |= SOUND_CHANNEL_SF_STOP;
                } else {
                    ClearChain(chan);
                }
            }

            if (track->flags & MPT_FLG_START) {
                Clear64byte(track);
                track->flags = MPT_FLG_EXIST;
                track->bendRange = 2;
                track->volX = 64;
                track->lfoSpeed = 22;
                track->tone.type = 1;
            }

            while (track->wait == 0) {
                u32 cmd;

                if ((uintptr_t)track->cmdPtr < GBA_ROM || (uintptr_t)track->cmdPtr >= GBA_ROM + GBA_ROM_SIZE) {
                    PortLog("m4a: player %p track %d command pointer %p left the ROM (song %p, last cmd %02x)",
                            (void*)mplayInfo, (int)(track - mplayInfo->tracks), (void*)track->cmdPtr,
                            (void*)mplayInfo->songHeader, mplayInfo->cmd + 0xB1);
                    ply_fine(mplayInfo, track);
                    goto nextTrack;
                }
                cmd = *track->cmdPtr;

                if (cmd < 0x80) {
                    cmd = track->runningStatus;
                } else {
                    track->cmdPtr++;
                    if (cmd >= 0xBD)
                        track->runningStatus = (u8)cmd;
                }
                if (cmd >= 0xCF) {
                    info->plynote(cmd - 0xCF, mplayInfo, track);
                } else if (cmd > 0xB0) {
                    mplayInfo->cmd = (u8)(cmd - 0xB1);
                    ((XcmdFunc)info->MPlayJumpTable[cmd - 0xB1])(mplayInfo, track);
                    if (track->flags == 0)
                        goto nextTrack;
                } else {
                    track->wait = gClockTable[cmd - 0x80];
                }
            }

            track->wait--;
            if (track->lfoSpeed != 0 && track->mod != 0) {
                if (track->lfoDelayC != 0) {
                    track->lfoDelayC--;
                } else {
                    s32 wave, m;
                    u8 c = track->lfoSpeedC = (u8)(track->lfoSpeedC + track->lfoSpeed);

                    if ((u8)(c - 0x40) & 0x80)
                        wave = (s8)c;
                    else
                        wave = 0x80 - c;
                    m = (track->mod * wave) >> 6;
                    if ((s8)(track->modM ^ m) != 0) {
                        track->modM = (s8)m;
                        track->flags |= track->modT == 0 ? MPT_FLG_PITCHG : MPT_FLG_VOLCHG;
                    }
                }
            }
        nextTrack:;
        }

        mplayInfo->clock++;
        if (active == 0) {
            mplayInfo->status = MUSICPLAYER_STATUS_PAUSE;
            goto done;
        }
        mplayInfo->status = active;
        tempo = (u16)(mplayInfo->tempoC - 150);
    }
    mplayInfo->tempoC = (u16)tempo;

    {
        MusicPlayerTrack* track = mplayInfo->tracks;
        int n;

        for (n = mplayInfo->trackCount; n > 0; n--, track++) {
            SoundChannel* chan;

            if (!(track->flags & MPT_FLG_EXIST) || !(track->flags & 0xF))
                continue;
            TrkVolPitSet(mplayInfo, track);
            for (chan = track->chan; chan != NULL; chan = NextChan(chan)) {
                u32 cgbType;

                if (!(chan->statusFlags & CHAN_ON)) {
                    ClearChain(chan);
                    continue;
                }
                cgbType = chan->type & 7;
                if (track->flags & MPT_FLG_VOLCHG) {
                    ChnVolSet(chan, track);
                    if (cgbType != 0)
                        ((CgbChannel*)chan)->modify |= CGB_CHANNEL_MO_VOL;
                }
                if (track->flags & MPT_FLG_PITCHG) {
                    s32 k = (s32)chan->key + (s8)track->keyM;
                    if (k < 0)
                        k = 0;
                    if (cgbType != 0) {
                        ((CgbChannel*)chan)->frequency = info->MidiKeyToCgbFreq((u8)cgbType, (u8)k, track->pitM);
                        ((CgbChannel*)chan)->modify |= CGB_CHANNEL_MO_PIT;
                    } else {
                        chan->frequency = MidiKeyToFreq(chan->wav, (u8)k, track->pitM);
                    }
                }
            }
            track->flags &= 0xF0;
        }
    }

done:
    mplayInfo->ident = ID_NUMBER;
}

/* ---- mixer ------------------------------------------------------------------ */

static int IsMusicChannel(const SoundChannel* chan) {
    const MusicPlayerTrack* t = chan->track;
    const MusicPlayerInfo* bgm = &gMPlayInfo_BGM;

    /* the BGM player's own tracks only: the sound effect players' tracks follow them in memory, so a bound of
       MAX_MUSICPLAYER_TRACKS counted button sounds as music (silenced with the HD music, issue of 22/10) */
    return t != NULL && bgm->tracks != NULL && t >= bgm->tracks && t < bgm->tracks + bgm->trackCount;
}

/* KHCOM_AUDIOCHECK=1 (harness): every sounding channel's track is looked up in the player table (its real track
   counts); a channel taken for music that is not the BGM player's, or the other way round, is logged */
static void AudioCheck(const SoundChannel* chan) {
    static int reported[32];
    const MusicPlayerTrack* t = chan->track;
    int i, owner = -1, music;
    if (t == NULL)
        return;
    for (i = 0; i < NUM_MUSIC_PLAYERS && i < 32; i++) {
        const MusicPlayer* p = &gMPlayTable[i];
        if (t >= p->track && t < p->track + p->numTracks)
            owner = i;
    }
    music = owner >= 0 && gMPlayTable[owner].info == &gMPlayInfo_BGM;
    if (music != IsMusicChannel(chan) && owner >= 0 && !reported[owner]++)
        PortLog("audio check: player %d's channel taken for %s", owner, music ? "a sound effect" : "music");
}

/* Reverb history, one slot per DMA period frame (as the PCM ring buffer). */
static int32_t sHistL[8][PORT_MIX_MAX];
static int32_t sHistR[8][PORT_MIX_MAX];

/* Each channel's volume as it was at the end of the last frame. The m4a
   engine updates envelopes once per VBlank; stepping the volume at every
   frame boundary is a 60 Hz "zipper" buzz, so the mixer ramps from the last
   frame's volume to this one's across the frame. */
static s32 sLastVolR[MAX_DIRECTSOUND_CHANNELS], sLastVolL[MAX_DIRECTSOUND_CHANNELS];

/* Options menu (main.c PortMenuSound*): the mixer's history is kept while
   the menu's clicks play on a silent mixer, and put back when it closes. */
static struct {
    int32_t histL[8][PORT_MIX_MAX], histR[8][PORT_MIX_MAX];
    s32 lastVolR[MAX_DIRECTSOUND_CHANNELS], lastVolL[MAX_DIRECTSOUND_CHANNELS];
} sMenuMix;

void PortM4aMixHold(int save) {
    if (save) {
        memcpy(sMenuMix.histL, sHistL, sizeof(sHistL));
        memcpy(sMenuMix.histR, sHistR, sizeof(sHistR));
        memcpy(sMenuMix.lastVolR, sLastVolR, sizeof(sLastVolR));
        memcpy(sMenuMix.lastVolL, sLastVolL, sizeof(sLastVolL));
        memset(sHistL, 0, sizeof(sHistL));
        memset(sHistR, 0, sizeof(sHistR));
    } else {
        memcpy(sHistL, sMenuMix.histL, sizeof(sHistL));
        memcpy(sHistR, sMenuMix.histR, sizeof(sHistR));
        memcpy(sLastVolR, sMenuMix.lastVolR, sizeof(sLastVolR));
        memcpy(sLastVolL, sMenuMix.lastVolL, sizeof(sLastVolL));
    }
}

static void MixChannel(SoundInfo* info, SoundChannel* chan, int n, int over) {
    WaveData* wav = chan->wav;
    u8 flags = chan->statusFlags;
    u32 env;
    s32 volR, volL;
    int64_t rampR, rampL, stepR, stepL; /* volume ramp, 16.16 */
    s8* loopStart = NULL;
    s32 loopLen = 0;
    int i;
    int32_t* outR = gPortMix.right;
    int32_t* outL = gPortMix.left;
    float gain = IsMusicChannel(chan) ? gPortMix.musicGain : gPortMix.sfxGain;

    if (!(flags & CHAN_ON))
        return;
    {
        static int check = -1;
        if (check < 0)
            check = SDL_getenv("KHCOM_AUDIOCHECK") != NULL;
        if (check)
            AudioCheck(chan);
    }

    if (flags & SOUND_CHANNEL_SF_START) {
        if (flags & SOUND_CHANNEL_SF_STOP) {
            chan->statusFlags = 0;
            return;
        }
        flags = SOUND_CHANNEL_SF_ENV_ATTACK;
        {
            int ci = (int)(chan - info->chans);
            if (ci >= 0 && ci < MAX_DIRECTSOUND_CHANNELS)
                sLastVolR[ci] = sLastVolL[ci] = 0; /* a new note rises from silence */
        }
        chan->currentPointer = wav->data;
        chan->count = wav->size;
        chan->fw = 0;
        env = 0;
        if (wav->status & 0xC000)
            flags |= 0x10;
        goto attack;
    }

    env = chan->envelopeVolume;
    if (flags & SOUND_CHANNEL_SF_IEC) {
        u8 old = chan->pseudoEchoLength;
        chan->pseudoEchoLength = old - 1;
        if (old < 2) {
            chan->statusFlags = 0;
            return;
        }
    } else if (flags & SOUND_CHANNEL_SF_STOP) {
        env = (env * chan->release) >> 8;
        if (env <= chan->pseudoEchoVolume) {
        echo:
            env = chan->pseudoEchoVolume;
            if (env == 0) {
                chan->statusFlags = 0;
                return;
            }
            flags |= SOUND_CHANNEL_SF_IEC;
        }
    } else if ((flags & SOUND_CHANNEL_SF_ENV) == SOUND_CHANNEL_SF_ENV_DECAY) {
        env = (env * chan->decay) >> 8;
        if (env <= chan->sustain) {
            env = chan->sustain;
            if (env == 0)
                goto echo;
            flags--;
        }
    } else if ((flags & SOUND_CHANNEL_SF_ENV) == SOUND_CHANNEL_SF_ENV_ATTACK) {
    attack:
        env += chan->attack;
        if (env >= 0xFF) {
            env = 0xFF;
            flags--;
        }
    }
    chan->statusFlags = flags;
    chan->envelopeVolume = (u8)env;

    env = (env * (info->masterVolume + 1)) >> 4;
    chan->envelopeVolumeRight = (u8)((chan->rightVolume * env) >> 8);
    chan->envelopeVolumeLeft = (u8)((chan->leftVolume * env) >> 8);
    volR = (s32)(chan->envelopeVolumeRight * gain * 256.0f);
    volL = (s32)(chan->envelopeVolumeLeft * gain * 256.0f);
    {
        int ci = (int)(chan - info->chans);
        if (ci >= 0 && ci < MAX_DIRECTSOUND_CHANNELS) {
            rampR = (int64_t)sLastVolR[ci] << 16;
            rampL = (int64_t)sLastVolL[ci] << 16;
            stepR = n > 0 ? (((int64_t)volR << 16) - rampR) / n : 0;
            stepL = n > 0 ? (((int64_t)volL << 16) - rampL) / n : 0;
            sLastVolR[ci] = volR;
            sLastVolL[ci] = volL;
        } else {
            rampR = (int64_t)volR << 16;
            rampL = (int64_t)volL << 16;
            stepR = stepL = 0;
        }
    }

    if (flags & 0x10) {
        loopStart = wav->data + wav->loopStart;
        loopLen = (s32)(wav->size - wav->loopStart);
    }

    if ((chan->type & TONEDATA_TYPE_FIX) && over == 1) {
        /* Fixed-rate sample: one source sample per output sample. */
        s8* p = chan->currentPointer;
        s32 count = (s32)chan->count;

        for (i = 0; i < n; i++) {
            s32 s = *p++;
            outR[i] += (s * (s32)(rampR >> 16)) >> 8;
            outL[i] += (s * (s32)(rampL >> 16)) >> 8;
            rampR += stepR;
            rampL += stepL;
            if (--count == 0) {
                if (loopLen) {
                    count = loopLen;
                    p = loopStart;
                } else {
                    chan->statusFlags = 0;
                    return;
                }
            }
        }
        chan->count = (u32)count;
        chan->currentPointer = p;
        return;
    }

    {
        /* Linear interpolation (Catmull-Rom with the experimental smooth
           audio option); positions in 23-bit fixed point. */
        int fixed = (chan->type & TONEDATA_TYPE_FIX) != 0;
        int smooth = gPortConfig.smoothAudio != 0;
        u32 step;
        u32 fw = chan->fw;
        s32 count = (s32)chan->count;
        s8* q;
        s32 cur, d;

        if (fixed) {
            /* Oversampled fixed-rate sample: currentPointer is the next sample. */
            step = (1u << 23) / over;
            q = chan->currentPointer;
            cur = q[0];
            q++;
            d = (count > 1 ? q[0] : (loopLen ? loopStart[0] : cur)) - cur;
        } else {
            step = (u32)(((u64)(u32)info->divFreq * chan->frequency) / over);
            q = chan->currentPointer;
            cur = q[0];
            q++;
            d = (count > 1 ? q[0] : (loopLen ? loopStart[0] : cur)) - cur;
        }

        for (i = 0; i < n; i++) {
            s32 s;
            u32 adv;

            if (smooth) {
                /* Catmull-Rom over the samples around the position (q[-1] is
                   the current one); neighbours outside the sample repeat the
                   edge, as the loop start follows the loop end */
                float t = fw * (1.0f / 8388608.0f);
                float p1 = (float)cur, p2 = (float)(cur + d);
                float p0 = q - 2 >= wav->data ? (float)q[-2] : p1;
                float p3 = count > 2 ? (float)q[1] :
                    (loopLen ? (float)loopStart[(2 - count) % loopLen] : p2);
                float v = p1 + 0.5f * t * (p2 - p0 + t * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3 +
                                                       t * (3.0f * (p1 - p2) + p3 - p0)));
                s = (s32)(v * 256.0f);
            } else {
                s = cur * 256 + (s32)(((int64_t)fw * d) >> 15);
            }

            outR[i] += (s32)(((int64_t)s * (rampR >> 16)) >> 16);
            outL[i] += (s32)(((int64_t)s * (rampL >> 16)) >> 16);
            rampR += stepR;
            rampL += stepL;

            fw += step;
            adv = fw >> 23;
            if (adv == 0)
                continue;
            fw &= 0x7FFFFF;
            count -= (s32)adv;
            if (count <= 0) {
                if (loopLen) {
                    s32 over9 = -count;
                    for (;;) {
                        count += loopLen;
                        if (count > 0)
                            break;
                        over9 -= loopLen;
                    }
                    q = loopStart + over9;
                    cur = *q;
                    q++;
                    d = (count > 1 ? *q : loopStart[0]) - cur;
                    continue;
                }
                chan->statusFlags = 0;
                return;
            }
            if (adv == 1) {
                cur += d;
            } else {
                q += adv - 1;
                cur = *q;
            }
            q++;
            d = (count > 1 ? *q : (loopLen ? loopStart[0] : cur)) - cur;
        }

        chan->fw = fw;
        chan->count = (u32)count;
        chan->currentPointer = fixed ? q - 1 : q - 1;
    }
}

void SoundMain(void) {
    SoundInfo* info = gSoundInfoPtr;
    int n, over, i, slot, next, period;
    int32_t* histR;
    int32_t* histL;

    if (info == NULL || info->ident != ID_NUMBER)
        return;
    info->ident++;

    if (info->MPlayMainHead != NULL)
        info->MPlayMainHead(info->musicPlayerHead);
    info->CgbSound();

    over = gPortMix.oversample;
    n = info->pcmSamplesPerVBlank * over;
    if (n > PORT_MIX_MAX)
        n = PORT_MIX_MAX;
    gPortMix.count = n;
    gPortMix.rate = info->pcmFreq * over;

    /* Slot of the PCM ring buffer this frame corresponds to. */
    period = info->pcmDmaPeriod ? info->pcmDmaPeriod : 1;
    if (period > 8)
        period = 8;
    slot = (info->pcmDmaCounter > 1) ? period - (info->pcmDmaCounter - 1) : 0;
    if (slot < 0 || slot >= period)
        slot = 0;
    next = (info->pcmDmaCounter == 2) ? 0 : (slot + 1) % period;
    histR = sHistR[slot];
    histL = sHistL[slot];

    if (info->reverb) {
        for (i = 0; i < n; i++) {
            int32_t v = histR[i] + histL[i] + sHistR[next][i] + sHistL[next][i];
            v = (v * info->reverb) >> 9;
            gPortMix.right[i] = v;
            gPortMix.left[i] = v;
        }
    } else {
        memset(gPortMix.right, 0, n * sizeof(int32_t));
        memset(gPortMix.left, 0, n * sizeof(int32_t));
    }

    for (i = 0; i < info->maxChans && i < MAX_DIRECTSOUND_CHANNELS; i++)
        MixChannel(info, &info->chans[i], n, over);

    /* Keep the 8-bit range of the hardware buffer for the reverb feedback. */
    for (i = 0; i < n; i++) {
        int32_t r = gPortMix.right[i], l = gPortMix.left[i];
        histR[i] = r > 32767 ? 32767 : (r < -32768 ? -32768 : r);
        histL[i] = l > 32767 ? 32767 : (l < -32768 ? -32768 : l);
    }
    gPortMix.ready = 1;
    info->ident = ID_NUMBER;
}

/* SoundMain runs the mixer itself; these labels only need to exist. */
char SoundMainRAM[4];
