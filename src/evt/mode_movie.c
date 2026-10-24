/**
 * mode_movie.c
 * Movie Playback Mode
 */

#include "mode_chkmov.h"
#include "registration_data.h"
#include "system_state.h"
#include "main.h"
#include "movie.h"
#include "msg_api.h"
#include "mode.h"
#include "display.h"
#include "pallet.h"
#include "mode_movie.h"
#include "sprite_palettes.h"
#include "gba/io_reg.h"
#include "malloc.h"
#include "fade.h"
#include <stddef.h>
#include "gba/keys.h"
#include "gba/oam.h"
#include "engine.h"
#include "gba/defines.h"
#include "gba/macro.h"
#include "gba/syscall.h"
#include "intr.h"
#include "m4a.h"
#include "obj_api.h"
#include "sprite.h"
#include "types.h"
#include "key_state.h"
#include "movie_subtitle_text.h"
#include <stdlib.h>
#include "movies.h"
#include "text_types.h"
#include "event_ids.h"

static vu16 sMovieModeState;
static s32 sMovieId;
static u16 sUnk_02034940;
static volatile s16 sMovieFrame;
static volatile s16 sMovieSubIndex;
static volatile u16 sMovieSubCount;
static const MovieSub* volatile sMovieSubUpper;
static const MovieSub* volatile sMovieSubLower;
static const MovieSub* sMovieSubs;
static volatile s16 sMovieSubUpperTimer;
static volatile u16 sMovieSubUpperLength;
static volatile u16 sMovieFlags;
static volatile u16 sMovieSubUpperAlpha;
static volatile s16 sMovieSubLowerTimer;
static volatile u16 sMovieSubLowerLength;
static volatile u16 sMovieSubLowerAlpha;

#ifdef VERSION_US
#include "movie_subtitles_1.inc"

const MovieSub gMovieSubsOpening[3] = {
    { 673, 0, gMovieSubTextUs_0886A6D4, 1, 45, 1 },
    { 825, 0, gMovieSubTextUs_0886A712, 0, 50, 1 },
    { 825, 0, gMovieSubTextUs_0886A752, 1, 50, 1 },
};

const MovieSub gMovieSubs6FGoal[1] = {
    { 30, 20, gMovieSubTextUs_0886AB3A, 1, 50, 1 },
};

const MovieSub gMovieSubs12FE2[1] = {
    { 30, 20, gMovieSubTextUs_0886AB3A, 1, 50, 1 },
};

const MovieSub gMovieSubsEnding[14] = {
    { 337, 0, gMovieSubTextUs_0886A78A, 1, 8, 0 },
    { 405, 0, gMovieSubTextUs_0886A798, 1, 30, 0 },
    { 450, 0, gMovieSubTextUs_0886A7B8, 1, 10, 0 },
    { 465, 0, gMovieSubTextUs_0886A7C8, 0, 60, 0 },
    { 465, 0, gMovieSubTextUs_0886A7E2, 1, 60, 0 },
    { 545, 0, gMovieSubTextUs_0886A806, 0, 40, 0 },
    { 545, 0, gMovieSubTextUs_0886A83A, 1, 40, 0 },
    { 605, 0, gMovieSubTextUs_0886A870, 1, 25, 0 },
    { 645, 0, gMovieSubTextUs_0886A8A0, 0, 90, 0 },
    { 645, 0, gMovieSubTextUs_0886A8E0, 1, 90, 0 },
    { 750, 0, gMovieSubTextUs_0886A91E, 0, 50, 0 },
    { 750, 0, gMovieSubTextUs_0886A960, 1, 50, 0 },
    { 810, 0, gMovieSubTextUs_0886A978, 0, 45, 0 },
    { 810, 0, gMovieSubTextUs_0886A988, 1, 45, 0 },
};

const MovieSub gMovieSubsRikuEnding[10] = {
    { 405, 0, gMovieSubTextUs_0886A9BA, 0, 35, 1 },
    { 405, 0, gMovieSubTextUs_0886A9E8, 1, 35, 1 },
    { 450, 0, gMovieSubTextUs_0886AA00, 1, 30, 1 },
    { 490, 0, gMovieSubTextUs_0886AA42, 1, 30, 1 },
    { 525, 0, gMovieSubTextUs_0886AA76, 1, 25, 1 },
    { 580, 0, gMovieSubTextUs_0886AA88, 1, 28, 1 },
    { 650, 0, gMovieSubTextUs_0886AAC0, 0, 50, 1 },
    { 650, 0, gMovieSubTextUs_0886AAF2, 1, 50, 1 },
    { 710, 0, gMovieSubTextUs_0886AB0E, 1, 15, 1 },
    { 755, 0, gMovieSubTextUs_0886AB16, 1, 25, 1 },
};
#endif

#ifdef VERSION_JP
const MovieSub gMovieSubsOpening[3] = {
    { 673, 35, gMovieSubTextJp_0885DFE4, 1, 45, 1 },
    { 825, 50, gMovieSubTextJp_0885DFC4, 1, 43, 1 },
    { 872, 60, gMovieSubTextJp_0885DFA8, 1, 45, 1 },
};

#include "movie_subtitles_1.inc"

const MovieSub gMovieSubs6FGoal[1] = {
    { 30, 20, gMovieSubTextJp_0885E018, 1, 50, 1 },
};

#include "movie_subtitles_2.inc"

const MovieSub gMovieSubs12FE2[1] = {
    { 30, 20, gMovieSubTextJp_0885E018, 1, 50, 1 },
};

