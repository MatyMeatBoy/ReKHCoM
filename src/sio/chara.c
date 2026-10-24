/**
 * chara.c
 * Link Connection and Command Protocol
 */

#include "macros.h"
#include "registration_data.h"
#include "chara_api.h"
#include "display.h"
#include "pallet.h"
#include "sio_api.h"
#include "key_state.h"
#include "chara.h"
#include "gba/keys.h"
#include "system_state.h"
#include "chara_types.h"
#include "malloc.h"
#include "fade.h"
#include "songs.h"
#include "mode_sio_api.h"
#include "battle_actor.h"
#include "battle_work.h"
#include "btl_effect.h"
#include "card_api.h"
#include "card_types.h"
#include "engine_math.h"
#include "game_state.h"
#include "gba/defines.h"
#include "key.h"
#include "m4a_song.h"
#include "mode.h"
#include "player_progression_types.h"
#include "taskpool.h"
#include "types.h"
#include "gba/macro.h"
#include <stddef.h>
#include "lockon.h"
#include "card_map_anim.h"

u32 gDebugLogC[100] EWRAM_COMMON(16);
u32 gVBlankTimerElapsed EWRAM_COMMON(4);
s16 gSioCancelTimer EWRAM_COMMON(4);
s16 gSioAutoConnectTimer EWRAM_COMMON(4);
void (*gSioCancelCallback)() EWRAM_COMMON(4);
u32 gVBlankTimerBase EWRAM_COMMON(4);
s16 gSioAutoConnectState EWRAM_COMMON(8);
u32 gDebugLogSeq EWRAM_COMMON(4);
u32 gDebugLogB[100] EWRAM_COMMON(16);
u32 gDebugLogA[100] EWRAM_COMMON(16);
u32 gDebugLogD[100] EWRAM_COMMON(16);
u16 gDebugLogIndex EWRAM_COMMON(4);
s8 gLinkDecksAllocated EWRAM_COMMON(4);
u16 gSioExchangeSeqEnd EWRAM_COMMON(4);
Deck* gSioSendDeck EWRAM_COMMON(4);
s8 gSioHandshakeDone EWRAM_COMMON(4);
s8 gSioHandshakeConfirm EWRAM_COMMON(4);
u16 gSioRelayKeysB EWRAM_COMMON(4);
s8 gSioConnectRetries EWRAM_COMMON(4);
u16* gSioExchangeSendData EWRAM_COMMON(4);
#ifdef VERSION_EU
u16 gRandomPartnerATimer EWRAM_COMMON(4);
#endif
u16 gSioExchangeSeq EWRAM_COMMON(4);
Deck* gSioRecvDeck EWRAM_COMMON(4);
#ifdef VERSION_EU
u16 gRandomPartnerDpadTimer EWRAM_COMMON(4);
#endif
u16* gSioExchangeRecvData EWRAM_COMMON(4);
s8 gSioConnectAccepted EWRAM_COMMON(4);
#ifdef VERSION_EU
u16 gRandomPartnerDpad EWRAM_COMMON(4);
#endif
s8 gSioHandshakeAck EWRAM_COMMON(4);
s8 gSioConnected EWRAM_COMMON(4);
void (*gSioConnectCallback)() EWRAM_COMMON(4);
u16 gSioConnectId EWRAM_COMMON(4);
u16 gSioRelayKeysA EWRAM_COMMON(4);
u16 gUnk_0203C3B8 EWRAM_COMMON(4);

static u8 sMaskFadeTileMasks[1440] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xF0,
    0xF0, 0x0F, 0xFF, 0xFF, 0x0F, 0xF0, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0x0F, 0xF0, 0xFF, 0xFF, 0xF0,
    0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xF0, 0x0F, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xF0,
    0xF0, 0x0F, 0xFF, 0x0F, 0x0F, 0xF0, 0xFF, 0x0F, 0x0F, 0x0F, 0xFF, 0x0F, 0xF0, 0x0F, 0xFF, 0xF0,
    0xF0, 0xFF, 0x0F, 0xFF, 0xF0, 0x0F, 0xF0, 0xFF, 0xF0, 0x0F, 0xFF, 0xF0, 0xFF, 0x0F, 0xFF, 0xF0,
    0xF0, 0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0xFF, 0x0F, 0x0F, 0x0F, 0xF0, 0x00, 0xF0, 0x0F, 0xFF, 0xF0,
    0x00, 0xFF, 0x0F, 0xFF, 0x00, 0x00, 0xF0, 0xFF, 0xF0, 0x0F, 0xFF, 0xF0, 0xF0, 0x0F, 0xFF, 0x00,
    0xF0, 0x00, 0x0F, 0x0F, 0x0F, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0x00, 0xF0, 0x0F, 0xF0, 0xF0,
    0x00, 0xF0, 0x0F, 0xF0, 0x00, 0x00, 0xF0, 0x0F, 0x00, 0x0F, 0x0F, 0xF0, 0xF0, 0x0F, 0xFF, 0x00,
    0x00, 0x00, 0x0F, 0x0F, 0x00, 0xF0, 0x0F, 0x0F, 0x00, 0x0F, 0xF0, 0x00, 0xF0, 0x0F, 0x00, 0xF0,
    0x00, 0xF0, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0xF0, 0x0F, 0x0F, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0xF0, 0x00, 0xF0, 0x0F, 0x00, 0xF0,
    0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x0F, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xF0, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x0F, 0xFF, 0xF0, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF,
    0xF0, 0xFF, 0xFF, 0x0F, 0x0F, 0xFF, 0xFF, 0x0F, 0xF0, 0xFF, 0xF0, 0xFF, 0xF0, 0xFF, 0x0F, 0xFF,
    0xFF, 0x0F, 0xFF, 0xF0, 0x0F, 0xF0, 0xFF, 0xF0, 0xFF, 0xFF, 0x0F, 0xF0, 0xFF, 0xFF, 0x0F, 0xFF,
    0xF0, 0xF0, 0xFF, 0x0F, 0x0F, 0xF0, 0xF0, 0x0F, 0xF0, 0xF0, 0xF0, 0xFF, 0xF0, 0x0F, 0x0F, 0xFF,
    0xFF, 0x0F, 0x0F, 0xF0, 0x0F, 0xF0, 0xFF, 0xF0, 0x0F, 0xFF, 0x0F, 0xF0, 0xFF, 0xF0, 0x0F, 0xFF,
    0x00, 0xF0, 0xF0, 0x0F, 0x0F, 0xF0, 0x00, 0x0F, 0xF0, 0xF0, 0xF0, 0xFF, 0x00, 0x0F, 0x0F, 0x0F,
    0xFF, 0x0F, 0x00, 0xF0, 0x0F, 0xF0, 0xFF, 0xF0, 0x0F, 0xFF, 0x0F, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F,
    0x00, 0xF0, 0xF0, 0x0F, 0x0F, 0xF0, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x0F, 0x00, 0x0F,
    0xFF, 0x0F, 0x00, 0x00, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0xF0, 0x0F, 0x00,
    0x00, 0xF0, 0xF0, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00,
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0x00, 0x0F, 0x00,
    0x00, 0xF0, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0xF0, 0x00, 0x00, 0x0F, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xF0,
    0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x0F, 0xF0, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0x0F, 0x0F, 0xFF, 0xF0,
    0xF0, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xFF, 0xF0, 0xFF, 0x0F,
    0x0F, 0xF0, 0xFF, 0x0F, 0xFF, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0x00, 0x0F, 0xF0, 0xF0,
    0xF0, 0x0F, 0xFF, 0x0F, 0xF0, 0xFF, 0x00, 0xFF, 0xF0, 0xF0, 0x0F, 0xFF, 0xFF, 0xF0, 0xF0, 0x0F,
    0x0F, 0xF0, 0xF0, 0x0F, 0x0F, 0xF0, 0xF0, 0xFF, 0xF0, 0xFF, 0xF0, 0xF0, 0x00, 0x0F, 0x00, 0xF0,
    0x00, 0x0F, 0xF0, 0x0F, 0x00, 0xFF, 0x00, 0xFF, 0xF0, 0xF0, 0x0F, 0xFF, 0xF0, 0xF0, 0xF0, 0x0F,
    0x00, 0xF0, 0xF0, 0x0F, 0x0F, 0xF0, 0xF0, 0x0F, 0xF0, 0x00, 0xF0, 0xF0, 0x00, 0x0F, 0x00, 0xF0,
    0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0xF0, 0xF0, 0xF0, 0x0F, 0xFF, 0xF0, 0xF0, 0x00, 0x0F,
    0x00, 0x00, 0xF0, 0x0F, 0x00, 0xF0, 0xF0, 0x0F, 0x00, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0xF0,
    0x00, 0x0F, 0x00, 0x00, 0x00, 0x0F, 0x00, 0xF0, 0x00, 0xF0, 0x0F, 0xF0, 0xF0, 0xF0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0,
    0x00, 0x0F, 0x00, 0x00, 0x00, 0x0F, 0x00, 0xF0, 0x00, 0x00, 0x0F, 0x00, 0xF0, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF0, 0x0F, 0xF0, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0x0F,
    0x0F, 0x0F, 0xF0, 0xFF, 0x0F, 0xF0, 0x0F, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF,
    0xF0, 0x0F, 0xF0, 0x0F, 0x00, 0xF0, 0x0F, 0x0F, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0x00,
    0x0F, 0x0F, 0x00, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0xFF, 0x0F, 0x0F, 0xFF, 0xFF, 0x0F, 0x0F, 0xF0,
    0x00, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0x0F, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x00, 0xF0, 0x00,
    0x0F, 0x0F, 0x00, 0xF0, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0xF0, 0x0F, 0x0F, 0x0F, 0x00,
    0x00, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x00,
    0x0F, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00,
    0x00, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00,
    0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00,
    0x00, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00,
    0x00, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xF0, 0xF0, 0xFF, 0xFF, 0x0F,
    0x0F, 0xFF, 0x0F, 0xFF, 0xFF, 0xF0, 0xF0, 0xFF, 0x0F, 0xF0, 0xFF, 0x0F, 0x0F, 0x0F, 0xFF, 0x0F,
    0x00, 0x0F, 0xF0, 0x0F, 0x0F, 0xF0, 0xFF, 0x0F, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0xF0, 0x00,
    0x0F, 0xF0, 0x0F, 0xFF, 0x0F, 0x00, 0xF0, 0xFF, 0x0F, 0xF0, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0x00,
    0x00, 0x00, 0x00, 0x0F, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00,
    0x0F, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F, 0x00,
    0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00,
    0x0F, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x0F, 0x0F, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x0F, 0x0F, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static CharaObj* sCharaObj;
