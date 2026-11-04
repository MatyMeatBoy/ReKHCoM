/**
 * sroll_tmr.c
 * Staff Roll Text and Audio Streaming
 */

#include "audio_block_codec.h"
#include "pcm_audio.h"
#include "m4a.h"
#include "sroll.h"
#include "sprites_staff_roll.h"
#include "gba/keys.h"
#include "sroll_api.h"
#include "fade.h"
#include <stddef.h>
#include "display.h"
#include "gba/macro.h"
#include "gba/syscall.h"
#include "intr.h"
#include "key.h"
#include "obj_api.h"
#include "taskpool.h"
#include "types.h"
#include "sprite_palettes.h"
#include "staff_roll_audio.h"
#include "staff_roll_font.h"

#define SROLL_FONT_PAGE_NONE 0xFFFF
#define SROLL_FONT_GLYPH_NONE 0xFF

static DmaStream sDmaStream;
static u8 sBlockAudioPlaying;
static s32 sDecodedAudioBuffer[0x810];
static u32* sAudioBlockNext;
static s32 sDecodedAudioWritePosition;
static s32 sDecodedAudioReadPosition;

static s32 Square(s32 x) {
    return x * x;
}

void task_sroll_tmr_0(SrollTmrWork* work, void* arg) {
    work->visible = 0;
    work->frameCount = 0;
    work->tiles = LoadObjTiles(gSrollTimerTiles, sizeof(gSrollTimerTiles));
    work->palette = LoadObjPalette(gSrollTimerPalette, sizeof(gSrollTimerPalette));
}

u8 task_sroll_tmr_1(SrollTmrWork* work) {
    u8 alive;

    alive = 1;

    if (GetKeysPressed() & SELECT_BUTTON) {
        if (work->visible == 1) {
            work->visible = 0;
        } else {
            work->visible = alive;
        }
    }

    FadeSetPaletteExcluded((BIOS_PTR(work->palette)->index & 15) + 16, TRUE);
    work->frameCount++;
    return alive;
}

void task_sroll_tmr_2(SrollTmrWork* work) {
    s32 frames;
    u16 hours;
    u16 minutes;
    u16 seconds;
    u16 zero;

    if (work->visible == 0) {
        return;
    }

    frames = work->frameCount;
    hours = frames / 3600;
    minutes = frames / 60 % 60;
    seconds = frames % 60;
    zero = 0;
    DrawSprite(8, 8, gSrollTimerFrames[hours / 10 % 10], work->tiles, work->palette, NULL, zero, zero);
    DrawSprite(16, 8, gSrollTimerFrames[hours % 10], work->tiles, work->palette, NULL, zero, zero);
    DrawSprite(24, 8, gSrollTimerFrames[10], work->tiles, work->palette, NULL, zero, zero);
    DrawSprite(32, 8, gSrollTimerFrames[minutes / 10], work->tiles, work->palette, NULL, zero, zero);
    DrawSprite(40, 8, gSrollTimerFrames[minutes % 10], work->tiles, work->palette, NULL, zero, zero);
    DrawSprite(48, 8, gSrollTimerFrames[10], work->tiles, work->palette, NULL, zero, zero);
    DrawSprite(56, 8, gSrollTimerFrames[seconds / 10], work->tiles, work->palette, NULL, zero, zero);
    DrawSprite(64, 8, gSrollTimerFrames[seconds % 10], work->tiles, work->palette, NULL, zero, zero);
}