const MovieSub gMovieSubsEnding[12] = {
    { 337, 100, gMovieSubTextJp_0885E1CC, 1, 8, 0 },
    { 405, 95, gMovieSubTextJp_0885E1C0, 1, 30, 0 },
    { 450, 95, gMovieSubTextJp_0885E1B4, 1, 10, 0 },
    { 465, 60, gMovieSubTextJp_0885E1AC, 0, 60, 0 },
    { 465, 60, gMovieSubTextJp_0885E190, 1, 60, 0 },
    { 545, 40, gMovieSubTextJp_0885E16C, 1, 40, 0 },
    { 605, 85, gMovieSubTextJp_0885E15C, 1, 25, 0 },
    { 645, 50, gMovieSubTextJp_0885E144, 0, 90, 0 },
    { 645, 50, gMovieSubTextJp_0885E124, 1, 90, 0 },
    { 750, 60, gMovieSubTextJp_0885E108, 1, 50, 0 },
    { 810, 75, gMovieSubTextJp_0885E100, 0, 45, 0 },
    { 810, 75, gMovieSubTextJp_0885E0EC, 1, 45, 0 },
};

#include "movie_subtitles_3.inc"

const MovieSub gMovieSubsRikuEnding[8] = {
    { 405, 60, gMovieSubTextJp_0885E2DC, 1, 35, 1 },
    { 450, 85, gMovieSubTextJp_0885E2CC, 1, 30, 1 },
    { 490, 95, gMovieSubTextJp_0885E2C0, 1, 30, 1 },
    { 525, 80, gMovieSubTextJp_0885E2AC, 1, 25, 1 },
    { 580, 85, gMovieSubTextJp_0885E29C, 1, 28, 1 },
    { 650, 40, gMovieSubTextJp_0885E278, 1, 50, 1 },
    { 710, 90, gMovieSubTextJp_0885E268, 1, 15, 1 },
    { 755, 90, gMovieSubTextJp_0885E258, 1, 25, 1 },
};

#include "movie_subtitles_4.inc"
#endif

#ifdef VERSION_EU
#include "movie_subtitles_1.inc"

const MovieSub gMovieSubsOpeningEn[3] = {
    { 673, 0, gMovieSubTextEu_0883DE0C, 1, 45, 1 },
    { 825, 0, gMovieSubTextEu_0883DE2B, 0, 50, 1 },
    { 825, 0, gMovieSubTextEu_0883DE4B, 1, 50, 1 },
};

const MovieSub gMovieSubsEndingEn[14] = {
    { 337, 0, gMovieSubTextEu_0883DE67, 1, 8, 0 },
    { 405, 0, gMovieSubTextEu_0883DE6E, 1, 30, 0 },
    { 450, 0, gMovieSubTextEu_0883DE7E, 1, 10, 0 },
    { 465, 0, gMovieSubTextEu_0883DE86, 0, 60, 0 },
    { 465, 0, gMovieSubTextEu_0883DE93, 1, 60, 0 },
    { 545, 0, gMovieSubTextEu_0883DEA5, 0, 40, 0 },
    { 545, 0, gMovieSubTextEu_0883DEBF, 1, 40, 0 },
    { 605, 0, gMovieSubTextEu_0883DEDA, 1, 25, 0 },
    { 645, 0, gMovieSubTextEu_0883DEF2, 0, 90, 0 },
    { 645, 0, gMovieSubTextEu_0883DF12, 1, 90, 0 },
    { 750, 0, gMovieSubTextEu_0883DF31, 0, 50, 0 },
    { 750, 0, gMovieSubTextEu_0883DF52, 1, 50, 0 },
    { 810, 0, gMovieSubTextEu_0883DF5E, 0, 45, 0 },
    { 810, 0, gMovieSubTextEu_0883DF66, 1, 45, 0 },
};

const MovieSub gMovieSubsRikuEndingEn[10] = {
    { 405, 0, gMovieSubTextEu_0883DF7F, 0, 35, 1 },
    { 405, 0, gMovieSubTextEu_0883DF96, 1, 35, 1 },
    { 450, 0, gMovieSubTextEu_0883DFA2, 1, 30, 1 },
    { 490, 0, gMovieSubTextEu_0883DFC3, 1, 30, 1 },
    { 525, 0, gMovieSubTextEu_0883DFDD, 1, 25, 1 },
    { 580, 0, gMovieSubTextEu_0883DFE6, 1, 28, 1 },
    { 650, 0, gMovieSubTextEu_0883E002, 0, 50, 1 },
    { 650, 0, gMovieSubTextEu_0883E01B, 1, 50, 1 },
    { 710, 0, gMovieSubTextEu_0883E029, 1, 15, 1 },
    { 755, 0, gMovieSubTextEu_0883E02D, 1, 25, 1 },
};

#include "movie_subtitles_2.inc"

const MovieSub gMovieSubsOpeningFr[4] = {
    { 673, 0, gMovieSubTextEu_0883E1F0, 0, 45, 1 },
    { 673, 0, gMovieSubTextEu_0883E205, 1, 45, 1 },
    { 825, 0, gMovieSubTextEu_0883E21B, 0, 50, 1 },
    { 825, 0, gMovieSubTextEu_0883E23A, 1, 50, 1 },
};