static TaskPool sCharaTaskPool;

void task_chara_mask_fade_0(MaskFadeWork* work, MaskFadeArgs* args) {
    s32 i;

    work->tiles = args->tiles;
    work->tileCount = args->tileCount;
    work->timer = 0;
    work->step = 1;
    work->stepDelay = args->stepDelay;

    for (i = 0; i < work->tileCount; i++) {
        work->patterns[i] = GetRandom() % 5;
    }
}

u8 task_chara_mask_fade_1(MaskFadeWork* work) {
    s32 i;
    s32 j;

    if (++work->timer > work->stepDelay) {
        work->timer = 0;

        for (i = 0; i < work->tileCount; i++) {
            CpuFastCopy(work->tiles + i * 32, work->tileBuffer, sizeof(work->tileBuffer));

            for (j = 0; j <= 31; j++) {
                work->maskedTile[j] = work->tileBuffer[j] & sMaskFadeTileMasks[j + work->step * 32 + work->patterns[i] * 288];
            }

            CpuFastCopy(work->maskedTile, work->tiles + i * 32, sizeof(work->maskedTile));
        }

        work->step++;

        if (work->step > 8) {
            work->step = 0;
            return 0;
        }
    }

    return 1;
}

void task_chara_mask_fade_2() {
}

void task_chara_mask_fade_3() {
}

enum ChgCardObjState {
    CHG_CARD_OBJ_STATE_DELAY,
    CHG_CARD_OBJ_STATE_FLY
};

void task_chgCardObj_0(ChgCardObjWork* work, ChgCardObjParam* arg) {
    s32 x;
    s32 y;

    work->x = arg->x;
    work->y = arg->y;
    work->scaleX = arg->scaleX;
    work->scaleY = arg->scaleY;
    work->angle = arg->angle;
    work->visible = arg->visible;
    work->targetX = arg->targetX;
    work->targetY = arg->targetY;
    work->delay = arg->delay;
    work->scale = Q_8_8(1);
    work->flipAngleY = 0;
    work->flipAngleX = 0;
    work->speed = 0x300;
    work->decel = 2;
    x = work->targetX - *work->x;
    y = work->targetY - *work->y;
    work->distance = NormalizeVector2D8(&x, &y);
    work->dirX = -x;
    work->dirY = -y;
    work->timer = 0;
    work->unk_02 = 0;
    work->state = CHG_CARD_OBJ_STATE_DELAY;
}

static inline u8 ChgCardRotation(ChgCardObjWork* work, u8 rotation) {
    u8 phase;

    if (work->scale < 0) {
        phase = -rotation;
    } else {
        phase = rotation;
    }

    return phase;
}

u8 task_chgCardObj_1(ChgCardObjWork* work) {
    u8 phase;
    u8 zero;
    u8 angleA;
    s32 x;
    s32 y;
    s32* scaleX;
    s32* scaleY;

    switch (work->state) {
    case CHG_CARD_OBJ_STATE_DELAY:
        if (work->timer >= work->delay) {
            work->timer = 0;
            work->state++;
        } else {
            work->timer++;
        }

        break;
    case CHG_CARD_OBJ_STATE_FLY:
        *work->x += (work->dirX * work->speed) >> 8;
        *work->y += (work->dirY * work->speed) >> 8;
        *work->angle += 32;
        angleA = work->flipAngleY + ((64 - work->flipAngleY) >> 4);
        zero = 0;
        work->flipAngleY = angleA;
        work->flipAngleX = zero;
        work->distance = VectorLength2D(work->targetX - *work->x, work->targetY - *work->y);
        work->speed -= work->decel;
        work->decel += 2;
        phase = ChgCardRotation(work, 128);
        scaleX = work->scaleX;
        *scaleX = (-COS(work->flipAngleX + phase) * work->scale) >> 8;
        scaleY = work->scaleY;
        *scaleY = (-COS(work->flipAngleY + 128) * work->scale) >> 8;

        if (*scaleX >= -2 && *scaleX <= 2) {
            *scaleX = 2;
        }

        if (*scaleY >= -2 && *scaleY <= 2) {
            *scaleY = 2;
        }

        if (work->speed < 0) {
            x = work->targetX - *work->x;
            y = work->targetY - *work->y;
            NormalizeVector2D8(&x, &y);
            work->dirX = -x;
            work->dirY = -y;

            if (work->distance <= 0x7FF) {
                m4aSongNumStart(SONG_SYS_ITEMGET);
                *work->visible = zero;
                return 0;
            }
        }

        break;
    }

    return 1;
}

void task_chgCardObj_2() {
}

void task_chgCardObj_3() {
}

u8 SioConnectUpdate() {
    u32* status;

    status = &gSioStatus;
    *status = SioRunStateMachine(&gSioHandshakeRequest, gSioSendFrame, gSioRecvFrame);
    gSioPlayerId = gSioStatus & SIO_STAT_LOCAL_ID;
    gSioPlayerCount = (gSioStatus & SIO_STAT_PLAYER_COUNT) >> SIO_STAT_PLAYER_COUNT_SHIFT;
    gSioHandshake = (gSioStatus & SIO_STAT_HANDSHAKE) >> SIO_STAT_HANDSHAKE_SHIFT;

    if ((gSioStatus & SIO_STAT_CONNECTED) && gSioPlayerId <= 1) {
        SioConnectSend();

        if ((gSioStatus & SIO_STAT_RECV_EMPTY) == 0) {
            gSioLinkResult = SioConnectRecv();
        }

        if (gSioStatus & SIO_STAT_ERRORS) {
            if (gSioConnected == TRUE) {
                gSioErrorFrameCount++;

                if (gSioErrorFrameCount > 180) {
                    gSioLinkResult = SIO_LINK_RESULT_ERROR;
                }
            }
        }

        if (gSioLinkResult == SIO_LINK_RESULT_ERROR) {
            gSystemFlags &= ~SYSTEM_FLAG_LINK_ACTIVE;
            gSioErrorStatus = gSioStatus;
            ModeRequest(&gModeSioError, 0);
            return gSioLinkResult;
        }
    } else {
        if (GetKeysPressed() & B_BUTTON) {
            gSioCancelTimer = 10;
        }

        if (gSioCancelTimer > 0) {
            gSioCancelTimer--;

            if (gSioCancelTimer == 0) {
                SioShutdown();

                if (gSioCancelCallback != NULL) {
                    gSioCancelCallback();
                }
            }
        }
    }

    return gSioLinkResult;
}

