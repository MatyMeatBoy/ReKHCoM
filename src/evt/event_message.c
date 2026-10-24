/**
 * event_message.c
 * Event Sequence Player
 */

#include "macros.h"
#include "registration_data.h"
#include "eventselect_api.h"
#include "system_state.h"
#include "msg_api.h"
#ifdef PLATFORM_PC
void PortSetBgIsoExtend(int bg, int enabled);
int PortIsoScene(int eventId);
#endif
#include "intr.h"
#include "m4a_song.h"
#include "obj_api.h"
#include "pallet.h"
#include "display.h"
#include "text.h"
#include "monsgage.h"
#include "anim.h"
#include "msg.h"
#include "sprites_deck_menu.h"
#include "sprites_evt.h"
#include "sprites_msg.h"
#include "sprites_card.h"
#include "msg_portrait_data.h"
#include "gba/keys.h"
#include "gba/io_reg.h"
#include "bos6_api.h"
#include "evt_obj_api.h"
#include "malloc.h"
#include "fade.h"
#include "songs.h"
#include "jiminy_data.h"
#include "common_text.h"
#include <stdlib.h>
#include <string.h>
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "engine_math.h"
#include "event_background_types.h"
#include "event_chara_types.h"
#include "event_index_data.h"
#include "evt_data.h"
#include "evt_object_types.h"
#include "evt_types.h"
#include "gba/defines.h"
#include "key.h"
#include "listpool.h"
#include "msg_types.h"
#include "obj.h"
#include "poo_api.h"
#include "sprite_palettes.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "default_bg_map.h"
#include "text_types.h"
#include "event_ids.h"
#include "fld_types.h"

static void msgwin_0(MsgWinWork* work, u8* arg);
static u8 msgwin_1(MsgWinWork* work, void* task);
static void msgwin_2(MsgWinWork* work);
static void msgwin_3(MsgWinWork* work);

static const MsgFaceAnim sTalk00FaceAnims[8] = {
    {
        gTalk0000Tiles,
        gTalk0000Palette,
        gTalk0000Frames,
        gTalk0000Anims,
        ARRAY_COUNT(gTalk0000Anims),
        1,
    },
    {
        gTalk0001Tiles,
        gTalk0000Palette,
        gTalk0001Frames,
        gTalk0001Anims,
        ARRAY_COUNT(gTalk0001Anims),
        1,
    },
    {
        gTalk0002Tiles,
        gTalk0000Palette,
        gTalk0002Frames,
        gTalk0002Anims,
        ARRAY_COUNT(gTalk0002Anims),
        1,
    },
    {
        gTalk0003Tiles,
        gTalk0000Palette,
        gTalk0003Frames,
        gTalk0003Anims,
        ARRAY_COUNT(gTalk0003Anims),
        1,
    },
    {
        gTalk0004Tiles,
        gTalk0000Palette,
        gTalk0004Frames,
        gTalk0004Anims,
        ARRAY_COUNT(gTalk0004Anims),
        1,
    },
    {
        gTalk0005Tiles,
        gTalk0000Palette,
        gTalk0005Frames,
        gTalk0005Anims,
        ARRAY_COUNT(gTalk0005Anims),
        1,
    },
    {
        gTalk0005Tiles,
        gTalk0000Palette,
        gTalk0005Frames,
        gTalk0005Anims + 2,
        4,
        1,
    },
    {
        gTalk0000Tiles,
        gTalk0000Palette,
        gTalk0000Frames,
        gTalk0000Anims + 2,
        4,
        1,
    },
};

static const MsgFaceAnim sTalk01FaceAnims[6] = {
    {
        gTalk0100Tiles,
        gTalk0100Palette,
        gTalk0100Frames,
        gTalk0100Anims,
        ARRAY_COUNT(gTalk0100Anims),
        1,
    },
    {
        gTalk0101Tiles,
        gTalk0100Palette,
        gTalk0101Frames,
        gTalk0101Anims,
        ARRAY_COUNT(gTalk0101Anims),
        1,
    },
    {
        gTalk0102Tiles,
        gTalk0100Palette,
        gTalk0102Frames,
        gTalk0102Anims,
        ARRAY_COUNT(gTalk0102Anims),
        1,
    },
    {
        gTalk0103Tiles,
        gTalk0100Palette,
        gTalk0103Frames,
        gTalk0103Anims,
        ARRAY_COUNT(gTalk0103Anims),
        1,
    },
    {
        gTalk0104Tiles,
        gTalk0100Palette,
        gTalk0104Frames,
        gTalk0104Anims,
        ARRAY_COUNT(gTalk0104Anims),
        1,
    },
    {
        gTalk0105Tiles,
        gTalk0100Palette,
        gTalk0105Frames,
        gTalk0105Anims,
        ARRAY_COUNT(gTalk0105Anims),
        1,
    },
};

static const MsgFaceAnim sTalk22FaceAnims[5] = {
    {
        gTalk0100Tiles,
        gTalk2200Palette,
        gTalk0100Frames,
        gTalk0100Anims + 2,
        4,
        1,
    },
    {
        gTalk0101Tiles,
        gTalk2200Palette,
        gTalk0101Frames,
        gTalk0101Anims + 2,
        4,
        1,
    },
    {
        gTalk0102Tiles,
        gTalk2200Palette,
        gTalk0102Frames,
        gTalk0102Anims + 2,
        4,
        1,
    },
    {
        gTalk0103Tiles,
        gTalk2200Palette,
        gTalk0103Frames,
        gTalk0103Anims + 2,
        4,
        1,
    },
    {
        gTalk0105Tiles,
        gTalk2200Palette,
        gTalk0105Frames,
        gTalk0105Anims + 2,
        4,
        1,
    },
};

static const MsgFaceAnim sTalk02FaceAnims[5] = {
    {
        gTalk0200Tiles,
        gTalk0200Palette,
        gTalk0200Frames,
        gTalk0200Anims,
        ARRAY_COUNT(gTalk0200Anims),
        1,
    },
    {
        gTalk0201Tiles,
        gTalk0200Palette,
        gTalk0201Frames,
        gTalk0201Anims,
        ARRAY_COUNT(gTalk0201Anims),
        1,
    },
    {
        gTalk0202Tiles,
        gTalk0200Palette,
        gTalk0202Frames,
        gTalk0202Anims,
        ARRAY_COUNT(gTalk0202Anims),
        1,
    },
    {
        gTalk0203Tiles,
        gTalk0200Palette,
        gTalk0203Frames,
        gTalk0203Anims,
        ARRAY_COUNT(gTalk0203Anims),
        1,
    },
    {
        gTalk0204Tiles,
        gTalk0200Palette,
        gTalk0204Frames,
        gTalk0204Anims,
        ARRAY_COUNT(gTalk0204Anims),
        1,
    },
};

static const MsgFaceAnim sTalk23FaceAnims[3] = {
    {
        gTalk0200Tiles,
        gTalk2300Palette,
        gTalk0200Frames,
        gTalk0200Anims + 2,
        4,
        1,
    },
    {
        gTalk0201Tiles,
        gTalk2300Palette,
        gTalk0201Frames,
        gTalk0201Anims + 2,
        4,
        1,
    },
    {
        gTalk0202Tiles,
        gTalk2300Palette,
        gTalk0202Frames,
        gTalk0202Anims + 2,
        4,
        1,
    },
};

static const MsgFaceAnim sTalk03FaceAnims = {
    gTalk0300Tiles,
    gTalk0300Palette,
    gTalk0300Frames,
    gTalk0300Anims,
    ARRAY_COUNT(gTalk0300Anims),
    1,
};

static const MsgFaceAnim sTalk04FaceAnims[4] = {
    {
        gTalk0400Tiles,
        gTalk0400Palette,
        gTalk0400Frames,
        gTalk0400Anims,
        ARRAY_COUNT(gTalk0400Anims),
        1,
    },
    {
        gTalk0401Tiles,
        gTalk0400Palette,
        gTalk0401Frames,
        gTalk0401Anims,
        ARRAY_COUNT(gTalk0401Anims),
        1,
    },
    {
        gTalk0402Tiles,
        gTalk0400Palette,
        gTalk0402Frames,
        gTalk0402Anims,
        ARRAY_COUNT(gTalk0402Anims),
        1,
    },
    {
        gTalk0403Tiles,
        gTalk0400Palette,
        gTalk0403Frames,
        gTalk0403Anims,
        ARRAY_COUNT(gTalk0403Anims),
        1,
    },
};

static const MsgFaceAnim sTalk05FaceAnims[4] = {
    {
        gTalk0500Tiles,
        gTalk0500Palette,
        gTalk0500Frames,
        gTalk0500Anims,
        ARRAY_COUNT(gTalk0500Anims),
        1,
    },
    {
        gTalk0500Tiles,
        gTalk0500Palette,
        gTalk0500Frames,
        gTalk0500Anims + 2,
        8,
        1,
    },
    {
        gTalk0500Tiles,
        gTalk0500Palette,
        gTalk0500Frames,
        gTalk0500Anims + 4,
        8,
        1,
    },
    {
        gTalk0500Tiles,
        gTalk0500Palette,
        gTalk0500Frames,
        gTalk0500Anims + 6,
        8,
        1,
    },
};

static const MsgFaceAnim sTalk06FaceAnims[5] = {
    {
        gTalk0600Tiles,
        gTalk0600Palette,
        gTalk0600Frames,
        gTalk0600Anims,
        ARRAY_COUNT(gTalk0600Anims),
        1,
    },
    {
        gTalk0600Tiles,
        gTalk0600Palette,
        gTalk0600Frames,
        gTalk0600Anims + 2,
        10,
        1,
    },
    {
        gTalk0600Tiles,
        gTalk0600Palette,
        gTalk0600Frames,
        gTalk0600Anims + 4,
        10,
        1,
    },
    {
        gTalk0600Tiles,
        gTalk0600Palette,
        gTalk0600Frames,
        gTalk0600Anims + 6,
        10,
        1,
    },
    {
        gTalk0600Tiles,
        gTalk0600Palette,
        gTalk0600Frames,
        gTalk0600Anims + 8,
        10,
        1,
    },
};

static const MsgFaceAnim sTalk07FaceAnims = {
    gTalk0700Tiles,
    gTalk0700Palette,
    gTalk0700Frames,
    gTalk0700Anims,
    ARRAY_COUNT(gTalk0700Anims),
    1,
};

static const MsgFaceAnim sTalk08FaceAnims[2] = {
    {
        gTalk0800Tiles,
        gTalk0800Palette,
        gTalk0800Frames,
        gTalk0800Anims,
        ARRAY_COUNT(gTalk0800Anims),
        1,
    },
    {
        gTalk0800Tiles,
        gTalk0800Palette,
        gTalk0800Frames,
        gTalk0800Anims + 2,
        4,
        1,
    },
};

static const MsgFaceAnim sTalk09FaceAnims[2] = {
    {
        gTalk0900Tiles,
        gTalk0900Palette,
        gTalk0900Frames,
        gTalk0900Anims,
        ARRAY_COUNT(gTalk0900Anims),
        1,
    },
    {
        gTalk0900Tiles,
        gTalk0900Palette,
        gTalk0900Frames,
        gTalk0900Anims + 1,
        2,
        1,
    },
};

static const MsgFaceAnim sTalk10FaceAnims[15] = {
    {
        gTalk1000Tiles,
        gTalk1000Palette,
        gTalk1000Frames,
        gTalk1000Anims,
        ARRAY_COUNT(gTalk1000Anims),
        1,
    },
    {
        gTalk1000Tiles,
        gTalk1000Palette,
        gTalk1000Frames,
        gTalk1000Anims + 2,
        12,
        1,
    },
    {
        gTalk1000Tiles,
        gTalk1000Palette,
        gTalk1000Frames,
        gTalk1000Anims + 4,
        12,
        1,
    },
    {
        gTalk1000Tiles,
        gTalk1000Palette,
        gTalk1000Frames,
        gTalk1000Anims + 6,
        12,
        1,
    },
    {
        gTalk1000Tiles,
        gTalk1000Palette,
        gTalk1000Frames,
        gTalk1000Anims + 8,
        12,
        1,
    },
    {
        gTalk1000Tiles,
        gTalk1000Palette,
        gTalk1000Frames,
        gTalk1000Anims + 10,
        12,
        1,
    },
    {
        gTalk1002Tiles,
        gTalk1000Palette,
        gTalk1002Frames,
        gTalk1002Anims,
        ARRAY_COUNT(gTalk1002Anims),
        1,
    },
    {
        gTalk1002Tiles,
        gTalk1000Palette,
        gTalk1002Frames,
        gTalk1002Anims + 2,
        6,
        1,
    },
    {
        gTalk1002Tiles,
        gTalk1000Palette,
        gTalk1002Frames,
        gTalk1002Anims + 4,
        6,
        1,
    },
    {
        gTalk1003Tiles,
        gTalk1000Palette,
        gTalk1003Frames,
        gTalk1003Anims,
        ARRAY_COUNT(gTalk1003Anims),
        1,
    },
    {
        gTalk1003Tiles,
        gTalk1000Palette,
        gTalk1003Frames,
        gTalk1003Anims + 2,
        6,
        1,
    },
    {
        gTalk1003Tiles,
        gTalk1000Palette,
        gTalk1003Frames,
        gTalk1003Anims + 4,
        6,
        1,
    },
    {
        gTalk1004Tiles,
        gTalk1000Palette,
        gTalk1004Frames,
        gTalk1004Anims,
        ARRAY_COUNT(gTalk1004Anims),
        1,
    },
    {
        gTalk1004Tiles,
        gTalk1000Palette,
        gTalk1004Frames,
        gTalk1004Anims + 2,
        6,
        1,
    },
    {
        gTalk1004Tiles,
        gTalk1000Palette,
        gTalk1004Frames,
        gTalk1004Anims + 4,
        6,
        1,
    },
};

static const MsgFaceAnim sTalk11FaceAnims[5] = {
    {
        gTalk1100Tiles,
        gTalk1100Palette,
        gTalk1100Frames,
        gTalk1100Anims,
        ARRAY_COUNT(gTalk1100Anims),
        1,
    },
    {
        gTalk1101Tiles,
        gTalk1100Palette,
        gTalk1101Frames,
        gTalk1101Anims,
        ARRAY_COUNT(gTalk1101Anims),
        1,
    },
    {
        gTalk1102Tiles,
        gTalk1100Palette,
        gTalk1102Frames,
        gTalk1102Anims,
        ARRAY_COUNT(gTalk1102Anims),
        1,
    },
    {
        gTalk1103Tiles,
        gTalk1100Palette,
        gTalk1103Frames,
        gTalk1103Anims,
        ARRAY_COUNT(gTalk1103Anims),
        1,
    },
    {
        gTalk1104Tiles,
        gTalk1100Palette,
        gTalk1104Frames,
        gTalk1104Anims,
        ARRAY_COUNT(gTalk1104Anims),
        1,
    },
};

static const MsgFaceAnim sTalk12FaceAnims[4] = {
    {
        gTalk1200Tiles,
        gTalk1200Palette,
        gTalk1200Frames,
        gTalk1200Anims,
        ARRAY_COUNT(gTalk1200Anims),
        1,
    },
    {
        gTalk1201Tiles,
        gTalk1200Palette,
        gTalk1201Frames,
        gTalk1201Anims,
        ARRAY_COUNT(gTalk1201Anims),
        1,
    },
    {
        gTalk1202Tiles,
        gTalk1200Palette,
        gTalk1202Frames,
        gTalk1202Anims,
        ARRAY_COUNT(gTalk1202Anims),
        1,
    },
    {
        gTalk1203Tiles,
        gTalk1200Palette,
        gTalk1203Frames,
        gTalk1203Anims,
        ARRAY_COUNT(gTalk1203Anims),
        1,
    },
};

static const MsgFaceAnim sTalk13FaceAnims[6] = {
    {
        gTalk1300Tiles,
        gTalk1300Palette,
        gTalk1300Frames,
        gTalk1300Anims,
        ARRAY_COUNT(gTalk1300Anims),
        1,
    },
    {
        gTalk1300Tiles,
        gTalk1300Palette,
        gTalk1300Frames,
        gTalk1300Anims + 2,
        8,
        1,
    },
    {
        gTalk1300Tiles,
        gTalk1300Palette,
        gTalk1300Frames,
        gTalk1300Anims + 4,
        8,
        1,
    },
    {
        gTalk1300Tiles,
        gTalk1300Palette,
        gTalk1300Frames,
        gTalk1300Anims + 6,
        8,
        1,
    },
    {
        gTalk1301Tiles,
        gTalk1300Palette,
        gTalk1301Frames,
        gTalk1301Anims,
        ARRAY_COUNT(gTalk1301Anims),
        1,
    },
    {
        gTalk1301Tiles,
        gTalk1300Palette,
        gTalk1301Frames,
        gTalk1301Anims + 2,
        4,
        1,
    },
};

static const MsgFaceAnim sTalk14FaceAnims[3] = {
    {
        gTalk1400Tiles,
        gTalk1400Palette,
        gTalk1400Frames,
        gTalk1400Anims,
        ARRAY_COUNT(gTalk1400Anims),
        1,
    },
    {
        gTalk1401Tiles,
        gTalk1400Palette,
        gTalk1401Frames,
        gTalk1401Anims,
        ARRAY_COUNT(gTalk1401Anims),
        1,
    },
    {
        gTalk1402Tiles,
        gTalk1400Palette,
        gTalk1402Frames,
        gTalk1402Anims,
        ARRAY_COUNT(gTalk1402Anims),
        1,
    },
};

static const MsgFaceAnim sTalk15FaceAnims[5] = {
    {
        gTalk1500Tiles,
        gTalk1500Palette,
        gTalk1500Frames,
        gTalk1500Anims,
        ARRAY_COUNT(gTalk1500Anims),
        1,
    },
    {
        gTalk1500Tiles,
        gTalk1500Palette,
        gTalk1500Frames,
        gTalk1500Anims + 2,
        6,
        1,
    },
    {
        gTalk1502Tiles,
        gTalk1500Palette,
        gTalk1502Frames,
        gTalk1502Anims,
        ARRAY_COUNT(gTalk1502Anims),
        1,
    },
    {
        gTalk1503Tiles,
        gTalk1500Palette,
        gTalk1503Frames,
        gTalk1503Anims,
        ARRAY_COUNT(gTalk1503Anims),
        1,
    },
    {
        gTalk1500Tiles,
        gTalk1500Palette,
        gTalk1500Frames,
        gTalk1500Anims + 4,
        6,
        1,
    },
};

static const MsgFaceAnim sTalk16FaceAnims[4] = {
    {
        gTalk1600Tiles,
        gTalk1600Palette,
        gTalk1600Frames,
        gTalk1600Anims,
        ARRAY_COUNT(gTalk1600Anims),
        1,
    },
    {
        gTalk1600Tiles,
        gTalk1600Palette,
        gTalk1600Frames,
        gTalk1600Anims + 2,
        8,
        1,
    },
    {
        gTalk1600Tiles,
        gTalk1600Palette,
        gTalk1600Frames,
        gTalk1600Anims + 4,
        8,
        1,
    },
    {
        gTalk1600Tiles,
        gTalk1600Palette,
        gTalk1600Frames,
        gTalk1600Anims + 6,
        8,
        1,
    },
};

static const MsgFaceAnim sTalk17FaceAnims[3] = {
    {
        gTalk1700Tiles,
        gTalk1700Palette,
        gTalk1700Frames,
        gTalk1700Anims,
        ARRAY_COUNT(gTalk1700Anims),
        1,
    },
    {
        gTalk1700Tiles,
        gTalk1700Palette,
        gTalk1700Frames,
        gTalk1700Anims + 2,
        6,
        1,
    },
    {
        gTalk1700Tiles,
        gTalk1700Palette,
        gTalk1700Frames,
        gTalk1700Anims + 4,
        6,
        1,
    },
};

static const MsgFaceAnim sTalk18FaceAnims[3] = {
    {
        gTalk1800Tiles,
        gTalk1800Palette,
        gTalk1800Frames,
        gTalk1800Anims,
        ARRAY_COUNT(gTalk1800Anims),
        1,
    },
    {
        gTalk1800Tiles,
        gTalk1800Palette,
        gTalk1800Frames,
        gTalk1800Anims + 2,
        6,
        1,
    },
    {
        gTalk1800Tiles,
        gTalk1800Palette,
        gTalk1800Frames,
        gTalk1800Anims + 4,
        6,
        1,
    },
};

static const MsgFaceAnim sTalk19FaceAnims[3] = {
    {
        gTalk1900Tiles,
        gTalk1900Palette,
        gTalk1900Frames,
        gTalk1900Anims,
        ARRAY_COUNT(gTalk1900Anims),
        1,
    },
    {
        gTalk1900Tiles,
        gTalk1900Palette,
        gTalk1900Frames,
        gTalk1900Anims + 2,
        6,
        1,
    },
    {
        gTalk1900Tiles,
        gTalk1900Palette,
        gTalk1900Frames,
        gTalk1900Anims + 4,
        6,
        1,
    },
};

static const MsgFaceAnim sTalk20FaceAnims[4] = {
    {
        gTalk2000Tiles,
        gTalk2000Palette,
        gTalk2000Frames,
        gTalk2000Anims,
        ARRAY_COUNT(gTalk2000Anims),
        1,
    },
    {
        gTalk2001Tiles,
        gTalk2000Palette,
        gTalk2001Frames,
        gTalk2001Anims,
        ARRAY_COUNT(gTalk2001Anims),
        1,
    },
    {
        gTalk2002Tiles,
        gTalk2000Palette,
        gTalk2002Frames,
        gTalk2002Anims,
        ARRAY_COUNT(gTalk2002Anims),
        1,
    },
    {
        gTalk2003Tiles,
        gTalk2000Palette,
        gTalk2003Frames,
        gTalk2003Anims,
        ARRAY_COUNT(gTalk2003Anims),
        1,
    },
};

static const MsgFaceAnim sTalk21FaceAnims[5] = {
    {
        gTalk2100Tiles,
        gTalk2100Palette,
        gTalk2100Frames,
        gTalk2100Anims,
        ARRAY_COUNT(gTalk2100Anims),
        1,
    },
    {
        gTalk2101Tiles,
        gTalk2100Palette,
        gTalk2101Frames,
        gTalk2101Anims,
        ARRAY_COUNT(gTalk2101Anims),
        1,
    },
    {
        gTalk2102Tiles,
        gTalk2100Palette,
        gTalk2102Frames,
        gTalk2102Anims,
        ARRAY_COUNT(gTalk2102Anims),
        1,
    },
    {
        gTalk2103Tiles,
        gTalk2100Palette,
        gTalk2103Frames,
        gTalk2103Anims,
        ARRAY_COUNT(gTalk2103Anims),
        1,
    },
    {
        gTalk2104Tiles,
        gTalk2100Palette,
        gTalk2104Frames,
        gTalk2104Anims,
        ARRAY_COUNT(gTalk2104Anims),
        1,
    },
};

static const MsgFaceAnim sTalk36FaceAnims = {
    gTalk3600Tiles,
    gTalk3600Palette,
    gTalk3600Frames,
    gTalk3600Anims,
    ARRAY_COUNT(gTalk3600Anims),
    1,
};

static const MsgFaceAnim sTalk37FaceAnims[2] = {
    {
        gTalk3700Tiles,
        gTalk3700Palette,
        gTalk3700Frames,
        gTalk3700Anims,
        ARRAY_COUNT(gTalk3700Anims),
        1,
    },
    {
        gTalk3700Tiles,
        gTalk3700Palette,
        gTalk3700Frames,
        gTalk3700Anims + 2,
        4,
        1,
    },
};

static const MsgFaceAnim sTalk24FaceAnims[6] = {
    {
        gTalk2400Tiles,
        gTalk2400Palette,
        gTalk2400Frames,
        gTalk2400Anims,
        ARRAY_COUNT(gTalk2400Anims),
        1,
    },
    {
        gTalk2401Tiles,
        gTalk2400Palette,
        gTalk2401Frames,
        gTalk2401Anims,
        ARRAY_COUNT(gTalk2401Anims),
        1,
    },
    {
        gTalk2402Tiles,
        gTalk2400Palette,
        gTalk2402Frames,
        gTalk2402Anims,
        ARRAY_COUNT(gTalk2402Anims),
        1,
    },
    {
        gTalk2403Tiles,
        gTalk2400Palette,
        gTalk2403Frames,
        gTalk2403Anims,
        ARRAY_COUNT(gTalk2403Anims),
        1,
    },
    {
        gTalk2404Tiles,
        gTalk2400Palette,
        gTalk2404Frames,
        gTalk2404Anims,
        ARRAY_COUNT(gTalk2404Anims),
        1,
    },
    {
        gTalk2405Tiles,
        gTalk2400Palette,
        gTalk2405Frames,
        gTalk2405Anims,
        ARRAY_COUNT(gTalk2405Anims),
        1,
    },
};

static const MsgFaceAnim sTalk25FaceAnims[6] = {
    {
        gTalk2500Tiles,
        gTalk2500Palette,
        gTalk2500Frames,
        gTalk2500Anims,
        ARRAY_COUNT(gTalk2500Anims),
        1,
    },
    {
        gTalk2501Tiles,
        gTalk2500Palette,
        gTalk2501Frames,
        gTalk2501Anims,
        ARRAY_COUNT(gTalk2501Anims),
        1,
    },
    {
        gTalk2502Tiles,
        gTalk2500Palette,
        gTalk2502Frames,
        gTalk2502Anims,
        ARRAY_COUNT(gTalk2502Anims),
        1,
    },
    {
        gTalk2503Tiles,
        gTalk2500Palette,
        gTalk2503Frames,
        gTalk2503Anims,
        ARRAY_COUNT(gTalk2503Anims),
        1,
    },
    {
        gTalk2504Tiles,
        gTalk2500Palette,
        gTalk2504Frames,
        gTalk2504Anims,
        ARRAY_COUNT(gTalk2504Anims),
        1,
    },
    {
        gTalk2505Tiles,
        gTalk2500Palette,
        gTalk2505Frames,
        gTalk2505Anims,
        ARRAY_COUNT(gTalk2505Anims),
        1,
    },
};

static const MsgFaceAnim sTalk26FaceAnims[2] = {
    {
        gTalk2600Tiles,
        gTalk2600Palette,
        gTalk2600Frames,
        gTalk2600Anims,
        ARRAY_COUNT(gTalk2600Anims),
        1,
    },
    {
        gTalk2601Tiles,
        gTalk2600Palette,
        gTalk2601Frames,
        gTalk2601Anims,
        ARRAY_COUNT(gTalk2601Anims),
        1,
    },
};

static const MsgFaceAnim sTalk30FaceAnims[4] = {
    {
        gTalk3000Tiles,
        gTalk3000Palette,
        gTalk3000Frames,
        gTalk3000Anims,
        ARRAY_COUNT(gTalk3000Anims),
        1,
    },
    {
        gTalk3001Tiles,
        gTalk3000Palette,
        gTalk3001Frames,
        gTalk3001Anims,
        ARRAY_COUNT(gTalk3001Anims),
        1,
    },
    {
        gTalk3002Tiles,
        gTalk3000Palette,
        gTalk3002Frames,
        gTalk3002Anims,
        ARRAY_COUNT(gTalk3002Anims),
        1,
    },
    {
        gTalk3003Tiles,
        gTalk3000Palette,
        gTalk3003Frames,
        gTalk3003Anims,
        ARRAY_COUNT(gTalk3003Anims),
        1,
    },
};

static const MsgFaceAnim sTalk31FaceAnims[4] = {
    {
        gTalk3100Tiles,
        gTalk3100Palette,
        gTalk3100Frames,
        gTalk3100Anims,
        ARRAY_COUNT(gTalk3100Anims),
        1,
    },
    {
        gTalk3101Tiles,
        gTalk3100Palette,
        gTalk3101Frames,
        gTalk3101Anims,
        ARRAY_COUNT(gTalk3101Anims),
        1,
    },
    {
        gTalk3102Tiles,
        gTalk3100Palette,
        gTalk3102Frames,
        gTalk3102Anims,
        ARRAY_COUNT(gTalk3102Anims),
        1,
    },
    {
        gTalk3103Tiles,
        gTalk3100Palette,
        gTalk3103Frames,
        gTalk3103Anims,
        ARRAY_COUNT(gTalk3103Anims),
        1,
    },
};

static const MsgFaceAnim sTalk33FaceAnims[6] = {
    {
        gTalk3300Tiles,
        gTalk3300Palette,
        gTalk3300Frames,
        gTalk3300Anims,
        ARRAY_COUNT(gTalk3300Anims),
        1,
    },
    {
        gTalk3301Tiles,
        gTalk3300Palette,
        gTalk3301Frames,
        gTalk3301Anims,
        ARRAY_COUNT(gTalk3301Anims),
        1,
    },
    {
        gTalk3302Tiles,
        gTalk3300Palette,
        gTalk3302Frames,
        gTalk3302Anims,
        ARRAY_COUNT(gTalk3302Anims),
        1,
    },
    {
        gTalk3303Tiles,
        gTalk3300Palette,
        gTalk3303Frames,
        gTalk3303Anims,
        ARRAY_COUNT(gTalk3303Anims),
        1,
    },
    {
        gTalk3304Tiles,
        gTalk3300Palette,
        gTalk3304Frames,
        gTalk3304Anims,
        ARRAY_COUNT(gTalk3304Anims),
        1,
    },
    {
        gTalk3300Tiles,
        gTalk3300Palette,
        gTalk3300Frames,
        gTalk3300Anims + 2,
        4,
        1,
    },
};

static const MsgFaceAnim sTalk38FaceAnims[5] = {
    {
        gTalk3800Tiles,
        gTalk3800Palette,
        gTalk3800Frames,
        gTalk3800Anims,
        ARRAY_COUNT(gTalk3800Anims),
        1,
    },
    {
        gTalk3800Tiles,
        gTalk3800Palette,
        gTalk3800Frames,
        gTalk3800Anims + 4,
        6,
        1,
    },
    {
        gTalk3802Tiles,
        gTalk3800Palette,
        gTalk3802Frames,
        gTalk3802Anims,
        ARRAY_COUNT(gTalk3802Anims),
        1,
    },
    {
        gTalk3803Tiles,
        gTalk3800Palette,
        gTalk3803Frames,
        gTalk3803Anims,
        ARRAY_COUNT(gTalk3803Anims),
        1,
    },
    {
        gTalk3800Tiles,
        gTalk3800Palette,
        gTalk3800Frames,
        gTalk3800Anims + 2,
        6,
        1,
    },
};