const MovieSub gMovieSubsEndingFr[14] = {
    { 337, 0, gMovieSubTextEu_0883E255, 1, 8, 0 },
    { 405, 0, gMovieSubTextEu_0883E25D, 1, 30, 0 },
    { 450, 0, gMovieSubTextEu_0883E26C, 1, 10, 0 },
    { 465, 0, gMovieSubTextEu_0883E275, 0, 60, 0 },
    { 465, 0, gMovieSubTextEu_0883E287, 1, 60, 0 },
    { 545, 0, gMovieSubTextEu_0883E297, 0, 40, 0 },
    { 545, 0, gMovieSubTextEu_0883E2B4, 1, 40, 0 },
    { 605, 0, gMovieSubTextEu_0883E2CC, 1, 25, 0 },
    { 645, 0, gMovieSubTextEu_0883E2EB, 0, 90, 0 },
    { 645, 0, gMovieSubTextEu_0883E303, 1, 90, 0 },
    { 750, 0, gMovieSubTextEu_0883E311, 0, 50, 0 },
    { 750, 0, gMovieSubTextEu_0883E32B, 1, 50, 0 },
    { 810, 0, gMovieSubTextEu_0883E340, 0, 45, 0 },
    { 810, 0, gMovieSubTextEu_0883E354, 1, 45, 0 },
};

const MovieSub gMovieSubsRikuEndingFr[12] = {
    { 405, 0, gMovieSubTextEu_0883E36D, 0, 35, 1 },
    { 405, 0, gMovieSubTextEu_0883E385, 1, 35, 1 },
    { 450, 0, gMovieSubTextEu_0883E392, 0, 30, 1 },
    { 450, 0, gMovieSubTextEu_0883E3A9, 1, 30, 1 },
    { 490, 0, gMovieSubTextEu_0883E3B5, 1, 30, 1 },
    { 525, 0, gMovieSubTextEu_0883E3CF, 1, 25, 1 },
    { 580, 0, gMovieSubTextEu_0883E3E4, 0, 28, 1 },
    { 580, 0, gMovieSubTextEu_0883E3FE, 1, 28, 1 },
    { 650, 0, gMovieSubTextEu_0883E409, 0, 50, 1 },
    { 650, 0, gMovieSubTextEu_0883E41F, 1, 50, 1 },
    { 710, 0, gMovieSubTextEu_0883E434, 1, 15, 1 },
    { 755, 0, gMovieSubTextEu_0883E439, 1, 25, 1 },
};

#include "movie_subtitles_3.inc"

const MovieSub gMovieSubsOpeningDe[4] = {
    { 673, 0, gMovieSubTextEu_0883E634, 0, 45, 1 },
    { 673, 0, gMovieSubTextEu_0883E649, 1, 45, 1 },
    { 825, 0, gMovieSubTextEu_0883E65B, 0, 50, 1 },
    { 825, 0, gMovieSubTextEu_0883E676, 1, 50, 1 },
};

const MovieSub gMovieSubsEndingDe[14] = {
    { 337, 0, gMovieSubTextEu_0883E69A, 1, 8, 0 },
    { 405, 0, gMovieSubTextEu_0883E6A1, 1, 30, 0 },
    { 450, 0, gMovieSubTextEu_0883E6B5, 1, 10, 0 },
    { 465, 0, gMovieSubTextEu_0883E6BD, 0, 60, 0 },
    { 465, 0, gMovieSubTextEu_0883E6CA, 1, 60, 0 },
    { 545, 0, gMovieSubTextEu_0883E6E5, 0, 40, 0 },
    { 545, 0, gMovieSubTextEu_0883E70B, 1, 40, 0 },
    { 605, 0, gMovieSubTextEu_0883E729, 1, 25, 0 },
    { 645, 0, gMovieSubTextEu_0883E746, 0, 90, 0 },
    { 645, 0, gMovieSubTextEu_0883E767, 1, 90, 0 },
    { 750, 0, gMovieSubTextEu_0883E784, 0, 50, 0 },
    { 750, 0, gMovieSubTextEu_0883E7A5, 1, 50, 0 },
    { 810, 0, gMovieSubTextEu_0883E7C0, 0, 45, 0 },
    { 810, 0, gMovieSubTextEu_0883E7CA, 1, 45, 0 },
};

const MovieSub gMovieSubsRikuEndingDe[11] = {
    { 405, 0, gMovieSubTextEu_0883E7EA, 0, 35, 1 },
    { 405, 0, gMovieSubTextEu_0883E802, 1, 35, 1 },
    { 450, 0, gMovieSubTextEu_0883E815, 1, 30, 1 },
    { 490, 0, gMovieSubTextEu_0883E832, 1, 30, 1 },
    { 525, 0, gMovieSubTextEu_0883E852, 1, 25, 1 },
    { 580, 0, gMovieSubTextEu_0883E85E, 1, 28, 1 },
    { 650, 0, gMovieSubTextEu_0883E877, 0, 50, 1 },
    { 650, 0, gMovieSubTextEu_0883E891, 1, 50, 1 },
    { 710, 0, gMovieSubTextEu_0883E8AC, 1, 15, 1 },
    { 755, 0, gMovieSubTextEu_0883E8B2, 0, 25, 1 },
    { 755, 0, gMovieSubTextEu_0883E8C1, 1, 25, 1 },
};

#include "movie_subtitles_4.inc"

const MovieSub gMovieSubsOpeningIt[4] = {
    { 673, 0, gMovieSubTextEu_0883EAA4, 0, 45, 1 },
    { 673, 0, gMovieSubTextEu_0883EAC1, 1, 45, 1 },
    { 825, 0, gMovieSubTextEu_0883EAD7, 0, 50, 1 },
    { 825, 0, gMovieSubTextEu_0883EAF2, 1, 50, 1 },
};

