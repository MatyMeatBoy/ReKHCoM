/**
 * pcm_audio.c
 * PCM Audio Playback
 */

#include "pcm_audio.h"
#include "sroll_api.h"
#include "gba/io_reg.h"
#include "types.h"

static SoundEntry sPcmPlaybackConfigs[13] = {
    {5734, 96, 62610},
    {7884, 132, 63408},
    {10512, 176, 63940},
    {13379, 224, 64282},
    {15768, 264, 64472},
    {18157, 304, 64612},
    {21024, 352, 64738},
    {26758, 448, 64909},
    {31536, 528, 65004},
    {36314, 608, 65073},
    {40137, 672, 65118},
    {42048, 704, 65137},
    {0, 0, 0},
};

static s32 sPcmActiveBufferIndex;
static s32 sPcmSamplesPerBuffer;
static s8 sPcmOutputBufferA[0x2C0];
static s8 sPcmOutputBufferB[0x2C0];
#ifdef PLATFORM_PC
/* The port: the same samples before the cartridge cuts them to 8 bits (>> 8). The staff roll's song is the
   game's one PCM stream, and at 8 bits it hisses well above the rest of the music (issue #641: "sounds really
   bad"); the mixer reads these instead (port_audio.c FifoRead), KHCOM_CREDITS_8BIT keeps the cartridge's. */
static s16 sPcmWideA[0x2C0];
static s16 sPcmWideB[0x2C0];
#define PCM_WIDE(v) ((s16)((v) > 32767 ? 32767 : (v) < -32768 ? -32768 : (v)))

/* the full-precision copy of the 8-bit buffer that holds addr (NULL: not one of them), and addr's index in it */
const s16* PcmPlaybackWide(u32 addr, s32* index) {
    u32 a = (u32)(s32)sPcmOutputBufferA, b = (u32)(s32)sPcmOutputBufferB;
    if (addr >= a && addr < a + sizeof(sPcmOutputBufferA)) { *index = (s32)(addr - a); return sPcmWideA; }
    if (addr >= b && addr < b + sizeof(sPcmOutputBufferB)) { *index = (s32)(addr - b); return sPcmWideB; }
    return 0;
}
#endif

#define DMA_SOUND_FIFO                                                        \
    ((DMA_START_SPECIAL | DMA_32BIT | DMA_REPEAT | DMA_DEST_FIXED) << 16)
u8 LookupPcmPlaybackConfig(u32 sampleRate, u16* timerReload, u32* samplesPerBuffer) {
    s32 i = 0;

    do {
        if (sPcmPlaybackConfigs[i].sampleRate == sampleRate) {
            *timerReload = sPcmPlaybackConfigs[i].timerReload;
            *samplesPerBuffer = sPcmPlaybackConfigs[i].samplesPerBuffer;
            return TRUE;
        }

        i++;
    } while (sPcmPlaybackConfigs[i].sampleRate != 0);

    return FALSE;
}

u8 PcmPlaybackInit(u32 sampleRate) {
    u16 timerReload;
    s32 i;

    if (!LookupPcmPlaybackConfig(sampleRate, &timerReload, (u32*)&sPcmSamplesPerBuffer)) {
        return FALSE;
    }

    REG_SOUNDCNT_H = (SOUND_CGB_MIX_FULL | SOUND_A_MIX_FULL | SOUND_A_RIGHT_OUTPUT | SOUND_A_LEFT_OUTPUT | SOUND_A_FIFO_RESET);
    REG_SOUNDCNT_X = SOUND_MASTER_ENABLE;
    REG_DMA1DAD = (s32)&REG_FIFO_A;
    REG_TM0CNT_L = timerReload;
    REG_DMA1CNT = DMA_SOUND_FIFO;

    for (i = 0; i < sPcmSamplesPerBuffer; i++) {
        sPcmOutputBufferA[i] = sPcmOutputBufferB[i] = 0;
    }

    sPcmActiveBufferIndex = 1;
    REG_DMA1SAD = (s32)sPcmOutputBufferA;
    return TRUE;
}

void PcmPlaybackStart() {
    REG_TM0CNT_H = TIMER_ENABLE;
    REG_DMA1CNT |= DMA_ENABLE << 16;
}

void PcmPlaybackStop() {
    REG_DMA1CNT = 0;
    REG_TM0CNT_H = 0;
    REG_SOUNDCNT_H |= SOUND_A_FIFO_RESET;
}

void PcmPlaybackUpdate() {
    s32* src;
    s8* dst;
    s32 pos;
    s32 i;

    src = GetDecodedAudioBuffer();
    pos = GetDecodedAudioReadPosition();
    REG_DMA1CNT ^= DMA_ENABLE << 16;
    REG_DMA1SAD = (s32)(sPcmActiveBufferIndex == 1 ? sPcmOutputBufferB : sPcmOutputBufferA);
    REG_DMA1CNT ^= DMA_ENABLE << 16;
    sPcmActiveBufferIndex = sPcmActiveBufferIndex == 1 ? 2 : 1;
    dst = sPcmActiveBufferIndex == 1 ? sPcmOutputBufferB : sPcmOutputBufferA;
#ifdef PLATFORM_PC
    {
        s16* wide = sPcmActiveBufferIndex == 1 ? sPcmWideB : sPcmWideA;
        s32 p = pos;
        for (i = 0; i < sPcmSamplesPerBuffer; i++, p++)
            wide[i] = PCM_WIDE(src[p & 0x7FF]);
    }
#endif

    if (pos + sPcmSamplesPerBuffer <= 0x7FF) {
        for (i = 0; i < sPcmSamplesPerBuffer; i++) {
            dst[i] = src[pos] >> 8;
            pos++;
        }

        SetDecodedAudioReadPosition(pos);
    } else {
        for (i = 0; i < 0x800 - pos; i++) {
            dst[i] = src[pos + i] >> 8;
        }

        for (; i < sPcmSamplesPerBuffer; i++) {
            dst[i] = src[pos + i - 0x800] >> 8;
        }

        SetDecodedAudioReadPosition(pos + sPcmSamplesPerBuffer - 0x800);
    }
}