u8 SioLinkUpdate() {
    u32* status;

    status = &gSioStatus;
    *status = SioTransferFrames(&gSioHandshakeRequest, gSioSendFrame, gSioRecvFrame);
    gSioPlayerId = gSioStatus & SIO_STAT_LOCAL_ID;
    gSioPlayerCount = (gSioStatus & SIO_STAT_PLAYER_COUNT) >> SIO_STAT_PLAYER_COUNT_SHIFT;
    gSioHandshake = (gSioStatus & SIO_STAT_HANDSHAKE) >> SIO_STAT_HANDSHAKE_SHIFT;

    if ((gSioStatus & SIO_STAT_CONNECTED) && gSioPlayerId <= 1) {
        if (gSioLinkRecvCallback != NULL) {
            gSioLinkResult = gSioLinkRecvCallback();
        }

        if (gSioLinkSendCallback != NULL) {
            gSioLinkSendCallback();
        }

        if (gSioStatus & SIO_STAT_ERRORS) {
            if (gSioConnected == TRUE) {
                gSioErrorFrameCount++;

                if (gSioErrorFrameCount > 180) {
                    gSystemFlags &= ~SYSTEM_FLAG_LINK_ACTIVE;
                    gSioErrorStatus = gSioStatus;
                    ModeRequest(&gModeSioError, 0);
                    gSioLinkResult = SIO_LINK_RESULT_ERROR;
                }
            }
        }
    }

    return gSioLinkResult;
}

u8 SioConnectUpdateAuto() {
    u32* status;

    status = &gSioStatus;
    *status = SioRunStateMachine(&gSioHandshakeRequest, gSioSendFrame, gSioRecvFrame);
    gSioPlayerId = gSioStatus & SIO_STAT_LOCAL_ID;
    gSioPlayerCount = (gSioStatus & SIO_STAT_PLAYER_COUNT) >> SIO_STAT_PLAYER_COUNT_SHIFT;
    gSioHandshake = (gSioStatus & SIO_STAT_HANDSHAKE) >> SIO_STAT_HANDSHAKE_SHIFT;

    if ((gSioStatus & SIO_STAT_CONNECTED) && gSioPlayerId <= 1) {
        SioConnectSendAuto();

        if ((gSioStatus & SIO_STAT_RECV_EMPTY) == 0) {
            gSioLinkResult = SioConnectRecvAuto();
        }

        if (gSioStatus & SIO_STAT_ERRORS) {
            if (gSioConnected == TRUE) {
                gSioErrorFrameCount++;

                if (gSioErrorFrameCount > 180) {
                    gSioLinkResult = SIO_LINK_RESULT_ERROR;
                }
            }
        }

        if (gSioLinkResult == SIO_LINK_RESULT_ERROR) {
            gSystemFlags &= ~SYSTEM_FLAG_LINK_ACTIVE;
            gSioErrorStatus = gSioStatus;
            ModeRequest(&gModeSioError, 0);
            return gSioLinkResult;
        }
    }

    return gSioLinkResult;
}

void FreeLinkDecks() {
    FreeLinkSendDeck();
    FreeLinkPartnerDeck();
}

void SioLinkClose() {
    if (gSystemFlags & SYSTEM_FLAG_LINK_ACTIVE) {
        SioShutdown();
        gSystemFlags &= ~SYSTEM_FLAG_LINK_ACTIVE;
    }

    if (gSystemFlags & SYSTEM_FLAG_DMA3_FLUSH_CPU) {
        gSystemFlags &= ~SYSTEM_FLAG_DMA3_FLUSH_CPU;
    }

    if (gLinkDecksAllocated == TRUE) {
        FreeLinkDecks();
        gLinkDecksAllocated = FALSE;
    }
}

void DebugLogClear() {
    s32 i;

    for (i = 0; i < 100; i++) {
        gDebugLogA[i] = 0;
        gDebugLogB[i] = 0;
        gDebugLogC[i] = 0;
        gDebugLogD[i] = 0;
    }

    gDebugLogIndex = 0;
    gDebugLogSeq = 0;
}

void DebugLogAdd(u16 a, u16 b, u16 c, u16 d) {
    gDebugLogA[gDebugLogIndex] = (gDebugLogSeq << 24) | a;
    gDebugLogB[gDebugLogIndex] = (gDebugLogSeq << 24) | b;
    gDebugLogC[gDebugLogIndex] = (gDebugLogSeq << 24) | c;
    gDebugLogD[gDebugLogIndex] = (gDebugLogSeq << 24) | d;
    gDebugLogIndex++;

    if (gDebugLogIndex > 99) {
        gDebugLogIndex = 0;
    }

    DebugLogNextSeq();
}

void DebugLogResetSeq() {
    gDebugLogSeq = 0;
}

void DebugLogNextSeq() {
    gDebugLogSeq = (gDebugLogSeq + 1) & 0xFF;
}

void VBlankTimerStart() {
    gVBlankTimerElapsed = 0;
    gVBlankTimerBase = gVBlankCounter;
}

void VBlankTimerUpdate() {
    gVBlankTimerElapsed = (gVBlankCounter - gVBlankTimerBase) & 0xFFFF;
}

u8 SioHasError() {
    if (gSioStatus & SIO_STAT_ERRORS) {
        return TRUE;
    }

    return FALSE;
}

/* frames an accepted side waits for the other's accept: the cable answers at once; over the network the other's
   words come a round trip later (port/progress/ONLINE_ESTUDIO.md), so it waits up to two seconds */
#ifdef PLATFORM_PC
#include "port/port_netlink.h"
#define SIO_CONNECT_RETRY_LIMIT (PortNetlinkRole() ? 120 : 10)
#else
#define SIO_CONNECT_RETRY_LIMIT 10
#endif

#ifdef VERSION_JP
#define SIO_CONNECT_ID 0xC0F0
#else
#ifdef VERSION_EU
#define SIO_CONNECT_ID 0xC2F0
#else
#define SIO_CONNECT_ID 0xC1F0
#endif
#endif

enum SioProtocolCommand {
    SIO_CMD_FILLER = 0xDDDD,
    SIO_CMD_CONNECT_REQUEST = 0xFEFE,
    SIO_CMD_CONNECT_CANCEL = 0xAFAF,
    SIO_CMD_CONNECT_ACCEPT = 0xECEC,
    SIO_CMD_SYNC_CONFIRM = 0xDF89,
    SIO_CMD_EXCHANGE_REQUEST = 0x1BFE,
    SIO_CMD_EXCHANGE_ACCEPT = 0xC5A0,
    SIO_CMD_AUTO_CONNECT_SYNC = 0x2811
};

enum SioAutoConnectState {
    SIO_AUTO_CONNECT_STATE_CONNECT,
    SIO_AUTO_CONNECT_STATE_START_LINK,
    SIO_AUTO_CONNECT_STATE_SYNC,
    SIO_AUTO_CONNECT_STATE_DONE
};

void SioAutoConnectStart() {
    SioReset();
    SioConnectInit(SioAutoConnectOnConnect, NULL, SIO_CONNECT_MODE_BATTLE);
    gSioAutoConnectState = SIO_AUTO_CONNECT_STATE_CONNECT;
    gSioAutoConnectTimer = 0;
}