const MovieSub gMovieSubsEndingIt[14] = {
    { 337, 0, gMovieSubTextEu_0883EB0A, 1, 8, 0 },
    { 405, 0, gMovieSubTextEu_0883EB11, 1, 30, 0 },
    { 450, 0, gMovieSubTextEu_0883EB27, 1, 10, 0 },
    { 465, 0, gMovieSubTextEu_0883EB2F, 0, 60, 0 },
    { 465, 0, gMovieSubTextEu_0883EB42, 1, 60, 0 },
    { 545, 0, gMovieSubTextEu_0883EB56, 0, 40, 0 },
    { 545, 0, gMovieSubTextEu_0883EB6B, 1, 40, 0 },
    { 605, 0, gMovieSubTextEu_0883EB85, 1, 25, 0 },
    { 645, 0, gMovieSubTextEu_0883EBA2, 0, 90, 0 },
    { 645, 0, gMovieSubTextEu_0883EBBC, 1, 90, 0 },
    { 750, 0, gMovieSubTextEu_0883EBDA, 0, 50, 0 },
    { 750, 0, gMovieSubTextEu_0883EBF0, 1, 50, 0 },
    { 810, 0, gMovieSubTextEu_0883EC05, 0, 45, 0 },
    { 810, 0, gMovieSubTextEu_0883EC0C, 1, 45, 0 },
};

const MovieSub gMovieSubsRikuEndingIt[10] = {
    { 405, 0, gMovieSubTextEu_0883EC29, 0, 35, 1 },
    { 405, 0, gMovieSubTextEu_0883EC36, 1, 35, 1 },
    { 450, 0, gMovieSubTextEu_0883EC48, 1, 30, 1 },
    { 490, 0, gMovieSubTextEu_0883EC67, 1, 30, 1 },
    { 525, 0, gMovieSubTextEu_0883EC7F, 1, 25, 1 },
    { 580, 0, gMovieSubTextEu_0883EC92, 1, 28, 1 },
    { 650, 0, gMovieSubTextEu_0883ECAC, 0, 50, 1 },
    { 650, 0, gMovieSubTextEu_0883ECBD, 1, 50, 1 },
    { 710, 0, gMovieSubTextEu_0883ECCD, 1, 15, 1 },
    { 755, 0, gMovieSubTextEu_0883ECD3, 1, 25, 1 },
};

#include "movie_subtitles_5.inc"

const MovieSub gMovieSubsOpeningEs[4] = {
    { 673, 0, gMovieSubTextEu_0883EEA8, 0, 45, 1 },
    { 673, 0, gMovieSubTextEu_0883EEC1, 1, 45, 1 },
    { 825, 0, gMovieSubTextEu_0883EECC, 0, 50, 1 },
    { 825, 0, gMovieSubTextEu_0883EEE6, 1, 50, 1 },
};

const MovieSub gMovieSubsEndingEs[14] = {
    { 337, 0, gMovieSubTextEu_0883EEFF, 1, 8, 0 },
    { 405, 0, gMovieSubTextEu_0883EF07, 1, 30, 0 },
    { 450, 0, gMovieSubTextEu_0883EF19, 1, 10, 0 },
    { 465, 0, gMovieSubTextEu_0883EF22, 0, 60, 0 },
    { 465, 0, gMovieSubTextEu_0883EF33, 1, 60, 0 },
    { 545, 0, gMovieSubTextEu_0883EF43, 0, 40, 0 },
    { 545, 0, gMovieSubTextEu_0883EF5D, 1, 40, 0 },
    { 605, 0, gMovieSubTextEu_0883EF7A, 1, 25, 0 },
    { 645, 0, gMovieSubTextEu_0883EF96, 0, 90, 0 },
    { 645, 0, gMovieSubTextEu_0883EFB2, 1, 90, 0 },
    { 750, 0, gMovieSubTextEu_0883EFC8, 0, 50, 0 },
    { 750, 0, gMovieSubTextEu_0883EFE6, 1, 50, 0 },
    { 810, 0, gMovieSubTextEu_0883EFFC, 0, 45, 0 },
    { 810, 0, gMovieSubTextEu_0883F003, 1, 45, 0 },
};

const MovieSub gMovieSubsRikuEndingEs[11] = {
    { 405, 0, gMovieSubTextEu_0883F01E, 0, 35, 1 },
    { 405, 0, gMovieSubTextEu_0883F029, 1, 35, 1 },
    { 450, 0, gMovieSubTextEu_0883F039, 1, 30, 1 },
    { 490, 0, gMovieSubTextEu_0883F05B, 1, 30, 1 },
    { 525, 0, gMovieSubTextEu_0883F075, 1, 25, 1 },
    { 580, 0, gMovieSubTextEu_0883F07E, 0, 28, 1 },
    { 580, 0, gMovieSubTextEu_0883F09C, 1, 28, 1 },
    { 650, 0, gMovieSubTextEu_0883F0AC, 0, 50, 1 },
    { 650, 0, gMovieSubTextEu_0883F0C6, 1, 50, 1 },
    { 710, 0, gMovieSubTextEu_0883F0DA, 1, 15, 1 },
    { 755, 0, gMovieSubTextEu_0883F0DE, 1, 25, 1 },
};
#endif

static const u8 sMovieHeapName[] = "MOVIE";

#ifndef VERSION_JP
s16 GetCenteredTextX(u16* widths, u16 count) {
    s16 i;
    s16 total;

    total = 0;

    for (i = 0; i < count; i++) {
        total += *widths;
        widths++;
    }

    if (total / 2 > 120) {
        total = 240;
    }

    return 120 - total / 2;
}
#endif

u16 CountNonSpaceChars(const TextChar* str) {
    s32 i;
#ifndef VERSION_JP
    s32 n;
    TextChar ch;
#endif

    i = 0;

#ifndef VERSION_JP
    n = 0;
#endif

    for (;;) {
#ifdef VERSION_JP
        if (str[i] == 0) {
            return i / 2;
        }
#else
        ch = str[i];

        if (ch == 0) {
            return n;
        }

        if (ch != 0x20) {
            n++;
        }
#endif

        i++;
    }
}