static const MsgFaceAnim sTalk40FaceAnims[4] = {
    {
        gTalk4000Tiles,
        gTalk4000Palette,
        gTalk4000Frames,
        gTalk4000Anims,
        ARRAY_COUNT(gTalk4000Anims),
        1,
    },
    {
        gTalk4001Tiles,
        gTalk4000Palette,
        gTalk4001Frames,
        gTalk4001Anims,
        ARRAY_COUNT(gTalk4001Anims),
        1,
    },
    {
        gTalk4002Tiles,
        gTalk4000Palette,
        gTalk4002Frames,
        gTalk4002Anims,
        ARRAY_COUNT(gTalk4002Anims),
        1,
    },
    {
        gTalk4003Tiles,
        gTalk4000Palette,
        gTalk4003Frames,
        gTalk4003Anims,
        ARRAY_COUNT(gTalk4003Anims),
        1,
    },
};

static const MsgFaceAnim sTalk41FaceAnims[4] = {
    {
        gTalk4100Tiles,
        gTalk4100Palette,
        gTalk4100Frames,
        gTalk4100Anims,
        ARRAY_COUNT(gTalk4100Anims),
        1,
    },
    {
        gTalk4101Tiles,
        gTalk4100Palette,
        gTalk4101Frames,
        gTalk4101Anims,
        ARRAY_COUNT(gTalk4101Anims),
        1,
    },
    {
        gTalk4101Tiles,
        gTalk4100Palette,
        gTalk4101Frames,
        gTalk4101Anims + 2,
        4,
        1,
    },
    {
        gTalk4100Tiles,
        gTalk4100Palette,
        gTalk4100Frames,
        gTalk4100Anims + 2,
        4,
        1,
    },
};

static const MsgFaceAnim sTalk42FaceAnims[3] = {
    {
        gTalk4200Tiles,
        gTalk4200Palette,
        gTalk4200Frames,
        gTalk4200Anims,
        ARRAY_COUNT(gTalk4200Anims),
        1,
    },
    {
        gTalk4200Tiles,
        gTalk4200Palette,
        gTalk4200Frames,
        gTalk4200Anims + 2,
        6,
        1,
    },
    {
        gTalk4200Tiles,
        gTalk4200Palette,
        gTalk4200Frames,
        gTalk4200Anims + 4,
        6,
        1,
    },
};

static const MsgFaceAnim sTalk43FaceAnims[3] = {
    {
        gTalk4300Tiles,
        gTalk4300Palette,
        gTalk4300Frames,
        gTalk4300Anims,
        ARRAY_COUNT(gTalk4300Anims),
        1,
    },
    {
        gTalk4300Tiles,
        gTalk4300Palette,
        gTalk4300Frames,
        gTalk4300Anims + 2,
        6,
        1,
    },
    {
        gTalk4300Tiles,
        gTalk4300Palette,
        gTalk4300Frames,
        gTalk4300Anims + 4,
        6,
        1,
    },
};

static const MsgFaceAnim sTalk44FaceAnims[2] = {
    {
        gTalk4400Tiles,
        gTalk4400Palette,
        gTalk4400Frames,
        gTalk4400Anims,
        ARRAY_COUNT(gTalk4400Anims),
        1,
    },
    {
        gTalk4400Tiles,
        gTalk4400Palette,
        gTalk4400Frames,
        gTalk4400Anims + 2,
        4,
        1,
    },
};

static const MsgFaceAnim sTalk45FaceAnims[2] = {
    {
        gTalk4500Tiles,
        gTalk4500Palette,
        gTalk4500Frames,
        gTalk4500Anims,
        ARRAY_COUNT(gTalk4500Anims),
        1,
    },
    {
        gTalk4500Tiles,
        gTalk4500Palette,
        gTalk4500Frames,
        gTalk4500Anims + 2,
        4,
        1,
    },
};

static const MsgFaceAnim sTalk46FaceAnims[3] = {
    {
        gTalk4600Tiles,
        gTalk4600Palette,
        gTalk4600Frames,
        gTalk4600Anims,
        ARRAY_COUNT(gTalk4600Anims),
        1,
    },
    {
        gTalk4600Tiles,
        gTalk4600Palette,
        gTalk4600Frames,
        gTalk4600Anims + 2,
        6,
        1,
    },
    {
        gTalk4600Tiles,
        gTalk4600Palette,
        gTalk4600Frames,
        gTalk4600Anims + 4,
        6,
        1,
    },
};

static const MsgFaceAnim sTalk47FaceAnims[3] = {
    {
        gTalk4700Tiles,
        gTalk4700Palette,
        gTalk4700Frames,
        gTalk4700Anims,
        ARRAY_COUNT(gTalk4700Anims),
        1,
    },
    {
        gTalk4700Tiles,
        gTalk4700Palette,
        gTalk4700Frames,
        gTalk4700Anims + 2,
        6,
        1,
    },
    {
        gTalk4700Tiles,
        gTalk4700Palette,
        gTalk4700Frames,
        gTalk4700Anims + 4,
        6,
        1,
    },
};

static const MsgFaceAnim sTalk50FaceAnims[4] = {
    {
        gTalk5000Tiles,
        gTalk5000Palette,
        gTalk5000Frames,
        gTalk5000Anims,
        ARRAY_COUNT(gTalk5000Anims),
        1,
    },
    {
        gTalk5001Tiles,
        gTalk5000Palette,
        gTalk5001Frames,
        gTalk5001Anims,
        ARRAY_COUNT(gTalk5001Anims),
        1,
    },
    {
        gTalk5002Tiles,
        gTalk5000Palette,
        gTalk5002Frames,
        gTalk5002Anims,
        ARRAY_COUNT(gTalk5002Anims),
        1,
    },
    {
        gTalk5001Tiles,
        gTalk5001Palette,
        gTalk5001Frames,
        gTalk5001Anims + 2,
        4,
        1,
    },
};

static const MsgFaceAnim sTalk51FaceAnims[3] = {
    {
        gTalk5100Tiles,
        gTalk5100Palette,
        gTalk5100Frames,
        gTalk5100Anims,
        ARRAY_COUNT(gTalk5100Anims),
        1,
    },
    {
        gTalk5100Tiles,
        gTalk5100Palette,
        gTalk5100Frames,
        gTalk5100Anims + 2,
        4,
        1,
    },
    {
        gTalk5102Tiles,
        gTalk5100Palette,
        gTalk5102Frames,
        gTalk5102Anims,
        ARRAY_COUNT(gTalk5102Anims),
        1,
    },
};

static const MsgFaceAnim sTalk55FaceAnims[4] = {
    {
        gTalk5500Tiles,
        gTalk5500Palette,
        gTalk5500Frames,
        gTalk5500Anims,
        ARRAY_COUNT(gTalk5500Anims),
        1,
    },
    {
        gTalk5500Tiles,
        gTalk5500Palette,
        gTalk5500Frames,
        gTalk5500Anims + 2,
        4,
        1,
    },
    {
        gTalk5502Tiles,
        gTalk5500Palette,
        gTalk5502Frames,
        gTalk5502Anims,
        ARRAY_COUNT(gTalk5502Anims),
        1,
    },
    {
        gTalk5503Tiles,
        gTalk5500Palette,
        gTalk5503Frames,
        gTalk5503Anims,
        ARRAY_COUNT(gTalk5503Anims),
        1,
    },
};

static const MsgFaceAnim sTalk39FaceAnims[4] = {
    {
        gTalk3900Tiles,
        gTalk3900Palette,
        gTalk3900Frames,
        gTalk3900Anims,
        ARRAY_COUNT(gTalk3900Anims),
        1,
    },
    {
        gTalk3900Tiles,
        gTalk3900Palette,
        gTalk3900Frames,
        gTalk3900Anims + 2,
        8,
        1,
    },
    {
        gTalk3900Tiles,
        gTalk3900Palette,
        gTalk3900Frames,
        gTalk3900Anims + 4,
        8,
        1,
    },
    {
        gTalk3900Tiles,
        gTalk3900Palette,
        gTalk3900Frames,
        gTalk3900Anims + 6,
        8,
        1,
    },
};

static const MsgFaceAnim sTalk52FaceAnims[5] = {
    {
        gTalk5200Tiles,
        gTalk5200Palette,
        gTalk5200Frames,
        gTalk5200Anims,
        ARRAY_COUNT(gTalk5200Anims),
        1,
    },
    {
        gTalk5201Tiles,
        gTalk5200Palette,
        gTalk5201Frames,
        gTalk5201Anims,
        ARRAY_COUNT(gTalk5201Anims),
        1,
    },
    {
        gTalk5202Tiles,
        gTalk5200Palette,
        gTalk5202Frames,
        gTalk5202Anims,
        ARRAY_COUNT(gTalk5202Anims),
        1,
    },
    {
        gTalk5203Tiles,
        gTalk5200Palette,
        gTalk5203Frames,
        gTalk5203Anims,
        ARRAY_COUNT(gTalk5203Anims),
        1,
    },
    {
        gTalk5204Tiles,
        gTalk5200Palette,
        gTalk5204Frames,
        gTalk5204Anims,
        ARRAY_COUNT(gTalk5204Anims),
        1,
    },
};

static const MsgFaceAnim sTalk54FaceAnims[3] = {
    {
        gTalk5400Tiles,
        gTalk5400Palette,
        gTalk5400Frames,
        gTalk5400Anims,
        ARRAY_COUNT(gTalk5400Anims),
        1,
    },
    {
        gTalk5401Tiles,
        gTalk5400Palette,
        gTalk5401Frames,
        gTalk5401Anims,
        ARRAY_COUNT(gTalk5401Anims),
        1,
    },
    {
        gTalk5402Tiles,
        gTalk5400Palette,
        gTalk5402Frames,
        gTalk5402Anims,
        ARRAY_COUNT(gTalk5402Anims),
        1,
    },
};

static const MsgFaceAnim sTalk49FaceAnims[4] = {
    {
        gTalk4900Tiles,
        gTalk4900Palette,
        gTalk4900Frames,
        gTalk4900Anims,
        ARRAY_COUNT(gTalk4900Anims),
        1,
    },
    {
        gTalk4900Tiles,
        gTalk4900Palette,
        gTalk4900Frames,
        gTalk4900Anims + 2,
        8,
        1,
    },
    {
        gTalk4900Tiles,
        gTalk4900Palette,
        gTalk4900Frames,
        gTalk4900Anims + 4,
        8,
        1,
    },
    {
        gTalk4900Tiles,
        gTalk4900Palette,
        gTalk4900Frames,
        gTalk4900Anims + 6,
        8,
        1,
    },
};

static const MsgFaceAnim sTalk48FaceAnims[5] = {
    {
        gTalk4800Tiles,
        gTalk4800Palette,
        gTalk4800Frames,
        gTalk4800Anims,
        ARRAY_COUNT(gTalk4800Anims),
        1,
    },
    {
        gTalk4801Tiles,
        gTalk4800Palette,
        gTalk4801Frames,
        gTalk4801Anims,
        ARRAY_COUNT(gTalk4801Anims),
        1,
    },
    {
        gTalk4802Tiles,
        gTalk4800Palette,
        gTalk4802Frames,
        gTalk4802Anims,
        ARRAY_COUNT(gTalk4802Anims),
        1,
    },
    {
        gTalk4803Tiles,
        gTalk4800Palette,
        gTalk4803Frames,
        gTalk4803Anims,
        ARRAY_COUNT(gTalk4803Anims),
        1,
    },
    {
        gTalk4804Tiles,
        gTalk4800Palette,
        gTalk4804Frames,
        gTalk4804Anims,
        ARRAY_COUNT(gTalk4804Anims),
        1,
    },
};

static const MsgFaceAnim sTalk56FaceAnims[3] = {
    {
        gTalk5600Tiles,
        gTalk5600Palette,
        gTalk5600Frames,
        gTalk5600Anims,
        ARRAY_COUNT(gTalk5600Anims),
        1,
    },
    {
        gTalk5600Tiles,
        gTalk5600Palette,
        gTalk5600Frames,
        gTalk5600Anims + 2,
        4,
        1,
    },
    {
        gTalk5602Tiles,
        gTalk5600Palette,
        gTalk5602Frames,
        gTalk5602Anims,
        ARRAY_COUNT(gTalk5602Anims),
        1,
    },
};

static const MsgFaceAnim sTalk58FaceAnims[2] = {
    {
        gTalk5800Tiles,
        gTalk5800Palette,
        gTalk5800Frames,
        gTalk5800Anims,
        ARRAY_COUNT(gTalk5800Anims),
        1,
    },
    {
        gTalk5800Tiles,
        gTalk5800Palette,
        gTalk5800Frames,
        gTalk5800Anims + 2,
        4,
        1,
    },
};

static const MsgFaceAnim sTalk59FaceAnims[3] = {
    {
        gTalk5900Tiles,
        gTalk5900Palette,
        gTalk5900Frames,
        gTalk5900Anims,
        ARRAY_COUNT(gTalk5900Anims),
        1,
    },
    {
        gTalk5900Tiles,
        gTalk5900Palette,
        gTalk5900Frames,
        gTalk5900Anims + 2,
        6,
        1,
    },
    {
        gTalk5900Tiles,
        gTalk5900Palette,
        gTalk5900Frames,
        gTalk5900Anims + 4,
        6,
        1,
    },
};

static const MsgFaceAnim sTalk60FaceAnims = {
    gTalk6000Tiles,
    gTalk6000Palette,
    gTalk6000Frames,
    gTalk6000Anims,
    ARRAY_COUNT(gTalk6000Anims),
    1,
};

static const MsgFaceAnim sTalk61FaceAnims[3] = {
    {
        gTalk6100Tiles,
        gTalk6100Palette,
        gTalk6100Frames,
        gTalk6100Anims,
        ARRAY_COUNT(gTalk6100Anims),
        1,
    },
    {
        gTalk6100Tiles,
        gTalk6100Palette,
        gTalk6100Frames,
        gTalk6100Anims + 2,
        6,
        1,
    },
    {
        gTalk6100Tiles,
        gTalk6100Palette,
        gTalk6100Frames,
        gTalk6100Anims + 4,
        6,
        1,
    },
};

static const MsgFaceAnim sTalk34FaceAnims = {
    gTalk3400Tiles,
    gTalk3400Palette,
    gTalk3400Frames,
    gTalk3400Anims,
    ARRAY_COUNT(gTalk3400Anims),
    1,
};

static const MsgFaceAnim sTalk35FaceAnims[3] = {
    {
        gTalk3500Tiles,
        gTalk3500Palette,
        gTalk3500Frames,
        gTalk3500Anims,
        ARRAY_COUNT(gTalk3500Anims),
        1,
    },
    {
        gTalk3501Tiles,
        gTalk3500Palette,
        gTalk3501Frames,
        gTalk3501Anims,
        ARRAY_COUNT(gTalk3501Anims),
        1,
    },
    {
        gTalk3502Tiles,
        gTalk3500Palette,
        gTalk3502Frames,
        gTalk3502Anims,
        ARRAY_COUNT(gTalk3502Anims),
        1,
    },
};

static const MsgFaceAnim sTalk53FaceAnims[5] = {
    {
        gTalk5300Tiles,
        gTalk5300Palette,
        gTalk5300Frames,
        gTalk5300Anims,
        ARRAY_COUNT(gTalk5300Anims),
        1,
    },
    {
        gTalk5301Tiles,
        gTalk5300Palette,
        gTalk5301Frames,
        gTalk5301Anims,
        ARRAY_COUNT(gTalk5301Anims),
        1,
    },
    {
        gTalk5302Tiles,
        gTalk5300Palette,
        gTalk5302Frames,
        gTalk5302Anims,
        ARRAY_COUNT(gTalk5302Anims),
        1,
    },
    {
        gTalk5303Tiles,
        gTalk5300Palette,
        gTalk5303Frames,
        gTalk5303Anims,
        ARRAY_COUNT(gTalk5303Anims),
        1,
    },
    {
        gTalk5304Tiles,
        gTalk5300Palette,
        gTalk5304Frames,
        gTalk5304Anims,
        ARRAY_COUNT(gTalk5304Anims),
        1,
    },
};

static const MsgFaceAnim sTalk29FaceAnims[5] = {
    {
        gTalk2900Tiles,
        gTalk2900Palette,
        gTalk2900Frames,
        gTalk2900Anims,
        ARRAY_COUNT(gTalk2900Anims),
        1,
    },
    {
        gTalk2901Tiles,
        gTalk2900Palette,
        gTalk2901Frames,
        gTalk2901Anims,
        ARRAY_COUNT(gTalk2901Anims),
        1,
    },
    {
        gTalk2902Tiles,
        gTalk2900Palette,
        gTalk2902Frames,
        gTalk2902Anims,
        ARRAY_COUNT(gTalk2902Anims),
        1,
    },
    {
        gTalk2903Tiles,
        gTalk2900Palette,
        gTalk2903Frames,
        gTalk2903Anims,
        ARRAY_COUNT(gTalk2903Anims),
        1,
    },
    {
        gTalk2904Tiles,
        gTalk2900Palette,
        gTalk2904Frames,
        gTalk2904Anims,
        ARRAY_COUNT(gTalk2904Anims),
        1,
    },
};

static const MsgFaceAnim sTalk28FaceAnims[6] = {
    {
        gTalk2800Tiles,
        gTalk2800Palette,
        gTalk2800Frames,
        gTalk2800Anims,
        ARRAY_COUNT(gTalk2800Anims),
        1,
    },
    {
        gTalk2801Tiles,
        gTalk2800Palette,
        gTalk2801Frames,
        gTalk2801Anims,
        ARRAY_COUNT(gTalk2801Anims),
        1,
    },
    {
        gTalk2802Tiles,
        gTalk2800Palette,
        gTalk2802Frames,
        gTalk2802Anims,
        ARRAY_COUNT(gTalk2802Anims),
        1,
    },
    {
        gTalk2803Tiles,
        gTalk2800Palette,
        gTalk2803Frames,
        gTalk2803Anims,
        ARRAY_COUNT(gTalk2803Anims),
        1,
    },
    {
        gTalk2804Tiles,
        gTalk2800Palette,
        gTalk2804Frames,
        gTalk2804Anims,
        ARRAY_COUNT(gTalk2804Anims),
        1,
    },
    {
        gTalk2805Tiles,
        gTalk2800Palette,
        gTalk2805Frames,
        gTalk2805Anims,
        ARRAY_COUNT(gTalk2805Anims),
        1,
    },
};

static const MsgFaceAnim sTalk32FaceAnims[3] = {
    {
        gTalk3200Tiles,
        gTalk3200Palette,
        gTalk3200Frames,
        gTalk3200Anims,
        ARRAY_COUNT(gTalk3200Anims),
        1,
    },
    {
        gTalk3201Tiles,
        gTalk3200Palette,
        gTalk3201Frames,
        gTalk3201Anims,
        ARRAY_COUNT(gTalk3201Anims),
        1,
    },
    {
        gTalk3202Tiles,
        gTalk3200Palette,
        gTalk3202Frames,
        gTalk3202Anims,
        ARRAY_COUNT(gTalk3202Anims),
        1,
    },
};

static const MsgFaceAnim sTalk57FaceAnims[3] = {
    {
        gTalk5700Tiles,
        gTalk5700Palette,
        gTalk5700Frames,
        gTalk5700Anims,
        ARRAY_COUNT(gTalk5700Anims),
        1,
    },
    {
        gTalk5700Tiles,
        gTalk5700Palette,
        gTalk5700Frames,
        gTalk5700Anims + 2,
        4,
        1,
    },
    {
        gTalk5702Tiles,
        gTalk5700Palette,
        gTalk5702Frames,
        gTalk5702Anims,
        ARRAY_COUNT(gTalk5702Anims),
        1,
    },
};

static const MsgFaceAnim sTalk27FaceAnims[5] = {
    {
        gTalk2700Tiles,
        gTalk2700Palette,
        gTalk2700Frames,
        gTalk2700Anims,
        ARRAY_COUNT(gTalk2700Anims),
        1,
    },
    {
        gTalk2700Tiles,
        gTalk2700Palette,
        gTalk2700Frames,
        gTalk2700Anims + 2,
        10,
        1,
    },
    {
        gTalk2700Tiles,
        gTalk2700Palette,
        gTalk2700Frames,
        gTalk2700Anims + 4,
        10,
        1,
    },
    {
        gTalk2700Tiles,
        gTalk2700Palette,
        gTalk2700Frames,
        gTalk2700Anims + 6,
        10,
        1,
    },
    {
        gTalk2700Tiles,
        gTalk2700Palette,
        gTalk2700Frames,
        gTalk2700Anims + 8,
        10,
        1,
    },
};

const MsgFaceAnim* gMsgFaceAnims[62] = {
    sTalk00FaceAnims,
    sTalk01FaceAnims,
    sTalk02FaceAnims,
    &sTalk03FaceAnims,
    sTalk04FaceAnims,
    sTalk05FaceAnims,
    sTalk06FaceAnims,
    &sTalk07FaceAnims,
    sTalk08FaceAnims,
    sTalk09FaceAnims,
    sTalk10FaceAnims,
    sTalk11FaceAnims,
    sTalk12FaceAnims,
    sTalk13FaceAnims,
    sTalk14FaceAnims,
    sTalk15FaceAnims,
    sTalk16FaceAnims,
    sTalk17FaceAnims,
    sTalk18FaceAnims,
    sTalk19FaceAnims,
    sTalk20FaceAnims,
    sTalk21FaceAnims,
    &sTalk36FaceAnims,
    sTalk37FaceAnims,
    sTalk22FaceAnims,
    sTalk23FaceAnims,
    sTalk24FaceAnims,
    sTalk25FaceAnims,
    sTalk26FaceAnims,
    sTalk38FaceAnims,
    sTalk40FaceAnims,
    sTalk41FaceAnims,
    sTalk43FaceAnims,
    sTalk44FaceAnims,
    sTalk50FaceAnims,
    sTalk51FaceAnims,
    sTalk55FaceAnims,
    sTalk42FaceAnims,
    sTalk30FaceAnims,
    sTalk45FaceAnims,
    sTalk46FaceAnims,
    sTalk47FaceAnims,
    sTalk39FaceAnims,
    sTalk52FaceAnims,
    sTalk54FaceAnims,
    sTalk49FaceAnims,
    sTalk48FaceAnims,
    sTalk56FaceAnims,
    sTalk58FaceAnims,
    sTalk59FaceAnims,
    &sTalk60FaceAnims,
    sTalk61FaceAnims,
    &sTalk34FaceAnims,
    sTalk53FaceAnims,
    sTalk29FaceAnims,
    sTalk33FaceAnims,
    sTalk35FaceAnims,
    sTalk32FaceAnims,
    sTalk31FaceAnims,
    sTalk57FaceAnims,
    sTalk28FaceAnims,
    sTalk27FaceAnims,
};

static const s32 sSpeakerFocusYOffsets[45] = {
    -8192, -8192, -8192, -12288, -8192, -8192, -8192, -12288,
    -8192, -8192, -8192, -12288, -8192, -8192, -8192, -12288,
    -8192, -8192, -8192, -12288, -8192, -8192, -8192, -12288,
    -8192, -8192, -8192, -12288, -8192, -8192, -8192, -12288,
    -8192, -8192, -8192, -12288, -8192, -8192, -8192, -12288,
    -8192, -8192, -8192, -12288, -12288,
};