void task_sroll_tmr_3(SrollTmrWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void SrollBlit1bppWidth0() {
}

void SrollBlit1bppWidth1(u32* dst, u8* src, u32* pal, s32 x) {
    SrollShift* shifts;
    s32 i;
    u32 pixels;
    u32 bits;

    shifts = &gStaffRollBlitShifts[x];

    for (i = 0; i <= 7; i++) {
        bits = src[i] << 24;
        pixels = pal[(bits >> 31) & 1];
        dst[i] |= pixels << shifts->shift;
        dst[i + 8] |= pixels >> shifts->spillShift;
    }
}

void SrollBlit1bppWidth2(u32* dst, u8* src, u32* pal, s32 x) {
    SrollShift* shifts;
    s32 i;
    u32 pixels;
    u32 bits;

    shifts = &gStaffRollBlitShifts[x];

    for (i = 0; i <= 7; i++) {
        bits = src[i] << 24;
        pixels = pal[(bits >> 31) & 1];
        pixels |= pal[(bits >> 30) & 1] << 4;
        dst[i] |= pixels << shifts->shift;
        dst[i + 8] |= pixels >> shifts->spillShift;
    }
}

void SrollBlit1bppWidth3(u32* dst, u8* src, u32* pal, s32 x) {
    SrollShift* shifts;
    s32 i;
    u32 pixels;
    u32 bits;

    shifts = &gStaffRollBlitShifts[x];

    for (i = 0; i <= 7; i++) {
        bits = src[i] << 24;
        pixels = pal[(bits >> 31) & 1];
        pixels |= pal[(bits >> 30) & 1] << 4;
        pixels |= pal[(bits >> 29) & 1] << 8;
        dst[i] |= pixels << shifts->shift;
        dst[i + 8] |= pixels >> shifts->spillShift;
    }
}

void SrollBlit1bppWidth4(u32* dst, u8* src, u32* pal, s32 x) {
    SrollShift* shifts;
    s32 i;
    u32 pixels;
    u32 bits;

    shifts = &gStaffRollBlitShifts[x];

    for (i = 0; i <= 7; i++) {
        bits = src[i] << 24;
        pixels = pal[(bits >> 31) & 1];
        pixels |= pal[(bits >> 30) & 1] << 4;
        pixels |= pal[(bits >> 29) & 1] << 8;
        pixels |= pal[(bits >> 28) & 1] << 12;
        dst[i] |= pixels << shifts->shift;
        dst[i + 8] |= pixels >> shifts->spillShift;
    }
}

void SrollBlit1bppWidth5(u32* dst, u8* src, u32* pal, s32 x) {
    SrollShift* shifts;
    s32 i;
    u32 pixels;
    u32 bits;

    shifts = &gStaffRollBlitShifts[x];

    for (i = 0; i <= 7; i++) {
        bits = src[i] << 24;
        pixels = pal[(bits >> 31) & 1];
        pixels |= pal[(bits >> 30) & 1] << 4;
        pixels |= pal[(bits >> 29) & 1] << 8;
        pixels |= pal[(bits >> 28) & 1] << 12;
        pixels |= pal[(bits >> 27) & 1] << 16;
        dst[i] |= pixels << shifts->shift;
        dst[i + 8] |= pixels >> shifts->spillShift;
    }
}

void SrollBlit1bppWidth6(u32* dst, u8* src, u32* pal, s32 x) {
    SrollShift* shifts;
    s32 i;
    u32 pixels;
    u32 bits;

    shifts = &gStaffRollBlitShifts[x];

    for (i = 0; i <= 7; i++) {
        bits = src[i] << 24;
        pixels = pal[(bits >> 31) & 1];
        pixels |= pal[(bits >> 30) & 1] << 4;
        pixels |= pal[(bits >> 29) & 1] << 8;
        pixels |= pal[(bits >> 28) & 1] << 12;
        pixels |= pal[(bits >> 27) & 1] << 16;
        pixels |= pal[(bits >> 26) & 1] << 20;
        dst[i] |= pixels << shifts->shift;
        dst[i + 8] |= pixels >> shifts->spillShift;
    }
}

void SrollBlit1bppWidth7(u32* dst, u8* src, u32* pal, s32 x) {
    SrollShift* shifts;
    s32 i;
    u32 pixels;
    u32 bits;

    shifts = &gStaffRollBlitShifts[x];

    for (i = 0; i <= 7; i++) {
        bits = src[i] << 24;
        pixels = pal[(bits >> 31) & 1];
        pixels |= pal[(bits >> 30) & 1] << 4;
        pixels |= pal[(bits >> 29) & 1] << 8;
        pixels |= pal[(bits >> 28) & 1] << 12;
        pixels |= pal[(bits >> 27) & 1] << 16;
        pixels |= pal[(bits >> 26) & 1] << 20;
        pixels |= pal[(bits >> 25) & 1] << 24;
        dst[i] |= pixels << shifts->shift;
        dst[i + 8] |= pixels >> shifts->spillShift;
    }
}

void SrollBlit1bppWidth8(u32* dst, u8* src, u32* pal, s32 x) {
    SrollShift* shifts;
    s32 i;
    u32 pixels;

    shifts = &gStaffRollBlitShifts[x];

    for (i = 0; i <= 7; i++) {
        pixels = pal[(src[i] >> 7) & 1];
        pixels |= pal[(src[i] >> 6) & 1] << 4;
        pixels |= pal[(src[i] >> 5) & 1] << 8;
        pixels |= pal[(src[i] >> 4) & 1] << 12;
        pixels |= pal[(src[i] >> 3) & 1] << 16;
        pixels |= pal[(src[i] >> 2) & 1] << 20;
        pixels |= pal[(src[i] >> 1) & 1] << 24;
        pixels |= pal[src[i] & 1] << 28;
        dst[i] |= pixels << shifts->shift;
        dst[i + 8] |= pixels >> shifts->spillShift;
    }
}

u32 SrollTextBlit1bpp(SrollBlit* blit) {
    SrollMask* mask;
    u32* dst;
    u32* buf;
    u32 keep;
    s32 end;
    u32 tileCount;

    end = blit->x + blit->width;
    buf = blit->buf;
    dst = blit->dst;
    mask = &gStaffRollBlitMasks[blit->width][blit->x];
    keep = mask->keepLeft | mask->keepRight;
    buf[0] = dst[0] & keep;
    buf[1] = dst[1] & keep;
    buf[2] = dst[2] & keep;
    buf[3] = dst[3] & keep;
    buf[4] = dst[4] & keep;
    buf[5] = dst[5] & keep;
    buf[6] = dst[6] & keep;
    buf[7] = dst[7] & keep;

    if (end > 8) {
        keep = mask->keepNext;
        buf[8] = dst[8] & keep;
        buf[9] = dst[9] & keep;
        buf[10] = dst[10] & keep;
        buf[11] = dst[11] & keep;
        buf[12] = dst[12] & keep;
        buf[13] = dst[13] & keep;
        buf[14] = dst[14] & keep;
        buf[15] = dst[15] & keep;
    }

    gStaffRollBlit1bppFuncs[blit->width](buf, blit->src, blit->colors, blit->x);
    dst[0] = buf[0];
    dst[1] = buf[1];
    dst[2] = buf[2];
    dst[3] = buf[3];
    dst[4] = buf[4];
    dst[5] = buf[5];
    dst[6] = buf[6];
    dst[7] = buf[7];

    if (end > 8) {
        dst[8] = buf[8];
        dst[9] = buf[9];
        dst[10] = buf[10];
        dst[11] = buf[11];
        dst[12] = buf[12];
        dst[13] = buf[13];
        dst[14] = buf[14];
        dst[15] = buf[15];
    }

    tileCount = 1;

    if (end > 8) {
        tileCount = 2;
    }

    return tileCount;
}

void SrollBlit2bppWidth0() {
}

void SrollBlit2bppWidth1(u32* dst, u16* src, u32* pal, s32 x) {
    SrollShift* shifts;
    u32* dstRow;
    u16* srcRow;
    s32 i;
    u32 pixels;
    u32 bits;

    shifts = &gStaffRollBlitShifts[x];
    dstRow = dst;
    srcRow = src;

    for (i = 0; i <= 7; i++) {
        bits = *srcRow << 16;
        pixels = pal[(bits >> 22) & 3];
        dstRow[0] |= pixels << shifts->shift;
        dstRow[8] |= pixels >> shifts->spillShift;
        dstRow++;
        srcRow++;
    }
}

void SrollBlit2bppWidth2(u32* dst, u16* src, u32* pal, s32 x) {
    SrollShift* shifts;
    s32 i;
    u32 pixels;
    u32 bits;

    shifts = &gStaffRollBlitShifts[x];

    for (i = 0; i <= 7; i++) {
        bits = src[i] << 16;
        pixels = pal[(bits >> 22) & 3];
        pixels |= pal[(bits >> 20) & 3] << 4;
        dst[i] |= pixels << shifts->shift;
        dst[i + 8] |= pixels >> shifts->spillShift;
    }
}

void SrollBlit2bppWidth3(u32* dst, u16* src, u32* pal, s32 x) {
    SrollShift* shifts;
    s32 i;
    u32 pixels;
    u32 bits;

    shifts = &gStaffRollBlitShifts[x];

    for (i = 0; i <= 7; i++) {
        bits = src[i] << 16;
        pixels = pal[(bits >> 22) & 3];
        pixels |= pal[(bits >> 20) & 3] << 4;
        pixels |= pal[(bits >> 18) & 3] << 8;
        dst[i] |= pixels << shifts->shift;
        dst[i + 8] |= pixels >> shifts->spillShift;
    }
}

void SrollBlit2bppWidth4(u32* dst, u16* src, u32* pal, s32 x) {
    SrollShift* shifts;
    s32 i;
    u32 pixels;

    shifts = &gStaffRollBlitShifts[x];

    for (i = 0; i <= 7; i++) {
        pixels = pal[(src[i] >> 6) & 3];
        pixels |= pal[(src[i] >> 4) & 3] << 4;
        pixels |= pal[(src[i] >> 2) & 3] << 8;
        pixels |= pal[src[i] & 3] << 12;
        dst[i] |= pixels << shifts->shift;
        dst[i + 8] |= pixels >> shifts->spillShift;
    }
}

void SrollBlit2bppWidth5(u32* dst, u16* src, u32* pal, s32 x) {
    SrollShift* shifts;
    s32 i;
    u32 pixels;

    shifts = &gStaffRollBlitShifts[x];

    for (i = 0; i <= 7; i++) {
        pixels = pal[(src[i] >> 6) & 3];
        pixels |= pal[(src[i] >> 4) & 3] << 4;
        pixels |= pal[(src[i] >> 2) & 3] << 8;
        pixels |= pal[src[i] & 3] << 12;
        pixels |= pal[(src[i] >> 14) & 3] << 16;
        dst[i] |= pixels << shifts->shift;
        dst[i + 8] |= pixels >> shifts->spillShift;
    }
}

void SrollBlit2bppWidth6(u32* dst, u16* src, u32* pal, s32 x) {
    SrollShift* shifts;
    s32 i;
    u32 pixels;

    shifts = &gStaffRollBlitShifts[x];

    for (i = 0; i <= 7; i++) {
        pixels = pal[(src[i] >> 6) & 3];
        pixels |= pal[(src[i] >> 4) & 3] << 4;
        pixels |= pal[(src[i] >> 2) & 3] << 8;
        pixels |= pal[src[i] & 3] << 12;
        pixels |= pal[(src[i] >> 14) & 3] << 16;
        pixels |= pal[(src[i] >> 12) & 3] << 20;
        dst[i] |= pixels << shifts->shift;
        dst[i + 8] |= pixels >> shifts->spillShift;
    }
}

void SrollBlit2bppWidth7(u32* dst, u16* src, u32* pal, s32 x) {
    SrollShift* shifts;
    s32 i;
    u32 pixels;

    shifts = &gStaffRollBlitShifts[x];

    for (i = 0; i <= 7; i++) {
        pixels = pal[(src[i] >> 6) & 3];
        pixels |= pal[(src[i] >> 4) & 3] << 4;
        pixels |= pal[(src[i] >> 2) & 3] << 8;
        pixels |= pal[src[i] & 3] << 12;
        pixels |= pal[(src[i] >> 14) & 3] << 16;
        pixels |= pal[(src[i] >> 12) & 3] << 20;
        pixels |= pal[(src[i] >> 10) & 3] << 24;
        dst[i] |= pixels << shifts->shift;
        dst[i + 8] |= pixels >> shifts->spillShift;
    }
}

void SrollBlit2bppWidth8(u32* dst, u16* src, u32* pal, s32 x) {
    SrollShift* shifts;
    s32 i;
    u32 pixels;

    shifts = &gStaffRollBlitShifts[x];

    for (i = 0; i <= 7; i++) {
        pixels = pal[(src[i] >> 6) & 3];
        pixels |= pal[(src[i] >> 4) & 3] << 4;
        pixels |= pal[(src[i] >> 2) & 3] << 8;
        pixels |= pal[src[i] & 3] << 12;
        pixels |= pal[(src[i] >> 14) & 3] << 16;
        pixels |= pal[(src[i] >> 12) & 3] << 20;
        pixels |= pal[(src[i] >> 10) & 3] << 24;
        pixels |= pal[(src[i] >> 8) & 3] << 28;
        dst[i] |= pixels << shifts->shift;
        dst[i + 8] |= pixels >> shifts->spillShift;
    }
}

u32 SrollTextBlit2bpp(SrollBlit* blit) {
    SrollMask* mask;
    u32* dst;
    u32* buf;
    u32 keep;
    s32 end;
    u32 tileCount;

    end = blit->x + blit->width;
    buf = blit->buf;
    dst = blit->dst;
    mask = &gStaffRollBlitMasks[blit->width][blit->x];
    keep = mask->keepLeft | mask->keepRight;
    buf[0] = dst[0] & keep;
    buf[1] = dst[1] & keep;
    buf[2] = dst[2] & keep;
    buf[3] = dst[3] & keep;
    buf[4] = dst[4] & keep;
    buf[5] = dst[5] & keep;
    buf[6] = dst[6] & keep;
    buf[7] = dst[7] & keep;

    if (end > 8) {
        keep = mask->keepNext;
        buf[8] = dst[8] & keep;
        buf[9] = dst[9] & keep;
        buf[10] = dst[10] & keep;
        buf[11] = dst[11] & keep;
        buf[12] = dst[12] & keep;
        buf[13] = dst[13] & keep;
        buf[14] = dst[14] & keep;
        buf[15] = dst[15] & keep;
    }

    gStaffRollBlit2bppFuncs[blit->width](buf, blit->src, blit->colors, blit->x);
    dst[0] = buf[0];
    dst[1] = buf[1];
    dst[2] = buf[2];
    dst[3] = buf[3];
    dst[4] = buf[4];
    dst[5] = buf[5];
    dst[6] = buf[6];
    dst[7] = buf[7];

    if (end > 8) {
        dst[8] = buf[8];
        dst[9] = buf[9];
        dst[10] = buf[10];
        dst[11] = buf[11];
        dst[12] = buf[12];
        dst[13] = buf[13];
        dst[14] = buf[14];
        dst[15] = buf[15];
    }

    tileCount = 1;

    if (end > 8) {
        tileCount = 2;
    }

    return tileCount;
}

u16 SrollTextGetGlyphIndex(u16 ch, u8* font) {
    u16 result;
    s32 off;
    s32 hi;
    s32 tableOffset;
    s32 firstGlyph;
    u8 pageGlyph;

    result = 0;
    off = (ch & 0x7F00) >> 6;
    hi = font[off + 1] << 8;
    tableOffset = font[off] | hi;
    hi = font[off + 3] << 8;
    firstGlyph = font[off + 2] | hi;

    if (tableOffset != SROLL_FONT_PAGE_NONE) {
        if (font[(u16)(tableOffset + 0xFFC0 + (ch & 0xFF))] != SROLL_FONT_GLYPH_NONE) {
            pageGlyph = font[(u16)(tableOffset + 0xFFC0 + (ch & 0xFF))];
            result = firstGlyph + pageGlyph;
        }
    }

    return result;
}

u8 SrollTextGetGlyphWidth(u16 ch, u8* font, u8* widths, u32 count) {
    u8 width;

    width = 0;

    if (widths != NULL) {
        u16 idx = SrollTextGetGlyphIndex(ch, font);

        if (idx < count) {
            width = widths[idx];
        }
    }

    if (width == 0) {
        width = 8;
    }

    return width;
}

s32 SrollTextMeasureWidth(SrollWork* work, const u8* str) {
    s32 total;
    u16 ch;
    s32 hi;

    total = 0;

    while (*str != 0) {
        if (*str & 0x80) {
            hi = str[0] << 8;
            ch = str[1] | hi;
            str += 2;
        } else {
            ch = SrollTextMapSingleByteChar(str[0]);
            str += 1;
        }

        total += SrollTextGetGlyphWidth(ch, work->fontPages, work->fontWidths, work->fontGlyphCount);
    }

    return total;
}

u8* SrollTextGetGlyphAddress(u16 ch, u8* font, u8* base, u16 bpp, u16 height) {
    return base + SrollTextGetGlyphIndex(ch, font) * (bpp << 3) * height;
}

u32 SrollTextBlitGlyph(SrollWork* work, u32* dst, u8* src, s32 width) {
    SrollBlit blit;
    u32 pal[16];
    u32 bg;
    u32 tileCount;
    u16 bgColor;
    u16 n;

    tileCount = 0;
    pal[0] = bgColor = work->bgColor;
    pal[1] = work->fgColor;
    pal[2] = work->shadowColor;
    pal[3] = work->edgeColor;
    bg = (bgColor << 4) | bgColor;
    bg |= bg << 8;
    bg |= bg << 16;
    blit.x = work->x & 7;
    blit.width = width;
    blit.src = src;
    blit.dst = dst;
    blit.colors = pal;
    n = work->glyphHeight;

    if (n-- != 0) {
        do {
            if (blit.x == 0) {
                CpuFastFill(bg, blit.dst, 32);
            }

            switch (work->glyphBpp) {
            case 1:
                tileCount = SrollTextBlit1bpp(&blit);
                break;
            case 2:
                tileCount = SrollTextBlit2bpp(&blit);
                break;
            }

            blit.src += work->glyphBpp * 8;
            blit.dst += work->textWidth * 8;
        } while (n-- != 0);
    }

    return tileCount;
}

void SrollTextSelectFont(SrollWork* work, u32 font) {
    if (font > 1) {
        font = 0;
    }

    work->glyphBpp = gStaffRollFonts[font].bpp;
    work->glyphHeight = gStaffRollFonts[font].height;
    work->fontPages = gStaffRollFonts[font].pages;
    work->fontGlyphs = gStaffRollFonts[font].glyphs;
    work->fontWidths = gStaffRollFonts[font].widths;
    work->fontGlyphCount = gStaffRollFonts[font].glyphCount;
    work->unk_44 = gStaffRollFonts[font].unk_14;
    SrollTextSetCursorTile(work, work->x, work->y);
}

void SrollTextInit(SrollWork* work, const SrollInit* init) {
    SrollTextSelectFont(work, init->font);
    SrollTextSetColors(work, init->fgColor, init->shadowColor, init->bgColor, init->edgeColor);
    work->mapWidth = 32;
    work->flags = 0;
    work->frameStyle = init->frameStyle;
    work->windowX = init->windowX;
    work->windowY = init->windowY;
    work->windowWidth = init->windowWidth;
    work->windowHeight = init->windowHeight;
    work->textX = init->textX;
    work->textY = init->textY;
    work->textWidth = init->textWidth;
    work->textHeight = init->textHeight;
    work->clearTile = init->clearTile;
    work->frameTileBase = init->frameTileBase;
    work->textTileBase = init->textTileBase;
    work->unk_48 = init->unk_10;
    work->tilemapBuffer = init->tilemapBuffer;
    work->tileData = init->tileData;
    work->tilemap = init->tilemap;
    SrollTextClearQueue(work);
    SrollTextResetWindow(work, TRUE);
}

void SrollTextClearWindowImmediate(SrollWork* work) {
    SrollTextClearWindow(work, TRUE);
}

void SrollTextClearQueue(SrollWork* work) {
    work->writeIdx = 0;
    work->readIdx = 0;
}

u8 SrollTextQueueIsEmpty(SrollWork* work) {
    u8 empty;

    empty = FALSE;

    if (work->writeIdx == work->readIdx) {
        empty = TRUE;
    }

    return empty;
}

void SrollTextEnqueueChar(SrollWork* work, u16 ch) {
    work->charQueue[work->writeIdx] = ch;
    work->writeIdx = (work->writeIdx + 1) & 0xFF;
}

u16 SrollTextDequeueChar(SrollWork* work) {
    u16 ch;

    if (work->writeIdx == work->readIdx) {
        return 0;
    }

    ch = work->charQueue[work->readIdx];
    work->readIdx = (work->readIdx + 1) & 0xFF;
    return ch;
}

void SrollTextSetCursorTile(SrollWork* work, u16 x, u16 y) {
    if (x >= work->textWidth) {
        x = 0;
    }

    if (y + work->glyphHeight > work->textHeight) {
        y = 0;
    }

    work->x = x * 8;
    work->y = y;
}

void SrollTextSetCursorPixelX(SrollWork* work, u16 x) {
    if (x >= work->textWidth * 8) {
        x = 0;
    }

    work->x = x;
}

void SrollTextSetColors(SrollWork* work, u16 fgColor, u16 shadowColor, u16 bgColor, u16 edgeColor) {
    work->fgColor = fgColor;
    work->shadowColor = shadowColor;
    work->bgColor = bgColor;
    work->edgeColor = edgeColor;
}

void SrollTextClearWindow(SrollWork* work, u8 flush) {
    u16* row;
    u16 i;

    row = (u16*)(SrollTextGetTilemap(work) + work->windowY * work->mapWidth * 2 + work->windowX * 2);

    for (i = 0; i < work->windowHeight; i++) {
        CpuFill16(work->clearTile, row, work->windowWidth * sizeof(u16));
        row += work->mapWidth;
    }

    if (flush == TRUE) {
        SrollTextFlushTilemap(work);
    }
}

void SrollTextDrawFrame(SrollWork* work) {
    u16* row;
    u16 i;
    u16 baseTile;

    row = (u16*)(SrollTextGetTilemap(work) + work->windowY * work->mapWidth * 2 + work->windowX * 2);
    baseTile = work->frameTileBase + 1;
    CpuFill16(baseTile + 2, row + 1, (u32)(work->windowWidth - 2) << 1);
    row[0] = baseTile + 1;
    row[work->windowWidth - 1] = baseTile + 3;
    row += work->mapWidth;

    for (i = 1; i < work->windowHeight - 1; i++) {
        CpuFill16(baseTile, row + 1, (u32)(work->windowWidth - 2) << 1);
        row[0] = baseTile + 4;
        row[work->windowWidth - 1] = baseTile + 5;
        row += work->mapWidth;
    }

    CpuFill16(baseTile + 7, row + 1, (u32)(work->windowWidth - 2) << 1);
    row[0] = baseTile + 6;
    row[work->windowWidth - 1] = baseTile + 8;
}

void SrollTextDrawFrameTailLeft(SrollWork* work) {
    u16* row;
    u16 i;
    u16 baseTile;

    row = (u16*)(SrollTextGetTilemap(work) + work->windowY * work->mapWidth * 2 + work->windowX * 2);
    baseTile = work->frameTileBase + 1;
    CpuFill16(baseTile + 2, row + 2, (u32)(work->windowWidth - 3) << 1);
    row[1] = baseTile + 1;
    row[work->windowWidth - 1] = baseTile + 3;
    row += work->mapWidth;

    for (i = 1; i < work->windowHeight - 2; i++) {
        CpuFill16(baseTile, row + 2, (u32)(work->windowWidth - 3) << 1);
        row[1] = baseTile + 4;
        row[work->windowWidth - 1] = baseTile + 5;
        row += work->mapWidth;
    }

    CpuFill16(baseTile, row + 2, (u32)(work->windowWidth - 3) << 1);
    row[0] = baseTile + 10;
    row[1] = baseTile + 11;
    row[work->windowWidth - 1] = baseTile + 5;
    row += work->mapWidth;

    CpuFill16(baseTile + 7, row + 2, (u32)(work->windowWidth - 3) << 1);
    row[1] = baseTile + 6;
    row[work->windowWidth - 1] = baseTile + 8;
}

void SrollTextDrawFrameTailRight(SrollWork* work) {
    u16* row;
    u16 i;
    u16 baseTile;

    row = (u16*)(SrollTextGetTilemap(work) + work->windowY * work->mapWidth * 2 + work->windowX * 2);
    baseTile = work->frameTileBase + 1;
    CpuFill16(baseTile + 2, row + 1, (u32)(work->windowWidth - 3) << 1);
    row[0] = baseTile + 1;
    row[work->windowWidth - 2] = baseTile + 3;
    row += work->mapWidth;

    for (i = 1; i < work->windowHeight - 2; i++) {
        CpuFill16(baseTile, row + 1, (u32)(work->windowWidth - 3) << 1);
        row[0] = baseTile + 4;
        row[work->windowWidth - 2] = baseTile + 5;
        row += work->mapWidth;
    }

    CpuFill16(baseTile, row + 1, (u32)(work->windowWidth - 3) << 1);
    row[0] = baseTile + 4;
    row[work->windowWidth - 2] = (baseTile | 0x400) + 11;
    row[work->windowWidth - 1] = (baseTile | 0x400) + 10;
    row += work->mapWidth;

    CpuFill16(baseTile + 7, row + 1, (u32)(work->windowWidth - 3) << 1);
    row[0] = baseTile + 6;
    row[work->windowWidth - 2] = baseTile + 8;
}

void SrollTextClearTextArea(SrollWork* work) {
    u16* row;
    u16 i;
    u16 baseTile;

    row = (u16*)(SrollTextGetTilemap(work) + work->textY * work->mapWidth * 2 + work->textX * 2);
    baseTile = work->frameTileBase + 1;

    for (i = 0; i < work->textHeight; i++) {
        CpuFill16(baseTile, row, work->textWidth * sizeof(u16));
        row += work->mapWidth;
    }
}

void SrollTextResetWindow(SrollWork* work, u8 flush) {
    u16 flags;

    switch (work->frameStyle) {
    case 1:
        SrollTextDrawFrame(work);
        break;
    case 2:
        SrollTextDrawFrameTailLeft(work);
        break;
    case 3:
        SrollTextDrawFrameTailRight(work);
        break;
    default:
        SrollTextClearTextArea(work);
        break;
    }

    SrollTextSetCursorTile(work, 0, 0);

    if (flush == TRUE) {
        SrollTextFlushTilemap(work);
    } else {
        flags = work->flags | SROLL_FLAG_TILEMAP_DIRTY;
        work->flags = flags;
    }
}

void SrollTextClearRect(SrollWork* work, u16 x, u16 y, u16 width, u16 height, u8 flush) {
    u16* row;
    u16 i;
    u16 flags;
    u16 baseTile;

    if (x >= work->textWidth) {
        return;
    }

    if (y >= work->textHeight) {
        return;
    }

    if (x + width > work->textWidth) {
        width = work->textWidth - x;
    }

    if (y + height > work->textHeight) {
        height = work->textHeight - y;
    }

    row = (u16*)(SrollTextGetTilemap(work) + (work->textY + y) * work->mapWidth * 2 + (work->textX + x) * 2);
    baseTile = work->frameTileBase + 1;
    i = 0;

    while (i < height) {
        CpuFill16(baseTile, row, (u32)(width << 1));
        row += work->mapWidth;
        i++;
    }

    if (flush == TRUE) {
        SrollTextFlushTilemap(work);
    } else {
        flags = work->flags | SROLL_FLAG_TILEMAP_DIRTY;
        work->flags = flags;
    }
}

void func_081167CC() {
}

u16 ParseLowercaseHexDigit(u16 ch) {
    u16 digit;

    digit = ch;

    if ((u16)(digit - '0') <= 9) {
        digit -= '0';
    } else {
        digit -= 'a';
        digit += 10;
    }

    return digit;
}

u8* SrollTextEnqueueString(SrollWork* work, u8* str) {
    s32 hi;

    while (*str != 0) {
        if (*str & 0x80) {
            hi = str[0] << 8;
            SrollTextEnqueueChar(work, str[1] | hi);
            str += 2;
        } else {
            SrollTextEnqueueChar(work, SrollTextMapSingleByteChar(str[0]));
            str += 1;
        }
    }

    return str + 1;
}

u8 SrollTextProcessNextChar(SrollWork* work) {
    u16 cmd[2];
    u16* row;
    u32 off;
    u8* glyph;
    u32 tileCount;
    u16 ch;
    u16 tile;
    u16 i;
    u8 drawn;
    u8 width;

    drawn = FALSE;

    if (work->x >= work->textWidth * 8) {
        work->x = 0;
    }

    ch = SrollTextDequeueChar(work);

    if (ch & 0xFF00) {
        off = (work->y * work->textWidth + (work->x >> 3)) * 32;
        glyph = SrollTextGetGlyphAddress(ch, work->fontPages, work->fontGlyphs, work->glyphBpp, work->glyphHeight);
        width = SrollTextGetGlyphWidth(ch, work->fontPages, work->fontWidths, work->fontGlyphCount);
        tileCount = SrollTextBlitGlyph(work, (u32*)(work->tileData + off), glyph, width);
        tile = work->y * work->textWidth + (work->x >> 3) + work->textTileBase;
        row = (u16*)(SrollTextGetTilemap(work) +
                   ((work->textY + work->y) * work->mapWidth + ((work->x >> 3) + work->textX)) * 2);

        for (i = 0; i < work->glyphHeight; i++) {
            if (tileCount == 1) {
                row[0] = tile;
            } else {
                row[0] = tile;
                row[1] = tile + 1;
            }

            row += work->mapWidth;
            tile += work->textWidth;
        }

        work->x += width;
        work->flags |= SROLL_FLAG_TILEMAP_DIRTY;
        drawn = TRUE;
    } else {
        switch (ch) {
        case '@':
            cmd[0] = SrollTextDequeueChar(work);
            cmd[1] = SrollTextDequeueChar(work);

            switch (cmd[0]) {
            case 'F':
                SrollTextSelectFont(work, ParseLowercaseHexDigit(cmd[1]));
                break;
            case 'f':
                work->fgColor = ParseLowercaseHexDigit(cmd[1]);
                break;
            case 's':
                work->shadowColor = ParseLowercaseHexDigit(cmd[1]);
                break;
            case 'b':
                work->bgColor = ParseLowercaseHexDigit(cmd[1]);
                break;
            case 'e':
                work->edgeColor = ParseLowercaseHexDigit(cmd[1]);
                break;
            }

            break;
        case 0:
            break;
        case '\n':
            work->x = 0;
            work->y += work->glyphHeight;

            if (work->y + work->glyphHeight > work->textHeight) {
                work->y = 0;
            }

            break;
        }
    }

    return drawn;
}

void SrollTextDrawNextGlyph(SrollWork* work, u8 flush) {
    u8 drawn;

    drawn = FALSE;

    while (!SrollTextQueueIsEmpty(work) && !drawn) {
        drawn = SrollTextProcessNextChar(work);
    }

    if (flush == TRUE && (work->flags & SROLL_FLAG_TILEMAP_DIRTY)) {
        SrollTextFlushTilemap(work);
    }
}

void SrollTextDrawQueued(SrollWork* work, u8 flush) {
    while (!SrollTextQueueIsEmpty(work)) {
        SrollTextProcessNextChar(work);
    }

    if (flush == TRUE && (work->flags & SROLL_FLAG_TILEMAP_DIRTY)) {
        SrollTextFlushTilemap(work);
    }
}

u8* SrollTextGetTilemap(SrollWork* work) {
    u8* tilemap;

    tilemap = work->tilemapBuffer;

    if (tilemap == NULL) {
        tilemap = work->tilemap;
    }

    return tilemap;
}

void SrollTextFlushTilemap(SrollWork* work) {
    u32 off;

    if (work->tilemapBuffer != NULL) {
        off = work->windowY * work->mapWidth * 2;
        RequestDma3Copy(work->tilemapBuffer + off, work->tilemap + off, work->windowHeight * work->mapWidth * 2);
    }

    work->flags &= ~SROLL_FLAG_TILEMAP_DIRTY;
}

void SrollTextDrawString(SrollWork* work, u8* str, u8 flush) {
    SrollTextDrawQueued(work, FALSE);
    SrollTextEnqueueString(work, str);
    SrollTextDrawQueued(work, FALSE);

    if (flush == TRUE && (work->flags & SROLL_FLAG_TILEMAP_DIRTY)) {
        SrollTextFlushTilemap(work);
    }
}

void SrollTextDrawStringAtTile(SrollWork* work, u16 x, u16 y, u8* str, u8 flush) {
    SrollTextDrawQueued(work, FALSE);
    SrollTextSetCursorTile(work, x, y);
    SrollTextEnqueueString(work, str);
    SrollTextDrawQueued(work, FALSE);

    if (flush == TRUE && (work->flags & SROLL_FLAG_TILEMAP_DIRTY)) {
        SrollTextFlushTilemap(work);
    }
}

void SrollTextDrawStringAtPixelX(SrollWork* work, u16 x, u16 y, u8* str, u8 flush) {
    u32 off;
    u8* glyph;
    s32 padWidth;

    SrollTextDrawQueued(work, FALSE);
    SrollTextSetCursorTile(work, x >> 3, y);
    padWidth = x & 7;

    if (padWidth != 0) {
        off = (work->y * work->textWidth + (x >> 3)) * 32;
        glyph = SrollTextGetGlyphAddress(0x8140, work->fontPages, work->fontGlyphs, work->glyphBpp, work->glyphHeight);
        SrollTextBlitGlyph(work, (u32*)(work->tileData + off), glyph, padWidth);
    }

    SrollTextSetCursorPixelX(work, x);
    SrollTextEnqueueString(work, str);
    SrollTextDrawQueued(work, FALSE);

    if ((work->x & 7) != 0) {
        off = (work->y * work->textWidth + (work->x >> 3)) * 32;
        glyph = SrollTextGetGlyphAddress(0x8140, work->fontPages, work->fontGlyphs, work->glyphBpp, work->glyphHeight);
        SrollTextBlitGlyph(work, (u32*)(work->tileData + off), glyph, 8 - (work->x & 7));
    }

    if (flush == TRUE && (work->flags & SROLL_FLAG_TILEMAP_DIRTY)) {
        SrollTextFlushTilemap(work);
    }
}

u16 SrollTextMapSingleByteChar(u8 ch) {
    return gStaffRollSingleByteCharMap[ch];
}

void ScanlineDmaReset() {

    DmaStop(0);
    sDmaStream.enabled = FALSE;
    sDmaStream.swapPending = FALSE;
    sDmaStream.update = NULL;
    sDmaStream.dst = NULL;
    sDmaStream.srcIdx = 0;
    sDmaStream.src[0] = NULL;
    sDmaStream.src[1] = NULL;
    sDmaStream.cnt = 0;
}

void ScanlineDmaUpdate() {
    u8* src;

    DmaStop(0);

    if (sDmaStream.enabled) {
        if (sDmaStream.swapPending) {
            sDmaStream.srcIdx ^= 1;
            src = sDmaStream.src[sDmaStream.srcIdx];
            sDmaStream.dmaSrc = src;

            if (!(sDmaStream.cnt & CPU_SET_SRC_FIXED)) {
                if (sDmaStream.cnt & CPU_SET_32BIT) {
                    sDmaStream.dmaSrc = src + 4;
                } else {
                    sDmaStream.dmaSrc = src + 2;
                }
            }

            sDmaStream.swapPending = FALSE;
        }

        if (sDmaStream.src[sDmaStream.srcIdx] != NULL && sDmaStream.dst != NULL &&
            sDmaStream.cnt != 0) {
            DmaSet(0, sDmaStream.dmaSrc, sDmaStream.dst, sDmaStream.cnt);
        }

        if (sDmaStream.update != NULL) {
            sDmaStream.update();
        }
    }
}

void ScanlineDmaPrime32Bit() {
    *sDmaStream.dst = *(u32*)sDmaStream.src[sDmaStream.srcIdx];
}

void ScanlineDmaPrime16Bit() {
    *sDmaStream.dst = *(u16*)sDmaStream.src[sDmaStream.srcIdx];
}

void ScanlineDmaInit(vu16* dst, void* src, u32 cnt) {
    ScanlineDmaReset();
    sDmaStream.src[0] = src;
    sDmaStream.src[1] = src;
    sDmaStream.dmaSrc = src;

    if (cnt & CPU_SET_32BIT) {
        sDmaStream.update = ScanlineDmaPrime32Bit;

        if (!(cnt & CPU_SET_SRC_FIXED)) {
            sDmaStream.dmaSrc = src + 4;
        }
    } else {
        sDmaStream.update = ScanlineDmaPrime16Bit;

        if (!(cnt & CPU_SET_SRC_FIXED)) {
            sDmaStream.dmaSrc = src + 2;
        }
    }

    sDmaStream.dst = dst;
    sDmaStream.cnt = cnt;
}

void ScanlineDmaQueueBuffer(void* src) {
    sDmaStream.src[sDmaStream.srcIdx ^ 1] = src;
    sDmaStream.swapPending = TRUE;
}

void ScanlineDmaEnable() {
    sDmaStream.enabled = TRUE;
}

void ScanlineDmaDisable() {
    sDmaStream.enabled = FALSE;
}

void BlockAudioStart() {
    sBlockAudioPlaying = TRUE;
    AudioBlockStreamInit(GetBlockAudioData());
    PcmPlaybackInit(GetBlockAudioSampleRate());
    SetVBlankCallback(VBlankIntrBlockAudio);
    PcmPlaybackStart();
}

void BlockAudioUpdate() {
    if (sBlockAudioPlaying == TRUE) {
        sBlockAudioPlaying = AudioBlockStreamUpdate();

        if (!sBlockAudioPlaying) {
            BlockAudioStop();
        }
    }
}

void BlockAudioVBlank() {
    if (sBlockAudioPlaying == TRUE) {
        PcmPlaybackUpdate();
    }
}

void BlockAudioStop() {
    ResetVBlankCallback();
    PcmPlaybackStop();
    m4aSoundInit();
    m4aSoundVSyncOn();
}

u16 GetBlockAudioSampleRate() {
    return 21024;
}

u32* GetBlockAudioData() {
    return gBlockAudioData;
}

u8* ReadNextAudioBlock(u32** next) {
    u32* base;
    u32* data;
    u32 header;

    base = *next;
    data = base;
    header = *data++;

    if ((header & 0xFF) != 0x53) {
        *next = NULL;
        return NULL;
    }

    *next = (u32*)((u8*)base + (((header >> 8) & 0xFF00) << 2) + ((header >> 24) << 2));
    return (u8*)data;
}

s32 AudioBlockStreamInit(u32* src) {
    s32* sample;
    u8* block;

    for (sample = sDecodedAudioBuffer; sample < sDecodedAudioBuffer + 0x810;) {
        *sample++ = 0;
    }

    sAudioBlockNext = src;

    for (sDecodedAudioWritePosition = 0; sDecodedAudioWritePosition <= 0x7FF; sDecodedAudioWritePosition += 0x200) {
        if (sAudioBlockNext != NULL) {
            block = ReadNextAudioBlock(&sAudioBlockNext);

            if (block != NULL) {
                DecodeAudioBlock(block, sDecodedAudioBuffer, sDecodedAudioWritePosition);
            }
        }
    }

    sDecodedAudioWritePosition &= 0x7FF;
    sDecodedAudioReadPosition = 0;
    return sAudioBlockNext != NULL;
}

s32 AudioBlockStreamUpdate() {
    u8* block;

    if (sDecodedAudioReadPosition > sDecodedAudioWritePosition + 0x200 || sDecodedAudioReadPosition < sDecodedAudioWritePosition) {
        if (sAudioBlockNext != NULL) {
            block = ReadNextAudioBlock(&sAudioBlockNext);

            if (block != NULL) {
                DecodeAudioBlock(block, sDecodedAudioBuffer, sDecodedAudioWritePosition);
            }

            sDecodedAudioWritePosition = (sDecodedAudioWritePosition + 0x200) & 0x7FF;
        }
    }

    return sAudioBlockNext != NULL;
}

s32* GetDecodedAudioBuffer() {
    return sDecodedAudioBuffer;
}

s32 GetDecodedAudioReadPosition() {
    return sDecodedAudioReadPosition;
}

void SetDecodedAudioReadPosition(s32 pos) {
    sDecodedAudioReadPosition = pos;
}

TaskDesc gTaskDescSrollTmr = {
    "task_sroll_tmr",
    (TaskInitFunc)task_sroll_tmr_0,
    (TaskUpdateFunc)task_sroll_tmr_1,
    (TaskDrawFunc)task_sroll_tmr_2,
    (TaskDestroyFunc)task_sroll_tmr_3,
    sizeof(SrollTmrWork),
};