enum MovieModeState {
    MOVIE_MODE_STATE_CLEAR,
    MOVIE_MODE_STATE_WAIT,
    MOVIE_MODE_STATE_PLAY,
    MOVIE_MODE_STATE_EXIT
};

void mode_movie_0(s32 movieId) {
#ifdef PLATFORM_PC
    PortSideStyle(PORT_SIDE_MOVIE); /* cropped to the video, as large as the window allows */
#endif
    sMovieModeState = MOVIE_MODE_STATE_CLEAR;
    sMovieId = movieId;
    sUnk_02034940 = 0;
    sMovieFrame = 0;
    sMovieSubIndex = 0;
    sMovieSubCount = 0;
    sMovieSubs = NULL;
    sMovieFlags = 0;
    sMovieSubUpperTimer = 0;
    sMovieSubUpperLength = 0;
    sMovieSubUpperAlpha = 0;
    sMovieSubUpper = NULL;
    sMovieSubLowerTimer = 0;
    sMovieSubLowerLength = 0;
    sMovieSubLowerAlpha = 0;
    sMovieSubLower = NULL;
}

#ifdef VERSION_JP
#define MOVIE_SUB_MAX_CHARS 24
#elif defined(VERSION_EU)
#define MOVIE_SUB_MAX_CHARS 48
#else
#define MOVIE_SUB_MAX_CHARS 40
#endif

#ifdef PLATFORM_PC
/* HD text: the pixel-font code of each subtitle glyph, as copied to VRAM. */
extern u16* gPortSubGlyphCodes;
void PortLog(const char* fmt, ...);
unsigned PortFrameCount(void);
void PortOamSetText(int oamIndex, int code, int style);
static u16 sMovieSubUpperCodes[MOVIE_SUB_MAX_CHARS];
static u16 sMovieSubLowerCodes[MOVIE_SUB_MAX_CHARS];
#endif
#ifndef VERSION_JP
static u16 sMovieSubUpperWidths[MOVIE_SUB_MAX_CHARS];
static u16 sMovieSubLowerWidths[MOVIE_SUB_MAX_CHARS];
#endif

#ifdef PLATFORM_PC
void PortUiOpenMoviePause(void);
unsigned PortUiOpenSerial(void);
u32 PortFrameCount(void);
int PortHdMoviePlay(int movieId);
static u8 sPortMovieSkip;

void PortMovieSkipRequest(void) {
    sPortMovieSkip = TRUE;
}
#endif

s32 HandleMovieFrame(s32 arg) {
    s32 i;
    u16 keys;

    keys = ~REG_KEYINPUT;

    if ((keys & SOFT_RESET_KEYS) == SOFT_RESET_KEYS) {
        sMovieFlags |= MOVIE_FLAG_SOFT_RESET;
        return 1;
    }

#ifdef PLATFORM_PC
    /* START pauses any cinematic under the scene pause (#24); its "skip cinematic"
       ends the movie here, after the pause has closed. */
    {
        /* a tap, as event_message.c's scene pause: START pressed and released within 32 frames, alone (not START +
           SELECT, the options) and with the options never opened meanwhile (23/10) */
        static u8 sStartHeld, sStartVoid;
        static unsigned sOptionsSerial;
        static u32 sStartAt; /* the frame START went down: the movie's own frames are fewer than the display's */
        u16 start = keys & START_BUTTON;

        if (sMovieFrame == 0) {
            sStartHeld = start != 0;
            sStartVoid = start != 0; /* held into the movie */
        }
        if (start) {
            if (!sStartHeld) {
                sStartHeld = TRUE;
                sStartAt = PortFrameCount();
                sOptionsSerial = PortUiOpenSerial();
            }
            if (keys & SELECT_BUTTON)
                sStartVoid = TRUE;
        } else {
            if (sStartHeld && !sStartVoid && PortFrameCount() - sStartAt <= 32 && sOptionsSerial == PortUiOpenSerial())
                PortUiOpenMoviePause();
            sStartHeld = FALSE;
            sStartVoid = FALSE;
        }
        if (sPortMovieSkip) {
            sPortMovieSkip = FALSE;
            PortUiSkipPrompt(FALSE);
            return 1;
        }
    }

    /* A shows a prompt; A again while it is up skips the opening. */
    if (sMovieId == MOVIE_OPENING) {
        static u16 sPrevKeys;
        static s16 sSkipTimer;
        u16 pressed;

        if (sMovieFrame == 0) {
            sPrevKeys = keys;
            sSkipTimer = 0;
        }
        pressed = keys & ~sPrevKeys;
        sPrevKeys = keys;
        if (pressed & A_BUTTON) {
            if (sSkipTimer > 0) {
                PortUiSkipPrompt(FALSE);
                return 1;
            }
            sSkipTimer = 15 * 4; /* movie frames: 4 seconds */
            PortUiSkipPrompt(TRUE);
        } else if (sSkipTimer > 0 && --sSkipTimer == 0) {
            PortUiSkipPrompt(FALSE);
        }
    }
#endif

    if (sMovieSubs != NULL) {
        for (i = 0; i < 2; i++) {
            if (sMovieSubs[sMovieSubIndex].frame == sMovieFrame) {
                if (sMovieSubs[sMovieSubIndex].line == 0) {
                    const MovieSub* sub;

                    sMovieSubUpper = sub = &sMovieSubs[sMovieSubIndex];
                    sMovieFlags |= MOVIE_FLAG_UPPER_SUB_PENDING;
                    sMovieSubUpperTimer = sub->duration;

                    if (sMovieSubIndex < sMovieSubCount - 1) {
                        sMovieSubIndex++;
                    }

                    sMovieSubUpperLength = CountNonSpaceChars(sMovieSubUpper->text);

                    if (sMovieSubUpperLength > MOVIE_SUB_MAX_CHARS) {
                        sMovieSubUpperLength = MOVIE_SUB_MAX_CHARS;
                    }
                } else {
                    const MovieSub* sub;

                    sMovieSubLower = sub = &sMovieSubs[sMovieSubIndex];
                    sMovieFlags |= MOVIE_FLAG_LOWER_SUB_PENDING;
                    sMovieSubLowerTimer = sub->duration;

                    if (sMovieSubIndex < sMovieSubCount - 1) {
                        sMovieSubIndex++;
                    }

                    sMovieSubLowerLength = CountNonSpaceChars(sub->text);

                    if (sMovieSubLowerLength > MOVIE_SUB_MAX_CHARS) {
                        sMovieSubLowerLength = MOVIE_SUB_MAX_CHARS;
                    }
                }
            }
        }

        if (sMovieSubUpperTimer > 0) {
            sMovieSubUpperTimer--;
        }

        if (sMovieSubLowerTimer > 0) {
            sMovieSubLowerTimer--;
        }
    }

    sMovieFrame++;
    return 0;
}