const EventCharaParams gEventCharaParams[94] = {
    { -42, 256, 460, 0, 0, 0 },
    { -34, 256, 384, 0, 0, 0 },
    { -56, 256, 307, 0, 0, 0 },
    { -64, 256, 384, 16, -32, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -54, 256, 384, 0, 0, 0 },
    { -44, 256, 384, 0, 0, 0 },
    { -16, 256, 384, 0, 0, 0 },
    { -32, 256, 384, 0, 0, 0 },
    { -32, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -64, 256, 384, 0, 0, 0 },
    { -64, 256, 384, 0, 0, 0 },
    { -64, 256, 460, 0, 0, 0 },
    { -54, 256, 460, 0, 0, 0 },
    { -76, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -56, 256, 384, 0, 0, 0 },
    { -60, 256, 460, 0, 0, 0 },
    { -70, 332, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -64, 256, 384, 0, 0, 0 },
    { -33, 256, 384, 0, 0, 0 },
    { -77, 256, 460, 0, 0, 0 },
    { -82, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -30, 256, 384, 0, 0, 0 },
    { -90, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -59, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -64, 256, 384, 16, -50, 0 },
    { -49, 256, 384, 0, 0, 0 },
    { -60, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -70, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -56, 256, 384, 0, 0, 0 },
    { -66, 256, 384, 0, 0, 0 },
    { -44, 256, 384, 0, 0, 0 },
    { -90, 256, 384, 0, 0, 0 },
    { -65, 256, 384, 0, 0, 0 },
    { -34, 256, 384, 0, 0, 0 },
    { -72, 256, 384, 0, 0, 0 },
    { -53, 256, 384, 0, 0, 0 },
    { -45, 256, 384, 0, 0, 0 },
    { -54, 256, 384, 0, 0, 0 },
    { -54, 256, 384, 0, 0, 0 },
    { -72, 256, 384, 0, 0, 0 },
    { -54, 256, 384, 0, 0, 0 },
    { -54, 256, 384, 0, 0, 0 },
    { -71, 256, 384, 0, 0, 0 },
    { -54, 256, 384, 0, 0, 0 },
    { -41, 256, 384, 0, 0, 0 },
    { -34, 128, 384, 0, 0, 0 },
    { -54, 204, 384, 0, 0, 0 },
    { -24, 256, 384, 0, 0, 0 },
    { -32, 256, 384, 0, 0, 0 },
    { -23, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -66, 256, 384, 0, 0, 0 },
    { -66, 256, 384, 0, 0, 0 },
    { -60, 256, 384, 0, 0, 0 },
    { -77, 256, 384, 0, 0, 0 },
    { -64, 256, 384, 0, 0, 0 },
    { -60, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { -40, 256, 384, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0 },
};

TaskDesc gTaskDescEventSeq = {
    "event_seq",
    (TaskInitFunc)event_seq_0,
    (TaskUpdateFunc)event_seq_1,
    (TaskDrawFunc)event_seq_2,
    (TaskDestroyFunc)event_seq_3,
    sizeof(EventSeqWork),
};

static TaskDesc sTaskDescEventChara = {
    "event_chara",
    (TaskInitFunc)event_chara_0,
    (TaskUpdateFunc)event_chara_1,
    (TaskDrawFunc)event_chara_2,
    (TaskDestroyFunc)event_chara_3,
    sizeof(EventCharaWork),
};

static const u16 sEventCharaToggleAnims[2] = {
    145, 167,
};

const u16 gUnk_09033C90[4] = {
    2048, 2048, 2048, 2048,
};

const u16 gMsgwinClosedScrollX[4] = {
    0, 0, 255, 255,
};

const u16 gMsgwinOpenScrollX[4] = {
    255, 255, 0, 0,
};

const s32 gMsgwinTextX[4] = {
    4096, 4096, 18432, 18432,
};

const s32 gMsgwinTextY[4] = {
    3584, 28160, 3584, 28160,
};

static const u16* sMsgwinMapBlockPairs[4][2] = {
    {gDefaultBgMap, gMsgwinTopFaceRightMap},
    {gDefaultBgMap, gMsgwinBottomFaceRightMap},
    {gMsgwinTopFaceLeftMap, gDefaultBgMap},
    {gMsgwinBottomFaceLeftMap, gDefaultBgMap},
};

const void* gMsgwinMapBlocks[4] = {
    sMsgwinMapBlockPairs[0],
    sMsgwinMapBlockPairs[1],
    sMsgwinMapBlockPairs[2],
    sMsgwinMapBlockPairs[3],
};

static TaskDesc sTaskDescMsgwin = {
    "msgwin",
    (TaskInitFunc)msgwin_0,
    (TaskUpdateFunc)msgwin_1,
    (TaskDrawFunc)msgwin_2,
    (TaskDestroyFunc)msgwin_3,
    sizeof(MsgWinWork),
};

const s32 gMsgfaceHiddenX[4] = {
    114944, 114944, -76544, -76544,
};

const s32 gMsgfaceShownX[4] = {
    52736, 52736, 9216, 9216,
};

const s32 gMsgfaceY[4] = {
    14336, 32768, 14336, 32768,
};

static TaskDesc sTaskDescMsgface = {
    "msgface",
    (TaskInitFunc)msgface_0,
    (TaskUpdateFunc)msgface_1,
    (TaskDrawFunc)msgface_2,
    (TaskDestroyFunc)msgface_3,
    sizeof(MsgFaceWork),
};

const s32 gMsgwaitIconPos[4][2] = {
    {26112, 15872},
    {26112, 40448},
    {36864, 15872},
    {36864, 40448},
};

const s32 gMsgwaitYesnoCursorY[2] = {
    16128, 19968,
};

static TaskDesc sTaskDescMsgwait = {
    "msgwait",
    (TaskInitFunc)msgwait_0,
    (TaskUpdateFunc)msgwait_1,
    (TaskDrawFunc)msgwait_2,
    (TaskDestroyFunc)msgwait_3,
    sizeof(MsgWaitWork),
};

EventScanlineScroll* gEventScanlineScroll EWRAM_COMMON(4);

void event_seq_0(EventSeqWork* work, u8* arg) {
#ifdef VERSION_EU
    EventBackgroundDef* bg;
#endif
    gEventSoundMix = NULL;
    gBtlWork = NULL;
    work->task = NULL;
    work->eventId = arg[0];
    work->fromGame = arg[1];
    work->seqDef = gEventSequenceDefs[work->eventId];
    work->unk_30 = 0;
    work->unk_31 = 0;
    gEventState->skipHoldTime = 0;
    work->ending = FALSE;
    work->hasBoss = FALSE;

#ifdef VERSION_EU
    work->hasMapAnim = FALSE;
    work->bg3MapUnpacked = FALSE;
    work->bg2MapUnpacked = FALSE;
    work->bg1MapUnpacked = FALSE;
#endif

    if (gEventState != NULL) {
        gEventState->running = TRUE;
        gEventState->talking = FALSE;
        gEventState->hasBg2Map = FALSE;
        gEventState->hasBg1Map = FALSE;
        gEventState->fadedOut = FALSE;
        gEventState->shakeX = 0;
        gEventState->shakeY = 0;
        gEventState->bgEffectActive = FALSE;
        gEventState->ending = FALSE;
        gEventState->answerYes = FALSE;
        gEventState->askedYesNo = FALSE;
        gEventState->endRequest = FALSE;

#ifndef VERSION_EU
        RequestDma3Clear(GetBgCharBase(1), 0x8000);
#endif

        if (work->seqDef->keyframes->flags & CAMERA_KEYFRAME_FLAG_FADE_IN_WHITE) {
            SetBackdropColor(31, 31, 31);
            FadeStartIn(FADE_MODE_WHITE, 0x40);
        }

#ifdef VERSION_EU
        bg = gEventBackgroundDefs[work->eventId];

        if (bg != NULL) {
            if (bg->tiles2 != NULL) {
                if ((bg->flags & EVENT_BG_FLAG_POOH_MAP) != 0) {
                    SetupBg(0, 3, 31, 14);
                    SetupBg(1, 0, 29, 0);
                    SetupBg(2, 2, 30, 0);
                    SetupBg(3, 0, 28, 0);
                } else {
                    SetupBg(0, 3, 31, 14);
                    SetupBg(1, 2, 30, 0);
                    SetupBg(2, 0, 22, 0);
                    SetupBg(3, 0, 23, 0);
                }
            }

            if (bg->isAffine != 0) {
                if (bg->compression[0] == EVENT_BG_COMPRESSION_TILES ||
                    bg->compression[0] == EVENT_BG_COMPRESSION_TILES_AND_MAPS) {
                    LoadBgTilesLz77(2, bg->tiles);
                } else {
                    LoadBgTiles(2, bg->tiles, bg->tilesSize);
                }

                LoadBgPalette(2, bg->palette, bg->paletteSize);
                SetBgColorMode(2, BGCNT_256COLOR);
                SetBgSize(2, BGCNT_AFF512x512);

                if (bg->compression[0] == EVENT_BG_COMPRESSION_MAPS ||
                    bg->compression[0] == EVENT_BG_COMPRESSION_TILES_AND_MAPS) {
                    LoadBgMapLz77(2, (void*)*bg->maps);
                } else {
                    LoadBgMap(2, *bg->maps, 0x1000);
                }

                SetBgAffine(2, 0, Q_8_8(1), Q_8_8(1), 0, 0);
            } else {
                LoadEventBg3(work);
                LoadEventBg2Map(work);
                LoadEventBg1(work);
            }
        }

        if ((work->seqDef->keyframes->flags & 0xFF0) == 0) {
            FadeStartIn(FADE_MODE_BLACK, 64);
        } else if ((work->seqDef->keyframes->flags & 0xFF0) == CAMERA_KEYFRAME_FLAG_FADE_IN_WHITE) {
            FadeStartIn(FADE_MODE_WHITE, 120);
        }

        work->timer = 0;
        InitEventState(work);
#endif
    }
}

#ifdef VERSION_EU
u8 LoadEventBg3(EventSeqWork* work) {
    EventBackgroundDef* bg = gEventBackgroundDefs[work->eventId];

    if (bg != NULL) {
        if (bg->compression[0] == EVENT_BG_COMPRESSION_TILES ||
            bg->compression[0] == EVENT_BG_COMPRESSION_TILES_AND_MAPS) {
            LoadBgTilesLz77(3, bg->tiles);
        } else {
            LoadBgTiles(3, bg->tiles, bg->tilesSize);
        }

        LoadBgPalette(3, bg->palette, bg->paletteSize);

        if (bg->maps != NULL) {
            if (bg->compression[0] == EVENT_BG_COMPRESSION_MAPS ||
                bg->compression[0] == EVENT_BG_COMPRESSION_TILES_AND_MAPS) {
                work->bg3MapUnpacked = TRUE;
                SetBgMapBlocksLz77(3, bg->maps, bg->mapWidth, bg->mapHeight);
#ifdef PLATFORM_PC
                PortSetBgIsoExtend(3, PortIsoScene(work->eventId)); /* wide view: continue the floor past the map */
#endif
            } else {
                work->bg3MapUnpacked = FALSE;
                SetBgMapBlocks(3, bg->maps, bg->mapWidth, bg->mapHeight);
#ifdef PLATFORM_PC
                PortSetBgIsoExtend(3, PortIsoScene(work->eventId)); /* wide view: continue the floor past the map */
#endif
            }

            RedrawBgMapAt(3, 0, 0);
        }
    }

    return 1;
}

u8 LoadEventBg2Map(EventSeqWork* work) {
    EventBackgroundDef* bg = gEventBackgroundDefs[work->eventId];

    if (bg != NULL) {
        if (bg->maps2 != NULL) {
            if (bg->compression[0] == EVENT_BG_COMPRESSION_MAPS ||
                bg->compression[0] == EVENT_BG_COMPRESSION_TILES_AND_MAPS) {
                work->bg2MapUnpacked = TRUE;
                SetBgMapBlocksLz77(2, bg->maps2, bg->mapWidth, bg->mapHeight);
            } else {
                work->bg2MapUnpacked = FALSE;
                SetBgMapBlocks(2, bg->maps2, bg->mapWidth, bg->mapHeight);
            }

            RedrawBgMapAt(2, 0, 0);
            gEventState->hasBg2Map = TRUE;
        } else {
            DisableBg(2);
        }
    }

    return 1;
}

u8 LoadEventBg1(EventSeqWork* work) {
    EventBackgroundDef* bg = gEventBackgroundDefs[work->eventId];

    if (bg != NULL) {
        if (bg->tiles2 != NULL) {
            if ((bg->flags & EVENT_BG_FLAG_POOH_MAP) != 0) {
                LoadBgTiles(2, bg->tiles2, bg->tilesSize2);
            } else if (bg->compression[0] == EVENT_BG_COMPRESSION_TILES ||
                       bg->compression[0] == EVENT_BG_COMPRESSION_TILES_AND_MAPS) {
                LoadBgTilesLz77(1, bg->tiles2);
            } else {
                LoadBgTiles(1, bg->tiles2, bg->tilesSize2);
            }
        }

        if (bg->maps3 != NULL) {
            if ((bg->flags & EVENT_BG_FLAG_ALPHA_BLEND) != 0) {
                gBldCnt = (BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_OBJ);
                gBldAlpha = BLDALPHA_BLEND(14, 5);
                SetBgPriority(2, 1);
                gEventState->bldCnt = (BLDCNT_TGT1_BG1 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_OBJ);
                gEventState->bldAlpha = BLDALPHA_BLEND(14, 5);
            } else {
                gEventState->bldCnt = 0;
                gEventState->bldAlpha = 0;
            }

            if (bg->compression[0] == EVENT_BG_COMPRESSION_MAPS ||
                bg->compression[0] == EVENT_BG_COMPRESSION_TILES_AND_MAPS) {
                work->bg1MapUnpacked = TRUE;
                SetBgMapBlocksLz77(1, bg->maps3, bg->mapWidth, bg->mapHeight);
            } else {
                work->bg1MapUnpacked = FALSE;
                SetBgMapBlocks(1, bg->maps3, bg->mapWidth, bg->mapHeight);
            }

            RedrawBgMapAt(1, 0, 0);
            gEventState->hasBg1Map = TRUE;
        } else {
            DisableBg(1);
        }
    }

    return 1;
}

u8 InitEventState(EventSeqWork* work) {
    EventBackgroundDef* bg = gEventBackgroundDefs[work->eventId];
    const EventCameraKeyframe* keyframe = work->seqDef->keyframes;
    u16 i;
    gEventState->centerX = keyframe->x;
    gEventState->centerY = keyframe->y;
    gEventState->cameraX = gEventState->centerX - 0x7800;
    gEventState->cameraY = gEventState->centerY - 0x5000;
    gEventState->flags = 0;
    gEventState->frame = 0;
    gEventState->focusSpeaker = FALSE;
    gEventState->msgWinPosition = 0;
    gEventState->msgWinOpen = FALSE;
    gEventState->speaker = 0;
    gEventState->focusSteps = 0;
    gEventState->msgWaitActive = FALSE;
    gEventState->unk_7E = 0;
    gEventState->bossChara = 0;

    // @bug bg is NULL for events without a background (NULL read).
    if (BIOS_PTR(bg)->mapAnim != 5) {
        gEventState->mapAnim = BIOS_PTR(bg)->mapAnim;
        work->hasMapAnim = TRUE;
    } else {
        gEventState->mapAnim = bg->mapAnim;
    }

    for (i = 0; i < ARRAY_COUNT(gEventState->charaObjs); i++) {
        gEventState->charaObjs[i] = NULL;
    }

    return 1;
}
#endif

u8 event_seq_1(EventSeqWork* work, void* task) {
    EventSeqArg arg;
#ifndef VERSION_EU
    s32 flag;
#endif
    EventBackgroundDef* bg;
    const EventSequenceDef* seqDef;
#ifndef VERSION_EU
    const EventCameraKeyframe* keyframe;
    u16 i;
#endif
    u8 j;

#ifndef VERSION_EU
    flag = FALSE;
#endif
    bg = gEventBackgroundDefs[work->eventId];

#ifndef VERSION_EU
    if (bg != NULL) {
        if (bg->tiles2 != NULL) {
            if ((bg->flags & EVENT_BG_FLAG_POOH_MAP) != 0) {
                SetupBg(0, 3, 31, 14);
                SetupBg(1, 0, 29, 0);
                SetupBg(2, 2, 30, 0);
                SetupBg(3, 0, 28, 0);
            } else {
                SetupBg(0, 3, 31, 14);
                SetupBg(1, 2, 30, 0);
                SetupBg(2, 0, 22, 0);
                SetupBg(3, 0, 23, 0);
            }
        }

        if (bg->isAffine != 0) {
            LoadBgTiles(2, bg->tiles, bg->tilesSize);
            LoadBgPalette(2, bg->palette, bg->paletteSize);
            SetBgColorMode(2, BGCNT_256COLOR);
            SetBgSize(2, BGCNT_AFF512x512);
            LoadBgMap(2, *bg->maps, 0x1000);
            SetBgAffine(2, 0, Q_8_8(1), Q_8_8(1), 0, 0);
        } else {
            LoadBgTiles(3, bg->tiles, bg->tilesSize);
            LoadBgPalette(3, bg->palette, bg->paletteSize);

            if (bg->tiles2 != NULL) {
                if ((bg->flags & EVENT_BG_FLAG_POOH_MAP) != 0) {
                    LoadBgTiles(2, bg->tiles2, bg->tilesSize2);
                } else {
                    LoadBgTiles(1, bg->tiles2, bg->tilesSize2);
                }
            }

            if (bg->maps != NULL) {
                SetBgMapBlocks(3, bg->maps, bg->mapWidth, bg->mapHeight);
#ifdef PLATFORM_PC
                PortSetBgIsoExtend(3, PortIsoScene(work->eventId)); /* wide view: continue the floor past the map */
#endif
                RedrawBgMapAt(3, 0, 0);
            }

            if (bg->maps2 != NULL) {
                SetBgMapBlocks(2, bg->maps2, bg->mapWidth, bg->mapHeight);
                RedrawBgMapAt(2, 0, 0);
                gEventState->hasBg2Map = TRUE;
            } else {
                DisableBg(2);
            }

            if (bg->maps3 != NULL) {
                if ((bg->flags & EVENT_BG_FLAG_ALPHA_BLEND) != 0) {
                    gBldCnt = (BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_OBJ);
                    gBldAlpha = BLDALPHA_BLEND(14, 5);
                    SetBgPriority(2, 1);
                    gEventState->bldCnt = (BLDCNT_TGT1_BG1 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_OBJ);
                    gEventState->bldAlpha = BLDALPHA_BLEND(14, 5);
                } else {
                    gEventState->bldCnt = 0;
                    gEventState->bldAlpha = 0;
                }

                SetBgMapBlocks(1, bg->maps3, bg->mapWidth, bg->mapHeight);
                RedrawBgMapAt(1, 0, 0);
                gEventState->hasBg1Map = TRUE;
            } else {
                DisableBg(1);
            }
        }
    }

    keyframe = work->seqDef->keyframes;
    gEventState->centerX = keyframe->x;
    gEventState->centerY = keyframe->y;
    gEventState->cameraX = gEventState->centerX - 0x7800;
    gEventState->cameraY = gEventState->centerY - 0x5000;
    gEventState->flags = 0;
    gEventState->frame = 0;
    gEventState->focusSpeaker = FALSE;
    gEventState->msgWinPosition = 0;
    gEventState->msgWinOpen = FALSE;
    gEventState->speaker = 0;
    gEventState->focusSteps = 0;
    gEventState->msgWaitActive = FALSE;
    gEventState->unk_7E = 0;
    gEventState->bossChara = 0;

    // @bug bg is NULL for events without a background (NULL read).
    if (BIOS_PTR(bg)->mapAnim != 5) {
        gEventState->mapAnim = BIOS_PTR(bg)->mapAnim;
        flag = TRUE;
    } else {
        gEventState->mapAnim = 5;
    }

    i = 0;
    seqDef = work->seqDef;

    while (i < ARRAY_COUNT(gEventState->charaObjs)) {
        gEventState->charaObjs[i] = NULL;
        i++;
    }

#else
    seqDef = work->seqDef;
#endif

    TaskPoolInit(&work->tasks, seqDef->charaCount + 8);
    TaskPoolInit(&work->tasks2, 1);
    work->task = TaskCreate(&work->tasks2, &sTaskDescMsgwin, &work->eventId);

    for (j = 0; j < seqDef->charaCount; j++) {
        arg.eventId = work->eventId;
        arg.chara = seqDef->charaTracks[j].chara;
        arg.track = j;

        if (arg.chara > 94) {
            work->hasBoss = TRUE;
            gEventState->bossChara = arg.chara;
        }

        TaskCreate(&work->tasks, &sTaskDescEventChara, &arg);
    }

    TaskCreate(&work->tasks, &gTaskDescView, &work->eventId);
    TaskCreate(&work->tasks, &gTaskDescEvSound, &work->eventId);
    TaskCreate(&work->tasks, &gTaskDescEVBGEFFECT, &work->eventId);

#ifdef VERSION_EU
    if (work->hasMapAnim) {
#else
    if (flag) {
#endif
        TaskCreate(&work->tasks, &gTaskDescMapAnim, NULL);
    }

#ifndef VERSION_EU
    if ((work->seqDef->keyframes->flags & 0xFF0) == 0) {
        FadeStartIn(FADE_MODE_BLACK, 64);
    } else if ((work->seqDef->keyframes->flags & 0xFF0) == CAMERA_KEYFRAME_FLAG_FADE_IN_WHITE) {
        FadeStartIn(FADE_MODE_WHITE, 120);
    }

    work->timer = 0;
#endif

    if (bg != NULL) {
        if (bg->mapObjects != NULL) {
            TaskCreate(&work->tasks, &gTaskDescEvMapObj, &work->eventId);
        }

        if ((bg->flags & EVENT_BG_FLAG_POOH_MAP) != 0) {
            func_080CA35C();
            TaskCreate(&work->tasks, &gTaskDescPooMapanime, NULL);
        }
    }

    SetTaskUpdate(task, (TaskUpdateFunc)UpdateEventSeq);
    return 1;
}

u8 UpdateEventSeqSkip() {
    u8 fading = FadeIsActive();
    u8 result;

    if (fading) {
        result = 1;
    } else {
        gEventState->running = FALSE;
        m4aMPlayAllStop();
        result = 0;
    }

    return result;
}

#ifdef PLATFORM_PC
/* The scene pause (port_ui.c): a short tap of START (not the hold that skips,
   not an answer to a yes/no choice) opens it; its "skip scene" ends the scene
   exactly as holding START does, where the game allows that. */
void PortUiOpenScenePause(int skippable);
static u8 sPortStartFrames, sPortStartChoice, sPortSkipRequest, sPortStartVoid;
static unsigned sPortStartSerial;
static const void* sPortStartWork; /* the event the START count belongs to */
static u16 sPortStartEvent;
unsigned PortUiOpenSerial(void);

void PortEventSkipScene(void) {
    sPortSkipRequest = TRUE;
}

int PortHdScenePlay(int eventId);
/* the event whose HD scene was offered (once per event): its task and number, forgotten when the event mode ends
   (mode_event.c Event_2). The task alone was not enough: an event that hands over to the next one (the last battle's
   event to the capsule room's ending, issue #42) gets its task at the same address, and the ending's scene was
   never offered. */
static const void* sPortHdSceneWork;
static u16 sPortHdSceneEvent;
void PortHdSceneForget(void) {
    sPortHdSceneWork = NULL;
}

static int PortEventSkippable(u16 eventId) {
    return eventId != EVENT_068_13F_CASTLE_OBLIVION_LAST3 && eventId != EVENT_083_MONSTORO_E3_FAILURE_1 &&
           eventId != EVENT_084_MONSTORO_E3_FAILURE_2;
}
#endif

u8 UpdateEventSeq(EventSeqWork* work, void* task) {
    const EventSequenceDef* seqDef;
    u8 i;
#ifdef PLATFORM_PC
    u8 portSkip = FALSE, portHd = FALSE;
#endif

    if (gEventState == NULL) {
        return 0;
    }

#ifdef PLATFORM_PC
    /* A tap is a START pressed and released within this event, alone, with the options never opened meanwhile: a
       START held into the event, or the one of START + SELECT (the options) released as they close, opened the
       pause on its own (23/10). */
    if (sPortStartWork != work || sPortStartEvent != work->eventId) {
        sPortStartWork = work;
        sPortStartEvent = work->eventId;
        sPortStartFrames = 0;
        sPortStartVoid = (GetKeysHeld() & START_BUTTON) != 0;
    }
    if (GetKeysHeld() & START_BUTTON) {
        if (sPortStartFrames == 0) {
            sPortStartChoice = gEventState->msgWaitActive && gEventState->askedYesNo;
            sPortStartSerial = PortUiOpenSerial();
        }
        if (GetKeysHeld() & SELECT_BUTTON)
            sPortStartVoid = TRUE;
        if (sPortStartFrames < 255)
            sPortStartFrames++;
    } else {
        if (sPortStartFrames > 0 && sPortStartFrames <= 32 && !sPortStartChoice && !work->ending && !sPortStartVoid
            && sPortStartSerial == PortUiOpenSerial())
            PortUiOpenScenePause(PortEventSkippable(work->eventId));
        sPortStartFrames = 0;
        sPortStartVoid = FALSE;
    }
    if (sPortSkipRequest) {
        sPortSkipRequest = FALSE;
        portSkip = PortEventSkippable(work->eventId) && !work->ending;
    }
    /* the full HD mod: the event's Re:CoM scene plays first, then the event ends as skipping it does */
    if ((sPortHdSceneWork != work || sPortHdSceneEvent != work->eventId) && !work->ending) {
        sPortHdSceneWork = work;
        sPortHdSceneEvent = work->eventId;
        if (PortEventSkippable(work->eventId) && PortHdScenePlay(work->eventId))
            portSkip = portHd = TRUE;
    }
#endif

#ifdef PLATFORM_PC
    /* holding START skips the scene, but not the START of START + SELECT (the options) */
    if ((GetKeysHeld() & START_BUTTON) != 0 && !sPortStartVoid && sPortStartSerial == PortUiOpenSerial()) {
#else
    if ((GetKeysHeld() & START_BUTTON) != 0) {
#endif
        switch (work->eventId) {
        case EVENT_068_13F_CASTLE_OBLIVION_LAST3:
        case EVENT_083_MONSTORO_E3_FAILURE_1:
        case EVENT_084_MONSTORO_E3_FAILURE_2:
            break;
        default:
            gEventState->skipHoldTime++;
            break;
        }
    } else {
        gEventState->skipHoldTime = 0;
    }

#ifdef PLATFORM_PC
    if (gEventState->skipHoldTime > 64 || gEventState->endRequest == TRUE || portSkip) {
#else
    if (gEventState->skipHoldTime > 64 || gEventState->endRequest == TRUE) {
#endif
        gEventState->skipHoldTime = 64;
        work->ending = TRUE;
        gEventState->ending = TRUE;
#ifdef PLATFORM_PC
        /* after an HD scene (which ends on black) the event's own picture never shows: its fade-out is already black */
        FadeStartOut(FADE_MODE_BLACK, portHd ? 1 : 64);
#else
        FadeStartOut(FADE_MODE_BLACK, 64);
#endif
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateEventSeqSkip);

        for (i = 0; i < 32; i++) {
            FadeSetPaletteExcluded(i, FALSE);
        }

        return 1;
    }

    if (work->timer == work->seqDef->startDelay) {
        gEventState->flags |= EVENT_FLAG_STARTED;
    } else {
        s32 fadeFlags = work->seqDef->keyframes->flags & 0xFF0;

        if (fadeFlags == 0) {
            FadeStartIn(FADE_MODE_BLACK, 64);
        } else if (fadeFlags == 128) {
            FadeStartIn(FADE_MODE_WHITE, 120);
        }

        work->timer++;
    }

    TaskPoolUpdate(&work->tasks);
    TaskPoolUpdate(&work->tasks2);

    if (work->hasBoss) {
        gBtlWork->viewX = gEventState->cameraX;
        gBtlWork->viewY = gEventState->cameraY;
    }

    if ((gEventState->flags & (EVENT_FLAG_PAUSED | EVENT_FLAG_STARTED)) == EVENT_FLAG_STARTED) {
        gEventState->frame++;
    }

    seqDef = gEventSequenceDefs[work->eventId];

    if (gEventState->frame >= seqDef->endFrame && !work->ending && !FadeIsActive()) {
        if (!gEventState->fadedOut) {
            FadeStartOut(FADE_MODE_BLACK, 64);
        }

        work->ending = TRUE;
        gEventState->ending = TRUE;
    }

    if (work->ending == TRUE) {
        for (i = 0; i < 32; i++) {
            FadeSetPaletteExcluded(i, FALSE);
        }

        if (!FadeIsActive()) {
            gEventState->running = FALSE;
            return 0;
        }
    }

    return 1;
}

void event_seq_2(EventSeqWork* work) {
    TaskPoolDraw(&work->tasks2);

    if (work->hasBoss) {
        TaskPoolDraw(&gBtlWork->taskPools[0]);
    }

    TaskPoolDraw(&work->tasks);
}

void event_seq_3(EventSeqWork* work) {
    TaskPoolDestroy(&work->tasks);

    if (work->task != NULL) {
        TaskPoolDestroy(&work->tasks2);
    }

#ifdef VERSION_EU
    if (work->bg3MapUnpacked) {
        FreeBgDecompressedMap(3);
    }

    if (work->bg2MapUnpacked) {
        FreeBgDecompressedMap(2);
    }

    if (work->bg1MapUnpacked) {
        FreeBgDecompressedMap(1);
    }
#endif
}

void event_chara_0(EventCharaWork* work, EventSeqArg* arg) {
    s32 viewX;
    s32 viewY;

    TaskPoolInit(&work->tasks, 8);
    work->arg = *arg;
    work->keyframes = gEventSequenceDefs[work->arg.eventId]->charaTracks[work->arg.track].keyframes;
    work->keyframe = 0;
    work->steps = work->keyframes->frame;
    work->hopVelocity = 0;
    work->unk_18C = 0;
    work->unk_198 = 0;
    work->effectLevel = 0;
    work->effectTimer = 0;
    work->spriteX = 0;
    work->spriteY = 0;
    work->tiles = NULL;
    work->palette = NULL;
    work->gfx = NULL;
    work->spriteFlipX = FALSE;
    work->callbackActive = FALSE;
    work->usesBtlWork = FALSE;
    work->finished = FALSE;
    work->bobPhase = 0;
    work->visible = 1;
    work->unk_1B8 = 0;
    gEventState->charaObjs[work->arg.track] = &work->obj;

    switch (work->arg.chara) {
    case EVENT_CHARA_BOSS_TRICKMASTER:
        // @bug Never sets gBtlWork->actor, which this intro dereferences (NULL read and write).
        gBtlWork = EwramAlloc(sizeof(BtlWork));
        BtlWorkInit();
        TaskPoolInit(&gBtlWork->taskPools[0], 32);
        TaskPoolInit(&gBtlWork->taskPools[1], 1);
        gBtlWork->flags = 0;
        work->usesBtlWork = TRUE;
        gBtlWork->viewX = gEventState->cameraX;
        gBtlWork->viewY = gEventState->cameraY;
        gBtlWork->scale = Q_8_8(1);
        gBtlWork->rotation = 0;
        gBtlWork->zoomScale = Q_8_8(1);
        gBtlWork->x = gEventState->cameraX;
        gBtlWork->y = gEventState->cameraY;
        gBtlWork->x2 = 0x10000;
        gBtlWork->y2 = 0x14000;
        gBtlWork->zoomX = 0x10000;
        gBtlWork->zoomY = 0x14000;
        gBtlWork->zoomSteps = 15;
        work->obj.x = work->keyframes->x;
        work->obj.y = work->keyframes->y;
        work->obj.z = work->keyframes->z;
        TaskCreate(&work->tasks, &gTaskDescBosTm, &work->obj);
        break;
    case EVENT_CHARA_BOSS_PARASITE_CAGE:
        gBtlWork = EwramAlloc(sizeof(BtlWork));
        SetBgPriority(0, 2);
        SetBgPriority(1, 1);
        SetBgPriority(2, 0);
        BtlWorkInit();
        gBtlWork->actor = &work->actor;
        gBtlWork->flags = BTL_FLAG_BOSS_BATTLE;
        TaskPoolInit(&gBtlWork->taskPools[0], 32);
        TaskPoolInit(&gBtlWork->taskPools[1], 1);
        work->usesBtlWork = TRUE;
        gBtlWork->viewY = 0x5400;
        gBtlWork->scale = Q_8_8(1);
        gBtlWork->rotation = 0;
        SetBattleBounds(128, 424, 294, 384);
        gEventState->bossTask = TaskCreate(&work->tasks, &gTaskDescBosPc, NULL);
        break;
    case EVENT_CHARA_BOSS_PARASITE_CAGE_2:
        gBtlWork = EwramAlloc(sizeof(BtlWork));
        SetBgPriority(0, 2);
        SetBgPriority(1, 1);
        SetBgPriority(2, 0);
        BtlWorkInit();
        gBtlWork->actor = &work->actor;
        gBtlWork->flags = BTL_FLAG_BOSS_BATTLE;
        TaskPoolInit(&gBtlWork->taskPools[0], 32);
        TaskPoolInit(&gBtlWork->taskPools[1], 1);
        work->usesBtlWork = TRUE;
        gBtlWork->viewY = 0x5400;
        gBtlWork->scale = Q_8_8(1);
        gBtlWork->rotation = 0;
        SetBattleBounds(128, 424, 294, 384);
        gEventState->bossTask = TaskCreate(&work->tasks, &gTaskDescBosPc, &work->tasks);
        work->hasObj = FALSE;
        gEventState->cameraX = viewX = gBtlWork->viewX;
        gEventState->cameraY = viewY = gBtlWork->viewY;
        gEventState->centerX = gBtlWork->x;
        gEventState->centerY = gBtlWork->y;
        gEventState->x = viewX;
        gEventState->y = viewY;
        break;
    case EVENT_CHARA_BOSS_MARLUXIA_2:
        SetBgSize(1, BGCNT_TXT512x256);
        SetBgPriority(0, 2);
        SetBgPriority(1, 1);
        SetBgPriority(2, 0);
        gBtlWork = EwramAlloc(sizeof(BtlWork));
        BtlWorkInit();
        gBtlWork->actor = &work->actor;
        gBtlWork->flags = BTL_FLAG_BOSS_BATTLE;
        TaskPoolInit(&gBtlWork->taskPools[0], 32);
        TaskPoolInit(&gBtlWork->taskPools[1], 1);
        work->usesBtlWork = TRUE;
        gBtlWork->viewY = 0x5400;
        gBtlWork->scale = Q_8_8(1);
        gBtlWork->rotation = 0;
        SetBattleBounds(128, 368, 480, 512);
        gEventState->bossTask = TaskCreate(&work->tasks, &gTaskDescBosLst, &work->tasks);
        break;
    case EVENT_CHARA_BOSS_JAFAR:
        SetupBg(0, 0, 24, 0);
        SetupBg(1, 0, 26, 0);
        SetupBg(2, 2, 28, 14);
        SetBgPriority(0, 2);
        SetBgPriority(1, 1);
        SetBgPriority(2, 0);
        gBtlWork = EwramAlloc(sizeof(BtlWork));
        BtlWorkInit();
        gBtlWork->actor = &work->actor;
        gBtlWork->flags = BTL_FLAG_BOSS_BATTLE;
        TaskPoolInit(&gBtlWork->taskPools[0], 32);
        TaskPoolInit(&gBtlWork->taskPools[1], 1);
        work->usesBtlWork = TRUE;
        gBtlWork->x = 0x26600;
        gBtlWork->y = 0x12800;
        gBtlWork->viewX = 0x26600;
        gBtlWork->viewY = 0x12800;
        gBtlWork->x2 = 0x26600;
        gBtlWork->y2 = 0x12800;
        work->obj.x = 0x2A200;
        work->obj.y = 0x15E00;
        work->obj.z = -0x3800;
        SetBattleBounds(420, 612, 328, 384);
        TaskCreate(&work->tasks, &gTaskDescBosJf, &work->obj);
        break;
    case EVENT_CHARA_BOSS_DARKSIDE:
        SetupBg(0, 0, 24, 0);
        SetupBg(1, 0, 26, 0);
        SetupBg(2, 2, 28, 14);
        SetBgPriority(0, 2);
        SetBgPriority(1, 1);
        SetBgPriority(2, 0);
        // @bug Never sets gBtlWork->actor, which this intro dereferences (NULL read and write).
        gBtlWork = EwramAlloc(sizeof(BtlWork));
        BtlWorkInit();
        TaskPoolInit(&gBtlWork->taskPools[0], 32);
        TaskPoolInit(&gBtlWork->taskPools[1], 1);
        gBtlWork->flags = 0;
        work->usesBtlWork = TRUE;
        gBtlWork->x = 0x12C00;
        gBtlWork->y = 0x16800;
        gBtlWork->viewX = 0x12C00;
        gBtlWork->viewY = 0x16800;
        gBtlWork->x2 = 0x12C00;
        gBtlWork->y2 = 0x16800;
        gEventState->cameraX = gBtlWork->x - 0x7800;
        gEventState->cameraY = gBtlWork->y - 0x5000;
        gEventState->centerX = gBtlWork->x;
        gEventState->centerY = gBtlWork->y;
        gEventState->shakeX = 0;
        gEventState->shakeY = 0;
        work->obj.x = work->keyframes->x;
        work->obj.y = work->keyframes->y;
        work->obj.z = work->keyframes->z;
        SetBattleBounds(0, 256, 328, 424);
        TaskCreate(&work->tasks, &gTaskDescBosDsd, &work->obj);
        break;
    case EVENT_CHARA_BOSS_OOGIE_BOOGIE:
        SetupBg(0, 0, 24, 0);
        SetupBg(1, 0, 26, 0);
        SetupBg(2, 2, 28, 10);
        SetBgPriority(0, 3);
        SetBgPriority(1, 2);
        SetBgPriority(2, 1);
        gBtlWork = EwramAlloc(sizeof(BtlWork));
        BtlWorkInit();
        gBtlWork->actor = &work->actor;
        gBtlWork->flags = BTL_FLAG_BOSS_BATTLE;
        TaskPoolInit(&gBtlWork->taskPools[0], 32);
        TaskPoolInit(&gBtlWork->taskPools[1], 1);
        work->usesBtlWork = TRUE;
        TaskCreate(&work->tasks, &gTaskDescBosBoogie, NULL);
        gBtlWork->fadeAmount = 5;
        break;
    case EVENT_CHARA_BOSS_URSULA:
        SetupBg(0, 0, 24, 0);
        SetupBg(1, 0, 26, 0);
        SetupBg(2, 2, 28, 10);
        SetBgPriority(0, 3);
        SetBgPriority(1, 2);
        SetBgPriority(2, 1);
        gBtlWork = EwramAlloc(sizeof(BtlWork));
        BtlWorkInit();
        gBtlWork->actor = &work->actor;
        gBtlWork->flags = BTL_FLAG_BOSS_BATTLE;
        TaskPoolInit(&gBtlWork->taskPools[0], 32);
        TaskPoolInit(&gBtlWork->taskPools[1], 1);
        work->usesBtlWork = TRUE;
        TaskCreate(&work->tasks, &gTaskDescBosUrsula, NULL);
        gBtlWork->fadeAmount = 5;
        break;
    case EVENT_CHARA_BOSS_GUARD_ARMOR:
        SetupBg(0, 0, 24, 0);
        SetupBg(1, 0, 26, 0);
        SetupBg(2, 2, 28, 10);
        SetBgPriority(0, 3);
        SetBgPriority(1, 2);
        SetBgPriority(2, 1);
        gBtlWork = EwramAlloc(sizeof(BtlWork));
        BtlWorkInit();
        gBtlWork->actor = &work->actor;
        gBtlWork->flags = BTL_FLAG_BOSS_BATTLE;
        TaskPoolInit(&gBtlWork->taskPools[0], 32);
        TaskPoolInit(&gBtlWork->taskPools[1], 1);
        work->usesBtlWork = TRUE;
        TaskCreate(&work->tasks, &gTaskDescBosGa, (void*)1);
        gBtlWork->fadeAmount = 5;
        break;
    default:
        if ((work->keyframes->flags & CHARA_KEYFRAME_FLAG_DEFER_SPAWN) == 0) {
            CreateEvtObjTask(&work->tasks, &work->obj, work->arg.chara, work->keyframes->anim, work->keyframes->x, work->keyframes->y, work->keyframes->z);
            work->hasObj = TRUE;
        } else {
            work->hasObj = FALSE;
        }

        SetupEventCharaShadow(work);
        break;
    }

    if (work->hasObj) {
        ApplyEventCharaDrawFlags(work);
    }
}

u8 event_chara_1(EventCharaWork* work, void* task) {
    u8 advanced;
    s32 viewX;
    s32 viewY;

    advanced = AdvanceEventCharaKeyframe(work);

    if (work->hasObj) {
        UpdateEventCharaMotion(work);
    }

    PlayEventCharaAnimSounds(work);

    if (advanced) {
        if (work->keyframes[work->keyframe].update != NULL) {
            SetTaskUpdate(task, work->keyframes[work->keyframe].update);
            ((void (*)(EventCharaWork*, void*))work->keyframes[work->keyframe].update)(work, task);
        }

        if (work->keyframes[work->keyframe].callback != NULL) {
            work->keyframes[work->keyframe].callback(work);
            work->callbackActive = TRUE;
        } else {
            work->callbackActive = FALSE;
        }
    }

    if (work->keyframes[work->keyframe].flags & CHARA_KEYFRAME_FLAG_NO_SHADOW) {
        work->obj.flags |= EVTOBJ_FLAG_NO_SHADOW;
    } else {
        work->obj.flags &= ~EVTOBJ_FLAG_NO_SHADOW;
    }

    if (work->keyframes[work->keyframe].flags & CHARA_KEYFRAME_FLAG_PLAYER_CONTROL) {
        gEventState->flags |= EVENT_FLAG_PAUSED;
        gEventState->flags |= EVENT_FLAG_PLAYER_CONTROL;
        work->speed = 0;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateEventCharaControl);
    }

    TaskPoolUpdate(&work->tasks);

    if (work->usesBtlWork) {
        switch (work->arg.chara) {
        case EVENT_CHARA_BOSS_OOGIE_BOOGIE:
            gEventState->x = gEventState->cameraX = gBtlWork->x2;
            gEventState->y = gEventState->cameraY = gBtlWork->y2;
            TaskPoolUpdate(&gBtlWork->taskPools[1]);
            break;
        case EVENT_CHARA_BOSS_URSULA:
            gBtlWork->viewY = gBtlWork->y;
            gBtlWork->x2 = gEventState->cameraX;
            gBtlWork->y2 = gEventState->cameraY;
            gEventState->x = gEventState->cameraX;
            gEventState->y = gEventState->cameraY;
            TaskPoolUpdate(&gBtlWork->taskPools[0]);
            TaskPoolUpdate(&gBtlWork->taskPools[1]);
            break;
        case EVENT_CHARA_BOSS_JAFAR:
            gEventState->cameraX = viewX = gBtlWork->viewX;
            gEventState->cameraY = viewY = gBtlWork->viewY;
            gEventState->centerX = gBtlWork->x;
            gEventState->centerY = gBtlWork->y;
            gEventState->x = viewX;
            gEventState->y = viewY;
            break;
        case EVENT_CHARA_BOSS_PARASITE_CAGE_2:
            gBtlWork->x2 = gEventState->centerX;
            gBtlWork->y2 = gEventState->centerY;
            break;
        case EVENT_CHARA_BOSS_DARKSIDE:
            break;
        }
    }

    if (work->arg.chara == EVENT_CHARA_SORA) {
        if (gBtlWork != NULL) {
            /* @bug: before a battle starts its actor is NULL; the cartridge writes to the BIOS (ignored) */
            BIOS_PTR(gBtlWork->actor)->x = work->obj.x - 0x7800;
            BIOS_PTR(gBtlWork->actor)->y = work->obj.y - 0x5000;
            BIOS_PTR(gBtlWork->actor)->z = 0;
        }
    }

    if (work->finished) {
        return 0;
    }

    if (work->keyframes[work->keyframe].flags & CHARA_KEYFRAME_FLAG_TRANSLUCENT) {
        if (gFrameCounter % 6 == 0) {
            u16 alpha = GetRandom() % 7 + 4;

            gBldAlpha = ((16 - alpha) << 8) | alpha;
        }
    }

    return 1;
}

static inline s16 GetEventCharaScreenX(EventCharaWork* work) {
    return (work->spriteX >> 8) - (gEventState->x >> 8);
}

void event_chara_2(EventCharaWork* work) {
    const EventCharaKeyframe* keyframe;
    s32 save;
    s32 x;
    s32 y;
    u16 flags;

    save = work->obj.z;
    keyframe = &work->keyframes[work->keyframe];

    if (keyframe->flags & CHARA_KEYFRAME_FLAG_BOB) {
        work->obj.z = gSineTable[work->bobPhase] * 2 + save;
    } else if (keyframe->flags & CHARA_KEYFRAME_FLAG_BOB_LARGE) {
        work->obj.z = gSineTable[work->bobPhase] * 3 + save;
    }

    if (work->visible != 0) {
        TaskPoolDraw(&work->tasks);
    }

    if (work->arg.chara == EVENT_CHARA_BOSS_URSULA) {
        TaskPoolDraw(&gBtlWork->taskPools[0]);
    }

    if (work->tiles != NULL) {
        flags = work->obj.drawFlags;

        if (!work->spriteFlipX) {
            flags &= ~SPRITE_FLAG_HFLIP;
        } else {
            flags |= SPRITE_FLAG_HFLIP;
        }

        x = GetEventCharaScreenX(work);
        y = (work->spriteY >> 8) + gEventCharaParams[work->arg.chara].spriteYOffset - (gEventState->y >> 8);
        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, flags, 50);
    }

    work->obj.z = save;
}

void event_chara_3(EventCharaWork* work) {
    TaskPoolDestroy(&work->tasks);

    if (work->usesBtlWork) {
        TaskPoolDestroy(&gBtlWork->taskPools[0]);
        TaskPoolDestroy(&gBtlWork->taskPools[1]);
        EwramFree(gBtlWork);
    }
}

u8 AdvanceEventCharaKeyframe(EventCharaWork* work) {
    const EventCharaKeyframe* keyframe = &work->keyframes[work->keyframe];
    u16 flags;

    if (work->keyframes[work->keyframe].frame > gEventState->frame) {
        return FALSE;
    }

    if ((work->keyframes[work->keyframe].flags & CHARA_KEYFRAME_FLAG_END) != 0) {
        return FALSE;
    }

    work->keyframe++;
    work->steps = work->keyframes[work->keyframe].frame - gEventState->frame;
    work->animId = work->keyframes[work->keyframe].anim;

    if ((work->keyframes[work->keyframe].flags & CHARA_KEYFRAME_FLAG_SPAWN) != 0) {
        CreateEvtObjTask(&work->tasks, &work->obj, work->arg.chara, work->keyframes[work->keyframe].anim,
                      work->keyframes[work->keyframe].x, work->keyframes[work->keyframe].y,
                      work->keyframes[work->keyframe].z);
        work->hasObj = TRUE;
        SetupEventCharaShadow(work);
    }

    if ((work->keyframes[work->keyframe].flags & CHARA_KEYFRAME_FLAG_DESPAWN) != 0) {
        work->finished = TRUE;
    }

    if ((work->keyframes[work->keyframe].flags & CHARA_KEYFRAME_FLAG_SILHOUETTE) != 0) {
        LoadPalette(gEventSilhouettePalette, (void*)(work->obj.paletteIndex * 32 + OBJ_PLTT), sizeof(gEventSilhouettePalette));
    } else if ((work->keyframes[work->keyframe - 1].flags & CHARA_KEYFRAME_FLAG_SILHOUETTE) != 0) {
        LoadPalette(gEvtObjResources[work->arg.chara].res.palette, (void*)(work->obj.paletteIndex * 32 + OBJ_PLTT), 32);
    }

    if ((work->keyframes[work->keyframe].flags & CHARA_KEYFRAME_FLAG_TRANSLUCENT) != 0) {
        gBldCnt = (BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3);
        flags = work->obj.drawFlags;
        EvtObjSetDrawFlags(&work->obj, flags | SPRITE_FLAG_BLEND);
    } else {
        flags = work->obj.drawFlags;
        EvtObjSetDrawFlags(&work->obj, work->obj.drawFlags & ~SPRITE_FLAG_BLEND);
        work->unk_1B8 = 0;
    }

    if (work->hasObj) {
        if ((work->keyframes[work->keyframe].flags & CHARA_KEYFRAME_MOTION_MASK) == CHARA_MOTION_SET_POSITION) {
            EvtObjSetPos(&work->obj, work->keyframes[work->keyframe].x, work->keyframes[work->keyframe].y, work->keyframes[work->keyframe].z);
        }

        EvtObjSetAnim(&work->obj, work->animId);
        ApplyEventCharaDrawFlags(work);
        return TRUE;
    }

    if (work->keyframes[work->keyframe].anim == 0x3AF) {
        BosPcStartEventAnim(gEventState->bossTask);
    }

    if (work->keyframes[work->keyframe].anim == 0x3AB) {
        BosLstAdvanceEventStep(gEventState->bossTask);
    }

    return FALSE;
}

void UpdateEventCharaMotion(EventCharaWork* work) {
    const EventCharaKeyframe* keyframe = &work->keyframes[work->keyframe];
    BtlObj* obj;

    if (keyframe->anim == 0x3A7) {
        obj = ListPoolFirst(&gBtlWork->pool);

        if (obj != NULL) {
            obj->flags |= BTLOBJ_FLAG_DAMAGE_PENDING;
        }
    }

    switch (keyframe->flags & CHARA_KEYFRAME_MOTION_MASK) {
    case CHARA_MOTION_SET_POSITION:
        EvtObjSetPos(&work->obj, work->keyframes[work->keyframe].x, work->keyframes[work->keyframe].y, work->keyframes[work->keyframe].z);
        break;
    case CHARA_MOTION_MOVE_TO:
        ApproachValue(&work->obj.x, work->keyframes[work->keyframe].x, work->steps);
        ApproachValue(&work->obj.y, work->keyframes[work->keyframe].y, work->steps);
        ApproachValue(&work->obj.z, work->keyframes[work->keyframe].z, work->steps);
        work->steps--;

        if (work->steps == 0) {
            if ((work->keyframes[work->keyframe].flags & CHARA_KEYFRAME_FLAG_KEEP_ANIM) == 0) {
                SetEventCharaEndAnim(work);
            }
        }

        break;
    case CHARA_MOTION_WALK:
        if ((keyframe->flags & CHARA_KEYFRAME_FLAG_FAST) == 0) {
            work->speed = gEventCharaParams[work->arg.chara].slowSpeed;
        } else {
            work->speed = gEventCharaParams[work->arg.chara].fastSpeed;
        }

        if (keyframe->motionArg == 1) {
            work->obj.z -= work->speed;
        } else if (keyframe->motionArg == 2) {
            work->obj.z += work->speed;
        } else {
            work->obj.x += (SIN(keyframe->motionArg) * work->speed) >> 8;
            work->obj.y += (-COS(keyframe->motionArg) * work->speed) >> 8;
        }

        break;
    }

    if ((keyframe->flags & CHARA_KEYFRAME_FLAG_BOB) != 0) {
        work->bobPhase += 4;
    } else if ((keyframe->flags & CHARA_KEYFRAME_FLAG_BOB_LARGE) != 0) {
        work->bobPhase += 4;
    } else {
        work->bobPhase = 0;
    }

    if ((keyframe->flags & CHARA_KEYFRAME_FLAG_BLINK) != 0) {
        work->visible ^= 1;
    } else {
        work->visible = 1;
    }
}

void SetEventCharaEndAnim(EventCharaWork* work) {
    const EventCharaKeyframe* keyframe = &work->keyframes[work->keyframe];

    EvtObjSetAnim(&work->obj, keyframe->motionArg);
}

u8 EventCharaHop(EventCharaWork* work, void* task) {
    work->hopVelocity = 0x800;
    work->unk_18C = 0;
    work->waitTimer = 0;
    work->unk_198 = work->obj.z;
    TaskPoolUpdate(&work->tasks);
    SetTaskUpdate(task, (TaskUpdateFunc)EventCharaHopUpdate);
    return 1;
}

u8 EventCharaHopUpdate(EventCharaWork* work, void* task) {
    u16 x;
    u16 y;
    u8 advanced;

    x = (work->obj.x >> 8) - (gEventState->x >> 8);
    y = (work->obj.y >> 8) + (work->obj.z >> 8) - (gEventState->y >> 8);
    advanced = AdvanceEventCharaKeyframe(work);
    UpdateEventCharaMotion(work);
    PlayEventCharaAnimSounds(work);

    if (work->waitTimer == 0) {
        work->obj.z -= work->hopVelocity / 4;
        work->hopVelocity -= work->unk_18C / 4;
        work->unk_18C += 51;
    } else {
        work->waitTimer--;

        if (work->waitTimer == 0) {
            if (work->arg.chara == EVENT_CHARA_MOOGLE) {
                m4aSongNumStart(SONG_SND_324);
                SetEventSoundPosition(SONG_SND_324, x, y);
            }
        }
    }

    if (work->obj.z > work->unk_198) {
        work->obj.z = work->unk_198;
        work->hopVelocity = 0x800;
        work->unk_18C = 0;
        work->waitTimer = 17;
    }

    if (advanced) {
        work->obj.z = work->unk_198;

        if (work->keyframes[work->keyframe].update != NULL) {
            SetTaskUpdate(task, work->keyframes[work->keyframe].update);
        } else {
            SetTaskUpdate(task, (TaskUpdateFunc)event_chara_1);
        }

        if (work->keyframes[work->keyframe].callback != NULL) {
            work->keyframes[work->keyframe].callback(work);
            work->callbackActive = TRUE;
        } else {
            work->callbackActive = FALSE;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 EventCharaHopHigh(EventCharaWork* work, void* task) {
    work->hopVelocity = 0xC00;
    work->unk_18C = 0;
    work->waitTimer = 0;
    work->unk_198 = work->obj.z;
    TaskPoolUpdate(&work->tasks);
    SetTaskUpdate(task, (TaskUpdateFunc)EventCharaHopHighUpdate);
    return 1;
}

u8 EventCharaHopHighUpdate(EventCharaWork* work, void* task) {
    u8 advanced;

    advanced = AdvanceEventCharaKeyframe(work);
    UpdateEventCharaMotion(work);
    PlayEventCharaAnimSounds(work);

    if (work->waitTimer == 0) {
        work->obj.z -= work->hopVelocity / 4;
        work->hopVelocity -= work->unk_18C / 4;
        work->unk_18C += 51;
    } else {
        work->waitTimer--;
    }

    if (work->obj.z > work->unk_198) {
        work->obj.z = work->unk_198;
        work->hopVelocity = 0;
        work->unk_18C = 0;
        work->waitTimer = 17;
    }

    if (advanced) {
        work->obj.z = work->unk_198;

        if (work->keyframes[work->keyframe].update != NULL) {
            SetTaskUpdate(task, work->keyframes[work->keyframe].update);
        } else {
            SetTaskUpdate(task, (TaskUpdateFunc)event_chara_1);
        }

        if (work->keyframes[work->keyframe].callback != NULL) {
            work->keyframes[work->keyframe].callback(work);
            work->callbackActive = TRUE;
        } else {
            work->callbackActive = FALSE;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 EventCharaHopLow(EventCharaWork* work, void* task) {
    work->hopVelocity = 0x300;
    work->unk_18C = 0;
    work->waitTimer = 0;
    work->unk_198 = work->obj.z;
    TaskPoolUpdate(&work->tasks);
    SetTaskUpdate(task, (TaskUpdateFunc)EventCharaHopLowUpdate);
    return 1;
}

u8 EventCharaHopLowUpdate(EventCharaWork* work, void* task) {
    u8 advanced;

    advanced = AdvanceEventCharaKeyframe(work);
    PlayEventCharaAnimSounds(work);
    UpdateEventCharaMotion(work);

    if (work->waitTimer == 0) {
        work->obj.z -= work->hopVelocity;
        work->hopVelocity -= work->unk_18C;
        work->unk_18C += 51;
    } else {
        work->waitTimer--;
    }

    if (work->obj.z > work->unk_198) {
        work->obj.z = work->unk_198;
        work->hopVelocity = 0x800;
        work->unk_18C = 0;
        work->waitTimer = 17;
    }

    if (advanced) {
        work->obj.z = work->unk_198;

        if (work->keyframes[work->keyframe].update != NULL) {
            SetTaskUpdate(task, work->keyframes[work->keyframe].update);
        } else {
            SetTaskUpdate(task, (TaskUpdateFunc)event_chara_1);
        }

        if (work->keyframes[work->keyframe].callback != NULL) {
            work->keyframes[work->keyframe].callback(work);
            work->callbackActive = TRUE;
        } else {
            work->callbackActive = FALSE;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 EventCharaDrop(EventCharaWork* work, void* task) {
    work->hopVelocity = 0x300;
    work->unk_18C = 0;
    work->waitTimer = 0;
    work->unk_198 = work->obj.z;
    TaskPoolUpdate(&work->tasks);
    SetTaskUpdate(task, (TaskUpdateFunc)EventCharaDropUpdate);
    return 1;
}

u8 EventCharaDropUpdate(EventCharaWork* work, void* task) {
    u8 advanced;

    advanced = AdvanceEventCharaKeyframe(work);
    UpdateEventCharaMotion(work);
    PlayEventCharaAnimSounds(work);

    if (work->waitTimer == 0) {
        work->obj.z -= work->hopVelocity;
        work->hopVelocity -= work->unk_18C;
        work->unk_18C += 51;
    } else {
        work->waitTimer--;
    }

    if (work->obj.z > 0) {
        work->obj.z = 0;
        work->waitTimer = 17;
    }

    if (advanced) {
        if (work->keyframes[work->keyframe].update != NULL) {
            SetTaskUpdate(task, work->keyframes[work->keyframe].update);
        } else {
            SetTaskUpdate(task, (TaskUpdateFunc)event_chara_1);
        }

        if (work->keyframes[work->keyframe].callback != NULL) {
            work->keyframes[work->keyframe].callback(work);
            work->callbackActive = TRUE;
        } else {
            work->callbackActive = FALSE;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 EventCharaFadeOut(void* work, void* task) {
    EventCharaWork* chara = work;
    const EventCharaKeyframe* keyframe;
    u16 z;

    UpdateEventCharaMotion(chara);
    PlayEventCharaAnimSounds(chara);

    if (AdvanceEventCharaKeyframe(chara)) {
        if (chara->keyframes[chara->keyframe].update != NULL) {
            SetTaskUpdate(task, chara->keyframes[chara->keyframe].update);
        }

        if (chara->keyframes[chara->keyframe].callback != NULL) {
            chara->keyframes[chara->keyframe].callback(chara);
            chara->callbackActive = TRUE;
        } else {
            chara->callbackActive = FALSE;
        }
    }

    z = chara->obj.drawFlags;
    z |= SPRITE_FLAG_BLEND;
    EvtObjSetDrawFlags(&chara->obj, z);
    gBldCnt = (BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3);
    gBldAlpha = 16;
    chara->effectTimer = 0;
    chara->effectLevel = 16;
    SetTaskUpdate(task, (TaskUpdateFunc)EventCharaFadeOutUpdate);

    if (chara->arg.chara == EVENT_CHARA_ROBED_FIGURE) {
        keyframe = &chara->keyframes[chara->keyframe];

        if ((keyframe->flags & CHARA_KEYFRAME_FLAG_BLINK) == 0) {
            m4aSongNumStart(SONG_EV_WARPIN);
        }
    }

    TaskPoolUpdate(&chara->tasks);
    return 1;
}

u8 EventCharaFadeOutUpdate(EventCharaWork* work, void* task) {
    UpdateEventCharaMotion(work);
    PlayEventCharaAnimSounds(work);
    work->effectTimer++;

    if (work->effectTimer % 2 == 0) {
        if (work->effectLevel != 0) {
            work->effectLevel--;
        }
    }

    gBldAlpha = ((16 - work->effectLevel) << 8) | work->effectLevel;

    if (AdvanceEventCharaKeyframe(work)) {
        gBldCnt = 0;

        if (work->keyframes[work->keyframe].update != NULL) {
            SetTaskUpdate(task, work->keyframes[work->keyframe].update);
        } else {
            SetTaskUpdate(task, (TaskUpdateFunc)event_chara_1);
        }

        if (work->keyframes[work->keyframe].callback != NULL) {
            work->keyframes[work->keyframe].callback(work);
            work->callbackActive = TRUE;
        } else {
            work->callbackActive = FALSE;
        }

        {
            u16 z = work->obj.drawFlags;

            z &= ~SPRITE_FLAG_BLEND;
            EvtObjSetDrawFlags(&work->obj, z);
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 EventCharaFadeIn(void* work, void* task) {
    EventCharaWork* chara = work;
    const EventCharaKeyframe* keyframe;
    u16 z;

    UpdateEventCharaMotion(chara);
    PlayEventCharaAnimSounds(chara);

    if (AdvanceEventCharaKeyframe(chara)) {
        if (chara->keyframes[chara->keyframe].update != NULL) {
            SetTaskUpdate(task, chara->keyframes[chara->keyframe].update);
        }

        if (chara->keyframes[chara->keyframe].callback != NULL) {
            chara->keyframes[chara->keyframe].callback(chara);
            chara->callbackActive = TRUE;
        } else {
            chara->callbackActive = FALSE;
        }
    }

    z = chara->obj.drawFlags;
    z |= SPRITE_FLAG_BLEND;
    EvtObjSetDrawFlags(&chara->obj, z);
    gBldCnt = (BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3);
    gBldAlpha = 0x1000;
    chara->effectTimer = 0;
    chara->effectLevel = 0;
    SetTaskUpdate(task, (TaskUpdateFunc)EventCharaFadeInUpdate);

    if (chara->arg.chara == EVENT_CHARA_ROBED_FIGURE) {
        keyframe = &chara->keyframes[chara->keyframe];

        if ((keyframe->flags & CHARA_KEYFRAME_FLAG_BLINK) == 0) {
            m4aSongNumStart(SONG_EV_WARPOUT);
        }
    }

    TaskPoolUpdate(&chara->tasks);
    return 1;
}

u8 EventCharaFadeInUpdate(EventCharaWork* work, void* task) {
    UpdateEventCharaMotion(work);
    PlayEventCharaAnimSounds(work);
    work->effectTimer++;

    if (work->effectTimer % 2 == 0) {
        if (work->effectLevel <= 15) {
            work->effectLevel++;
        }
    }

    gBldAlpha = ((16 - work->effectLevel) << 8) | work->effectLevel;

    if (AdvanceEventCharaKeyframe(work)) {
        gBldCnt = 0;

        if (work->keyframes[work->keyframe].update != NULL) {
            SetTaskUpdate(task, work->keyframes[work->keyframe].update);
        } else {
            SetTaskUpdate(task, (TaskUpdateFunc)event_chara_1);
        }

        if (work->keyframes[work->keyframe].callback != NULL) {
            work->keyframes[work->keyframe].callback(work);
            work->callbackActive = TRUE;
        } else {
            work->callbackActive = FALSE;
        }

        {
            u16 z = work->obj.drawFlags;

            z &= ~SPRITE_FLAG_BLEND;
            EvtObjSetDrawFlags(&work->obj, z);
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 EventCharaBlendUp(void* work, void* task) {
    EventCharaWork* chara = work;
    u16 z;

    UpdateEventCharaMotion(chara);
    PlayEventCharaAnimSounds(chara);

    if (AdvanceEventCharaKeyframe(chara)) {
        if (chara->keyframes[chara->keyframe].update != NULL) {
            SetTaskUpdate(task, chara->keyframes[chara->keyframe].update);
        }

        if (chara->keyframes[chara->keyframe].callback != NULL) {
            chara->keyframes[chara->keyframe].callback(chara);
            chara->callbackActive = TRUE;
        } else {
            chara->callbackActive = FALSE;
        }
    }

    z = chara->obj.drawFlags;
    z |= SPRITE_FLAG_BLEND;
    EvtObjSetDrawFlags(&chara->obj, z);
    gBldCnt = (BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3);
    gBldAlpha = 16;
    chara->effectTimer = 0;
    chara->effectLevel = 0;
    SetTaskUpdate(task, (TaskUpdateFunc)EventCharaBlendUpUpdate);
    TaskPoolUpdate(&chara->tasks);
    return 1;
}

u8 EventCharaBlendUpUpdate(EventCharaWork* work, void* task) {
    UpdateEventCharaMotion(work);
    PlayEventCharaAnimSounds(work);
    work->effectTimer++;

    if (work->effectTimer % 2 == 0) {
        if (work->effectLevel <= 15) {
            work->effectLevel++;
        }
    }

    gBldAlpha = (work->effectLevel << 8) | 16;

    if (AdvanceEventCharaKeyframe(work)) {
        gBldCnt = 0;

        if (work->keyframes[work->keyframe].update != NULL) {
            SetTaskUpdate(task, work->keyframes[work->keyframe].update);
        } else {
            SetTaskUpdate(task, (TaskUpdateFunc)event_chara_1);
        }

        if (work->keyframes[work->keyframe].callback != NULL) {
            work->keyframes[work->keyframe].callback(work);
            work->callbackActive = TRUE;
        } else {
            work->callbackActive = FALSE;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 EventCharaBlendDown(void* work, void* task) {
    EventCharaWork* chara = work;
    u16 z;

    UpdateEventCharaMotion(chara);
    PlayEventCharaAnimSounds(chara);

    if (AdvanceEventCharaKeyframe(chara)) {
        if (chara->keyframes[chara->keyframe].update != NULL) {
            SetTaskUpdate(task, chara->keyframes[chara->keyframe].update);
        }

        if (chara->keyframes[chara->keyframe].callback != NULL) {
            chara->keyframes[chara->keyframe].callback(chara);
            chara->callbackActive = TRUE;
        } else {
            chara->callbackActive = FALSE;
        }
    }

    z = chara->obj.drawFlags;
    z |= SPRITE_FLAG_BLEND;
    EvtObjSetDrawFlags(&chara->obj, z);
    gBldCnt = (BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3);
    gBldAlpha = 0x1010;
    chara->effectTimer = 0;
    chara->effectLevel = 16;
    SetTaskUpdate(task, (TaskUpdateFunc)EventCharaBlendDownUpdate);
    TaskPoolUpdate(&chara->tasks);
    return 1;
}

u8 EventCharaBlendDownUpdate(EventCharaWork* work, void* task) {
    UpdateEventCharaMotion(work);
    PlayEventCharaAnimSounds(work);
    work->effectTimer++;

    if (work->effectTimer % 2 == 0) {
        if (work->effectLevel != 0) {
            work->effectLevel--;
        }
    }

    gBldAlpha = (work->effectLevel << 8) | 16;

    if (AdvanceEventCharaKeyframe(work)) {
        gBldCnt = 0;

        if (work->keyframes[work->keyframe].update != NULL) {
            SetTaskUpdate(task, work->keyframes[work->keyframe].update);
        } else {
            SetTaskUpdate(task, (TaskUpdateFunc)event_chara_1);
        }

        if (work->keyframes[work->keyframe].callback != NULL) {
            work->keyframes[work->keyframe].callback(work);
            work->callbackActive = TRUE;
        } else {
            work->callbackActive = FALSE;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 EventCharaCircleSlow(EventCharaWork* work, void* task) {
    work->unk_18C = 0;
    work->unk_198 = 0;
    work->startX = work->obj.x;
    work->startZ = work->obj.z;
    SetTaskUpdate(task, (TaskUpdateFunc)EventCharaCircleSlowUpdate);
    return 1;
}

u8 EventCharaCircleSlowUpdate(EventCharaWork* work, void* task) {
    UpdateEventCharaMotion(work);
    PlayEventCharaAnimSounds(work);
    work->obj.x += gSineTable[(u8)work->unk_18C] * (work->unk_198 >> 8);
    work->obj.y += -gSineTable[(u8)work->unk_18C + 64] * (work->unk_198 >> 9);
    work->unk_18C += 2;

    if (work->unk_198 < 0x200) {
        work->unk_198 += 25;
    }

    if (AdvanceEventCharaKeyframe(work)) {
        if (work->keyframes[work->keyframe].update != NULL) {
            SetTaskUpdate(task, work->keyframes[work->keyframe].update);
        } else {
            SetTaskUpdate(task, (TaskUpdateFunc)event_chara_1);
        }

        if (work->keyframes[work->keyframe].callback != NULL) {
            work->keyframes[work->keyframe].callback(work);
            work->callbackActive = TRUE;
        } else {
            work->callbackActive = FALSE;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 EventCharaCircleFast(EventCharaWork* work, void* task) {
    work->unk_18C = 0;
    work->unk_198 = 0;
    work->startX = work->obj.x;
    work->startZ = work->obj.z;
    SetTaskUpdate(task, (TaskUpdateFunc)EventCharaCircleFastUpdate);
    return 1;
}

u8 EventCharaCircleFastUpdate(EventCharaWork* work, void* task) {
    UpdateEventCharaMotion(work);
    PlayEventCharaAnimSounds(work);
    work->obj.x += gSineTable[(u8)work->unk_18C] * (work->unk_198 >> 8);
    work->obj.y += -gSineTable[(u8)work->unk_18C + 64] * (work->unk_198 >> 9);
    work->unk_18C += 6;

    if (work->unk_198 < 0x200) {
        work->unk_198 += 25;
    }

    if (AdvanceEventCharaKeyframe(work)) {
        if (work->keyframes[work->keyframe].update != NULL) {
            SetTaskUpdate(task, work->keyframes[work->keyframe].update);
        } else {
            SetTaskUpdate(task, (TaskUpdateFunc)event_chara_1);
        }

        if (work->keyframes[work->keyframe].callback != NULL) {
            work->keyframes[work->keyframe].callback(work);
            work->callbackActive = TRUE;
        } else {
            work->callbackActive = FALSE;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 EventCharaJitter(EventCharaWork* work, void* task) {
    work->unk_18C = 1;
    work->unk_198 = 0;
    work->startX = work->obj.x;
    work->startZ = work->obj.z;
    SetTaskUpdate(task, (TaskUpdateFunc)EventCharaJitterUpdate);
    return 1;
}

u8 EventCharaJitterUpdate(EventCharaWork* work, void* task) {
    UpdateEventCharaMotion(work);
    PlayEventCharaAnimSounds(work);

    if (work->unk_198 == 2) {
        work->obj.z += work->unk_18C << 10;
        work->unk_18C = -work->unk_18C;
        work->unk_198 = 0;
    } else {
        work->unk_198++;
    }

    if (AdvanceEventCharaKeyframe(work)) {
        if (work->keyframes[work->keyframe].update != NULL) {
            SetTaskUpdate(task, work->keyframes[work->keyframe].update);
        } else {
            SetTaskUpdate(task, (TaskUpdateFunc)event_chara_1);
        }

        if (work->keyframes[work->keyframe].callback != NULL) {
            work->keyframes[work->keyframe].callback(work);
            work->callbackActive = TRUE;
        } else {
            work->callbackActive = FALSE;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void ApplyEventCharaDrawFlags(EventCharaWork* work) {
    u16 z;

    z = work->obj.drawFlags;

    if (work->keyframes[work->keyframe].flags & CHARA_KEYFRAME_FLAG_HFLIP) {
        if (work->keyframes[work->keyframe].flags & CHARA_KEYFRAME_FLAG_PRIORITY_1) {
            z |= SPRITE_PRIORITY(1) | SPRITE_FLAG_HFLIP;
            z &= ~SPRITE_PRIORITY(2);
            EvtObjSetDrawFlags(&work->obj, z);
        } else {
            z |= SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP;
            z &= ~SPRITE_PRIORITY(1);
            EvtObjSetDrawFlags(&work->obj, z);

            if ((work->keyframes[work->keyframe].flags & CHARA_KEYFRAME_FLAG_PRIORITY_0) == 0) {
                z |= SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP;
                EvtObjSetDrawFlags(&work->obj, z);
            } else {
                z |= SPRITE_FLAG_HFLIP;
                z &= ~SPRITE_PRIORITY(2);
                EvtObjSetDrawFlags(&work->obj, z);
            }
        }
    } else {
        if (work->keyframes[work->keyframe].flags & CHARA_KEYFRAME_FLAG_PRIORITY_1) {
            z |= SPRITE_PRIORITY(1);
            z &= ~SPRITE_PRIORITY(2);
            z &= ~SPRITE_FLAG_HFLIP;
            EvtObjSetDrawFlags(&work->obj, z);
        } else {
            z |= SPRITE_PRIORITY(2);
            z &= ~SPRITE_PRIORITY(1);
            z &= ~SPRITE_FLAG_HFLIP;
            EvtObjSetDrawFlags(&work->obj, z);

            if ((work->keyframes[work->keyframe].flags & CHARA_KEYFRAME_FLAG_PRIORITY_0) == 0) {
                z |= SPRITE_PRIORITY(2);
                z &= ~SPRITE_FLAG_HFLIP;
                EvtObjSetDrawFlags(&work->obj, z);
            } else {
                z &= ~SPRITE_PRIORITY(2);
                z &= ~SPRITE_FLAG_HFLIP;
                EvtObjSetDrawFlags(&work->obj, z);
            }
        }
    }
}

u8 EventCharaToggleAnim(EventCharaWork* work, void* task) {
    work->effectTimer = 0;
    work->effectLevel = 0;
    SetTaskUpdate(task, (TaskUpdateFunc)EventCharaToggleAnimUpdate);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 EventCharaToggleAnimUpdate(EventCharaWork* work, void* task) {
    u16 anims[2];

    memcpy(anims, sEventCharaToggleAnims, sizeof(anims));
    EvtObjSetAnim(&work->obj, anims[work->effectLevel]);
    work->effectTimer++;

    if (work->effectTimer == 12) {
        work->effectTimer = 0;
        work->effectLevel ^= 1;
    }

    if (AdvanceEventCharaKeyframe(work)) {
        if (work->tiles != NULL) {
            ReleaseObjTiles(work->tiles);
        }

        if (work->palette != NULL) {
            ReleaseObjPalette(work->palette);
        }

        work->tiles = NULL;
        work->palette = NULL;

        if (work->keyframes[work->keyframe].update != NULL) {
            SetTaskUpdate(task, work->keyframes[work->keyframe].update);
        } else {
            SetTaskUpdate(task, (TaskUpdateFunc)event_chara_1);
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 EventCharaFadeToBlack(void* work, void* task) {
    EventCharaWork* chara = work;
    u16 z;

    UpdateEventCharaMotion(chara);
    PlayEventCharaAnimSounds(chara);

    if (AdvanceEventCharaKeyframe(chara)) {
        if (chara->keyframes[chara->keyframe].update != NULL) {
            SetTaskUpdate(task, chara->keyframes[chara->keyframe].update);
        }

        if (chara->keyframes[chara->keyframe].callback != NULL) {
            chara->keyframes[chara->keyframe].callback(chara);
            chara->callbackActive = TRUE;
        } else {
            chara->callbackActive = FALSE;
        }
    }

    z = chara->obj.drawFlags;
    z |= SPRITE_FLAG_BLEND;
    EvtObjSetDrawFlags(&chara->obj, z);
    gBldCnt = (BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3);
    gBldAlpha = 16;
    chara->effectTimer = 0;
    chara->effectLevel = 16;
    SetTaskUpdate(task, (TaskUpdateFunc)EventCharaFadeToBlackUpdate);
    TaskPoolUpdate(&chara->tasks);
    return 1;
}

u8 EventCharaFadeToBlackUpdate(EventCharaWork* work, void* task) {
    UpdateEventCharaMotion(work);
    PlayEventCharaAnimSounds(work);
    work->effectTimer++;

    if (work->effectTimer % 2 == 0) {
        if (work->effectLevel != 0) {
            work->effectLevel--;
        }
    }

    gBldAlpha = work->effectLevel;

    if (AdvanceEventCharaKeyframe(work)) {
        gBldCnt = 0;

        if (work->keyframes[work->keyframe].update != NULL) {
            SetTaskUpdate(task, work->keyframes[work->keyframe].update);
        } else {
            SetTaskUpdate(task, (TaskUpdateFunc)event_chara_1);
        }

        if (work->keyframes[work->keyframe].callback != NULL) {
            work->keyframes[work->keyframe].callback(work);
            work->callbackActive = TRUE;
        } else {
            work->callbackActive = FALSE;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 EventCharaBlendDown2(void* work, void* task) {
    EventCharaWork* chara = work;
    u16 z;

    UpdateEventCharaMotion(chara);
    PlayEventCharaAnimSounds(chara);

    if (AdvanceEventCharaKeyframe(chara)) {
        if (chara->keyframes[chara->keyframe].update != NULL) {
            SetTaskUpdate(task, chara->keyframes[chara->keyframe].update);
        }

        if (chara->keyframes[chara->keyframe].callback != NULL) {
            chara->keyframes[chara->keyframe].callback(chara);
            chara->callbackActive = TRUE;
        } else {
            chara->callbackActive = FALSE;
        }
    }

    z = chara->obj.drawFlags;
    z |= SPRITE_FLAG_BLEND;
    EvtObjSetDrawFlags(&chara->obj, z);
    gBldCnt = (BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3);
    gBldAlpha = 0;
    chara->effectTimer = 0;
    chara->effectLevel = 16;
    SetTaskUpdate(task, (TaskUpdateFunc)EventCharaBlendDown2Update);
    TaskPoolUpdate(&chara->tasks);
    return 1;
}

u8 EventCharaBlendDown2Update(EventCharaWork* work, void* task) {
    UpdateEventCharaMotion(work);
    PlayEventCharaAnimSounds(work);
    work->effectTimer++;

    if (work->effectTimer % 2 == 0) {
        if (work->effectLevel != 0) {
            work->effectLevel--;
        }
    }

    gBldAlpha = (16 - work->effectLevel) | (work->effectLevel << 8);
    gBldAlpha = (work->effectLevel << 8) | 16;

    if (AdvanceEventCharaKeyframe(work)) {
        gBldCnt = 0;

        if (work->keyframes[work->keyframe].update != NULL) {
            SetTaskUpdate(task, work->keyframes[work->keyframe].update);
        } else {
            SetTaskUpdate(task, (TaskUpdateFunc)event_chara_1);
        }

        if (work->keyframes[work->keyframe].callback != NULL) {
            work->keyframes[work->keyframe].callback(work);
            work->callbackActive = TRUE;
        } else {
            work->callbackActive = FALSE;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void ReadEventCharaDpadAngle(EventCharaWork* work) {
    u16 keys = GetKeysHeld();

    switch (keys & DPAD_ANY) {
    case DPAD_UP:
        if (GetKeyReleaseTime(DPAD_LEFT) <= 4) {
            work->angle = FLD_ANGLE_UP_LEFT;
        } else if (GetKeyReleaseTime(DPAD_RIGHT) <= 4) {
            work->angle = FLD_ANGLE_UP_RIGHT;
        } else {
            work->angle = FLD_ANGLE_UP;
        }

        break;
    case DPAD_DOWN:
        if (GetKeyReleaseTime(DPAD_LEFT) <= 4) {
            work->angle = FLD_ANGLE_DOWN_LEFT;
        } else if (GetKeyReleaseTime(DPAD_RIGHT) <= 4) {
            work->angle = FLD_ANGLE_DOWN_RIGHT;
        } else {
            work->angle = FLD_ANGLE_DOWN;
        }

        break;
    case DPAD_LEFT:
        if (GetKeyReleaseTime(DPAD_UP) <= 4) {
            work->angle = FLD_ANGLE_UP_LEFT;
        } else if (GetKeyReleaseTime(DPAD_DOWN) <= 4) {
            work->angle = FLD_ANGLE_DOWN_LEFT;
        } else {
            work->angle = FLD_ANGLE_LEFT;
        }

        break;
    case DPAD_RIGHT:
        if (GetKeyReleaseTime(DPAD_UP) <= 4) {
            work->angle = FLD_ANGLE_UP_RIGHT;
        } else if (GetKeyReleaseTime(DPAD_DOWN) <= 4) {
            work->angle = FLD_ANGLE_DOWN_RIGHT;
        } else {
            work->angle = FLD_ANGLE_RIGHT;
        }

        break;
    case (DPAD_RIGHT | DPAD_UP):
        work->angle = FLD_ANGLE_UP_RIGHT;
        break;
    case (DPAD_LEFT | DPAD_UP):
        work->angle = FLD_ANGLE_UP_LEFT;
        break;
    case (DPAD_RIGHT | DPAD_DOWN):
        work->angle = FLD_ANGLE_DOWN_RIGHT;
        break;
    case (DPAD_LEFT | DPAD_DOWN):
        work->angle = FLD_ANGLE_DOWN_LEFT;
        break;
    }
}

void UpdateEventCharaAngle(EventCharaWork* work) {
    u8 old = work->angle;

    ReadEventCharaDpadAngle(work);

    if (old != work->angle) {
        if (abs((s8)GetAngleDiff(old, work->angle)) > 100) {
            work->speed = 0;
        } else {
            work->speed >>= 1;
        }
    }
}

void SetEventCharaMoveAnim(EventCharaWork* work, s32 animId) {
    u16 flags;

    flags = work->obj.drawFlags;

    switch (work->angle) {
    case FLD_ANGLE_UP_LEFT:
        flags &= ~SPRITE_FLAG_HFLIP;
        break;
    case FLD_ANGLE_UP_RIGHT:
    case FLD_ANGLE_RIGHT:
    case FLD_ANGLE_DOWN_RIGHT:
        flags |= SPRITE_FLAG_HFLIP;
        break;
    case FLD_ANGLE_UP:
    case FLD_ANGLE_DOWN:
    case FLD_ANGLE_DOWN_LEFT:
    case FLD_ANGLE_LEFT:
        flags &= ~SPRITE_FLAG_HFLIP;
        break;
    }

    if (animId != work->animId) {
        EvtObjSetAnim(&work->obj, animId);
        work->animId = animId;
    }

    EvtObjSetDrawFlags(&work->obj, flags);
}

enum EventCharaMoveMode {
    EVENT_CHARA_MOVE_MODE_WALK = 1,
    EVENT_CHARA_MOVE_MODE_RUN
};

enum EventCharaJumpPhase {
    EVENT_CHARA_JUMP_PHASE_START,
    EVENT_CHARA_JUMP_PHASE_RISE,
    EVENT_CHARA_JUMP_PHASE_APEX_RISE,
    EVENT_CHARA_JUMP_PHASE_APEX_FALL,
    EVENT_CHARA_JUMP_PHASE_FALL,
    EVENT_CHARA_JUMP_PHASE_LAND
};

u8 UpdateEventCharaControl(EventCharaWork* work, void* task) {
    u16 keys;
    s32 next;

    keys = GetKeysHeld();
    ReadEventCharaDpadAngle(work);

    switch (work->angle) {
    case FLD_ANGLE_UP:
        if ((keys & A_BUTTON) != 0) {
            work->moveMode = EVENT_CHARA_MOVE_MODE_WALK;
            SetEventCharaMoveAnim(work, 5);
        } else {
            work->moveMode = EVENT_CHARA_MOVE_MODE_RUN;
            SetEventCharaMoveAnim(work, 10);
        }

        break;
    case FLD_ANGLE_DOWN:
        if ((keys & A_BUTTON) != 0) {
            work->moveMode = EVENT_CHARA_MOVE_MODE_WALK;
            SetEventCharaMoveAnim(work, 6);
        } else {
            work->moveMode = EVENT_CHARA_MOVE_MODE_RUN;
            SetEventCharaMoveAnim(work, 11);
        }

        break;
    case FLD_ANGLE_LEFT:
        if ((keys & A_BUTTON) != 0) {
            work->moveMode = EVENT_CHARA_MOVE_MODE_WALK;
            SetEventCharaMoveAnim(work, 8);
        } else {
            work->moveMode = EVENT_CHARA_MOVE_MODE_RUN;
            SetEventCharaMoveAnim(work, 13);
        }

        break;
    case FLD_ANGLE_RIGHT:
        if ((keys & A_BUTTON) != 0) {
            work->moveMode = EVENT_CHARA_MOVE_MODE_WALK;
            SetEventCharaMoveAnim(work, 8);
        } else {
            work->moveMode = EVENT_CHARA_MOVE_MODE_RUN;
            SetEventCharaMoveAnim(work, 13);
        }

        break;
    case FLD_ANGLE_UP_LEFT:
        if ((keys & A_BUTTON) != 0) {
            work->moveMode = EVENT_CHARA_MOVE_MODE_WALK;
            SetEventCharaMoveAnim(work, 9);
        } else {
            work->moveMode = EVENT_CHARA_MOVE_MODE_RUN;
            SetEventCharaMoveAnim(work, 14);
        }

        break;
    case FLD_ANGLE_UP_RIGHT:
        if ((keys & A_BUTTON) != 0) {
            work->moveMode = EVENT_CHARA_MOVE_MODE_WALK;
            SetEventCharaMoveAnim(work, 9);
        } else {
            work->moveMode = EVENT_CHARA_MOVE_MODE_RUN;
            SetEventCharaMoveAnim(work, 14);
        }

        break;
    case FLD_ANGLE_DOWN_LEFT:
        if ((keys & A_BUTTON) != 0) {
            work->moveMode = EVENT_CHARA_MOVE_MODE_WALK;
            SetEventCharaMoveAnim(work, 7);
        } else {
            work->moveMode = EVENT_CHARA_MOVE_MODE_RUN;
            SetEventCharaMoveAnim(work, 12);
        }

        break;
    case FLD_ANGLE_DOWN_RIGHT:
        if ((keys & A_BUTTON) != 0) {
            work->moveMode = EVENT_CHARA_MOVE_MODE_WALK;
            SetEventCharaMoveAnim(work, 7);
        } else {
            work->moveMode = EVENT_CHARA_MOVE_MODE_RUN;
            SetEventCharaMoveAnim(work, 12);
        }

        break;
    }

    if ((keys & DPAD_ANY) != 0) {
        next = work->speed + 51;
        work->speed = next;

        switch (work->moveMode) {
        case EVENT_CHARA_MOVE_MODE_WALK:
            if (next > gEventCharaParams[work->arg.chara].slowSpeed) {
                work->speed = gEventCharaParams[work->arg.chara].slowSpeed;
            }

            break;
        case EVENT_CHARA_MOVE_MODE_RUN:
            if (next > gEventCharaParams[work->arg.chara].fastSpeed) {
                work->speed = gEventCharaParams[work->arg.chara].fastSpeed;
            }

            break;
        }
    } else {
        work->speed -= 102;

        if (work->speed < 0) {
            work->speed = 0;
        }

        switch (work->angle) {
        case FLD_ANGLE_UP:
            SetEventCharaMoveAnim(work, 0);
            break;
        case FLD_ANGLE_DOWN:
            SetEventCharaMoveAnim(work, 1);
            break;
        case FLD_ANGLE_LEFT:
            SetEventCharaMoveAnim(work, 3);
            break;
        case FLD_ANGLE_RIGHT:
            SetEventCharaMoveAnim(work, 3);
            break;
        case FLD_ANGLE_DOWN_LEFT:
            SetEventCharaMoveAnim(work, 2);
            break;
        case FLD_ANGLE_DOWN_RIGHT:
            SetEventCharaMoveAnim(work, 2);
            break;
        case FLD_ANGLE_UP_LEFT:
            SetEventCharaMoveAnim(work, 4);
            break;
        case FLD_ANGLE_UP_RIGHT:
            SetEventCharaMoveAnim(work, 4);
            break;
        }
    }

    if ((GetKeysPressed() & B_BUTTON) != 0) {
        work->jumpPhase = EVENT_CHARA_JUMP_PHASE_START;
        work->waitTimer = 0;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateEventCharaJump);
    }

    next = work->obj.x + (gSineTable[work->angle] * work->speed >> 8);
    work->obj.x = next;
    work->obj.y += work->speed * -gSineTable[work->angle + 64] >> 8;

    if (work->arg.chara == EVENT_CHARA_SORA) {
        if (gBtlWork != NULL) {
            gBtlWork->actor->x = next - 0x7800;
            gBtlWork->actor->y = work->obj.y - 0x5000;
            gBtlWork->actor->z = 0;
        }
    }

    work->lastAngle = work->angle;
    work->lastMoveMode = work->moveMode;
    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 UpdateEventCharaJump(EventCharaWork* work, void* task) {
    u16 keys = GetKeysHeld();

    if ((keys & DPAD_ANY) != 0) {
        work->speed += 5;
    }

    switch (work->jumpPhase) {
    case EVENT_CHARA_JUMP_PHASE_START:
        switch (work->angle) {
        case FLD_ANGLE_UP:
            SetEventCharaMoveAnim(work, 38);
            break;
        case FLD_ANGLE_DOWN:
            SetEventCharaMoveAnim(work, 44);
            break;
        case FLD_ANGLE_LEFT:
            SetEventCharaMoveAnim(work, 56);
            break;
        case FLD_ANGLE_RIGHT:
            SetEventCharaMoveAnim(work, 56);
            break;
        case FLD_ANGLE_DOWN_LEFT:
            SetEventCharaMoveAnim(work, 50);
            break;
        case FLD_ANGLE_DOWN_RIGHT:
            SetEventCharaMoveAnim(work, 50);
            break;
        case FLD_ANGLE_UP_LEFT:
            SetEventCharaMoveAnim(work, 62);
            break;
        case FLD_ANGLE_UP_RIGHT:
            SetEventCharaMoveAnim(work, 62);
            break;
        }

        work->obj.x += gSineTable[work->angle] * (work->speed >> 2) >> 8;
        work->obj.y += -gSineTable[work->angle + 64] * (work->speed >> 2) >> 8;

        if (work->waitTimer > 3) {
            work->jumpPhase = EVENT_CHARA_JUMP_PHASE_RISE;
            work->unk_18C = -0x540;
            work->unk_198 = work->obj.z;
            work->waitTimer = 0;
        } else {
            work->waitTimer++;
        }

        break;
    case EVENT_CHARA_JUMP_PHASE_RISE:
        UpdateEventCharaAngle(work);

        switch (work->angle) {
        case FLD_ANGLE_UP:
            SetEventCharaMoveAnim(work, 39);
            break;
        case FLD_ANGLE_DOWN:
            SetEventCharaMoveAnim(work, 45);
            break;
        case FLD_ANGLE_LEFT:
            SetEventCharaMoveAnim(work, 57);
            break;
        case FLD_ANGLE_RIGHT:
            SetEventCharaMoveAnim(work, 57);
            break;
        case FLD_ANGLE_DOWN_LEFT:
            SetEventCharaMoveAnim(work, 51);
            break;
        case FLD_ANGLE_DOWN_RIGHT:
            SetEventCharaMoveAnim(work, 51);
            break;
        case FLD_ANGLE_UP_LEFT:
            SetEventCharaMoveAnim(work, 63);
            break;
        case FLD_ANGLE_UP_RIGHT:
            SetEventCharaMoveAnim(work, 63);
            break;
        }

        work->obj.x += gSineTable[work->angle] * work->speed >> 8;
        work->obj.y += work->speed * -gSineTable[work->angle + 64] >> 8;
        work->unk_18C += 51;
        work->obj.z += work->unk_18C;

        if ((GetKeysHeld() & B_BUTTON) == 0) {
            work->unk_18C += 64;
        }

        if (work->unk_18C > -0x200) {
            work->jumpPhase = EVENT_CHARA_JUMP_PHASE_APEX_RISE;
            work->waitTimer = 0;
        }

        break;
    case EVENT_CHARA_JUMP_PHASE_APEX_RISE:
        UpdateEventCharaAngle(work);

        switch (work->angle) {
        case FLD_ANGLE_UP:
            SetEventCharaMoveAnim(work, 40);
            break;
        case FLD_ANGLE_DOWN:
            SetEventCharaMoveAnim(work, 46);
            break;
        case FLD_ANGLE_LEFT:
            SetEventCharaMoveAnim(work, 58);
            break;
        case FLD_ANGLE_RIGHT:
            SetEventCharaMoveAnim(work, 58);
            break;
        case FLD_ANGLE_DOWN_LEFT:
            SetEventCharaMoveAnim(work, 52);
            break;
        case FLD_ANGLE_DOWN_RIGHT:
            SetEventCharaMoveAnim(work, 52);
            break;
        case FLD_ANGLE_UP_LEFT:
            SetEventCharaMoveAnim(work, 64);
            break;
        case FLD_ANGLE_UP_RIGHT:
            SetEventCharaMoveAnim(work, 64);
            break;
        }

        work->obj.x += gSineTable[work->angle] * work->speed >> 8;
        work->obj.y += work->speed * -gSineTable[work->angle + 64] >> 8;
        work->unk_18C += 51;
        work->obj.z += work->unk_18C;

        if ((GetKeysHeld() & B_BUTTON) == 0) {
            work->unk_18C += 64;
        }

        if (work->unk_18C > 0) {
            work->jumpPhase = EVENT_CHARA_JUMP_PHASE_APEX_FALL;
            work->waitTimer = 0;
        }

        break;
    case EVENT_CHARA_JUMP_PHASE_APEX_FALL:
        UpdateEventCharaAngle(work);

        switch (work->angle) {
        case FLD_ANGLE_UP:
            SetEventCharaMoveAnim(work, 40);
            break;
        case FLD_ANGLE_DOWN:
            SetEventCharaMoveAnim(work, 46);
            break;
        case FLD_ANGLE_LEFT:
            SetEventCharaMoveAnim(work, 58);
            break;
        case FLD_ANGLE_RIGHT:
            SetEventCharaMoveAnim(work, 58);
            break;
        case FLD_ANGLE_DOWN_LEFT:
            SetEventCharaMoveAnim(work, 52);
            break;
        case FLD_ANGLE_DOWN_RIGHT:
            SetEventCharaMoveAnim(work, 52);
            break;
        case FLD_ANGLE_UP_LEFT:
            SetEventCharaMoveAnim(work, 64);
            break;
        case FLD_ANGLE_UP_RIGHT:
            SetEventCharaMoveAnim(work, 64);
            break;
        }

        work->obj.x += gSineTable[work->angle] * work->speed >> 8;
        work->obj.y += work->speed * -gSineTable[work->angle + 64] >> 8;
        work->obj.z += work->unk_18C;
        work->unk_18C += 51;

        if (work->unk_18C > 0x1FF) {
            work->jumpPhase = EVENT_CHARA_JUMP_PHASE_FALL;
            work->waitTimer = 0;
        }

        break;
    case EVENT_CHARA_JUMP_PHASE_FALL:
        UpdateEventCharaAngle(work);

        switch (work->angle) {
        case FLD_ANGLE_UP:
            SetEventCharaMoveAnim(work, 41);
            break;
        case FLD_ANGLE_DOWN:
            SetEventCharaMoveAnim(work, 47);
            break;
        case FLD_ANGLE_LEFT:
            SetEventCharaMoveAnim(work, 59);
            break;
        case FLD_ANGLE_RIGHT:
            SetEventCharaMoveAnim(work, 59);
            break;
        case FLD_ANGLE_DOWN_LEFT:
            SetEventCharaMoveAnim(work, 53);
            break;
        case FLD_ANGLE_DOWN_RIGHT:
            SetEventCharaMoveAnim(work, 53);
            break;
        case FLD_ANGLE_UP_LEFT:
            SetEventCharaMoveAnim(work, 65);
            break;
        case FLD_ANGLE_UP_RIGHT:
            SetEventCharaMoveAnim(work, 65);
            break;
        }

        work->obj.x += gSineTable[work->angle] * work->speed >> 8;
        work->obj.y += work->speed * -gSineTable[work->angle + 64] >> 8;
        work->obj.z += work->unk_18C;
        work->unk_18C += 51;

        if (work->obj.z > work->unk_198) {
            work->obj.z = work->unk_198;
            work->jumpPhase = EVENT_CHARA_JUMP_PHASE_LAND;
            work->waitTimer = 0;
        }

        break;
    case EVENT_CHARA_JUMP_PHASE_LAND:
        switch (work->angle) {
        case FLD_ANGLE_UP:
            SetEventCharaMoveAnim(work, 42);
            break;
        case FLD_ANGLE_DOWN:
            SetEventCharaMoveAnim(work, 48);
            break;
        case FLD_ANGLE_LEFT:
            SetEventCharaMoveAnim(work, 60);
            break;
        case FLD_ANGLE_RIGHT:
            SetEventCharaMoveAnim(work, 60);
            break;
        case FLD_ANGLE_DOWN_LEFT:
            SetEventCharaMoveAnim(work, 54);
            break;
        case FLD_ANGLE_DOWN_RIGHT:
            SetEventCharaMoveAnim(work, 54);
            break;
        case FLD_ANGLE_UP_LEFT:
            SetEventCharaMoveAnim(work, 66);
            break;
        case FLD_ANGLE_UP_RIGHT:
            SetEventCharaMoveAnim(work, 66);
            break;
        }

        work->speed = 204 * work->speed >> 8;

        if ((GetKeysPressed() & B_BUTTON) != 0) {
            work->jumpPhase = EVENT_CHARA_JUMP_PHASE_RISE;
            work->unk_18C = -0x540;
        } else if (work->waitTimer > 10) {
            work->lastAngle = 255;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateEventCharaControl);
        } else {
            work->waitTimer++;
        }

        break;
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

s32 PlayEventCharaAnimSounds(EventCharaWork* work) {
    u16 x;
    u16 y;

    x = (work->obj.x >> 8) - (gEventState->x >> 8);
    y = (work->obj.y >> 8) + (work->obj.z >> 8) - (gEventState->y >> 8);

    // @bug obj.anim is NULL for characters without an animation (NULL read), and a stale word when the debug menu
    // starts the same event again (0x40004000 seen in the events battery, 23/10).
    // BUS_PTR: the port reads what the GBA's bus gives there (its BIOS area, open bus).
    switch (work->keyframes[work->keyframe].anim) {
    case 0x2EB:
    case 0x2F1:
    case 0x2F2:
        if (work->arg.eventId != EVENT_158_RIKU_B11F_ENTRANCE) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
                if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                    m4aSongNumStart(SONG_SND_958);
                    SetEventSoundPosition(SONG_SND_958, x, y);
                }

                if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 6) {
                    m4aSongNumStart(SONG_SND_959);
                    SetEventSoundPosition(SONG_SND_959, x, y);
                }
            }
        }

        break;
    case 0x2E6:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_EV_MAN2_STONEL);
                SetEventSoundPosition(SONG_EV_MAN2_STONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 6) {
                m4aSongNumStart(SONG_EV_MAN2_STONER);
                SetEventSoundPosition(SONG_EV_MAN2_STONER, x, y);
            }
        }

        break;
    case 0x375:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 1) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 0) {
                m4aSongNumStart(SONG_EV_WOMAN_RSTONEL);
                SetEventSoundPosition(SONG_EV_WOMAN_RSTONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 4) {
                m4aSongNumStart(SONG_EV_WOMAN_RSTONER);
                SetEventSoundPosition(SONG_EV_WOMAN_RSTONER, x, y);
            }
        }

        break;
    case 0x398:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_SND_373);
                SetEventSoundPosition(SONG_SND_373, x, y);
            }
        }

        break;
    case 0x399:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_SND_374);
                SetEventSoundPosition(SONG_SND_374, x, y);
            }
        }

        break;
    case 0x5E:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 9) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 3) {
                m4aSongNumStart(SONG_EV_EV01_01);
            }
        }

        break;
    case 0x1C2:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                m4aSongNumStart(SONG_EV_MAN2_STONEL);
                SetEventSoundPosition(SONG_EV_MAN2_STONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 5) {
                m4aSongNumStart(SONG_EV_MAN2_STONER);
                SetEventSoundPosition(SONG_EV_MAN2_STONER, x, y);
            }
        }

        break;
    case 0x1C0:
    case 0x1C1:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                m4aSongNumStart(SONG_EV_MAN2_STONEL);
                SetEventSoundPosition(SONG_EV_MAN2_STONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 5) {
                m4aSongNumStart(SONG_EV_MAN2_STONER);
                SetEventSoundPosition(SONG_EV_MAN2_STONER, x, y);
            }
        }

        break;
    case 0x34B:
    case 0x34C:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 1) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 0) {
                m4aSongNumStart(SONG_EV_WOMAN_RSTONEL);
                SetEventSoundPosition(SONG_EV_WOMAN_RSTONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 4) {
                m4aSongNumStart(SONG_EV_WOMAN_RSTONER);
                SetEventSoundPosition(SONG_EV_WOMAN_RSTONER, x, y);
            }
        }

        break;
    case 0x2B2:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 1) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 0) {
                m4aSongNumStart(SONG_EV_WOMAN_RSTONEL);
                SetEventSoundPosition(SONG_EV_WOMAN_RSTONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 4) {
                m4aSongNumStart(SONG_EV_WOMAN_RSTONER);
                SetEventSoundPosition(SONG_EV_WOMAN_RSTONER, x, y);
            }
        }

        break;
    case 0x204:
    case 0x206:
    case 0x207:
    case 0x208:
    case 0x209:
    case 0x20A:
    case 0x20B:
    case 0x2C7:
    case 0x2C8:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 3) {
                if ((u16)(work->arg.eventId - EVENT_177_RIKU_B3F_E0) <= 1) {
                    m4aSongNumStart(SONG_EV_SR_DIRTL);
                    SetEventSoundPosition(SONG_EV_SR_DIRTL, x, y);
                } else {
                    m4aSongNumStart(SONG_EV_MAN_RMARBLEL);
                    SetEventSoundPosition(SONG_EV_MAN_RMARBLEL, x, y);
                }
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 7) {
                if ((u16)(work->arg.eventId - EVENT_177_RIKU_B3F_E0) <= 1) {
                    m4aSongNumStart(SONG_EV_SR_DIRTR);
                    SetEventSoundPosition(SONG_EV_SR_DIRTR, x, y);
                } else {
                    m4aSongNumStart(SONG_EV_MAN_RMARBLER);
                    SetEventSoundPosition(SONG_EV_MAN_RMARBLER, x, y);
                }
            }
        }

        break;
    case 0x2C5:
    case 0x2C6:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                if ((u16)(work->arg.eventId - EVENT_177_RIKU_B3F_E0) <= 1) {
                    m4aSongNumStart(SONG_EV_SR_DIRTL);
                    SetEventSoundPosition(SONG_EV_SR_DIRTL, x, y);
                } else {
                    m4aSongNumStart(SONG_EV_MAN_RMARBLEL);
                    SetEventSoundPosition(SONG_EV_MAN_RMARBLEL, x, y);
                }
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 5) {
                if ((u16)(work->arg.eventId - EVENT_177_RIKU_B3F_E0) <= 1) {
                    m4aSongNumStart(SONG_EV_SR_DIRTR);
                    SetEventSoundPosition(SONG_EV_SR_DIRTR, x, y);
                } else {
                    m4aSongNumStart(SONG_EV_MAN_RMARBLER);
                    SetEventSoundPosition(SONG_EV_MAN_RMARBLER, x, y);
                }
            }
        }

        break;
    case 0x371:
    case 0x372:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 3) {
                m4aSongNumStart(SONG_EV_WOMAN_STONEL);
                SetEventSoundPosition(SONG_EV_WOMAN_STONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 7) {
                m4aSongNumStart(SONG_EV_WOMAN_STONER);
                SetEventSoundPosition(SONG_EV_WOMAN_STONER, x, y);
            }
        }

        break;
    case 0x1EE:
    case 0x1F6:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                m4aSongNumStart(SONG_EV_MAN_STONEL);
                SetEventSoundPosition(SONG_EV_MAN_STONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 4) {
                m4aSongNumStart(SONG_EV_MAN_STONER);
                SetEventSoundPosition(SONG_EV_MAN_STONER, x, y);
            }
        }

        break;
    case 0x345:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 1) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 0) {
                m4aSongNumStart(SONG_SYS_POO_FOOTL);
                SetEventSoundPosition(SONG_SYS_POO_FOOTL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 4) {
                m4aSongNumStart(SONG_SYS_POO_FOOTR);
                SetEventSoundPosition(SONG_SYS_POO_FOOTR, x, y);
            }
        }

        break;
    case 0x344:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_SYS_POO_FOOTL);
                SetEventSoundPosition(SONG_SYS_POO_FOOTL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 5) {
                m4aSongNumStart(SONG_SYS_POO_FOOTR);
                SetEventSoundPosition(SONG_SYS_POO_FOOTR, x, y);
            }
        }

        break;
    case 0x341:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 3) {
                m4aSongNumStart(SONG_SYS_POO_FOOTL);
                SetEventSoundPosition(SONG_SYS_POO_FOOTL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 9) {
                m4aSongNumStart(SONG_SYS_POO_FOOTR);
                SetEventSoundPosition(SONG_SYS_POO_FOOTR, x, y);
            }
        }

        break;
    case 0x30C:
    case 0x30D:
    case 0x30E:
    case 0x30F:
    case 0x310:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 3) {
                m4aSongNumStart(SONG_SYS_POO_FOOTL);
                SetEventSoundPosition(SONG_SYS_POO_FOOTL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 9) {
                m4aSongNumStart(SONG_SYS_POO_FOOTR);
                SetEventSoundPosition(SONG_SYS_POO_FOOTR, x, y);
            }
        }

        break;
    case 0x275:
    case 0x276:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                m4aSongNumStart(SONG_EV_WOMAN_DIRT_L);
                SetEventSoundPosition(SONG_EV_WOMAN_DIRT_L, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 5) {
                m4aSongNumStart(SONG_EV_WOMAN_DIRT_R);
                SetEventSoundPosition(SONG_EV_WOMAN_DIRT_R, x, y);
            }
        }

        break;
    case 0x277:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 3) {
                m4aSongNumStart(SONG_EV_WOMAN_DIRT_L);
                SetEventSoundPosition(SONG_EV_WOMAN_DIRT_L, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 7) {
                m4aSongNumStart(SONG_EV_WOMAN_DIRT_R);
                SetEventSoundPosition(SONG_EV_WOMAN_DIRT_R, x, y);
            }
        }

        break;
    case 0x2A5:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 3) {
                m4aSongNumStart(SONG_EV_MAN2_DIRTL);
                SetEventSoundPosition(SONG_EV_MAN2_DIRTL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 7) {
                m4aSongNumStart(SONG_EV_MAN2_DIRTR);
                SetEventSoundPosition(SONG_EV_MAN2_DIRTR, x, y);
            }
        }

        break;
    case 0x2A8:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_EV_MAN_DIRTL);
                SetEventSoundPosition(SONG_EV_MAN_DIRTL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 6) {
                m4aSongNumStart(SONG_EV_MAN_DIRTR);
                SetEventSoundPosition(SONG_EV_MAN_DIRTR, x, y);
            }
        }

        break;
    case 0x271:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                m4aSongNumStart(SONG_EV_MAN_STONEL);
                SetEventSoundPosition(SONG_EV_MAN_STONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 5) {
                m4aSongNumStart(SONG_EV_MAN_STONER);
                SetEventSoundPosition(SONG_EV_MAN_STONER, x, y);
            }
        }

        break;
    case 0x241:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                m4aSongNumStart(SONG_EV_MAN_MARBLEL);
                SetEventSoundPosition(SONG_EV_MAN_MARBLEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 4) {
                m4aSongNumStart(SONG_EV_MAN_MARBLER);
                SetEventSoundPosition(SONG_EV_MAN_MARBLER, x, y);
            }
        }

        break;
    case 0x17A:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_EV_WOMAN_STONEL);
                SetEventSoundPosition(SONG_EV_WOMAN_STONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 5) {
                m4aSongNumStart(SONG_EV_WOMAN_STONER);
                SetEventSoundPosition(SONG_EV_WOMAN_STONER, x, y);
            }
        }

        break;
    case 0x178:
    case 0x179:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_EV_WOMAN_STONEL);
                SetEventSoundPosition(SONG_EV_WOMAN_STONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 6) {
                m4aSongNumStart(SONG_EV_WOMAN_STONER);
                SetEventSoundPosition(SONG_EV_WOMAN_STONER, x, y);
            }
        }

        break;
    case 0x19C:
    case 0x19D:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_EV_WOMAN_RSTONEL);
                SetEventSoundPosition(SONG_EV_WOMAN_RSTONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 6) {
                m4aSongNumStart(SONG_EV_WOMAN_RSTONER);
                SetEventSoundPosition(SONG_EV_WOMAN_RSTONER, x, y);
            }
        }

        break;
    case 0x14A:
    case 0x14B:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 0) {
                m4aSongNumStart(SONG_EV_WOMAN2_DIRTL);
                SetEventSoundPosition(SONG_EV_WOMAN2_DIRTL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 4) {
                m4aSongNumStart(SONG_EV_WOMAN2_DIRTR);
                SetEventSoundPosition(SONG_EV_WOMAN2_DIRTR, x, y);
            }
        }

        break;
    case 0x157:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 0) {
                m4aSongNumStart(SONG_EV_MAN_WOODL);
                SetEventSoundPosition(SONG_EV_MAN_WOODL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 4) {
                m4aSongNumStart(SONG_EV_MAN_WOODR);
                SetEventSoundPosition(SONG_EV_MAN_WOODR, x, y);
            }
        }

        break;
    case 0x23E:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                m4aSongNumStart(SONG_EV_MAN_RMARBLEL);
                SetEventSoundPosition(SONG_EV_MAN_RMARBLEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 5) {
                m4aSongNumStart(SONG_EV_MAN_RMARBLER);
                SetEventSoundPosition(SONG_EV_MAN_RMARBLER, x, y);
            }
        }

        break;
    case 0x16C:
    case 0x16E:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                m4aSongNumStart(SONG_EV_MAN_DIRTL);
                SetEventSoundPosition(SONG_EV_MAN_DIRTL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 5) {
                m4aSongNumStart(SONG_EV_MAN_DIRTR);
                SetEventSoundPosition(SONG_EV_MAN_DIRTR, x, y);
            }
        }

        break;
    case 0x122:
    case 0x123:
    case 0x128:
    case 0x129:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_EV_MAN_STONEL);
                SetEventSoundPosition(SONG_EV_MAN_STONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 6) {
                m4aSongNumStart(SONG_EV_MAN_STONER);
                SetEventSoundPosition(SONG_EV_MAN_STONER, x, y);
            }
        }

        break;
    case 0xDF:
    case 0xE0:
    case 0xE1:
    case 0xE2:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                if (work->arg.eventId == EVENT_097_WONDERLAND_E2) {
                    m4aSongNumStart(SONG_EV_WOMAN_DIRT_L);
                    SetEventSoundPosition(SONG_EV_WOMAN_DIRT_L, x, y);
                } else {
                    m4aSongNumStart(SONG_EV_MAN_RMARBLEL);
                    SetEventSoundPosition(SONG_EV_MAN_RMARBLEL, x, y);
                }
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 5) {
                if (work->arg.eventId == EVENT_097_WONDERLAND_E2) {
                    m4aSongNumStart(SONG_EV_WOMAN_DIRT_R);
                    SetEventSoundPosition(SONG_EV_WOMAN_DIRT_R, x, y);
                } else {
                    m4aSongNumStart(SONG_EV_MAN_RMARBLER);
                    SetEventSoundPosition(SONG_EV_MAN_RMARBLER, x, y);
                }
            }
        }

        break;
    case 0xD6:
    case 0xD9:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                m4aSongNumStart(SONG_EV_WOMAN_STONEL);
                SetEventSoundPosition(SONG_EV_WOMAN_STONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 5) {
                m4aSongNumStart(SONG_EV_WOMAN_STONER);
                SetEventSoundPosition(SONG_EV_WOMAN_STONER, x, y);
            }
        }

        break;
    case 0xCA:
    case 0xCB:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                m4aSongNumStart(SONG_SND_954);
                SetEventSoundPosition(SONG_SND_954, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 4) {
                m4aSongNumStart(SONG_SND_955);
                SetEventSoundPosition(SONG_SND_955, x, y);
            }
        }

        break;
    case 0xD3:
    case 0xD4:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                m4aSongNumStart(SONG_SND_956);
                SetEventSoundPosition(SONG_SND_956, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 4) {
                m4aSongNumStart(SONG_SND_957);
                SetEventSoundPosition(SONG_SND_957, x, y);
            }
        }

        break;
    case 0x10A:
    case 0x10B:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                m4aSongNumStart(SONG_SYS_POO_FOOTL);
                SetEventSoundPosition(SONG_SYS_POO_FOOTL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 4) {
                m4aSongNumStart(SONG_SYS_POO_FOOTR);
                SetEventSoundPosition(SONG_SYS_POO_FOOTR, x, y);
            }
        }

        break;
    case 0x83:
    case 0x88:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 3) {
                m4aSongNumStart(SONG_EV_EV01_03);
                SetEventSoundPosition(SONG_EV_EV01_03, x, y);
            }
        }

        break;
    case 0x12:
    case 0x13:
    case 0x66:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                m4aSongNumStart(SONG_EV_EV01_01);
                SetEventSoundPosition(SONG_EV_EV01_01, x, y);
            }
        }

        break;
    case 0x5:
    case 0x6:
    case 0x7:
    case 0x8:
    case 0x9:
        if (gEventBackgroundDefs[work->arg.eventId] != NULL) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
                if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                    PlaySoraFootstep(work, gEventBackgroundDefs[work->arg.eventId]->groundType, TRUE);
                }

                if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 5) {
                    PlaySoraFootstep(work, gEventBackgroundDefs[work->arg.eventId]->groundType, FALSE);
                }
            }
        }

        break;
    case 0x75:
    case 0x77:
    case 0x288:
    case 0x28C:
        if (gEventBackgroundDefs[work->arg.eventId] != NULL) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
                if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                    PlayDonaldFootstep(work, gEventBackgroundDefs[work->arg.eventId]->groundType, TRUE);
                }

                if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 6) {
                    PlayDonaldFootstep(work, gEventBackgroundDefs[work->arg.eventId]->groundType, FALSE);
                }
            }
        }

        break;
    case 0x78:
    case 0x79:
    case 0x289:
    case 0x28D:
        if (gEventBackgroundDefs[work->arg.eventId] != NULL) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
                if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 3) {
                    PlayDonaldFootstep(work, gEventBackgroundDefs[work->arg.eventId]->groundType, TRUE);
                }

                if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 7) {
                    PlayDonaldFootstep(work, gEventBackgroundDefs[work->arg.eventId]->groundType, FALSE);
                }
            }
        }

        break;
    case 0x95:
    case 0x97:
    case 0x98:
    case 0x99:
    case 0x265:
    case 0x266:
    case 0x267:
        if (gEventBackgroundDefs[work->arg.eventId] != NULL) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
                if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 3) {
                    PlayGoofyFootstep(work, gEventBackgroundDefs[work->arg.eventId]->groundType, TRUE);
                }

                if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 7) {
                    PlayGoofyFootstep(work, gEventBackgroundDefs[work->arg.eventId]->groundType, FALSE);
                }
            }
        }

        break;
    case 0xA:
    case 0xB:
    case 0xC:
    case 0xD:
    case 0xE:
        if (gEventBackgroundDefs[work->arg.eventId] != NULL) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
                if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 3) {
                    PlaySoraFootstep(work, gEventBackgroundDefs[work->arg.eventId]->groundType, TRUE);
                }

                if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 7) {
                    PlaySoraFootstep(work, gEventBackgroundDefs[work->arg.eventId]->groundType, FALSE);
                }
            }
        }

        break;
    case 0xB8:
    case 0xBA:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_EV_WOMAN_RSTONEL);
                SetEventSoundPosition(SONG_EV_WOMAN_RSTONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 6) {
                m4aSongNumStart(SONG_EV_WOMAN_RSTONER);
                SetEventSoundPosition(SONG_EV_WOMAN_RSTONER, x, y);
            }
        }

        break;
    case 0xBB:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 0) {
                m4aSongNumStart(SONG_EV_EV34_00);
                SetEventSoundPosition(SONG_EV_EV34_00, x, y);
            }
        }

        break;
    case 0xC1:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 0) {
                m4aSongNumStart(SONG_EV_CARDTHR);
                SetEventSoundPosition(SONG_EV_CARDTHR, x, y);
            }
        }

        break;
    case 0xE9:
    case 0xEA:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                m4aSongNumStart(SONG_SND_324);
                SetEventSoundPosition(SONG_SND_324, x, y);
            }
        }

        break;
    case 0xF2:
    case 0xF3:
    case 0xF4:
    case 0xF8:
    case 0xF9:
    case 0xFA:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                m4aSongNumStart(SONG_EV_PI_FOOTR);
                SetEventSoundPosition(SONG_EV_PI_FOOTR, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 5) {
                m4aSongNumStart(SONG_EV_PI_FOOTL);
                SetEventSoundPosition(SONG_EV_PI_FOOTL, x, y);
            }
        }

        break;
    case 0xF5:
    case 0xF6:
    case 0xF7:
    case 0xFB:
    case 0xFC:
    case 0xFD:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                m4aSongNumStart(SONG_EV_PI_FOOTL);
                SetEventSoundPosition(SONG_EV_PI_FOOTL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 4) {
                m4aSongNumStart(SONG_EV_PI_FOOTR);
                SetEventSoundPosition(SONG_EV_PI_FOOTR, x, y);
            }
        }

        break;
    case 0x280:
    case 0x281:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 7) {
                m4aSongNumStart(SONG_EV_WOMAN_STONEL);
                SetEventSoundPosition(SONG_EV_WOMAN_STONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_EV_WOMAN_STONER);
                SetEventSoundPosition(SONG_EV_WOMAN_STONER, x, y);
            }
        }

        break;
    case 0x18D:
    case 0x18E:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 6) {
                m4aSongNumStart(SONG_EV_MAN_STONEL);
                SetEventSoundPosition(SONG_EV_MAN_STONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_EV_MAN_STONER);
                SetEventSoundPosition(SONG_EV_MAN_STONER, x, y);
            }
        }

        break;
    case 0x18F:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 4) {
                m4aSongNumStart(SONG_BTL_GMIC_OK);
            }
        }

        break;
    case 0x159:
    case 0x15D:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 6) {
                m4aSongNumStart(SONG_EV_WOMAN_STONEL);
                SetEventSoundPosition(SONG_EV_WOMAN_STONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_EV_WOMAN_STONER);
                SetEventSoundPosition(SONG_EV_WOMAN_STONER, x, y);
            }
        }

        break;
    case 0x15E:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 0) {
                m4aSongNumStart(SONG_EV_CARDTHR);
                SetEventSoundPosition(SONG_EV_CARDTHR, x, y);
            }
        }

        break;
    case 0x185:
    case 0x186:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 6) {
                m4aSongNumStart(SONG_EV_MAN_STONEL);
                SetEventSoundPosition(SONG_EV_MAN_STONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_EV_MAN_STONER);
                SetEventSoundPosition(SONG_EV_MAN_STONER, x, y);
            }
        }

        break;
    case 0x187:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 7) {
                m4aSongNumStart(SONG_EV_MAN_STONEL);
                SetEventSoundPosition(SONG_EV_MAN_STONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 3) {
                m4aSongNumStart(SONG_EV_MAN_STONER);
                SetEventSoundPosition(SONG_EV_MAN_STONER, x, y);
            }
        }

        break;
    case 0x2E7:
    case 0x2E8:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 1) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 0) {
                m4aSongNumStart(SONG_EV_GE_ENTRY);
                SetEventSoundPosition(SONG_EV_GE_ENTRY, x, y);
            }
        }

        break;
    case 0x2AE:
    case 0x2AF:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 6) {
                m4aSongNumStart(SONG_EV_MAN2_STONEL);
                SetEventSoundPosition(SONG_EV_MAN2_STONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_EV_MAN2_STONER);
                SetEventSoundPosition(SONG_EV_MAN2_STONER, x, y);
            }
        }

        break;
    case 0x1C5:
    case 0x1C6:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 6) {
                m4aSongNumStart(SONG_EV_MAN2_STONEL);
                SetEventSoundPosition(SONG_EV_MAN2_STONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_EV_MAN2_STONER);
                SetEventSoundPosition(SONG_EV_MAN2_STONER, x, y);
            }
        }

        break;
    case 0x27A:
    case 0x27D:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 6) {
                m4aSongNumStart(SONG_EV_WOMAN_RSTONEL);
                SetEventSoundPosition(SONG_EV_WOMAN_RSTONEL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_EV_WOMAN_RSTONER);
                SetEventSoundPosition(SONG_EV_WOMAN_RSTONER, x, y);
            }
        }

        break;
    case 0x10F:
    case 0x11B:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 6) {
                m4aSongNumStart(SONG_SYS_POO_FOOTL);
                SetEventSoundPosition(SONG_SYS_POO_FOOTL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_SYS_POO_FOOTR);
                SetEventSoundPosition(SONG_SYS_POO_FOOTR, x, y);
            }
        }

        break;
    case 0x110:
    case 0x11C:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 5) {
                m4aSongNumStart(SONG_SYS_POO_FOOTL);
                SetEventSoundPosition(SONG_SYS_POO_FOOTL, x, y);
            }

            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 2) {
                m4aSongNumStart(SONG_SYS_POO_FOOTR);
                SetEventSoundPosition(SONG_SYS_POO_FOOTR, x, y);
            }
        }

        break;
    case 0x118:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 1) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 0) {
                m4aSongNumStart(SONG_EV_AL_LAND);
                SetEventSoundPosition(SONG_EV_AL_LAND, x, y);
            }
        }

        break;
    case 0x29E:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 3) {
                m4aSongNumStart(SONG_VO_GE_ATTACK02);
                SetEventSoundPosition(SONG_VO_GE_ATTACK02, x, y);
            }
        }

        break;
    case 0x2A2:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 0) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 1) {
                m4aSongNumStart(SONG_EV_GE_FOOTUP);
            }
        }

        break;
    case 0x1DE:
        if (BUS_PTR(struct AnimState*, work->obj.anim)->timer == 1) {
            if (BUS_PTR(struct AnimState*, work->obj.anim)->frame == 0) {
                m4aSongNumStart(SONG_BTL_DARKDEAD);
                SetEventSoundPosition(SONG_BTL_DARKDEAD, x, y);
            }
        }

        break;
    }
}