u8 SioAutoConnectUpdate() {
    switch (gSioAutoConnectState) {
    case SIO_AUTO_CONNECT_STATE_CONNECT:
        SioConnectUpdateAuto();
        break;
    case SIO_AUTO_CONNECT_STATE_START_LINK:
        gSioAutoConnectTimer++;

        if (gSioAutoConnectTimer > 4) {
            gSioAutoConnectTimer = 0;
            SioSetLinkCallbacks(SioKeySyncSend, SioKeySyncRecv);
            gSystemFlags |= SYSTEM_FLAG_LINK_ACTIVE;
            gSystemFlags &= ~SYSTEM_FLAG_DMA3_FLUSH_CPU;
            gSioAutoConnectState++;
        }

        break;
    case SIO_AUTO_CONNECT_STATE_SYNC:
        gSioAutoConnectTimer++;

        if (gSioAutoConnectTimer > 4) {
            gSioSendFrame[1] = SIO_CMD_AUTO_CONNECT_SYNC;

            if (gSioRecvFrame[1][0] == SIO_CMD_AUTO_CONNECT_SYNC && gSioRecvFrame[1][1] == gSioRecvFrame[1][0]) {
                gSioAutoConnectTimer = 0;
                gSioAutoConnectState++;
            }
        }

        break;
    default:
        return 1;
    }

    return 0;
}

void SioAutoConnectOnConnect() {
    gSioAutoConnectState = SIO_AUTO_CONNECT_STATE_START_LINK;
}

void SioConnectInit(void (*onConnect)(), void (*onCancel)(), u8 mode) {
    gSioConnectId = (mode & 0xF) | SIO_CONNECT_ID;
    gSioConnectAccepted = FALSE;
    gSioConnected = FALSE;
    gSioConnectRetries = 0;
    gSioConnectCallback = onConnect;
    gSioCancelCallback = onCancel;
    gSioCancelTimer = 0;
    gSioSendFrame[0] = SIO_CMD_FILLER;
    gSioSendFrame[1] = SIO_CMD_FILLER;
}

s32 SioConnectSend() {
    u16* param;
    u16* send;
    s32 i;

    if (!gSioConnected) {
        if (!gSioConnectAccepted) {
            if (GetKeysPressed() & A_BUTTON) {
                gSioSendFrame[0] = SIO_CMD_CONNECT_REQUEST;
                send = gSioSendFrame;
                param = &gSioConnectId;
            } else if (GetKeysPressed() & B_BUTTON) {
                gSioSendFrame[0] = SIO_CMD_CONNECT_CANCEL;
                send = gSioSendFrame;
                param = &gSioConnectId;
            } else {
                send = gSioSendFrame;
                param = &gSioConnectId;

                for (i = 0; i < 4; i++) {
                    gSioSendFrame[i] = 0;
                }
            }

            send[1] = *param;
        } else {
            gSioSendFrame[0] = SIO_CMD_CONNECT_ACCEPT;
        }
    } else {
        for (i = 0; i < 4; i++) {
            gSioSendFrame[i] = 0;
        }
    }

    return 0;
}

s32 SioConnectRecv() {
    u16 cancelWord;
    u16 sentWord;

    if (!gSioConnected) {
        if (!gSioConnectAccepted) {
            if (gSioRecvFrame[0][0] == SIO_CMD_CONNECT_REQUEST || gSioRecvFrame[0][1] == SIO_CMD_CONNECT_REQUEST) {
                if (gSioRecvFrame[1][0] == gSioConnectId && gSioRecvFrame[1][1] == gSioRecvFrame[1][0]) {
                    gSioConnectAccepted = TRUE;
                }
            } else {
                cancelWord = SIO_CMD_CONNECT_CANCEL;

                if (gSioRecvFrame[0][0] == cancelWord || gSioRecvFrame[0][1] == cancelWord) {
                    SioShutdown();
                    sentWord = gSioPlayerId == SIO_PLAYER_PARENT ? gSioRecvFrame[0][0] : gSioRecvFrame[0][1];

                    if (sentWord == cancelWord) {
                        if (gSioCancelCallback != NULL) {
                            gSioCancelCallback();
                        }
                    }
                }
            }
        } else if (gSioRecvFrame[0][0] == SIO_CMD_CONNECT_ACCEPT) {
            gSioConnected = TRUE;

            if (gSioConnectCallback != NULL) {
                gSioConnectCallback();
            }
        } else {
            gSioConnectRetries++;

            if (gSioConnectRetries > SIO_CONNECT_RETRY_LIMIT) {
                return SIO_LINK_RESULT_ERROR;
            }
        }
    }

    return SIO_LINK_RESULT_NONE;
}

s32 SioConnectSendAuto() {
    s32 i;

    if (!gSioConnected) {
        if (!gSioConnectAccepted) {
            gSioSendFrame[0] = SIO_CMD_CONNECT_REQUEST;
        } else {
            gSioSendFrame[0] = SIO_CMD_CONNECT_ACCEPT;
        }
    } else {
        for (i = 0; i < 4; i++) {
            gSioSendFrame[i] = 0;
        }
    }

    return 0;
}

s32 SioConnectRecvAuto() {
    if (!gSioConnected) {
        if (!gSioConnectAccepted) {
            if (gSioRecvFrame[0][0] == SIO_CMD_CONNECT_REQUEST || gSioRecvFrame[0][1] == SIO_CMD_CONNECT_REQUEST) {
                gSioConnectAccepted = TRUE;
            }
        } else if (gSioRecvFrame[0][0] == SIO_CMD_CONNECT_ACCEPT) {
            gSioConnected = TRUE;

            if (gSioConnectCallback != NULL) {
                gSioConnectCallback();
            }
        } else {
            gSioConnectRetries++;

            if (gSioConnectRetries > SIO_CONNECT_RETRY_LIMIT) {
                return SIO_LINK_RESULT_ERROR;
            }
        }
    }

    return SIO_LINK_RESULT_NONE;
}

void SioCommandReset() {
    SioCommandClearSend();
    SioCommandClearRecv();
}

void SioCommandClearSend() {
    s32 i;

    for (i = 0; i < 4; i++) {
        gSioCommandSend[i] = 0;
    }
}

void SioCommandClearRecv() {
    s32 i;
    s32 j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 2; j++) {
            gSioCommandRecv[i][j] = 0;
        }
    }
}

s32 SioCommandSend() {
    gSioCommandSend[0] = SIO_CMD_DATA;
    gSioSendFrame[0] = gSioCommandSend[0];
    gSioSendFrame[1] = gSioCommandSend[1];
    gSioSendFrame[2] = gSioCommandSend[2];
    gSioSendFrame[3] = gSioCommandSend[3];
    SioCommandClearSend();
    return 0;
}

s32 SioCommandRecv() {
    if (gSioRecvFrame[0][0] == SIO_CMD_DATA) {
        gSioCommandRecv[0][0] = gSioRecvFrame[0][0];
        gSioCommandRecv[1][0] = gSioRecvFrame[1][0];
        gSioCommandRecv[2][0] = gSioRecvFrame[2][0];
        gSioCommandRecv[3][0] = gSioRecvFrame[3][0];
    }

    if (gSioRecvFrame[0][1] == SIO_CMD_DATA) {
        gSioCommandRecv[0][1] = gSioRecvFrame[0][1];
        gSioCommandRecv[1][1] = gSioRecvFrame[1][1];
        gSioCommandRecv[2][1] = gSioRecvFrame[2][1];
        gSioCommandRecv[3][1] = gSioRecvFrame[3][1];
    }

    return SIO_LINK_RESULT_NONE;
}

void SioSetLinkCallbacks(s32 (*send)(), s32 (*recv)()) {
    s32 i;
    s32 j;
    s32 (**recvCallback)();
    u16* relayKeysA;
    u16* relayKeysB;
    s32 (**sendCallback)();

    gSioConnectRetries = 0;
    gSioErrorStatus = 0;
    gSioLinkResult = SIO_LINK_RESULT_NONE;
    sendCallback = &gSioLinkSendCallback;
    recvCallback = &gSioLinkRecvCallback;
    relayKeysA = &gSioRelayKeysA;
    relayKeysB = &gSioRelayKeysB;

    for (i = 0; i < 4; i++) {
        gSioSendFrame[i] = 0;
    }

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 4; j++) {
            gSioRecvFrame[j][i] = 0;
        }
    }

    *sendCallback = send;
    *recvCallback = recv;
    *relayKeysA = 0;
    *relayKeysB = 0;
}