void MovieVBlankIntr() {
    u16* oam;
#ifndef VERSION_JP
    s16 x;
    s16 y;
#endif
    u16 i;
    u32 attr0;
    u32 attr1;

    if (sMovieFlags & MOVIE_FLAG_PLAYING) {
        REG_DISPCNT = (DISPCNT_MODE_3 | DISPCNT_OBJ_1D_MAP | DISPCNT_BG_ALL_ON);
        MovieUpdate();

        if (sMovieSubs != NULL) {
            if (sMovieFlags & MOVIE_FLAG_UPPER_SUB_PENDING) {
                sMovieFlags &= ~MOVIE_FLAG_UPPER_SUB_PENDING;
                sMovieSubUpperAlpha = 0;
#ifdef PLATFORM_PC
                if (getenv("KHCOM_MOVIELOG"))
                    PortLog("movie: upper subtitle at movie frame %d, game frame %u", (int)sMovieFrame, (unsigned)PortFrameCount());
#endif
#ifdef VERSION_JP
                #ifdef PLATFORM_PC
                gPortSubGlyphCodes = sMovieSubUpperCodes;
#endif
                CopySjisGlyphsToVram(sMovieSubUpper->text);
#ifdef PLATFORM_PC
                gPortSubGlyphCodes = NULL;
#endif
#else
                #ifdef PLATFORM_PC
                gPortSubGlyphCodes = sMovieSubUpperCodes;
#endif
                CopyLatinGlyphsToVram(sMovieSubUpper->text, sMovieSubUpperWidths, 0);
#ifdef PLATFORM_PC
                gPortSubGlyphCodes = NULL;
#endif
#endif
            }

            if (sMovieFlags & MOVIE_FLAG_LOWER_SUB_PENDING) {
                sMovieFlags &= ~MOVIE_FLAG_LOWER_SUB_PENDING;
                sMovieSubLowerAlpha = 0;
#ifdef VERSION_JP
                #ifdef PLATFORM_PC
                gPortSubGlyphCodes = sMovieSubLowerCodes;
#endif
                CopySjisGlyphsToVramAt(sMovieSubLower->text, 0x100);
#ifdef PLATFORM_PC
                gPortSubGlyphCodes = NULL;
#endif
#else
                #ifdef PLATFORM_PC
                gPortSubGlyphCodes = sMovieSubLowerCodes;
#endif
                CopyLatinGlyphsToVram(sMovieSubLower->text, sMovieSubLowerWidths, 0x100);
#ifdef PLATFORM_PC
                gPortSubGlyphCodes = NULL;
#endif
#endif
            }

            if (sMovieSubUpperTimer > 0 || sMovieSubUpperAlpha != 0 ||
                sMovieSubLowerTimer > 0 || sMovieSubLowerAlpha != 0) {
                REG_DISPCNT |= DISPCNT_OBJ_ON;
                REG_BLDCNT = (BLDCNT_TGT1_OBJ | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3);

                if (sMovieSubUpperAlpha < 16) {
                    if (sMovieSubUpperAlpha == 0) {
                        attr0 = OAM_DISABLE;
                    } else {
                        REG_BLDALPHA = ((16 - sMovieSubUpperAlpha) << 8) | sMovieSubUpperAlpha;
                        attr0 = OAM_BLEND;
                    }
                } else {
                    attr0 = 0;
                }

                if (sMovieSubLowerAlpha < 16) {
                    if (sMovieSubLowerAlpha == 0) {
                        attr1 = OAM_DISABLE;
                    } else {
                        REG_BLDALPHA = ((16 - sMovieSubLowerAlpha) << 8) | sMovieSubLowerAlpha;
                        attr1 = OAM_BLEND;
                    }
                } else {
                    attr1 = 0;
                }

                oam = (u16*)OAM;

#ifndef VERSION_JP
                x = 0;
                y = 0;

                if (sMovieSubUpperLength != 0) {
                    x = sMovieSubUpper->x;
                    x += GetCenteredTextX(sMovieSubUpperWidths, sMovieSubUpperLength);
                    y = 0x74;
                }
#endif

                for (i = 0; i < sMovieSubUpperLength; i++) {
                    u16 tile;
                    u16 palette;
#ifdef VERSION_JP
                    const MovieSub* sub;
                    s16 x;
#endif

                    tile = i * 4;
#ifdef VERSION_JP
                    sub = sMovieSubUpper;
                    x = sub->x;
                    palette = (sub->palette & 15) << 12;
                    oam[0] = attr0 | 0x74;
                    oam[1] = (x + i * 10) | OAM_SIZE(1);
#else
                    palette = (sMovieSubUpper->palette & 15) << 12;
                    oam[0] = attr0 | y;
                    oam[1] = x | OAM_SIZE(1);
#endif
                    oam[2] = palette | (tile + 0x200);
#ifdef PLATFORM_PC
                    PortOamSetText((int)((oam - (u16*)OAM) / 4), sMovieSubUpperCodes[i], 1);
#endif
                    oam += 4;
#ifndef VERSION_JP
                    x += sMovieSubUpperWidths[i];
#endif
                }

#ifndef VERSION_JP
                if (sMovieSubLowerLength != 0) {
                    x = sMovieSubLower->x;
                    x += GetCenteredTextX(sMovieSubLowerWidths, sMovieSubLowerLength);
                    y = 0x84;
                }
#endif

                for (i = 0; i < sMovieSubLowerLength; i++) {
                    u16 tile;
                    u16 palette;
#ifdef VERSION_JP
                    const MovieSub* sub;
                    s16 x;
#endif

                    tile = i * 4;
#ifdef VERSION_JP
                    sub = sMovieSubLower;
                    x = sub->x;
                    palette = (sub->palette & 15) << 12;
                    oam[0] = attr1 | 0x84;
                    oam[1] = (x + i * 10) | OAM_SIZE(1);
#else
                    palette = (sMovieSubLower->palette & 15) << 12;
                    oam[0] = attr1 | y;
                    oam[1] = x | OAM_SIZE(1);
#endif
                    oam[2] = palette | (tile + 0x300);
#ifdef PLATFORM_PC
                    PortOamSetText((int)((oam - (u16*)OAM) / 4), sMovieSubLowerCodes[i], 1);
#endif
                    oam += 4;
#ifndef VERSION_JP
                    x += sMovieSubLowerWidths[i];
#endif
                }

                for (i = sMovieSubUpperLength + sMovieSubLowerLength;
#ifdef VERSION_JP
                     i < 48;
#elif defined(VERSION_EU)
                     i < 96;
#else
                     i < 80;
#endif
                     i++) {
                    oam[0] = OAM_DISABLE;
#ifdef PLATFORM_PC
                    PortOamSetText((int)((oam - (u16*)OAM) / 4), 0, 0);
#endif
                    oam += 4;
                }

                if (sMovieSubUpperTimer > 0) {
                    if (sMovieSubUpperAlpha < 16) {
                        sMovieSubUpperAlpha += 4;
                    }
                } else if (sMovieSubUpperAlpha != 0) {
                    sMovieSubUpperAlpha -= 4;

                    if (sMovieSubUpperAlpha == 0) {
                        sMovieSubUpperLength = 0;
                    }
                }

                if (sMovieSubLowerTimer > 0) {
                    if (sMovieSubLowerAlpha < 16) {
                        sMovieSubLowerAlpha += 4;
                    }
                } else if (sMovieSubLowerAlpha != 0) {
                    sMovieSubLowerAlpha -= 4;

                    if (sMovieSubLowerAlpha == 0) {
                        sMovieSubLowerLength = 0;
                    }
                }
            } else {
                REG_DISPCNT &= ~DISPCNT_OBJ_ON;
            }
        }
    }

    gIntrCheck |= INTR_FLAG_VBLANK;
}