void PlaySoraFootstep(EventCharaWork* work, u8 kind, u8 flag) {
    u16 x;
    u16 y;

    x = (work->obj.x >> 8) - (gEventState->x >> 8);
    y = (work->obj.y >> 8) + (work->obj.z >> 8) - (gEventState->y >> 8);

    switch (kind) {
    case 0:
        if (flag) {
            m4aSongNumStart(SONG_EV_SR_DIRTL);
            SetEventSoundPosition(SONG_EV_SR_DIRTL, x, y);
        } else {
            m4aSongNumStart(SONG_EV_SR_DIRTR);
            SetEventSoundPosition(SONG_EV_SR_DIRTR, x, y);
        }

        break;
    case 1:
        if (!flag) {
            m4aSongNumStart(SONG_EV_SR_STONER);
            SetEventSoundPosition(SONG_EV_SR_STONER, x, y);
        } else {
            m4aSongNumStart(SONG_EV_SR_STONEL);
            SetEventSoundPosition(SONG_EV_SR_STONEL, x, y);
        }

        break;
    case 2:
        if ((work->arg.eventId == EVENT_075_MONSTORO_E1 && gEventState->frame > 0x2BC) || (work->arg.eventId == EVENT_054_12F_DESTINY_ISLAND_E1 && gEventState->frame <= 0x4F)) {
            if (flag) {
                m4aSongNumStart(SONG_EV_SR_STONEL);
                SetEventSoundPosition(SONG_EV_SR_STONEL, x, y);
            } else {
                m4aSongNumStart(SONG_EV_SR_STONER);
                SetEventSoundPosition(SONG_EV_SR_STONER, x, y);
            }

            if (flag) {
                m4aSongNumStart(SONG_EV_SR_STONEL);
                SetEventSoundPosition(SONG_EV_SR_STONEL, x, y);
            } else {
                m4aSongNumStart(SONG_EV_SR_STONER);
                SetEventSoundPosition(SONG_EV_SR_STONER, x, y);
            }
        } else {
            if (flag) {
                m4aSongNumStart(SONG_EV_SR_MUDL);
                SetEventSoundPosition(SONG_EV_SR_MUDL, x, y);
            } else {
                m4aSongNumStart(SONG_EV_SR_MUDR);
                SetEventSoundPosition(SONG_EV_SR_MUDR, x, y);
            }
        }

        break;
    case 3:
        if (flag) {
            m4aSongNumStart(SONG_EV_SR_STONEL);
            SetEventSoundPosition(SONG_EV_SR_STONEL, x, y);
        } else {
            m4aSongNumStart(SONG_EV_SR_STONER);
            SetEventSoundPosition(SONG_EV_SR_STONER, x, y);
        }

        break;
    }
}