s32 SioKeySyncSend() {
    if (gSioPlayerId == SIO_PLAYER_PARENT) {
        gSioSendFrame[0] = SIO_CMD_DATA;
        gSioSendFrame[1] = GetKeysHeld() & KEYS_MASK;
        gSioSendFrame[2] = gSioRelayKeysA;
        gSioSendFrame[3] = gSioRelayKeysB;
    } else {
        gSioSendFrame[0] = SIO_CMD_DATA;
        gSioSendFrame[1] = GetKeysHeld() & KEYS_MASK;
        gSioSendFrame[2] = 0x1234;
        gSioSendFrame[3] = 0x3456;
    }

    return 0;
}

s32 SioKeySyncRecv() {
    if (gSioPlayerId == SIO_PLAYER_PARENT) {
        if (gSioRecvFrame[0][0] == SIO_CMD_DATA && gSioRecvFrame[0][1] == gSioRecvFrame[0][0]) {
            gSioRelayKeysA = gSioRecvFrame[1][0];
            gSioRelayKeysB = gSioRecvFrame[1][1];
            gSioStatus &= ~SIO_STAT_RECV_EMPTY;
        } else {
            gSioStatus |= SIO_STAT_RECV_EMPTY;
        }

        if (gSioRecvFrame[0][0] == SIO_CMD_DATA) {
            SioKeyStateUpdateA(gSioRecvFrame[2][0]);
            SioKeyStateUpdateB(gSioRecvFrame[3][0]);
            gSioStatus &= ~SIO_STAT_RECV_EMPTY;
        } else {
            gSioStatus |= SIO_STAT_RECV_EMPTY;
        }
    } else {
        if (gSioRecvFrame[0][0] == SIO_CMD_DATA) {
            SioKeyStateUpdateA(gSioRecvFrame[2][0]);
            SioKeyStateUpdateB(gSioRecvFrame[3][0]);
            gSioStatus &= ~SIO_STAT_RECV_EMPTY;
        } else {
            gSioStatus |= SIO_STAT_RECV_EMPTY;
        }
    }

    return SIO_LINK_RESULT_NONE;
}

void SioPrepareDeckExchange() {
    Deck* sendDeck;
    Deck* recvDeck;

    sendDeck = CreateLinkSendDeck();
    gSioSendDeck = sendDeck;
    recvDeck = CreateLinkPartnerDeck();
    gSioRecvDeck = recvDeck;
    gLinkDecksAllocated = TRUE;
    gSioExchangeSeqEnd = 59;
    gSioExchangeSeq = 1;
    gSioHandshakeAck = FALSE;
    gSioHandshakeDone = FALSE;
    gSioHandshakeConfirm = FALSE;
    gUnk_0203C3B8 = 0;
    gSioExchangeSendData = (u16*)gSioSendDeck;
    gSioExchangeRecvData = (u16*)gSioRecvDeck;
}

s32 SioExchangeSend() {
    u16 n;

    if (!gSioHandshakeDone) {
        if (!gSioHandshakeAck) {
            gSioSendFrame[0] = SIO_CMD_EXCHANGE_REQUEST;
        } else {
            gSioSendFrame[0] = SIO_CMD_EXCHANGE_ACCEPT;
        }
    } else {
        if (gSioExchangeSeq <= 3) {
            gSioSendFrame[0] = SIO_CMD_DATA;
            gSioSendFrame[1] = SIO_CMD_FILLER;
            gSioSendFrame[2] = SIO_CMD_FILLER;
            gSioSendFrame[3] = SIO_CMD_FILLER;
        } else if (gSioExchangeSeq <= gSioExchangeSeqEnd) {
            n = gSioExchangeSeq - 3;
            gSioSendFrame[0] = SIO_CMD_DATA;
            gSioSendFrame[1] = gSioExchangeSeq;
            gSioSendFrame[2] = gSioExchangeSendData[n * 2 - 2];
            gSioSendFrame[3] = gSioExchangeSendData[n * 2 - 1];
        } else {
            gSioSendFrame[0] = SIO_CMD_DATA;
            gSioSendFrame[1] = gSioExchangeSeq;
            gSioSendFrame[2] = 0;
            gSioSendFrame[3] = 0;
        }

        gSioExchangeSeq++;
    }

    return 0;
}

s32 SioExchangeRecv() {
    u16 n;

    if (!gSioHandshakeDone) {
        if (!gSioHandshakeAck) {
            if (gSioRecvFrame[0][0] == SIO_CMD_EXCHANGE_REQUEST || gSioRecvFrame[0][1] == SIO_CMD_EXCHANGE_REQUEST) {
                gSioHandshakeAck = TRUE;
            }
        } else if (gSioRecvFrame[0][0] == SIO_CMD_EXCHANGE_ACCEPT && gSioRecvFrame[0][1] == gSioRecvFrame[0][0]) {
            gSioHandshakeDone = TRUE;
            gSioExchangeSeq = 1;
        }
    } else if (gSioPlayerId == SIO_PLAYER_PARENT) {
        if (gSioRecvFrame[1][1] != SIO_CMD_FILLER && gSioRecvFrame[1][1] > 3) {
            if (gSioRecvFrame[1][1] > gSioExchangeSeqEnd) {
                return SIO_LINK_RESULT_EXCHANGE_DONE;
            }

            n = gSioRecvFrame[1][1] - 3;
            gSioExchangeRecvData[n * 2 - 2] = gSioRecvFrame[2][1];
            gSioExchangeRecvData[n * 2 - 1] = gSioRecvFrame[3][1];
        }
    } else {
        if (gSioRecvFrame[1][0] != SIO_CMD_FILLER && gSioRecvFrame[1][0] > 3) {
            if (gSioRecvFrame[1][0] > gSioExchangeSeqEnd) {
                return SIO_LINK_RESULT_EXCHANGE_DONE;
            }

            n = gSioRecvFrame[1][0] - 3;
            gSioExchangeRecvData[n * 2 - 2] = gSioRecvFrame[2][0];
            gSioExchangeRecvData[n * 2 - 1] = gSioRecvFrame[3][0];
        }
    }

    return SIO_LINK_RESULT_NONE;
}

#ifdef VERSION_EU
void SioExchangeLoopback() {
    s32 i;
    s16 count;
    count = 112;

    for (i = 0; i < count; i++) {
        gSioExchangeRecvData[i] = gSioExchangeSendData[i];
    }
}
#endif

void SioPrepareCharaLinkExchange() {
    s32 i;
    CharaLinkData* send;
    u16* recv;

    send = &gCharaLinkSend;
    recv = (u16*)&gCharaLinkRecv;
    send->hp = gGameState.hp;
    gCharaLinkSend.maxHp = gGameState.progression.maxHp;
    gCharaLinkSend.level = gGameState.progression.level;
    gCharaLinkSend.winCount = gSioWinCount;
    gCharaLinkSend.loseCount = gSioLoseCount;
    gCharaLinkSend.ap = gGameState.progression.ap;
    gCharaLinkSend.learnedStocks = gGameState.progression.learnedStocks;
    gCharaLinkSend.learnedStocks2 = gGameState.progression.learnedStocks2;
    gCharaLinkSend.worldFlags = 0;

    for (i = 0; i < 13; i++) {
        if ((u8)(gGameState.floors[i].world - 1) <= 11) {
            gCharaLinkSend.worldFlags |= 1 << gGameState.floors[i].world;
        }
    }

    if (gSioPlayerId == SIO_PLAYER_PARENT) {
        SeedRandom(gFrameCounter & 0xFFFF);
        gCharaLinkSend.seed = GetRandom() % 0xFFFF;
    } else {
        gCharaLinkSend.seed = 0;
    }

    gSioExchangeSeqEnd = 11;
    gSioExchangeSeq = 1;
    gSioHandshakeAck = FALSE;
    gSioHandshakeDone = FALSE;
    gSioHandshakeConfirm = FALSE;
    gSioExchangeSendData = (u16*)send;
    gSioExchangeRecvData = recv;
}

void SioSyncInit(void (*onConnect)()) {
    gSioHandshakeAck = FALSE;
    gSioHandshakeDone = FALSE;
    gSioHandshakeConfirm = FALSE;
    gSioConnectCallback = onConnect;
}