void mode_movie_1() {
    void* movie;

    switch (sMovieModeState) {
    case MOVIE_MODE_STATE_CLEAR: {
        InitDisplayRegs();
        gDispCnt &= ~(DISPCNT_BG_ALL_ON | DISPCNT_OBJ_ON);
        CpuFill32(0, (void*)VRAM, VRAM_SIZE);
        sMovieModeState++;
        break;
    }
    case MOVIE_MODE_STATE_WAIT:
        sMovieModeState++;
        break;
    case MOVIE_MODE_STATE_PLAY:
#ifdef PLATFORM_PC
        /* the player's imported HD movie (port/progress/HD_MOD.md) plays instead; the game goes on as after its own */
        if (PortHdMoviePlay(sMovieId)) {
            sMovieModeState = MOVIE_MODE_STATE_EXIT;
            break;
        }
#endif
        m4aSoundVSyncOff();
        gVBlankHandlerOverride = MovieVBlankIntr;
        IwramHeapInit(GetIwramHeapStart(), GetIwramHeapSize());
        EwramHeapInit(GetEwramHeapStart(), GetEwramHeapSize());
        SetEwramHeapName(sMovieHeapName);
        SetIwramHeapName(sMovieHeapName);
        CpuCopy16(gMovieObjPalette, (void*)OBJ_PLTT, sizeof(gMovieObjPalette));
        CpuCopy16(gTextWhitePalette, (void*)(OBJ_PLTT + PLTT_SIZE_4BPP), sizeof(gTextWhitePalette));
        MovieSetCallbacks(IwramAlloc, EwramAlloc, IwramFree, EwramFree);

        switch (sMovieId) {
        case MOVIE_OPENING:
            movie = gMovieOpening;

#ifdef VERSION_EU
            switch (gLanguage) {
            case LANGUAGE_ENGLISH:
                sMovieSubs = gMovieSubsOpeningEn;
                sMovieSubCount = 3;
                break;
            case LANGUAGE_FRENCH:
                sMovieSubs = gMovieSubsOpeningFr;
                sMovieSubCount = 4;
                break;
            case LANGUAGE_GERMAN:
                sMovieSubs = gMovieSubsOpeningDe;
                sMovieSubCount = 4;
                break;
            case LANGUAGE_ITALIAN:
                sMovieSubs = gMovieSubsOpeningIt;
                sMovieSubCount = 4;
                break;
            case LANGUAGE_SPANISH:
            default:
                sMovieSubs = gMovieSubsOpeningEs;
                sMovieSubCount = 4;
                break;
            }
#else
            sMovieSubs = gMovieSubsOpening;
            sMovieSubCount = 3;
#endif
            break;
        case MOVIE_6F_GOAL:
            movie = gMovieEvent026;
            sMovieSubs = NULL;
            sMovieSubCount = 0;
            break;
        case MOVIE_12F_E2:
            movie = gMovieEvent057;
            sMovieSubs = NULL;
            sMovieSubCount = 0;
            break;
        case MOVIE_ENDING:
            movie = gMovieEnding;

#ifdef VERSION_EU
            switch (gLanguage) {
            case LANGUAGE_ENGLISH:
                sMovieSubs = gMovieSubsEndingEn;
                sMovieSubCount = 14;
                break;
            case LANGUAGE_FRENCH:
                sMovieSubs = gMovieSubsEndingFr;
                sMovieSubCount = 14;
                break;
            case LANGUAGE_GERMAN:
                sMovieSubs = gMovieSubsEndingDe;
                sMovieSubCount = 14;
                break;
            case LANGUAGE_ITALIAN:
                sMovieSubs = gMovieSubsEndingIt;
                sMovieSubCount = 14;
                break;
            case LANGUAGE_SPANISH:
            default:
                sMovieSubs = gMovieSubsEndingEs;
                sMovieSubCount = 14;
                break;
            }
#else
            sMovieSubs = gMovieSubsEnding;
#ifdef VERSION_JP
            sMovieSubCount = 12;
#else
            sMovieSubCount = 14;
#endif
#endif
            break;
#ifdef VERSION_EU
        default:
#endif
        case MOVIE_RIKU_ENDING:
            movie = gMovieRikuEnding;

#ifdef VERSION_EU
            switch (gLanguage) {
            case LANGUAGE_ENGLISH:
                sMovieSubs = gMovieSubsRikuEndingEn;
                sMovieSubCount = 10;
                break;
            case LANGUAGE_FRENCH:
                sMovieSubs = gMovieSubsRikuEndingFr;
                sMovieSubCount = 12;
                break;
            case LANGUAGE_GERMAN:
                sMovieSubs = gMovieSubsRikuEndingDe;
                sMovieSubCount = 11;
                break;
            case LANGUAGE_ITALIAN:
                sMovieSubs = gMovieSubsRikuEndingIt;
                sMovieSubCount = 10;
                break;
            case LANGUAGE_SPANISH:
            default:
                sMovieSubs = gMovieSubsRikuEndingEs;
                sMovieSubCount = 11;
                break;
            }
#else
            sMovieSubs = gMovieSubsRikuEnding;
#ifdef VERSION_JP
            sMovieSubCount = 8;
#else
            sMovieSubCount = 10;
#endif
#endif
            break;
#ifndef VERSION_EU
        default:
            movie = gMovieEnding;
            sMovieSubs = gMovieSubsOpening;
            sMovieSubCount = 3;
            break;
#endif
        }

        if (MovieStart(movie)) {
            sMovieFlags |= MOVIE_FLAG_PLAYING;
            MoviePlay(HandleMovieFrame, 0);
            sMovieFlags &= ~MOVIE_FLAG_PLAYING;
#ifdef PLATFORM_PC
            PortUiSkipPrompt(FALSE);
#endif
        }

        MovieClose();
        IwramHeapInit(GetIwramHeapStart(), GetIwramHeapSize());
        EwramHeapInit(GetEwramHeapStart(), GetEwramHeapSize());
        VTransInit();
        SpriteInit();
        BgInit();
        FadeInit();
        PalletInit();
        SioKeyInit();
        VTransReset();
        BgReset();
        SpriteReset();
        FadeReset();
        MosaicReset();
        InitDisplayRegs();
        gVBlankHandlerOverride = NULL;
        m4aSoundInit();
        m4aSoundVSyncOn();
        sMovieModeState++;
        break;
    case MOVIE_MODE_STATE_EXIT: {
        CpuFill32(0, (void*)VRAM, VRAM_SIZE);

        if (sMovieFlags & MOVIE_FLAG_SOFT_RESET) {
#ifdef VERSION_EU
            DoSoftReset();
#else
            SoftReset(RESET_ALL);
#endif
#ifdef VERSION_EU
        } else if (gDebugFlags & DEBUG_FLAG_DEBUG_MENU) {
            ModeRequest(&gModeMovieDebugEu, 0);
#endif
        } else {
            switch (sMovieId) {
            case MOVIE_OPENING:
                RequestEventMode(EVENT_000_1F_ENTRANCE_PART1);
                break;
            case MOVIE_6F_GOAL:
                RequestEventMode(EVENT_026_6F_GOAL_2);
                break;
            case MOVIE_12F_E2:
                RequestEventMode(EVENT_057_12F_DESTINY_ISLAND_E3);
                break;
            case MOVIE_ENDING:
                ModeRequest(&gModeStaffRoll, 0);
                break;
            case MOVIE_RIKU_ENDING:
                ModeRequest(&gModeStaffRoll, 0);
                break;
            default:
                ModeRequest(&gModeDebug, 0);
                break;
            }
        }

        sMovieModeState++;
        break;
    }
    }
}

void mode_movie_2() {
    gVBlankHandlerOverride = NULL;
}

Mode gModeMovie = {
    "mode_movie",
    mode_movie_0,
    mode_movie_1,
    mode_movie_2,
};