void PlayDonaldFootstep(EventCharaWork* work, u8 kind, u8 flag) {
    u16 x;
    u16 y;

    x = (work->obj.x >> 8) - (gEventState->x >> 8);
    y = (work->obj.y >> 8) + (work->obj.z >> 8) - (gEventState->y >> 8);

    switch (kind) {
    case 0:
        if (flag) {
            m4aSongNumStart(SONG_EV_DL_DIRTL);
            SetEventSoundPosition(SONG_EV_DL_DIRTL, x, y);
        } else {
            m4aSongNumStart(SONG_EV_DL_DIRTR);
            SetEventSoundPosition(SONG_EV_DL_DIRTR, x, y);
        }

        break;
    case 1:
        if (!flag) {
            m4aSongNumStart(SONG_EV_DL_STONE_R);
            SetEventSoundPosition(SONG_EV_DL_STONE_R, x, y);
        } else {
            m4aSongNumStart(SONG_EV_DL_STONE_L);
            SetEventSoundPosition(SONG_EV_DL_STONE_L, x, y);
        }

        break;
    case 2:
        if (work->arg.eventId == EVENT_075_MONSTORO_E1 && gEventState->frame > 0x2BC) {
            if (!flag) {
                m4aSongNumStart(SONG_EV_DL_STONE_R);
                SetEventSoundPosition(SONG_EV_DL_STONE_R, x, y);
            } else {
                m4aSongNumStart(SONG_EV_DL_STONE_L);
                SetEventSoundPosition(SONG_EV_DL_STONE_L, x, y);
            }
        } else {
            if (flag) {
                m4aSongNumStart(SONG_EV_DL_MUDL);
                SetEventSoundPosition(SONG_EV_DL_MUDL, x, y);
            } else {
                m4aSongNumStart(SONG_EV_DL_MUDR);
                SetEventSoundPosition(SONG_EV_DL_MUDR, x, y);
            }
        }

        break;
    case 3:
        if (flag) {
            m4aSongNumStart(SONG_EV_DL_STONE_L);
            SetEventSoundPosition(SONG_EV_DL_STONE_L, x, y);
        } else {
            m4aSongNumStart(SONG_EV_DL_STONE_R);
            SetEventSoundPosition(SONG_EV_DL_STONE_R, x, y);
        }

        break;
    }
}