s32 SioSyncSend() {
    if (!gSioHandshakeDone) {
        if (!gSioHandshakeAck) {
            gSioSendFrame[0] = SIO_CMD_CONNECT_REQUEST;
        } else if (!gSioHandshakeConfirm) {
            gSioSendFrame[0] = SIO_CMD_CONNECT_ACCEPT;
        } else {
            gSioSendFrame[0] = SIO_CMD_SYNC_CONFIRM;
        }
    }

    return 0;
}

s32 SioSyncRecv() {
    if (!gSioHandshakeDone) {
        if (!gSioHandshakeAck) {
            if (gSioRecvFrame[0][0] == SIO_CMD_CONNECT_REQUEST || gSioRecvFrame[0][1] == SIO_CMD_CONNECT_REQUEST) {
                gSioHandshakeAck = TRUE;
            }
        } else if (gSioRecvFrame[0][0] != SIO_CMD_SYNC_CONFIRM) {
            if (gSioPlayerId == SIO_PLAYER_PARENT && gSioRecvFrame[0][0] == SIO_CMD_CONNECT_ACCEPT &&
                gSioRecvFrame[0][1] == gSioRecvFrame[0][0]) {
                gSioHandshakeConfirm = TRUE;
            }
        } else {
            if (gSioConnectCallback != NULL) {
                gSioConnectCallback();
            }

            gSioHandshakeDone = TRUE;
        }
    }

    return SIO_LINK_RESULT_NONE;
}

#ifdef VERSION_EU
s32 SioRandomPartnerSend() {
    return 0;
}

s32 SioRandomPartnerRecv() {
    u16 held;
    u16 keys;
    u16 roll;
    held = GetKeysHeld() & KEYS_MASK;
    keys = 0;

    if (gRandomPartnerDpadTimer != 0) {
        keys = gRandomPartnerDpad;
        gRandomPartnerDpadTimer--;
    } else {
        gRandomPartnerDpadTimer = GetRandom() % 91 + 30;
        roll = GetRandom();

        switch (roll & 7) {
        case 0:
            gRandomPartnerDpad = DPAD_RIGHT;
            break;
        case 1:
            gRandomPartnerDpad = DPAD_LEFT;
            break;
        case 2:
            gRandomPartnerDpad = DPAD_UP;
            break;
        case 3:
            gRandomPartnerDpad = DPAD_DOWN;
            break;
        case 4:
            gRandomPartnerDpad = DPAD_UP | DPAD_RIGHT;
            break;
        case 5:
            gRandomPartnerDpad = DPAD_DOWN | DPAD_RIGHT;
            break;
        case 6:
            gRandomPartnerDpad = DPAD_UP | DPAD_LEFT;
            break;
        case 7:
            gRandomPartnerDpad = DPAD_DOWN | DPAD_LEFT;
            break;
        }
    }

    if (gRandomPartnerATimer != 0) {
        keys |= A_BUTTON;
        gRandomPartnerATimer--;
    } else {
        gRandomPartnerATimer = GetRandom() % 61 + 60;
    }

    if ((u16)(GetRandom() % 30) == 0) {
        keys |= L_BUTTON;
    }

    if ((u16)(GetRandom() % 50) == 0) {
        keys |= (L_BUTTON | R_BUTTON);
    }

    SioKeyStateUpdateA(held);
    SioKeyStateUpdateB(keys);
    return SIO_LINK_RESULT_NONE;
}
#endif

enum CharaDefeat2State {
    CHARA_DEFEAT2_STATE_WAIT_BGFX,
    CHARA_DEFEAT2_STATE_PAUSE,
    CHARA_DEFEAT2_STATE_START,
    CHARA_DEFEAT2_STATE_DARKEN,
    CHARA_DEFEAT2_STATE_SAVE_DARK_PALETTE,
    CHARA_DEFEAT2_STATE_HOLD,
    CHARA_DEFEAT2_STATE_DISSOLVE,
    CHARA_DEFEAT2_STATE_START_FLASH,
    CHARA_DEFEAT2_STATE_FLASH,
    CHARA_DEFEAT2_STATE_FLASH_HOLD,
    CHARA_DEFEAT2_STATE_CALLBACK,
    CHARA_DEFEAT2_STATE_FADE_FROM_WHITE,
    CHARA_DEFEAT2_STATE_DROP_PRIZES,
    CHARA_DEFEAT2_STATE_DRIFT_FX,
    CHARA_DEFEAT2_STATE_START_RISE,
    CHARA_DEFEAT2_STATE_RISE_FX,
    CHARA_DEFEAT2_STATE_STOP_FX,
    CHARA_DEFEAT2_STATE_FADE_FROM_BLACK,
    CHARA_DEFEAT2_STATE_DONE
};

void CharaObjInitDefeat2(CharaObjParam2* param) {
    s32 i;

    sCharaObj = EwramAlloc(sizeof(CharaObj));
    sCharaObj->tilesAddr = param->tilesAddr;
    sCharaObj->tileCount = param->tileCount;
    sCharaObj->paletteAddr = param->paletteAddr;
    sCharaObj->paletteSize = param->paletteSize;
    sCharaObj->x = param->x;
    sCharaObj->y = param->y;
    sCharaObj->z = param->z;
    sCharaObj->fadeLevel = 0;
    sCharaObj->bgFxVz = -76;
    sCharaObj->fadeTick = 0;
    sCharaObj->timer = 0;
    sCharaObj->state = CHARA_DEFEAT2_STATE_WAIT_BGFX;
    sCharaObj->callback = param->callback;
    sCharaObj->tilesAddr4 = 0;
    sCharaObj->tileCount4 = 0;
    sCharaObj->paletteAddr2 = 0;
    sCharaObj->paletteSize2 = 0;
    sCharaObj->prizeObj = param->prizeObj;

    for (i = 0; i < 32; i++) {
        sCharaObj->bankFadeEnabled[i] = 0;
    }

    TaskPoolInit(&sCharaTaskPool, 2);
}