void PlayGoofyFootstep(EventCharaWork* work, u8 kind, u8 flag) {
    u16 x;
    u16 y;

    x = (work->obj.x >> 8) - (gEventState->x >> 8);
    y = (work->obj.y >> 8) + (work->obj.z >> 8) - (gEventState->y >> 8);

    switch (kind) {
    case 0:
        if (flag) {
            m4aSongNumStart(SONG_EV_GF_DIRTL);
            SetEventSoundPosition(SONG_EV_GF_DIRTL, x, y);
        } else {
            m4aSongNumStart(SONG_EV_GF_DIRTR);
            SetEventSoundPosition(SONG_EV_GF_DIRTR, x, y);
        }

        break;
    case 1:
        if (!flag) {
            m4aSongNumStart(SONG_EV_GF_STONE_R);
            SetEventSoundPosition(SONG_EV_GF_STONE_R, x, y);
        } else {
            m4aSongNumStart(SONG_EV_GF_STONE_L);
            SetEventSoundPosition(SONG_EV_GF_STONE_L, x, y);
        }

        break;
    case 2:
        if (work->arg.eventId == EVENT_075_MONSTORO_E1 && gEventState->frame > 0x2BC) {
            if (!flag) {
                m4aSongNumStart(SONG_EV_GF_STONE_R);
                SetEventSoundPosition(SONG_EV_GF_STONE_R, x, y);
            } else {
                m4aSongNumStart(SONG_EV_GF_STONE_L);
                SetEventSoundPosition(SONG_EV_GF_STONE_L, x, y);
            }
        } else {
            if (flag) {
                m4aSongNumStart(SONG_EV_GF_MUDL);
                SetEventSoundPosition(SONG_EV_GF_MUDL, x, y);
            } else {
                m4aSongNumStart(SONG_EV_GF_MUDR);
                SetEventSoundPosition(SONG_EV_GF_MUDR, x, y);
            }
        }

        break;
    case 3:
        if (flag) {
            m4aSongNumStart(SONG_EV_GF_STONE_L);
            SetEventSoundPosition(SONG_EV_GF_STONE_L, x, y);
        } else {
            m4aSongNumStart(SONG_EV_GF_STONE_R);
            SetEventSoundPosition(SONG_EV_GF_STONE_R, x, y);
        }

        break;
    }
}

void SetupEventCharaShadow(EventCharaWork* work) {
    switch (work->arg.chara) {
    case EVENT_CHARA_QUEEN_OF_HEARTS:
    case EVENT_CHARA_BEAST:
    case EVENT_CHARA_URSULA:
    case EVENT_CHARA_OOGIE_BOOGIE:
    case EVENT_CHARA_BELLE:
    case EVENT_CHARA_SIMBA:
        work->obj.flags |= EVTOBJ_FLAG_SHADOW_WIDE;
        break;
    case EVENT_CHARA_TINKER_BELL:
        CreateTinkerbellTask(work);
    case EVENT_CHARA_JIMINY_CRICKET:
    case EVENT_CHARA_MOOGLE:
    case EVENT_CHARA_MUSHU:
    case EVENT_CHARA_SEBASTIAN:
    case EVENT_CHARA_FLOUNDER:
    case EVENT_CHARA_WINNIE_THE_POOH:
    case EVENT_CHARA_TRAP:
    case EVENT_CHARA_PIGLET:
    case EVENT_CHARA_OWL:
    case EVENT_CHARA_ROO:
        work->obj.flags |= EVTOBJ_FLAG_SHADOW_SMALL;
        break;
    case EVENT_CHARA_SORA:
        break;
    }
}

static void msgwin_0(MsgWinWork* work, u8* arg) {
    const EventSequenceDef* seqDef;

    work->eventId = arg[0];

    switch (work->eventId) {
    case EVENT_011_1F_GOAL_2:
        work->glyphPaletteIndex = InitMsgGlyphSpritesAltPalette3(0);
        break;
    case EVENT_003_1F_TRAVERSE_TOWN_E0_2:
    case EVENT_134_100ACREWOOD_START:
    case EVENT_156_RIKU_B12F_GOAL_2:
        work->glyphPaletteIndex = InitMsgGlyphSpritesAltPalette5(0);
        break;
    default:
        work->glyphPaletteIndex = InitMsgGlyphSprites(0);
        break;
    }

    if (gEventBackgroundDefs[work->eventId] != NULL) {
        work->bg = 0;
    } else {
        work->bg = 2;
    }

    LoadBgTiles(work->bg, gMsgwinTiles, sizeof(gMsgwinTiles));
    LoadBgPalette(work->bg, gMsgwinPalette, sizeof(gMsgwinPalette));
    LoadBgMap(work->bg, gDefaultBgMap, sizeof(gDefaultBgMap));
    SetBgPriority(work->bg, 0);
    seqDef = gEventSequenceDefs[work->eventId];
    work->palette = NULL;
    work->steps = 0;
    work->shownChars = 0;
    work->charTimer = 0;
    work->charCount = 0;
    work->scriptIndex = 0;
    work->textLoaded = FALSE;
    work->started = FALSE;
    work->script = seqDef->script;
    work->waitCreated = FALSE;
    work->scrollX = 0;
    work->nextText = NULL;
    gEventState->msgWinOpen = FALSE;
    gEventState->msgWaitActive = FALSE;
    gEventState->msgWinCentered = FALSE;
    TaskPoolInit(&work->tasks, 2);
    CreateMsgfaceTask(&work->tasks, &work->face, work->script->portraitId, work->script->expressionId, work->script->positionIndex);
}