u8 CharaObjUpdateDefeat2() {
    CharaPrizeArgs prize;
    MaskFadeArgs fade;

    switch (sCharaObj->state) {
    case CHARA_DEFEAT2_STATE_WAIT_BGFX:
        if (!BgFxIsActive()) {
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT2_STATE_PAUSE:
        if (++sCharaObj->timer > 59) {
            sCharaObj->timer = 0;
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT2_STATE_START:
        CpuCopy16((void*)PLTT, sCharaObj->savedPalette, 0x400);
        BgFxStartCharaDefeat(sCharaObj->x, sCharaObj->y + sCharaObj->z - 0x1000);
        m4aSongNumStart(SONG_EF_BOSS_DEAD1);
        sCharaObj->state++;
        break;
    case CHARA_DEFEAT2_STATE_DARKEN:
        sCharaObj->fadeLevel++;
        FadePaletteToBlack(sCharaObj->savedPalette, (u16*)PLTT, 320, sCharaObj->fadeLevel);

        if (++sCharaObj->timer > 9) {
            sCharaObj->timer = 0;
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT2_STATE_SAVE_DARK_PALETTE:
        sCharaObj->fadeLevel = 0;
        CpuCopy16((void*)PLTT, sCharaObj->fadedPalette, 0x400);
        sCharaObj->state++;
        break;
    case CHARA_DEFEAT2_STATE_HOLD:
        if (++sCharaObj->timer > 89) {
            sCharaObj->timer = 0;
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT2_STATE_DISSOLVE:
        if (sCharaObj->fadeTick > 1) {
            sCharaObj->fadeTick = 0;
            sCharaObj->fadeLevel++;
        }

        sCharaObj->fadeTick++;

        if (sCharaObj->paletteSize != 0) {
            FadePaletteToWhite((u16*)sCharaObj->paletteAddr, (u16*)sCharaObj->paletteAddr, sCharaObj->paletteSize, sCharaObj->fadeLevel);
        }

        if (sCharaObj->timer == 20) {
            BgAnimStop();
            m4aSongNumStart(SONG_EF_BOSS_DEAD2);
            fade.tiles = (u8*)sCharaObj->tilesAddr;
            fade.tileCount = sCharaObj->tileCount;
            fade.stepDelay = 1;
            TaskCreate(&sCharaTaskPool, &gTaskDescCharaMaskFade, &fade);
        }

        if (++sCharaObj->timer > 39) {
            sCharaObj->timer = 0;
            m4aSongNumStop(SONG_EF_BOSS_DEAD2);
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT2_STATE_START_FLASH:
        sCharaObj->fadeLevel = 0;
        sCharaObj->fadeTick = 0;
        m4aSongNumStart(SONG_EF_BOSS_DEAD3);
        FadeStartOut(FADE_MODE_ADD_WHITE, 20);
        FadeLock();
        sCharaObj->state++;
        break;
    case CHARA_DEFEAT2_STATE_FLASH:
        if (sCharaObj->fadeTick > 1) {
            sCharaObj->fadeTick = 0;
            sCharaObj->fadeLevel++;
        }

        sCharaObj->fadeTick++;

        if (++sCharaObj->timer > 37) {
            sCharaObj->timer = 0;
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT2_STATE_FLASH_HOLD:
        if (++sCharaObj->timer > 20) {
            sCharaObj->timer = 0;
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT2_STATE_CALLBACK:
        sCharaObj->fadeTick = 0;

        if (sCharaObj->callback != NULL) {
            sCharaObj->callback();
        }

        sCharaObj->state++;
        break;
    case CHARA_DEFEAT2_STATE_FADE_FROM_WHITE:
        sCharaObj->fadeLevel -= 2;
        FadePaletteToWhite(sCharaObj->fadedPalette, (u16*)PLTT, 1024, sCharaObj->fadeLevel);

        if (++sCharaObj->timer > 8) {
            sCharaObj->timer = 0;
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT2_STATE_DROP_PRIZES:
        BgFxStartCharaDefeatEnd(sCharaObj->x, sCharaObj->y + sCharaObj->z - 0x1000);
        prize.x = sCharaObj->x;
        prize.y = sCharaObj->y;
        prize.z = sCharaObj->z;
        CreateBossPrizeCardTask(&gBtlWork->taskPools[0], &prize);
        DropBossPrizes(sCharaObj->prizeObj);
        sCharaObj->state++;
        break;
    case CHARA_DEFEAT2_STATE_DRIFT_FX:
        BgFxAddPosition(76, 0, sCharaObj->bgFxVz);

        if (++sCharaObj->timer > 79) {
            sCharaObj->timer = 0;
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT2_STATE_START_RISE:
        sCharaObj->bgFxVz = 0;
        sCharaObj->state++;
        break;
    case CHARA_DEFEAT2_STATE_RISE_FX:
        BgFxAddPosition(0, 0, sCharaObj->bgFxVz);
        sCharaObj->bgFxVz -= 25;

        if (++sCharaObj->timer > 39) {
            sCharaObj->timer = 0;
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT2_STATE_STOP_FX:
        BgAnimStop();
        sCharaObj->fadeLevel = 11;
        sCharaObj->state++;
        break;
    case CHARA_DEFEAT2_STATE_FADE_FROM_BLACK:
        sCharaObj->fadeLevel--;
        FadePaletteToBlack(sCharaObj->savedPalette, (u16*)PLTT, 320, sCharaObj->fadeLevel);

        if (++sCharaObj->timer > 10) {
            sCharaObj->timer = 0;
            CharaObjFree();
            // @bug Writes to sCharaObj after freeing it.
            sCharaObj->state++;
        }

        break;
    default:
        return FALSE;
    }

    TaskPoolUpdate(&sCharaTaskPool);
    TaskPoolDraw(&sCharaTaskPool);
    return TRUE;
}

void CharaObjFree() {
    EwramFree(sCharaObj);
    TaskPoolDestroy(&sCharaTaskPool);
}

enum CharaDefeatState {
    CHARA_DEFEAT_STATE_WAIT_BGFX,
    CHARA_DEFEAT_STATE_PAUSE,
    CHARA_DEFEAT_STATE_START,
    CHARA_DEFEAT_STATE_SAVE_PALETTE,
    CHARA_DEFEAT_STATE_DARKEN,
    CHARA_DEFEAT_STATE_END_DARKEN,
    CHARA_DEFEAT_STATE_HOLD,
    CHARA_DEFEAT_STATE_DISSOLVE,
    CHARA_DEFEAT_STATE_START_FLASH,
    CHARA_DEFEAT_STATE_FLASH,
    CHARA_DEFEAT_STATE_FLASH_HOLD,
    CHARA_DEFEAT_STATE_CALLBACK,
    CHARA_DEFEAT_STATE_FADE_FROM_WHITE,
    CHARA_DEFEAT_STATE_REDARKEN,
    CHARA_DEFEAT_STATE_DROP_PRIZES,
    CHARA_DEFEAT_STATE_DRIFT_FX,
    CHARA_DEFEAT_STATE_START_RISE,
    CHARA_DEFEAT_STATE_RISE_FX,
    CHARA_DEFEAT_STATE_STOP_FX,
    CHARA_DEFEAT_STATE_FADE_FROM_BLACK,
    CHARA_DEFEAT_STATE_DONE
};

void CharaObjInitDefeat(CharaObjParam* param) {
    s32 i;
    u16 idx;

    sCharaObj = EwramAlloc(sizeof(CharaObj));
    sCharaObj->flags = 0;
    sCharaObj->tilesAddr = param->tilesAddr;
    sCharaObj->tileCount = param->tileCount;
    sCharaObj->tilesAddr2 = param->tilesAddr2;
    sCharaObj->tileCount2 = param->tileCount2;
    sCharaObj->tilesAddr3 = param->tilesAddr3;
    sCharaObj->tileCount3 = param->tileCount3;
    sCharaObj->paletteAddr = param->paletteAddr;
    sCharaObj->paletteSize = param->paletteSize;
    sCharaObj->tilesAddr4 = param->tilesAddr4;
    sCharaObj->tileCount4 = param->tileCount4;
    sCharaObj->paletteAddr2 = param->paletteAddr2;
    sCharaObj->paletteSize2 = param->paletteSize2;
    sCharaObj->x = param->x;
    sCharaObj->y = param->y;
    sCharaObj->z = param->z;
    sCharaObj->fadeLevel = 0;
    sCharaObj->bgFxVz = -76;
    sCharaObj->fadeTick = 0;
    sCharaObj->timer = 0;
    sCharaObj->state = CHARA_DEFEAT_STATE_WAIT_BGFX;
    sCharaObj->callback = param->callback;
    sCharaObj->prizeObj = param->prizeObj;
    sCharaObj->flags = param->flags;

    for (i = 0; i < 10; i++) {
        sCharaObj->bankFadeEnabled[i] = 1;
    }

    for (i = 10; i < 32; i++) {
        sCharaObj->bankFadeEnabled[i] = 0;
    }

    idx = sCharaObj->paletteAddr >> 5;

    if (sCharaObj->paletteSize == 32) {
        sCharaObj->bankFadeEnabled[(s16)idx] = 1;
    }

    TaskPoolInit(&sCharaTaskPool, 4);
}

void CharaObjSetBankFadeEnabled(u16 bank, u8 enabled) {
    if (bank < ARRAY_COUNT(sCharaObj->bankFadeEnabled)) {
        sCharaObj->bankFadeEnabled[bank] = enabled;
    }
}

u8 CharaObjUpdateDefeat() {
    s32 i;
    CharaPrizeArgs prize;
    MaskFadeArgs fade0;
    MaskFadeArgs fade1;
    MaskFadeArgs fade2;
    MaskFadeArgs fade3;

    switch (sCharaObj->state) {
    case CHARA_DEFEAT_STATE_WAIT_BGFX:
        if (!BgFxIsActive()) {
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT_STATE_PAUSE:
        if (++sCharaObj->timer > 59) {
            sCharaObj->timer = 0;
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT_STATE_START:
        BgFxStartCharaDefeat(sCharaObj->x, sCharaObj->y + sCharaObj->z - 0x1000);
        m4aSongNumStart(SONG_EF_BOSS_DEAD1);

        for (i = 0; i < 32; i++) {
            SetPaletteBankFadeEnabled(i, sCharaObj->bankFadeEnabled[i]);
        }

        sCharaObj->state++;
        break;
    case CHARA_DEFEAT_STATE_SAVE_PALETTE:
        CpuCopy16((void*)PLTT, sCharaObj->savedPalette, 0x400);
        sCharaObj->state++;
        break;
    case CHARA_DEFEAT_STATE_DARKEN:
        sCharaObj->fadeLevel++;
        FadeAllPalettesToBlack(sCharaObj->savedPalette, sCharaObj->fadeLevel);

        if (++sCharaObj->timer > 9) {
            sCharaObj->timer = 0;
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT_STATE_END_DARKEN:
        sCharaObj->fadeLevel = 0;
        sCharaObj->state++;
        break;
    case CHARA_DEFEAT_STATE_HOLD:
        if (++sCharaObj->timer > 89) {
            sCharaObj->timer = 0;
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT_STATE_DISSOLVE:
        if (sCharaObj->fadeTick > 1) {
            sCharaObj->fadeTick = 0;
            sCharaObj->fadeLevel++;
        }

        sCharaObj->fadeTick++;

        if (sCharaObj->timer == 20) {
            BgAnimStop();
            m4aSongNumStart(SONG_EF_BOSS_DEAD2);
            fade0.tiles = (u8*)sCharaObj->tilesAddr;
            fade0.tileCount = sCharaObj->tileCount;
            fade0.stepDelay = 1;

            if (fade0.tileCount != 0) {
                TaskCreate(&sCharaTaskPool, &gTaskDescCharaMaskFade, &fade0);
            }

            fade1.tiles = (u8*)sCharaObj->tilesAddr2;
            fade1.tileCount = sCharaObj->tileCount2;
            fade1.stepDelay = 1;

            if (fade1.tileCount != 0) {
                TaskCreate(&sCharaTaskPool, &gTaskDescCharaMaskFade, &fade1);
            }

            fade2.tiles = (u8*)sCharaObj->tilesAddr3;
            fade2.tileCount = sCharaObj->tileCount3;
            fade2.stepDelay = 1;

            if (fade2.tileCount != 0) {
                TaskCreate(&sCharaTaskPool, &gTaskDescCharaMaskFade, &fade2);
            }

            fade3.tiles = (u8*)sCharaObj->tilesAddr4;
            fade3.tileCount = sCharaObj->tileCount4;
            fade3.stepDelay = 1;

            if (fade3.tileCount != 0) {
                TaskCreate(&sCharaTaskPool, &gTaskDescCharaMaskFade, &fade3);
            }
        }

        if (sCharaObj->paletteSize != 0) {
            FadePaletteToWhite((u16*)sCharaObj->paletteAddr, (u16*)sCharaObj->paletteAddr, sCharaObj->paletteSize, sCharaObj->fadeLevel);
        }

        if (sCharaObj->paletteSize2 != 0) {
            FadePaletteToWhite((u16*)sCharaObj->paletteAddr2, (u16*)sCharaObj->paletteAddr2, sCharaObj->paletteSize2, sCharaObj->fadeLevel);
        }

        if (++sCharaObj->timer > 39) {
            sCharaObj->timer = 0;
            m4aSongNumStop(SONG_EF_BOSS_DEAD2);
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT_STATE_START_FLASH:
        sCharaObj->fadeLevel = 0;
        sCharaObj->fadeTick = 0;
        m4aSongNumStart(SONG_EF_BOSS_DEAD3);
        FadeStartOut(FADE_MODE_ADD_WHITE, 20);
        FadeLock();
        sCharaObj->state++;
        break;
    case CHARA_DEFEAT_STATE_FLASH:
        if (sCharaObj->fadeTick > 1) {
            sCharaObj->fadeTick = 0;
            sCharaObj->fadeLevel++;
        }

        sCharaObj->fadeTick++;

        if (++sCharaObj->timer > 37) {
            sCharaObj->timer = 0;
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT_STATE_FLASH_HOLD:
        if (++sCharaObj->timer > 20) {
            sCharaObj->timer = 0;
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT_STATE_CALLBACK:
        sCharaObj->fadeTick = 0;

        if (sCharaObj->callback != NULL) {
            sCharaObj->callback();
        }

        sCharaObj->fadeLevel = 32;
        sCharaObj->state++;
        break;
    case CHARA_DEFEAT_STATE_FADE_FROM_WHITE:
        FadeAllPalettesToWhite(sCharaObj->savedPalette, sCharaObj->fadeLevel);

        if ((sCharaObj->fadeLevel -= 2) <= 0) {
            sCharaObj->fadeLevel = 0;
            sCharaObj->timer = 0;
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT_STATE_REDARKEN:
        FadeAllPalettesToBlack(sCharaObj->savedPalette, sCharaObj->fadeLevel);

        if ((sCharaObj->fadeLevel += 2) > 11) {
            sCharaObj->fadeLevel = 12;
            sCharaObj->timer = 0;
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT_STATE_DROP_PRIZES:
        prize.x = sCharaObj->x;
        prize.y = sCharaObj->y;
        prize.z = sCharaObj->z;
        CreateBossPrizeCardTask(&gBtlWork->taskPools[0], &prize);
        DropBossPrizes(sCharaObj->prizeObj);

        if ((sCharaObj->flags & 1) == 0) {
            BgFxStartCharaDefeatEnd(sCharaObj->x, sCharaObj->y + sCharaObj->z - 0x1000);
            sCharaObj->timer = 0;
            sCharaObj->state++;
        } else {
            sCharaObj->fadeLevel = 12;
            sCharaObj->timer = 0;
            sCharaObj->state = CHARA_DEFEAT_STATE_FADE_FROM_BLACK;
        }

        break;
    case CHARA_DEFEAT_STATE_DRIFT_FX:
        BgFxAddPosition(76, 0, sCharaObj->bgFxVz);

        if (++sCharaObj->timer > 79) {
            sCharaObj->timer = 0;
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT_STATE_START_RISE:
        sCharaObj->bgFxVz = 0;
        sCharaObj->state++;
        break;
    case CHARA_DEFEAT_STATE_RISE_FX:
        BgFxAddPosition(0, 0, sCharaObj->bgFxVz);
        sCharaObj->bgFxVz -= 25;

        if (++sCharaObj->timer > 39) {
            sCharaObj->timer = 0;
            sCharaObj->state++;
        }

        break;
    case CHARA_DEFEAT_STATE_STOP_FX:
        BgAnimStop();
        sCharaObj->fadeLevel = 12;
        sCharaObj->state++;
        break;
    case CHARA_DEFEAT_STATE_FADE_FROM_BLACK:
        sCharaObj->fadeLevel--;
        FadeAllPalettesToBlack(sCharaObj->savedPalette, sCharaObj->fadeLevel);

        if (sCharaObj->fadeLevel <= 0) {
            sCharaObj->fadeLevel = 0;
            sCharaObj->timer = 0;
            CharaObjFree();
            // @bug Writes to sCharaObj after freeing it.
            sCharaObj->state++;
        }

        break;
    default:
        return FALSE;
    }

    TaskPoolUpdate(&sCharaTaskPool);
    TaskPoolDraw(&sCharaTaskPool);
    return TRUE;
}

void RequestTileRowsCopy(u8* src, u8* dst, u16 size, s16 count) {
    s32 i;
    s32 n;

    i = count;
    n = i;

    for (i = 0; i < n; i++) {
        RequestDma3Copy(src + i * 0x400, dst + i * 0x400, size * 32);
    }
}

void RequestMapRowsCopy(u8* src, u8* dst, u16 size, s16 count) {
    s32 i;
    s32 n;

    i = count;
    n = i;

    for (i = 0; i < n; i++) {
        RequestDma3Copy(src + i * 0x40, dst + i * 0x40, size * 2);
    }
}

TaskDesc gTaskDescCharaMaskFade = {
    "task_chara_mask_fade",
    (TaskInitFunc)task_chara_mask_fade_0,
    (TaskUpdateFunc)task_chara_mask_fade_1,
    (TaskDrawFunc)task_chara_mask_fade_2,
    (TaskDestroyFunc)task_chara_mask_fade_3,
    sizeof(MaskFadeWork),
};

TaskDesc gTaskDescChgCardObj = {
    "task_chgCardObj",
    (TaskInitFunc)task_chgCardObj_0,
    (TaskUpdateFunc)task_chgCardObj_1,
    (TaskDrawFunc)task_chgCardObj_2,
    (TaskDestroyFunc)task_chgCardObj_3,
    sizeof(ChgCardObjWork),
};