static u8 msgwin_1(MsgWinWork* work, void* task) {
    const MessageScriptEntry* entry;

    if (!work->textLoaded) {
        if (!gEventState->bgEffectActive) {
            MsgwinLoadEntry(work);
        }
    } else {
        MsgwinCheckStart(work);
    }

    if (work->started) {
        gEventState->msgWinPosition = work->position;

        if (gEventState->focusSpeaker) {
            if (gEventState->focusSteps != 0) {
                gEventState->focusSteps--;
            } else {
                entry = &work->script[work->scriptIndex];

                if (entry->portraitId == PORTRAIT_NONE) {
                    void* palette;

                    palette = (void*)(BG_PLTT + 15 * PLTT_SIZE_4BPP);
                    LoadBgTiles(work->bg, gSysMsgWinTiles, sizeof(gSysMsgWinTiles));
                    LoadBgMap(work->bg, gSysMsgWinMap, sizeof(gSysMsgWinMap));
                    LoadPalette(gCard00Palette, palette, sizeof(gCard00Palette));

                    if ((entry->flags & MSG_SCRIPT_FLAG_NO_FADE) != 0) {
                        FadeSetPaletteExcluded(15, TRUE);
                    }

                    gEventState->msgWinOpen = TRUE;

                    switch (entry->positionIndex) {
                    case MSG_WIN_POSITION_TOP_FACE_RIGHT:
                    case MSG_WIN_POSITION_TOP_FACE_LEFT:
                        SetBgScroll(work->bg, (u16)-0x28, 0);
                        break;
                    case MSG_WIN_POSITION_BOTTOM_FACE_RIGHT:
                    case MSG_WIN_POSITION_BOTTOM_FACE_LEFT:
                        SetBgScroll(work->bg, (u16)-0x28, (u16)-0x60);
                        break;
                    }

                    if (work->palette == NULL) {
                        work->palette = LoadObjPalette(gTextWhitePalette, sizeof(gTextWhitePalette));

                        if ((entry->flags & MSG_SCRIPT_FLAG_NO_FADE) != 0) {
                            FadeSetPaletteExcluded(work->palette->index + 16, TRUE);
                        }
                    }

                    RequestMsgfaceSlideIn(&work->face);
                    SetTaskUpdate(task, (TaskUpdateFunc)MsgwinOpenUpdate);
                    gEventState->msgWinCentered = TRUE;
                } else {
                    LoadBgTiles(work->bg, gMsgwinTiles, sizeof(gMsgwinTiles));
                    LoadBgPalette(work->bg, gMsgwinPalette, sizeof(gMsgwinPalette));
                    SetBgMapBlocks(work->bg, gMsgwinMapBlocks[work->position], 2, 1);
                    RedrawBgMapAt(work->bg, work->scrollX, 0);
                    SetTaskUpdate(task, (TaskUpdateFunc)MsgwinOpenUpdate);
                    RequestMsgfaceSlideIn(&work->face);
                    gEventState->msgWinOpen = TRUE;

                    if (work->palette != NULL) {
                        ReleaseObjPalette(work->palette);
                        work->palette = NULL;
                    }

                    gEventState->msgWinCentered = FALSE;
                }
            }
        } else {
        entry = &work->script[work->scriptIndex];

        if (entry->portraitId == PORTRAIT_NONE) {
            void* palette;

            palette = (void*)(BG_PLTT + 15 * PLTT_SIZE_4BPP);
            LoadBgTiles(work->bg, gSysMsgWinTiles, sizeof(gSysMsgWinTiles));
            LoadBgMap(work->bg, gSysMsgWinMap, sizeof(gSysMsgWinMap));
            LoadPalette(gCard00Palette, palette, sizeof(gCard00Palette));

            if ((entry->flags & MSG_SCRIPT_FLAG_NO_FADE) != 0) {
                FadeSetPaletteExcluded(15, TRUE);
            }

            gEventState->msgWinOpen = TRUE;

            switch (entry->positionIndex) {
            case MSG_WIN_POSITION_TOP_FACE_RIGHT:
            case MSG_WIN_POSITION_TOP_FACE_LEFT:
                SetBgScroll(work->bg, (u16)-0x18, 0);
                break;
            case MSG_WIN_POSITION_BOTTOM_FACE_RIGHT:
            case MSG_WIN_POSITION_BOTTOM_FACE_LEFT:
                SetBgScroll(work->bg, (u16)-0x18, (u16)-0x60);
                break;
            }

            if (work->palette == NULL) {
                work->palette = LoadObjPalette(gTextWhitePalette, sizeof(gTextWhitePalette));

                if ((entry->flags & MSG_SCRIPT_FLAG_NO_FADE) != 0) {
                FadeSetPaletteExcluded(work->palette->index + 16, TRUE);
                }
            }

            RequestMsgfaceSlideIn(&work->face);
            SetTaskUpdate(task, (TaskUpdateFunc)MsgwinOpenUpdate);
            gEventState->msgWinCentered = TRUE;
        } else {
            LoadBgTiles(work->bg, gMsgwinTiles, sizeof(gMsgwinTiles));
            LoadBgPalette(work->bg, gMsgwinPalette, sizeof(gMsgwinPalette));
            SetBgMapBlocks(work->bg, gMsgwinMapBlocks[work->position], 2, 1);
            RedrawBgMapAt(work->bg, work->scrollX, 0);
            SetTaskUpdate(task, (TaskUpdateFunc)MsgwinOpenUpdate);
            RequestMsgfaceSlideIn(&work->face);
            gEventState->msgWinOpen = TRUE;

            if (work->palette != NULL) {
                ReleaseObjPalette(work->palette);
                work->palette = NULL;
            }

            gEventState->msgWinCentered = FALSE;
        }
        }
    }

    if (gEventState->ending == TRUE) {
        FadeSetPaletteExcluded(work->glyphPaletteIndex + 16, FALSE);
        FadeSetPaletteExcluded(14, FALSE);
    } else {
        FadeSetPaletteExcluded(work->glyphPaletteIndex + 16, TRUE);
        FadeSetPaletteExcluded(14, TRUE);
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 MsgwinContinueUpdate(MsgWinWork* work, void* task) {
    if (!work->textLoaded) {
        if (!gEventState->bgEffectActive) {
            MsgwinLoadEntry(work);
        }
    } else {
        MsgwinCheckStart(work);
    }

    if (work->started) {
        gEventState->msgWinPosition = work->position;

        if (gEventState->focusSpeaker) {
            if (gEventState->focusSteps != 0) {
                gEventState->focusSteps--;
            } else {
                SetTaskUpdate(task, (TaskUpdateFunc)MsgwinOpenUpdate);
                RequestMsgfaceSlideIn(&work->face);
                gEventState->msgWinOpen = TRUE;
            }
        } else {
            SetTaskUpdate(task, (TaskUpdateFunc)MsgwinOpenUpdate);
            RequestMsgfaceSlideIn(&work->face);
            gEventState->msgWinOpen = TRUE;
        }
    }

    if (gEventState->ending == TRUE) {
        FadeSetPaletteExcluded(work->glyphPaletteIndex + 16, FALSE);
        FadeSetPaletteExcluded(14, FALSE);
    } else {
        FadeSetPaletteExcluded(work->glyphPaletteIndex + 16, TRUE);
        FadeSetPaletteExcluded(14, TRUE);
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

#ifdef PLATFORM_PC
#include "engine.h"

/* Message windows are drawn flush with one screen edge (the portrait side);
   in the wide view they dock to that edge of the larger picture. */
static int MsgwinDockShift(void) {
    if (gEventState->msgWinCentered)
        return 0;
    switch (gEventState->msgWinPosition) {
    case MSG_WIN_POSITION_TOP_FACE_RIGHT:
    case MSG_WIN_POSITION_BOTTOM_FACE_RIGHT:
        return PortWideMarginX();
    case MSG_WIN_POSITION_TOP_FACE_LEFT:
    case MSG_WIN_POSITION_BOTTOM_FACE_LEFT:
        return -PortWideMarginX();
    }
    return 0;
}
#endif

static void msgwin_2(MsgWinWork* work) {
    const MessageScriptEntry* entry = &work->script[work->scriptIndex];

#ifdef PLATFORM_PC
    /* Without a portrait the window is a centred box: nothing to dock. */
    gPortSpriteShiftX = entry->portraitId != PORTRAIT_NONE ? MsgwinDockShift() : 0;
    PortDockBg(work->bg, gPortSpriteShiftX);
#endif
    if (entry->portraitId != PORTRAIT_NONE) {
        DrawMsgGlyphs(work->shownChars);
    } else {
        DrawMsgGlyphsWithPalette(work->shownChars, work->palette);
    }

    TaskPoolDraw(&work->tasks);
#ifdef PLATFORM_PC
    gPortSpriteShiftX = 0;
#endif
}

static void msgwin_3(MsgWinWork* work) {
#ifdef PLATFORM_PC
    /* The layer keeps the parked window: still no world-map continuation. */
    PortDockBg(work->bg, 0);
#endif
    if (work->palette != NULL) {
        ReleaseObjPalette(work->palette);
    }

    FreeMsgGlyphSprites();
    TaskPoolDestroy(&work->tasks);
}

u8 MsgwinOpenUpdate(MsgWinWork* work, void* task) {
    const MessageScriptEntry* entry = &work->script[work->scriptIndex];

    ApproachValue(&work->scrollX, gMsgwinOpenScrollX[work->position], work->steps);

    if (entry->portraitId != PORTRAIT_NONE) {
        ScrollBgMapTo(work->bg, work->scrollX, 0);
    }

    if (work->steps != 0) {
        work->steps--;
    } else {
        work->steps = 0;

        if ((entry->flags & 0xF) == 0) {
            gEventState->talking = TRUE;
        }

        SetTaskUpdate(task, (TaskUpdateFunc)MsgwinTypeUpdate);
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 MsgwinTypeUpdate(MsgWinWork* work, void* task) {
    const MessageScriptEntry* entry = &work->script[work->scriptIndex];

    MsgwinTypeStep(work);

    if (entry->portraitId == PORTRAIT_NONE) {
        work->shownChars = work->charCount;
    } else if (GetKeysPressed() & A_BUTTON) {
        if (work->shownChars < work->charCount) {
            work->shownChars = work->charCount;
        }
    }

    if (work->waitCreated == TRUE && !gEventState->msgWaitActive) {
        MsgLatinChar* text = work->nextText;

        if (text != NULL) {
            work->steps = 0;
            work->textLoaded = FALSE;
            work->started = TRUE;
            work->face.shown = TRUE;
            SetTaskUpdate(task, (TaskUpdateFunc)MsgwinContinueUpdate);
        } else {
            HideMsgGlyphs();

            if ((entry->flags & MSG_SCRIPT_FLAG_END) == 0) {
                if (work->script[work->scriptIndex + 1].positionIndex != MSG_WIN_POSITION_KEEP) {
                    work->steps = 8;
                    RequestMsgfaceSlideOut(&work->face);
                    SetTaskUpdate(task, (TaskUpdateFunc)MsgwinCloseUpdate);
                    gEventState->msgWinOpen = FALSE;
                    work->face.shown = FALSE;
                } else {
                    work->steps = 0;
                    work->textLoaded = FALSE;
                    work->started = FALSE;
                    work->scriptIndex++;
                    gEventState->flags &= ~EVENT_FLAG_PAUSED;
                    work->face.shown = TRUE;
                    SetTaskUpdate(task, (TaskUpdateFunc)msgwin_1);
                }
            } else {
                work->steps = 8;
                RequestMsgfaceSlideOut(&work->face);
                SetTaskUpdate(task, (TaskUpdateFunc)MsgwinCloseUpdate);
                gEventState->msgWinOpen = FALSE;
                work->face.shown = FALSE;
            }
        }

        work->waitCreated = FALSE;
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 MsgwinCloseUpdate(MsgWinWork* work, void* task) {
    const MessageScriptEntry* entry = &work->script[work->scriptIndex];

    ApproachValue(&work->scrollX, gMsgwinClosedScrollX[work->position], work->steps);

    if (entry->portraitId != PORTRAIT_NONE) {
        ScrollBgMapTo(work->bg, work->scrollX, 0);
    } else {
        DisableBg(work->bg);
    }

    if (work->steps != 0) {
        work->steps--;
    } else {
        work->steps = 0;

        if ((gEventState->flags & EVENT_FLAG_PLAYER_CONTROL) == 0) {
            gEventState->flags &= ~EVENT_FLAG_PAUSED;
        }

        gEventState->focusSpeaker = FALSE;

        if ((entry->flags & MSG_SCRIPT_FLAG_END) == 0) {
            work->started = FALSE;
            work->textLoaded = FALSE;
            work->scriptIndex++;
            SetTaskUpdate(task, (TaskUpdateFunc)msgwin_1);
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

#ifdef PLATFORM_PC
/* @bug: an entry with no text (EVENT_196 in EU and others) reads the BIOS for its language's string, then that value
   as the text; the port lays out an empty one instead */
static const u8 sMsgwinNoText[] = { MSG_CODE_END };
#define MSGWIN_TEXT(entry) ((entry)->text != NULL ? LANGSTR((entry)->text) : (void*)sMsgwinNoText)
#else
#define MSGWIN_TEXT(entry) LANGSTR((entry)->text)
#endif

void MsgwinLoadEntry(MsgWinWork* work) {
    const MessageScriptEntry* entry = &work->script[work->scriptIndex];
    s32 position;

    position = entry->positionIndex;

    if (position != MSG_WIN_POSITION_KEEP) {
        work->position = position;
        work->scrollX = gMsgwinClosedScrollX[position];
    }

    if ((entry->flags & MSG_SCRIPT_FLAG_SILHOUETTE) != 0) {
        work->face.silhouette = TRUE;
    } else {
        work->face.silhouette = FALSE;
    }

    SetMsgfacePortrait(&work->face, entry->portraitId, entry->expressionId, work->position);

#ifdef VERSION_JP
    if (entry->portraitId == PORTRAIT_NONE) {
        work->charCount = LayoutMsgGlyphsSjis(0x2E00, gMsgwinTextY[work->position], (u8*)MSGWIN_TEXT(entry));
    } else {
        work->charCount = LayoutMsgGlyphsSjis(gMsgwinTextX[work->position], gMsgwinTextY[work->position], (u8*)MSGWIN_TEXT(entry));
    }
#else
    // @bug In EU, EVENT_196_DAMI_JUUJIRO has no text (NULL read).
    if (entry->portraitId == PORTRAIT_NONE) {
        if (work->nextText != NULL) {
            work->charCount = LayoutMsgGlyphsPage(0x2E00, gMsgwinTextY[work->position] - 0x200, work->nextText, &work->nextText);
        } else {
            work->charCount = LayoutMsgGlyphsPage(0x2E00, gMsgwinTextY[work->position] - 0x200, MSGWIN_TEXT(entry), &work->nextText);
        }
    } else {
        if (work->nextText != NULL) {
            work->charCount = LayoutMsgGlyphsPage(gMsgwinTextX[work->position], gMsgwinTextY[work->position] - 0x200, work->nextText, &work->nextText);
        } else {
            work->charCount = LayoutMsgGlyphsPage(gMsgwinTextX[work->position], gMsgwinTextY[work->position] - 0x200, MSGWIN_TEXT(entry), &work->nextText);
        }
    }
#endif

    work->charTimer = 0;
    work->shownChars = 0;
    work->textLoaded = TRUE;
}

void MsgwinTypeStep(MsgWinWork* work) {
    const MessageScriptEntry* entry = &work->script[work->scriptIndex];
    u8 nextPosition;

#ifdef PLATFORM_PC
    if (PortFastText()) /* experimental: one character a frame */
        work->charTimer = entry->charDelay;
#endif

    if (work->charTimer >= entry->charDelay) {
        if (work->shownChars < work->charCount) {
            work->shownChars++;
            m4aSongNumStart(SONG_SYS_MESSAGE);
        } else {
            gEventState->talking = FALSE;

            if (!work->waitCreated) {
                if ((work->script[work->scriptIndex].flags & MSG_SCRIPT_FLAG_END) == 0) {
                    if ((work->script[work->scriptIndex].flags & MSG_SCRIPT_FLAG_YES_NO) == 0) {
                        TaskCreate(&work->tasks, &sTaskDescMsgwait, &work->script[work->scriptIndex + 1].positionIndex);
                    } else {
                        TaskCreate(&work->tasks, &gTaskDescMsgwaitYesno, &work->script[work->scriptIndex + 1].positionIndex);
                    }
                } else {
                    nextPosition = 0;

                    if ((work->script[work->scriptIndex].flags & MSG_SCRIPT_FLAG_YES_NO) == 0) {
                        TaskCreate(&work->tasks, &sTaskDescMsgwait, &nextPosition);
                    } else {
                        TaskCreate(&work->tasks, &gTaskDescMsgwaitYesno, &nextPosition);
                    }
                }

                work->waitCreated = TRUE;
            }
        }

        work->charTimer = 0;
    } else {
        work->charTimer++;
    }
}

void MsgwinCheckStart(MsgWinWork* work) {
    const MessageScriptEntry* entry = &work->script[work->scriptIndex];

    if (gEventState->frame >= entry->frame) {
        if (!work->started) {
            gEventState->flags |= EVENT_FLAG_PAUSED;
            work->started = TRUE;
            work->steps = 8;

            if ((entry->flags & MSG_SCRIPT_FLAG_FOCUS_SPEAKER) != 0) {
                gEventState->focusSpeaker = TRUE;
                gEventState->speaker = entry->portraitId;
                gEventState->focusSteps = 32;
            } else {
                gEventState->focusSpeaker = FALSE;
            }
        }
    }
}

enum MsgfaceCommand {
    MSGFACE_COMMAND_NONE,
    MSGFACE_COMMAND_SLIDE_IN,
    MSGFACE_COMMAND_SLIDE_OUT,
    MSGFACE_COMMAND_CHANGE_PORTRAIT,
    MSGFACE_COMMAND_FLIP
};

void msgface_0(MsgFaceWork* work, MsgFaceControl* ctl) {
    const MsgFaceAnim* anim;
    u32 position;

    work->tiles = AllocObjTiles(0x12C0, NULL);
    work->palette = AllocObjPalette(32);
    work->face = ctl;
    work->steps = 0;
    work->x = gMsgfaceHiddenX[position = work->face->positionIndex];
    work->y = gMsgfaceY[position];
    work->scaleX = Q_8_8(1);
    work->arrived = FALSE;
    work->talking = FALSE;
    work->visible = TRUE;

    if (work->face->portraitId != PORTRAIT_NONE) {
        anim = gMsgFaceAnims[work->face->portraitId];
    } else {
        anim = gMsgFaceAnims[0];
    }

    if (work->face->positionIndex <= MSG_WIN_POSITION_BOTTOM_FACE_RIGHT) {
        work->flipX = TRUE;
    } else if (work->face->positionIndex <= MSG_WIN_POSITION_BOTTOM_FACE_LEFT) {
        work->flipX = FALSE;
    }

    if (work->face->portraitId != PORTRAIT_NONE) {
        SetObjTileSource(work->tiles, anim[work->face->expressionId].tiles);
        UpdateAllocatedObjPalette(work->palette, anim[work->face->expressionId].palette);
        AnimInit(&work->anim, anim[work->face->expressionId].anims, anim[work->face->expressionId].gfxTable);
        AnimStart(&work->anim, 0, anim[work->face->expressionId].animFlags);
        work->gfx = AnimGetGfx(&work->anim);
    } else {
        SetObjTileSource(work->tiles, anim->tiles);
        UpdateAllocatedObjPalette(work->palette, anim->palette);
        AnimInit(&work->anim, anim->anims, anim->gfxTable);
        AnimStart(&work->anim, 0, anim->animFlags);
        work->gfx = AnimGetGfx(&work->anim);
    }
}

u8 msgface_1(MsgFaceWork* work, void* task) {
    const MsgFaceAnim* anim = NULL;
    u32 position;

    if (work->face->portraitId != PORTRAIT_NONE) {
        anim = gMsgFaceAnims[work->face->portraitId];
        work->visible = TRUE;
    } else {
        work->visible = FALSE;
    }

    switch (work->face->command) {
    case MSGFACE_COMMAND_SLIDE_IN:
        if (!work->face->shown) {
            work->x = gMsgfaceHiddenX[position = work->face->positionIndex];
            work->y = gMsgfaceY[position];
        }

        work->steps = 8;

        if (anim != NULL) {
            if (work->face->silhouette == TRUE) {
                UpdateAllocatedObjPalette(work->palette, gEventSilhouettePalette);
            } else {
                UpdateAllocatedObjPalette(work->palette, anim[work->face->expressionId].palette);
            }
        }

        SetTaskUpdate(task, (TaskUpdateFunc)MsgfaceSlideInUpdate);
        break;
    case MSGFACE_COMMAND_SLIDE_OUT:
        work->steps = 8;
        work->arrived = FALSE;
        SetTaskUpdate(task, (TaskUpdateFunc)MsgfaceSlideOutUpdate);
        break;
    case MSGFACE_COMMAND_FLIP:
        work->steps = 4;

        if (work->face->positionIndex <= MSG_WIN_POSITION_BOTTOM_FACE_RIGHT) {
            work->scaleX = -255;
        } else if (work->face->positionIndex <= MSG_WIN_POSITION_BOTTOM_FACE_LEFT) {
            work->scaleX = Q_8_8(1);
        }

        work->y = gMsgfaceY[work->face->positionIndex];
        SetTaskUpdate(task, (TaskUpdateFunc)MsgfaceFlipOutUpdate);
        break;
    case MSGFACE_COMMAND_CHANGE_PORTRAIT:
        SetTaskUpdate(task, (TaskUpdateFunc)MsgfaceChangeUpdate);
        break;
    }

    if (gEventState->talking == TRUE) {
        if (!work->talking) {
            if (anim != NULL && anim[work->face->expressionId].animCount > 1) {
                AnimStart(&work->anim, 1, anim[work->face->expressionId].animFlags);
            }

            work->talking = TRUE;
        }
    } else {
        if (work->talking == TRUE) {
            if (anim != NULL) {
                AnimStart(&work->anim, 0, anim[work->face->expressionId].animFlags);
            }

            work->talking = FALSE;
        }
    }

    if (gEventState->ending == TRUE) {
        FadeSetPaletteExcluded(work->palette->index + 16, FALSE);
    } else {
        FadeSetPaletteExcluded(work->palette->index + 16, TRUE);
    }

    work->gfx = AnimUpdate(&work->anim);
    return 1;
}

void msgface_2(MsgFaceWork* work) {
    ObjAffine* affine;
    u8 flipX;

#ifdef PLATFORM_PC
    int clipPortrait = getenv("KHCOM_PORTRAIT_UNCLIPPED") == NULL;
    /* Hidden positions are parking coordinates, not another part of the scene.
     * Their initial palette may deliberately precede the silhouette command. */
    if (clipPortrait && work->face->positionIndex < 4 &&
        work->x == gMsgfaceHiddenX[work->face->positionIndex]) return;
    if (clipPortrait)
        PortSpriteClipX(gPortSpriteShiftX, gPortSpriteShiftX + 240);
#endif

    if (work->visible) {
        affine = AllocObjAffine(0, work->scaleX, Q_8_8(1), FALSE);

        if (affine != NULL) {
            DrawSprite(work->x >> 8, work->y >> 8, work->gfx, work->tiles, work->palette, affine, 0, 50);
        } else {
            flipX = work->flipX;

            if (flipX) {
                DrawSprite(work->x >> 8, work->y >> 8, work->gfx, work->tiles, work->palette, affine, SPRITE_FLAG_HFLIP, 50);
            } else {
                DrawSprite(work->x >> 8, work->y >> 8, work->gfx, work->tiles, work->palette, NULL, flipX, 50);
            }
        }
    }
#ifdef PLATFORM_PC
    if (clipPortrait) PortSpriteClipX(0, 0);
#endif
}

void msgface_3(MsgFaceWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

u8 MsgfaceSlideInUpdate(MsgFaceWork* work, void* task) {
    ApproachValue(&work->x, gMsgfaceShownX[work->face->positionIndex], work->steps);
    work->steps--;

    if (work->steps == 0) {
        work->face->command = MSGFACE_COMMAND_NONE;
        work->arrived = TRUE;
        SetTaskUpdate(task, (TaskUpdateFunc)msgface_1);
    }

    return 1;
}

u8 MsgfaceSlideOutUpdate(MsgFaceWork* work, void* task) {
    ApproachValue(&work->x, gMsgfaceHiddenX[work->face->positionIndex], work->steps);
    work->steps--;

    if (work->steps == 0) {
        work->face->command = MSGFACE_COMMAND_NONE;
        SetTaskUpdate(task, (TaskUpdateFunc)msgface_1);
    }

    return 1;
}

u8 MsgfaceChangeUpdate(MsgFaceWork* work, void* task) {
    const MsgFaceAnim* anims;
    s32 position;

    anims = NULL;

    if (work->face->portraitId != PORTRAIT_NONE) {
        anims = gMsgFaceAnims[work->face->portraitId];
        work->visible = TRUE;
    } else {
        work->visible = FALSE;
    }

    if (work->face->positionIndex <= MSG_WIN_POSITION_BOTTOM_FACE_RIGHT) {
        work->flipX = TRUE;
    } else if (work->face->positionIndex <= MSG_WIN_POSITION_BOTTOM_FACE_LEFT) {
        work->flipX = FALSE;
    }

    if (anims != NULL) {
        SetObjTileSource(work->tiles, anims[work->face->expressionId].tiles);
        UpdateAllocatedObjPalette(work->palette, anims[work->face->expressionId].palette);
        AnimInit(&work->anim, anims[work->face->expressionId].anims, anims[work->face->expressionId].gfxTable);
        AnimStart(&work->anim, 0, anims[work->face->expressionId].animFlags);
        work->gfx = AnimGetGfx(&work->anim);
        work->arrived = FALSE;
        work->steps = 8;
        work->face->command = MSGFACE_COMMAND_NONE;
    }

    work->x = gMsgfaceHiddenX[position = work->face->positionIndex];
    work->y = gMsgfaceY[position];
    work->scaleX = Q_8_8(1);
    work->steps = 8;
    SetTaskUpdate(task, (TaskUpdateFunc)msgface_1);
    return 1;
}

u8 MsgfaceFlipOutUpdate(MsgFaceWork* work, void* task) {
    const MsgFaceAnim* anims;

    if (work->scaleX < 0) {
        ApproachValue(&work->scaleX, -2, work->steps);
    } else {
        ApproachValue(&work->scaleX, 2, work->steps);
    }

    work->steps--;

    if (work->steps == 0) {
        anims = NULL;

        if (work->face->portraitId != PORTRAIT_NONE) {
            anims = gMsgFaceAnims[work->face->portraitId];
        }

        if (work->face->positionIndex <= MSG_WIN_POSITION_BOTTOM_FACE_RIGHT) {
            work->flipX = TRUE;
        } else if (work->face->positionIndex <= MSG_WIN_POSITION_BOTTOM_FACE_LEFT) {
            work->flipX = FALSE;
        }

        if (anims != NULL) {
            SetObjTileSource(work->tiles, anims[work->face->expressionId].tiles);
            UpdateAllocatedObjPalette(work->palette, anims[work->face->expressionId].palette);
            AnimInit(&work->anim, anims[work->face->expressionId].anims, anims[work->face->expressionId].gfxTable);
            AnimStart(&work->anim, 0, anims[work->face->expressionId].animFlags);
            work->gfx = AnimGetGfx(&work->anim);
            work->arrived = FALSE;
            work->steps = 8;
        }

        SetTaskUpdate(task, (TaskUpdateFunc)MsgfaceFlipInUpdate);
    }

    return 1;
}

u8 MsgfaceFlipInUpdate(MsgFaceWork* work, void* task) {
    if (work->scaleX < 0) {
        ApproachValue(&work->scaleX, -255, work->steps);
    } else {
        ApproachValue(&work->scaleX, Q_8_8(1), work->steps);
    }

    work->steps--;

    if (work->steps == 0) {
        work->arrived = TRUE;
        work->face->command = MSGFACE_COMMAND_NONE;
        work->scaleX = Q_8_8(1);
        SetTaskUpdate(task, (TaskUpdateFunc)msgface_1);
    }

    return 1;
}

void CreateMsgfaceTask(void* pool, MsgFaceControl* ctl, u8 portraitId, u8 expressionId, u8 positionIndex) {
    ctl->portraitId = portraitId;
    ctl->expressionId = expressionId;
    ctl->positionIndex = positionIndex;
    ctl->command = MSGFACE_COMMAND_NONE;
    ctl->silhouette = FALSE;
    TaskCreate(pool, &sTaskDescMsgface, ctl);
}

void SetMsgfacePortrait(MsgFaceControl* ctl, u8 portraitId, u8 expressionId, u8 positionIndex) {
    u8 command;

    if (ctl->portraitId != portraitId) {
        command = MSGFACE_COMMAND_CHANGE_PORTRAIT;
    } else {
        if (ctl->expressionId == expressionId && ctl->positionIndex == positionIndex) {
            return;
        }

        command = MSGFACE_COMMAND_FLIP;
    }

    ctl->command = command;
    ctl->portraitId = portraitId;
    ctl->expressionId = expressionId;
    ctl->positionIndex = positionIndex;
}

void RequestMsgfaceSlideIn(MsgFaceControl* ctl) {
    ctl->command = MSGFACE_COMMAND_SLIDE_IN;
}

void RequestMsgfaceSlideOut(MsgFaceControl* ctl) {
    ctl->command = MSGFACE_COMMAND_SLIDE_OUT;
}

void msgwait_0(MsgWaitWork* work, u8* arg) {
    work->nextPosition = arg[0];
    work->tiles = AllocObjTiles(64, NULL);
    work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    LoadObjPaletteBank(work->palette->index, gBStatesPalette);
    FadeSetPaletteExcluded(work->palette->index + 16, TRUE);
    SetObjTileSource(work->tiles, gFEventTiles);
    AnimInit(&work->anim, gFEventAnims, gFEventFrames);
    AnimStart(&work->anim, 2, ANIM_FLAG_LOOP);
    work->timer = 0;
    gEventState->msgWaitActive = TRUE;
}

u8 msgwait_1(MsgWaitWork* work, void* task) {
    work->gfx = AnimUpdate(&work->anim);

    if (GetKeysPressed() & A_BUTTON) {
        AnimStart(&work->anim, 3, ANIM_FLAG_LOOP);

        if (work->nextPosition == 4) {
            gEventState->msgWaitActive = FALSE;
            m4aSongNumStart(SONG_SYS_KETTEI);
            return 0;
        } else {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateMsgwaitClosing);
            m4aSongNumStart(SONG_SYS_KETTEI);
        }
    }

    return 1;
}

u8 UpdateMsgwaitClosing(MsgWaitWork* work) {
    u8 active;

    work->gfx = AnimUpdate(&work->anim);
    work->timer++;

    if (work->timer <= 15) {
        active = 1;
    } else {
        gEventState->msgWaitActive = FALSE;
        active = 0;
    }

    return active;
}

void msgwait_2(MsgWaitWork* work) {
    u8 centered = gEventState->msgWinCentered;

    if (centered) {
        DrawSprite(120, gMsgwaitIconPos[gEventState->msgWinPosition][1] >> 8, work->gfx,
                   work->tiles, work->palette, NULL, 0, 0);
    } else {
        DrawSprite(gMsgwaitIconPos[gEventState->msgWinPosition][0] >> 8,
                   gMsgwaitIconPos[gEventState->msgWinPosition][1] >> 8, work->gfx, work->tiles,
                   work->palette, NULL, 0, 0);
    }
}

void msgwait_3(MsgWaitWork* work) {
    FadeSetPaletteExcluded(work->palette->index + 16, FALSE);
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void msgwait_yesno_0(MsgWaitWork* work, u8* arg) {
    work->nextPosition = *arg;
    work->tiles = AllocObjTiles(64, NULL);
    work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    LoadObjPaletteBank(work->palette->index, gBStatesPalette);
    FadeSetPaletteExcluded(work->palette->index + 16, TRUE);
    SetObjTileSource(work->tiles, gFEventTiles);
    AnimInit(&work->anim, gFEventAnims, gFEventFrames);
    AnimStart(&work->anim, 2, ANIM_FLAG_LOOP);
    work->timer = 0;
    work->tiles2 = AllocObjTiles(288, NULL);
    work->palette2 = LoadObjPalette(gDialogBoxPalette, sizeof(gDialogBoxPalette));
    LoadObjPaletteBank(work->palette2->index, gDialogBoxPalette);
    SetObjTileSource(work->tiles2, gHandCursorTiles);
    AnimInit(&work->anim2, gHandCursorAnims, gHandCursorFrames);
    AnimStart(&work->anim2, 2, ANIM_FLAG_LOOP);
    work->gfx2 = AnimGetGfx(&work->anim2);
    work->tiles3 = LoadObjTiles(gMsgBoxTiles, sizeof(gMsgBoxTiles));
    work->palette3 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
    LoadObjPaletteBank(work->palette3->index, gCard00Palette);
    FadeSetPaletteExcluded(work->palette->index + 16, TRUE);
    InitTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    InitTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    work->palette4 = LoadTextPalette(1);
    work->textSlotCount = LoadTextSlots(LOCALIZED_STRING(gYesChoiceText), work->textSlots);
    work->textSlotCount2 = LoadTextSlots(LOCALIZED_STRING(gNoChoiceText), work->textSlots2);
    work->x = 0x5800;
    work->cursor = 1;
    work->y = gMsgwaitYesnoCursorY[1];
    work->timer = 0;
    gEventState->msgWaitActive = TRUE;
    gEventState->askedYesNo = TRUE;
    gEventState->answerYes = FALSE;
    work->choiceShown = FALSE;
}

u8 UpdateMsgwaitYesnoChoice(MsgWaitWork* work, void* task) {
    switch (GetKeysPressed()) {
    case DPAD_UP:
        if (work->cursor != 0) {
            work->cursor--;
            m4aSongNumStart(SONG_SYS_CLICK);
        }

        work->timer = 1;
        break;
    case DPAD_DOWN:
        if (work->cursor == 0) {
            work->cursor++;
            m4aSongNumStart(SONG_SYS_CLICK);
        }

        work->timer = 1;
        break;
    case A_BUTTON:
    case START_BUTTON:
        if (work->cursor == 0) {
            gEventState->answerYes = TRUE;
        } else {
            gEventState->answerYes = FALSE;

            if (gEventState->eventId == EVENT_068_13F_CASTLE_OBLIVION_LAST3) {
                gEventState->endRequest = TRUE;
                gEventState->skipHoldTime = 255;
            }
        }

        m4aSongNumStart(SONG_SYS_KETTEI);
        gEventState->msgWaitActive = FALSE;
        return 0;
    case B_BUTTON:
        gEventState->answerYes = FALSE;

        if (gEventState->eventId == EVENT_068_13F_CASTLE_OBLIVION_LAST3) {
            gEventState->endRequest = TRUE;
            gEventState->skipHoldTime = 255;
        }

        m4aSongNumStart(SONG_SYS_KETTEI);
        gEventState->msgWaitActive = FALSE;
        return 0;
    }

    if (work->timer != 0) {
        ApproachValue(&work->y, gMsgwaitYesnoCursorY[work->cursor], work->timer);
        work->timer--;
    }

    work->gfx = AnimUpdate(&work->anim);
    return 1;
}

u8 msgwait_yesno_1(MsgWaitWork* work, void* task) {
    work->gfx = AnimUpdate(&work->anim);

    if (GetKeysPressed() & A_BUTTON) {
        AnimStart(&work->anim, 3, ANIM_FLAG_LOOP);
        work->choiceShown = TRUE;
        m4aSongNumStart(SONG_SYS_CANSEL);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateMsgwaitYesnoChoice);
    }

    return 1;
}

void msgwait_yesno_2(MsgWaitWork* work) {
    switch (work->choiceShown) {
    case FALSE:
        if (gEventState->msgWinCentered) {
            DrawSprite(120, gMsgwaitIconPos[gEventState->msgWinPosition][1] >> 8, work->gfx, work->tiles, work->palette, NULL, 0, 0);
        } else {
            DrawSprite(gMsgwaitIconPos[gEventState->msgWinPosition][0] >> 8, gMsgwaitIconPos[gEventState->msgWinPosition][1] >> 8, work->gfx, work->tiles, work->palette, NULL, 0, 0);
        }

        break;
    case TRUE:
#ifdef PLATFORM_PC
        gPortSpriteShiftX = 0; /* the yes/no box is centred */
#endif
        DrawSprite(120, 80, gMsgBoxFrames[1], work->tiles3, work->palette3, NULL, 0, 10);
        DrawSprite(work->x >> 8, work->y >> 8, work->gfx2, work->tiles2, work->palette2, NULL, SPRITE_FLAG_HFLIP, 9);
        DrawTextSlots((DISPLAY_WIDTH - GetTextSlotsWidth(work->textSlots, work->textSlotCount)) >> 1, 67, work->textSlots, work->palette4, 0, work->textSlotCount);
        DrawTextSlots((DISPLAY_WIDTH - GetTextSlotsWidth(work->textSlots2, work->textSlotCount2)) >> 1, 82, work->textSlots2, work->palette4, 0, work->textSlotCount2);
        break;
    }
}

void msgwait_yesno_3(MsgWaitWork* work) {
    ReleaseObjTiles(work->tiles2);
    ReleaseObjTiles(work->tiles3);
    ReleaseObjPalette(work->palette3);
    ReleaseObjPalette(work->palette2);
    ReleaseObjPalette(work->palette4);
    FreeTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    FreeTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
}

void HBlankIntrEventScanlineScroll() {
    vu16 line;

    line = REG_VCOUNT;
    line = (line + 1) % 228;

    if (line < DISPLAY_HEIGHT) {
        if (gEventScanlineScroll->enabled == TRUE) {
            REG_BG2HOFS = gEventScanlineScroll->scrollX[line];
            REG_BG3HOFS = gEventScanlineScroll->scrollX[line];
        }
    }
}

void HBlankIntrEventBgWave() {
    gIntrCheck |= INTR_FLAG_HBLANK;
    HBlankIntrEventScanlineScroll();
}

void view_0(EventCameraWork* work, u8* arg) {
    const EventSequenceDef* seqDef;
    EventBackgroundDef* bg;
    const EventCameraKeyframe* keyframe;
    EvtObj* obj;
    u8 targetIndex;

    gEventScanlineScroll = &work->scanline;
    work->wavePhase = 0;
    work->scanline.enabled = FALSE;
    work->eventId = arg[0];
    work->keyframe = 0;
    work->steps = 0;
    work->angle = 0;
    work->approachMode = FALSE;
    work->effectStarted = FALSE;
    seqDef = gEventSequenceDefs[work->eventId];
    bg = gEventBackgroundDefs[work->eventId];
    keyframe = seqDef->keyframes;
    work->keyframes = keyframe;

    if (keyframe->target != EVENT_CHARA_NONE) {
        targetIndex = FindEventCameraTarget(work);
        obj = gEventState->charaObjs[targetIndex];
        work->targetX = obj->x;
        work->targetY = obj->y;
    } else {
        work->targetX = keyframe->x;
        work->targetY = keyframe->y;
    }

    gEventState->cameraX = work->targetX - 0x7800;
    gEventState->cameraY = work->targetY - 0x5000;
    gEventState->centerX = work->targetX;
    gEventState->centerY = work->targetY;
    gEventState->shakeX = 0;
    gEventState->shakeY = 0;

    if (bg != NULL) {
        if (bg->isAffine != 0) {
            SetBgAffine(2, 0, Q_8_8(1), Q_8_8(1), gEventState->centerX, gEventState->centerY);
        } else {
            if (work->eventId == EVENT_077_MONSTORO_E2_2) {
                ScrollBgMapTo(3, (gEventState->cameraX >> 8) + 8 + gEventState->shakeX, (gEventState->cameraY >> 8) + 40);
            } else {
                ScrollBgMapTo(3, (gEventState->cameraX >> 8) + gEventState->shakeX, gEventState->cameraY >> 8);
            }

            if (gEventState->hasBg2Map) {
                ScrollBgMapTo(2, (gEventState->cameraX >> 8) + gEventState->shakeX, gEventState->cameraY >> 8);
            }

            if (gEventState->hasBg1Map) {
                ScrollBgMapTo(1, (gEventState->cameraX >> 8) + gEventState->shakeX, gEventState->cameraY >> 8);
            }
        }
    }
}

void ClearEventObjPaletteExclusions() {
    u8 i;

    for (i = 0; i < 16; i++) {
        FadeSetPaletteExcluded(i + 16, FALSE);
    }
}

u8 view_1(EventCameraWork* work, Task* task) {
    EventBackgroundDef* bg = gEventBackgroundDefs[work->eventId];
    const EventCameraKeyframe* keyframe;
    EvtObj* obj;
    u8 targetIndex;

    if (gEventState->bossChara == EVENT_CHARA_BOSS_OOGIE_BOOGIE) {
        return 1;
    }

    if (gEventState->bossChara == EVENT_CHARA_BOSS_JAFAR) {
        return 1;
    }

    if (!gEventState->focusSpeaker) {
        keyframe = &work->keyframes[work->keyframe];

        if (gEventState->frame >= (u16)keyframe->frame && !(keyframe->flags & CAMERA_KEYFRAME_FLAG_END)) {
            work->keyframe++;
            keyframe = &work->keyframes[work->keyframe];
            work->effectStarted = FALSE;

            if (keyframe->callback != NULL) {
                ((void (*)(EventCameraWork*))keyframe->callback)(work);
            }

            switch (keyframe->flags & CAMERA_KEYFRAME_MODE_MASK) {
            case CAMERA_MODE_FOLLOW:
                work->approachMode = FALSE;
                break;
            case CAMERA_MODE_APPROACH:
                work->approachMode = TRUE;
                work->steps = keyframe->duration;
                break;
            }
        }

        if ((keyframe->flags & CAMERA_KEYFRAME_MODE_MASK) != CAMERA_MODE_KEEP) {
            if (keyframe->target == EVENT_CHARA_NONE) {
                work->targetX = keyframe->x;
                work->targetY = keyframe->y;
            } else {
                targetIndex = FindEventCameraTarget(work);
                obj = gEventState->charaObjs[targetIndex];
                work->targetX = obj->x;
                work->targetY = obj->y + obj->z + keyframe->yOffset;
            }
        }

        if (keyframe->flags & CAMERA_KEYFRAME_FLAG_FLASH) {
            if (!work->effectStarted) {
                ClearEventObjPaletteExclusions();
                FadeStartIn(FADE_MODE_ADD_WHITE, keyframe->duration);

                if (!(keyframe->flags & CAMERA_KEYFRAME_FLAG_NO_FLASH_SOUND)) {
                    m4aSongNumStart(SONG_EV_FLASH01);
                }

                gEventState->fadedOut = FALSE;
                work->effectStarted = TRUE;
            }
        }

        if (keyframe->flags & CAMERA_KEYFRAME_FLAG_FADE_OUT_WHITE) {
            if (!work->effectStarted) {
                ClearEventObjPaletteExclusions();
                FadeStartOut(FADE_MODE_WHITE, keyframe->duration);
                gEventState->fadedOut = TRUE;
                work->effectStarted = TRUE;

                if (keyframe->flags & CAMERA_KEYFRAME_FLAG_END) {
                    m4aSongNumStart(SONG_EV_WHITEOUT);
                }
            }
        }

        if (keyframe->flags & CAMERA_KEYFRAME_FLAG_FADE_OUT_BLACK) {
            if (!work->effectStarted) {
                ClearEventObjPaletteExclusions();
                FadeStartOut(FADE_MODE_BLACK, keyframe->duration);
                gEventState->fadedOut = TRUE;
                work->effectStarted = TRUE;
            }
        }

        if (keyframe->flags & CAMERA_KEYFRAME_FLAG_FADE_IN_WHITE) {
            if (!work->effectStarted) {
                FadeStartIn(FADE_MODE_WHITE, keyframe->duration);
                gEventState->fadedOut = FALSE;
                work->effectStarted = TRUE;
            }
        }

        if (keyframe->flags & CAMERA_KEYFRAME_FLAG_FADE_IN_BLACK) {
            if (!work->effectStarted) {
                ClearEventObjPaletteExclusions();
                FadeStartIn(FADE_MODE_BLACK, keyframe->duration);
                gEventState->fadedOut = FALSE;
                work->effectStarted = TRUE;
            }
        }

        if (keyframe->flags & CAMERA_KEYFRAME_FLAG_HALF_FLASH) {
            if (!work->effectStarted) {
                ClearEventObjPaletteExclusions();
                FadeFromAmount(FADE_MODE_ADD_WHITE, 16, keyframe->duration);

                if (!(keyframe->flags & CAMERA_KEYFRAME_FLAG_NO_FLASH_SOUND)) {
                    m4aSongNumStart(SONG_EV_FLASH00);
                }

                work->effectStarted = TRUE;
            }
        }

        if (keyframe->flags & CAMERA_KEYFRAME_FLAG_WAVE_START) {
            StartBgWave(HBlankIntrEventBgWave);
            work->scanline.enabled = TRUE;
        }

        if (keyframe->flags & CAMERA_KEYFRAME_FLAG_WAVE_STOP) {
            work->scanline.enabled = FALSE;
            ResetHBlankCallback();
            DisableHBlankIntr();
        }

        if (keyframe->flags & CAMERA_KEYFRAME_FLAG_FADE_IN_WHITE_LINEAR) {
            if (!work->effectStarted) {
                FadeStartIn(FADE_MODE_WHITE_BLEND, keyframe->duration);
            }

            work->effectStarted = TRUE;
        }

        if (keyframe->flags & CAMERA_KEYFRAME_FLAG_SHAKE_SMALL) {
            gEventState->shakeX = GetRandom() % 4;
            gEventState->shakeY = GetRandom() % 4;
        } else if (keyframe->flags & CAMERA_KEYFRAME_FLAG_SHAKE_MEDIUM) {
            gEventState->shakeX = GetRandom() % 8;
            gEventState->shakeY = GetRandom() % 8;
        } else if (keyframe->flags & CAMERA_KEYFRAME_FLAG_SHAKE_LARGE) {
            gEventState->shakeX = GetRandom() % 16;
            gEventState->shakeY = GetRandom() % 16;
        } else if (keyframe->flags & CAMERA_KEYFRAME_FLAG_SWAY) {
            gEventState->shakeX = 0;
            gEventState->shakeY = SIN(work->angle >> 3) >> 5;
            work->angle += 4;
        } else {
            gEventState->shakeX = 0;
            gEventState->shakeY = 0;
        }

        if (work->approachMode) {
            EventCameraApproach(work);
        } else {
            EventCameraFollow(work);
        }

        if (bg != NULL) {
            if (bg->isAffine != 0) {
                SetBgAffine(2, 0, Q_8_8(1), Q_8_8(1), gEventState->centerX, gEventState->centerY);
            } else {
                if (work->eventId == EVENT_077_MONSTORO_E2_2) {
                    ScrollBgMapTo(3, (gEventState->x >> 8) + 8, (gEventState->y >> 8) + 40);
                } else {
                    ScrollBgMapTo(3, gEventState->x >> 8, gEventState->y >> 8);
                }

                if (gEventState->hasBg2Map) {
                    ScrollBgMapTo(2, gEventState->x >> 8, gEventState->y >> 8);
                }

                if (gEventState->hasBg1Map) {
                    ScrollBgMapTo(1, gEventState->x >> 8, gEventState->y >> 8);
                }
            }
        } else {
            switch (work->eventId) {
            case EVENT_077_MONSTORO_E2_2:
            case EVENT_078_MONSTORO_E2_3:
                gBtlWork->viewX = gEventState->x;
                gBtlWork->viewY = gEventState->y;
                gBtlWork->x = gEventState->x;
                gBtlWork->y = gEventState->y;
                gBtlWork->x2 = gEventState->x;
                gBtlWork->y2 = gEventState->y;
                ScrollBgMapTo(0, (gEventState->x >> 8) + 8, (gEventState->y >> 8) + 40);
                ScrollBgMapTo(1, gEventState->x >> 8, gEventState->y >> 8);
                break;
            case EVENT_105_ATLANTICA_BOSS:
                gBtlWork->viewX = gEventState->x;
                gBtlWork->viewY = gEventState->y;
                gBtlWork->x = gEventState->x;
                gBtlWork->y = gEventState->y;
                gBtlWork->x2 = gEventState->x;
                gBtlWork->y2 = gEventState->y;
                ScrollBgMapTo(0, gEventState->x >> 8, gEventState->y >> 8);
                break;
            case EVENT_180_RIKU_B3F_BOSS:
                break;
            default:
                gBtlWork->viewX = gEventState->x;
                gBtlWork->viewY = gEventState->y;
                gBtlWork->x = gEventState->x;
                gBtlWork->y = gEventState->y;
                gBtlWork->x2 = gEventState->x;
                gBtlWork->y2 = gEventState->y;
                ScrollBgMapTo(0, gEventState->x >> 8, gEventState->y >> 8);
                ScrollBgMapTo(1, gEventState->x >> 8, gEventState->y >> 8);
                break;
            }
        }
    } else {
        targetIndex = FindEventCharaTrack(work, gEventState->speaker);
        work->targetX = gEventState->charaObjs[targetIndex]->x;

        switch (gEventState->msgWinPosition) {
        case MSG_WIN_POSITION_TOP_FACE_RIGHT:
        case MSG_WIN_POSITION_TOP_FACE_LEFT:
            work->targetY = gEventState->charaObjs[targetIndex]->y + gEventState->charaObjs[targetIndex]->z + sSpeakerFocusYOffsets[targetIndex];
            break;
        case MSG_WIN_POSITION_BOTTOM_FACE_RIGHT:
        case MSG_WIN_POSITION_BOTTOM_FACE_LEFT:
            work->targetY = gEventState->charaObjs[targetIndex]->y + gEventState->charaObjs[targetIndex]->z;
            break;
        }

        work->steps = gEventState->focusSteps;
        EventCameraApproach(work);

        if (bg != NULL) {
            if (bg->isAffine != 0) {
                SetBgAffine(2, 0, Q_8_8(1), Q_8_8(1), gEventState->centerX, gEventState->centerY);
            } else {
                if (work->eventId == EVENT_077_MONSTORO_E2_2) {
                    ScrollBgMapTo(3, (gEventState->x >> 8) + 8, (gEventState->y >> 8) + 40);
                } else {
                    ScrollBgMapTo(3, gEventState->x >> 8, gEventState->y >> 8);
                }

                if (gEventState->hasBg2Map) {
                    ScrollBgMapTo(2, gEventState->x >> 8, gEventState->y >> 8);
                }

                if (gEventState->hasBg1Map) {
                    ScrollBgMapTo(1, gEventState->x >> 8, gEventState->y >> 8);
                }
            }
        } else {
            gBtlWork->viewX = gEventState->x;
            gBtlWork->viewY = gEventState->y;
            ScrollBgMapTo(0, (gEventState->x >> 8) + 8, (gEventState->y >> 8) + 40);
            ScrollBgMapTo(1, gEventState->x >> 8, gEventState->y >> 8);
        }
    }

    if (gEventState->flags & EVENT_FLAG_PLAYER_CONTROL) {
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateEventCameraFollowPlayer);
    }

    if (bg != NULL && (bg->flags & EVENT_BG_FLAG_POOH_MAP)) {
        func_080CA368(3, gEventState->cameraX >> 8, gEventState->cameraY >> 8);
    }

    UpdateEventScanlineWave(work);
    return 1;
}

void view_2() {
}

void view_3() {
}

void SetEventCameraCenter(EventCameraWork* work) {
    gEventState->centerX = work->targetX;
    gEventState->centerY = work->targetY;
}

void EventCameraFollow(EventCameraWork* work) {
    const EventCameraKeyframe* keyframe;
    s32 x;
    s32 y;

    SetEventCameraCenter(work);
    x = gEventState->centerX - 0x7800;
    y = gEventState->centerY - 0x5000;
    keyframe = &work->keyframes[work->keyframe];

    if (keyframe->flags & (CAMERA_KEYFRAME_FLAG_SHAKE_MEDIUM | CAMERA_KEYFRAME_FLAG_SHAKE_LARGE | CAMERA_KEYFRAME_FLAG_SHAKE_SMALL)) {
        gEventState->cameraX = x;
        gEventState->cameraY = y;
    } else {
        gEventState->cameraX += (x - gEventState->cameraX) >> 3;
        gEventState->cameraY += (y - gEventState->cameraY) >> 3;
    }

    gEventState->x = gEventState->cameraX + (gEventState->shakeX << 8);
    gEventState->y = gEventState->cameraY + (gEventState->shakeY << 8);
}

void EventCameraSnap(EventCameraWork* work) {
    s32 x;
    s32 y;

    SetEventCameraCenter(work);
    x = gEventState->centerX - 0x7800;
    y = gEventState->centerY - 0x5000;
    gEventState->cameraX = x;
    gEventState->cameraY = y;
}

void EventCameraApproach(EventCameraWork* work) {
    s32 x;
    s32 y;

    SetEventCameraCenter(work);
    x = gEventState->centerX - 0x7800;
    y = gEventState->centerY - 0x5000;

    if (work->steps != 0) {
        ApproachValue(&gEventState->cameraX, x, work->steps);
        ApproachValue(&gEventState->cameraY, y, work->steps);
        work->steps--;
    } else {
        gEventState->cameraX = x;
        gEventState->cameraY = y;
    }

    gEventState->x = gEventState->cameraX + (gEventState->shakeX << 8);
    gEventState->y = gEventState->cameraY + (gEventState->shakeY << 8);
}

u8 FindEventCameraTarget(EventCameraWork* work) {
    const EventSequenceDef* seqDef = gEventSequenceDefs[work->eventId];
    u8 n = seqDef->charaCount;
    const EventCharaTrack* tracks = seqDef->charaTracks;
    const EventCameraKeyframe* keyframe = &work->keyframes[work->keyframe];
    u8 i;

    for (i = 0; i < n; i++) {
        if (keyframe->target == tracks[i].chara) {
            return i;
        }
    }

    return 0xFF;
}

u8 FindEventCharaTrack(EventCameraWork* work, u8 chara) {
    const EventSequenceDef* seqDef = gEventSequenceDefs[work->eventId];
    u8 n = seqDef->charaCount;
    const EventCharaTrack* tracks = seqDef->charaTracks;
    u8 i;

    for (i = 0; i < n; i++) {
        if (chara == tracks[i].chara) {
            return i;
        }
    }

    return 0xFF;
}

u8 UpdateEventCameraFollowPlayer(EventCameraWork* work) {
    EventBackgroundDef* bg;
    EvtObj* obj;
    u8 trackIndex;

    trackIndex = FindEventCharaTrack(work, EVENT_CHARA_SORA);
    bg = gEventBackgroundDefs[work->eventId];
    obj = gEventState->charaObjs[trackIndex];
    work->targetX = obj->x;
    work->targetY = obj->y + obj->z;

    if (bg != NULL) {
        if (bg->isAffine != 0) {
            EventCameraSnap(work);
            SetBgAffine(2, 0, Q_8_8(1), Q_8_8(1), gEventState->centerX, gEventState->centerY);
        } else {
            EventCameraFollow(work);

            if (work->eventId == EVENT_077_MONSTORO_E2_2) {
                ScrollBgMapTo(3, (gEventState->cameraX >> 8) + 8 + gEventState->shakeX, (gEventState->cameraY >> 8) + 40);
            } else {
                ScrollBgMapTo(3, (gEventState->cameraX >> 8) + gEventState->shakeX, gEventState->cameraY >> 8);
            }

            if (gEventState->hasBg2Map) {
                ScrollBgMapTo(2, gEventState->cameraX >> 8, gEventState->cameraY >> 8);
            }

            if (gEventState->hasBg1Map) {
                ScrollBgMapTo(1, gEventState->cameraX >> 8, gEventState->cameraY >> 8);
            }
        }
    } else {
        EventCameraFollow(work);
        gBtlWork->viewX = gEventState->cameraX;
        gBtlWork->viewY = gEventState->cameraY;
        ScrollBgMapTo(0, (gEventState->cameraX >> 8) + 8, (gEventState->cameraY >> 8) + 40);
        ScrollBgMapTo(1, gEventState->cameraX >> 8, gEventState->cameraY >> 8);
    }

    return 1;
}

void UpdateEventScanlineWave(EventCameraWork* work) {
    u8 i;
    s32 wave;

    if (work->scanline.enabled == TRUE) {
        for (i = 0; i < DISPLAY_HEIGHT; i++) {
            work->scanline.scrollX[i] = (gEventState->x >> 8) + (wave = (u8)SIN((i + work->wavePhase) * 2)) / 32;
        }

        work->wavePhase++;
    }
}

TaskDesc gTaskDescMsgwaitYesno = {
    "msgwait_yesno",
    (TaskInitFunc)msgwait_yesno_0,
    (TaskUpdateFunc)msgwait_yesno_1,
    (TaskDrawFunc)msgwait_yesno_2,
    (TaskDestroyFunc)msgwait_yesno_3,
    sizeof(MsgWaitWork),
};

TaskDesc gTaskDescView = {
    "view",
    (TaskInitFunc)view_0,
    (TaskUpdateFunc)view_1,
    (TaskDrawFunc)view_2,
    (TaskDestroyFunc)view_3,
    sizeof(EventCameraWork),
};
