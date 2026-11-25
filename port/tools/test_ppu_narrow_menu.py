"""Host check of the opt-in narrow menu composition paths of port_ppu.c.

The actual ComposeLine, NeonOpaqueComposeRow and BuildNarrowSides are compiled
with a portable stand-in for the few NEON intrinsics they use. Every optimized
route must leave the same pixels, occlusion buffers and journal captures as the
scalar compositor (flags off, NEON off), over random rows including the edges.
"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'port/src/port_ppu.c').read_text(encoding='utf8')


def cut(start, end, after=0):
    a = source.index(start, after)
    return source[a:source.index(end, a)]


blend = cut('static uint16_t Blend(', 'static void BuildNarrowSides(int gy')
compose = cut('static uint32_t JiminyBgColor(', '/* ---- frame ----')
# Title fade sits between the colour helpers and the compositor in no part of this.
sides = cut('static void BuildNarrowSides(int gy, uint16_t dispcnt, const int* bgOn, const int* bgPrio) {\n    int side',
            '/* Start of a line:')

NEON = r'''
typedef uint16_t uint16x8_t __attribute__((vector_size(16)));
typedef int16_t int16x8_t __attribute__((vector_size(16)));
typedef uint8_t uint8x8_t __attribute__((vector_size(8)));
typedef struct { uint8x8_t val[4]; } uint8x8x4_t;
#define EACH(i) for (int i = 0; i < 8; i++)
#define OP16(name, expr) static uint16x8_t name(uint16x8_t a, uint16x8_t b) { uint16x8_t r; EACH(i) r[i] = (uint16_t)(expr); return r; }
OP16(vandq_u16, a[i] & b[i]) OP16(vorrq_u16, a[i] | b[i]) OP16(vaddq_u16, a[i] + b[i])
OP16(vceqq_u16, a[i] == b[i] ? 0xffff : 0) OP16(vcgtq_u16, a[i] > b[i] ? 0xffff : 0)
OP16(vcleq_u16, a[i] <= b[i] ? 0xffff : 0) OP16(vbicq_u16, a[i] & ~b[i])
static uint16x8_t vdupq_n_u16(uint16_t v) { uint16x8_t r; EACH(i) r[i] = v; return r; }
static int16x8_t vdupq_n_s16(int16_t v) { int16x8_t r; EACH(i) r[i] = v; return r; }
static uint8x8_t vdup_n_u8(uint8_t v) { uint8x8_t r; EACH(i) r[i] = v; return r; }
static uint16x8_t vmvnq_u16(uint16x8_t a) { EACH(i) a[i] = (uint16_t)~a[i]; return a; }
static uint16x8_t vbslq_u16(uint16x8_t m, uint16x8_t a, uint16x8_t b) { EACH(i) a[i] = (uint16_t)((a[i] & m[i]) | (b[i] & ~m[i])); return a; }
static uint16x8_t vld1q_u16(const uint16_t* p) { uint16x8_t r; memcpy(&r, p, 16); return r; }
static uint8x8_t vld1_u8(const uint8_t* p) { uint8x8_t r; memcpy(&r, p, 8); return r; }
static void vst1_u8(uint8_t* p, uint8x8_t v) { memcpy(p, &v, 8); }
static void vst1q_u16(uint16_t* p, uint16x8_t v) { memcpy(p, &v, 16); }
static uint32_t* vdupq_n_u32(uint32_t v) { static uint32_t r[4]; EACH(i) if (i < 4) r[i] = v; return r; }
static void vst1q_u32(uint32_t* p, const uint32_t* v) { memcpy(p, v, 16); }
static uint8x8_t vmovn_u16(uint16x8_t a) { uint8x8_t r; EACH(i) r[i] = (uint8_t)a[i]; return r; }
static uint16x8_t vmovl_u8(uint8x8_t a) { uint16x8_t r; EACH(i) r[i] = a[i]; return r; }
static uint16x8_t vmlaq_n_u16(uint16x8_t a, uint16x8_t b, uint16_t n) { EACH(i) a[i] = (uint16_t)(a[i] + b[i] * n); return a; }
#define vshlq_n_u16(a, n) ((a) << (n))
#define vshrq_n_u16(a, n) ((a) >> (n))
static int16x8_t vreinterpretq_s16_u16(uint16x8_t a) { return (int16x8_t)a; }
static uint16x8_t vcltq_s16(int16x8_t a, int16x8_t b) { uint16x8_t r; EACH(i) r[i] = a[i] < b[i] ? 0xffff : 0; return r; }
static uint16x8_t vcgeq_s16(int16x8_t a, int16x8_t b) { uint16x8_t r; EACH(i) r[i] = a[i] >= b[i] ? 0xffff : 0; return r; }
static void vst4_u8(uint8_t* p, uint8x8x4_t v) { EACH(i) for (int c = 0; c < 4; c++) p[i * 4 + c] = v.val[c][i]; }
'''

fixture = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __ARM_NEON 1
''' + NEON + r'''
#define PORT_MAX_W 512
#define PORT_MAX_H 320
#define PORT_OAM_CAPACITY 256
#define GBA_SCREEN_W 240
#define GBA_SCREEN_H 160
#define TRANSPARENT 0x8000
enum { LAYER_BG0, LAYER_BG1, LAYER_BG2, LAYER_BG3, LAYER_OBJ, LAYER_BD };
enum { PORT_JIMINY_LIST = 1, PORT_JIMINY_DETAIL, PORT_JIMINY_COVER };
#define WIN_BG(n) (1 << (n))
#define WIN_OBJ 0x10
#define WIN_SFX 0x20
#define W 320
#define H 240
static uint16_t io[64], palette[512];
#define IO16(o) io[(o) / 2]
#define PLTT16 palette
#define PLTT16_0() palette[0]
static struct { int width, height, originX, originY, wide; } gPortScreen = {W, H, 40, 40, 0};
typedef struct { uint32_t fill, border, ground, guide; uint16_t owned; uint8_t leftOwned, kind; } PortJiminyPageRow;
static uint16_t sBgLine[4][PORT_MAX_W], sObjLine[PORT_MAX_W], sObjIndex[PORT_MAX_W];
static uint8_t sInWorld[PORT_MAX_W], sObjPrio[PORT_MAX_W], sObjSemi[PORT_MAX_W], sWinMask[PORT_MAX_W];
static int sLineHasObjects, sTitleArtHd, sBattleView, sHudLayer, sHdText = 1, sMainHdAny, sSpotlight, sLutMode;
static int sJiminyDetailCapture, sJiminyPageCapture, sJiminyPaperCapture, sJiminyCoverCapture, sOverlay;
static int sScreenBg[4], sBgDockedLatched[4], sWorldBlendActive, sWorldBlendScreen, sSpotlightClearRows;
static unsigned sScreenBgMask;
static uint16_t sWorldPadding[4], gTitleSoraBgPalette[32], gBgPaletteBank[4];
static struct { int valid; } sBgLatch[4];
static const struct { int v[1]; } *sPreliminarRecipe[4];
static int sLeft, sRight = 240;
static uint32_t sColorLut[32768], *gPortMenuBackground;
static uint8_t sOccBg[W * H], sOccBgLayer[W * H], sMsgMask[W * H], sWorldBlendMask[W * H];
static uint16_t sOccObj[W * H];
static uint32_t sWorldBlendNative[W * H], sWorldBlendAdd[W * H];
static PortJiminyPageRow sJiminyPageRows[160];
static uint8_t sJiminyPaperMask[240 * 160];
static const uint16_t *sJiminyBookFrame;
static struct { const void *tiles, *frame; } sOamHd[PORT_OAM_CAPACITY];
static uint16_t sScrBlendLine[PORT_MAX_W];
static uint8_t sScrBlendPrio[PORT_MAX_W], sScrBlendSemi[PORT_MAX_W];
static int sJiminyNameEdges[1], edgeCalls[2][H];
static const uint8_t gJiminyBinderRingTiles[1], gJiminyBinderRingFrame0[1];
static int PortHd2dActive(void) { return 0; }
static uint16_t Preliminar102ViewUnderlay(int x, int y) { (void)x; (void)y; return 0; }
static int PortJiminyPaperPixel(int l, int b, int x, int y) { return !b && l == 1 && ((x + y) & 1); }
static int PortJiminyBookPixel(int a, int b, int c, int d, int e) { (void)a; (void)b; (void)c; (void)d; (void)e; return 0; }
static int PortJiminyBookObject(int a, int b, int c, int d, int e, int f) { (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; return 0; }
static void PortJiminyRecordNameEdge(int* o, int layer, int blend, int x, int y, uint32_t c) {
    (void)o; edgeCalls[x != 0][y] = (int)(c ^ (unsigned)(layer * 7 + blend));
}
''' + blend + compose + r'''
#define NARROW_SIDE_W 96
#define OBJ_BAND_COLORS 16
static uint32_t sNarrowSide[2][GBA_SCREEN_H][NARROW_SIDE_W], sNarrowEdge[2][GBA_SCREEN_H];
static uint8_t sNarrowEdgeKind[2][GBA_SCREEN_H], sObjBandCount[2][GBA_SCREEN_H][OBJ_BAND_COLORS];
static uint8_t sObjBandKinds[2][GBA_SCREEN_H], sObjBandTotal[2][GBA_SCREEN_H];
static uint16_t sObjBandColor[2][GBA_SCREEN_H][OBJ_BAND_COLORS];
static int sNarrowObjBands, sNarrowSideFill;
''' + sides + r'''
#define WIDE_MODES 3
#define WIDE_OFF (sWidePlainVector = sWideSecondVector = false)
#define WIDE_SET(m) (sWidePlainVector = (m) != 1, sWideSecondVector = (m) != 0)
typedef struct {
    uint32_t frame[W], back[W];
    uint8_t bg[W], layer[W], msg[W];
    uint16_t obj[W];
    PortJiminyPageRow page;
    int edge[2];
} Result;
static uint32_t frameBuf[W * H], backBuf[W * H];

static int pick(int n) { return rand() % n; }

static void run(Result* r, int y, uint16_t dispcnt, const int* on, const int* prio) {
    int at = y * W, gy = y - 40;
    memset(frameBuf, 0x5a, sizeof frameBuf); memset(backBuf, 0xa5, sizeof backBuf);
    memset(sOccBg, 9, sizeof sOccBg); memset(sOccBgLayer, 9, sizeof sOccBgLayer);
    memset(sMsgMask, 9, sizeof sMsgMask); memset(sOccObj, 9, sizeof sOccObj);
    memset(sJiminyPageRows, 0, sizeof sJiminyPageRows); memset(edgeCalls, 0, sizeof edgeCalls);
    ComposeLine(y, dispcnt, on, prio, frameBuf, 1);
    ComposeLine(y, dispcnt, on, prio, backBuf, 0);
    memset(r, 0, sizeof *r);
    memcpy(r->frame, frameBuf + at, sizeof r->frame); memcpy(r->back, backBuf + at, sizeof r->back);
    memcpy(r->bg, sOccBg + at, W); memcpy(r->layer, sOccBgLayer + at, W); memcpy(r->msg, sMsgMask + at, W);
    memcpy(r->obj, sOccObj + at, sizeof r->obj);
    r->page = sJiminyPageRows[gy]; r->edge[0] = edgeCalls[0][gy]; r->edge[1] = edgeCalls[1][gy];
}

int main(void) {
    static Result ref, got;
    long used[4] = {0}, copies = 0, fills = 0;
    for (int i = 0; i < 32768; i++)
        sColorLut[i] = 0xff000000u | ((i & 31) * 8 + (i & 31) / 4) << 16 |
            (((i >> 5) & 31) * 8 + ((i >> 5) & 31) / 4) << 8 | (((i >> 10) & 31) * 8 + ((i >> 10) & 31) / 4);
    srand(1);
    for (int t = 0; t < 60000; t++) {
        int on[4], prio[4], kind = pick(5), y = 40 + (t % 4 == 0 ? pick(18) : t % 4 == 1 ? 158 + pick(2) : pick(160));
        int objects = pick(3), layers = pick(5);
        uint16_t dispcnt = pick(12) == 0 ? 0x2000 : 0;
        sJiminyCoverCapture = kind == 1; sJiminyDetailCapture = kind == 3;
        sJiminyPageCapture = kind >= 1 && kind <= 3; sJiminyPaperCapture = kind == 3 && pick(8) == 0;
        for (int b = 0; b < 4; b++) { on[b] = b < layers; prio[b] = pick(4); sBgDockedLatched[b] = pick(12) == 0; }
        IO16(0x50) = pick(2) ? 0 : (uint16_t)((pick(3) ? 0 : pick(4) << 6) | (pick(2) ? pick(64) : 0) | (pick(2) ? pick(64) << 8 : 0));
        IO16(0x52) = (uint16_t)(pick(32) | pick(32) << 8); IO16(0x54) = (uint16_t)pick(32);
        for (int i = 0; i < 512; i++) palette[i] = (uint16_t)pick(32768);
        sLineHasObjects = 0; sHudLayer = pick(8) == 0; sWorldBlendActive = 0;
        for (int x = 0; x < W; x++) {
            for (int b = 0; b < 4; b++) sBgLine[b][x] = pick(3) ? (uint16_t)pick(32768) : TRANSPARENT;
            sObjLine[x] = objects && !pick(objects == 1 ? 40 : 3) ? (uint16_t)pick(32768) : TRANSPARENT;
            if (sObjLine[x] != TRANSPARENT) sLineHasObjects = 1;
            sObjPrio[x] = (uint8_t)pick(4); sObjIndex[x] = (uint16_t)pick(128);
            sObjSemi[x] = objects == 2 && pick(t % 2 ? 400 : 3) == 0;
            sInWorld[x] = pick(4) != 0; sWinMask[x] = dispcnt ? (uint8_t)pick(64) : 0x3f;
        }
        sNeonOpaqueCompose = false; sNeonNarrowCompose = false; sJournalCentralVector = false;
        sJournalPageVector = sNarrowSecondVector = sNarrowPlainVector = sMenuBgVector = false;
        run(&ref, y, dispcnt, on, prio);
        /* what the established vector paths already take on this row */
        sNeonOpaqueCompose = sNeonNarrowCompose = sJournalCentralVector = true;
        memset(sNeonOpaqueRows, 0, sizeof sNeonOpaqueRows); memset(sJournalCentralRows, 0, sizeof sJournalCentralRows);
        run(&got, y, dispcnt, on, prio);
        assert(!memcmp(&ref, &got, sizeof ref));
        const bool before = sNeonOpaqueRows[y] || sJournalCentralRows[y - 40];
        /* every flag alone, then all of them */
        for (int mode = 0; mode < 5; mode++) {
            sNeonOpaqueCompose = sNeonNarrowCompose = true;
            sJournalCentralVector = true;
            sJournalPageVector = mode == 0 || mode == 4; sNarrowSecondVector = mode == 1 || mode == 4;
            sNarrowPlainVector = mode == 2 || mode == 4; sMenuBgVector = mode == 3 || mode == 4;
            memset(sNeonOpaqueRows, 0, sizeof sNeonOpaqueRows); memset(sJournalCentralRows, 0, sizeof sJournalCentralRows);
            run(&got, y, dispcnt, on, prio);
            if (mode < 4 && !before && (sNeonOpaqueRows[y] || sJournalCentralRows[y - 40])) used[mode]++;
            if (memcmp(&ref, &got, sizeof ref)) {
                printf("MISMATCH case %d mode %d row %d kind %d bld %04x frame %d back %d occ %d page %d\n", t, mode, y, kind, IO16(0x50),
                       !!memcmp(ref.frame, got.frame, sizeof ref.frame), !!memcmp(ref.back, got.back, sizeof ref.back),
                       !!memcmp(ref.bg, got.bg, W), !!memcmp(&ref.page, &got.page, sizeof ref.page));
                return 1;
            }
        }
        /* the background copy: no sprite pixel means both passes agree */
        if (!sLineHasObjects) { assert(!memcmp(ref.frame, ref.back, sizeof ref.frame)); copies++; }
        /* narrow side columns, with long edge runs so bands and flat scenery occur */
        for (int b = 0; b < 4; b++)
            for (int side = 0; side < 2; side++) {
                int run_ = pick(3) ? pick(64) : 0; uint16_t c = pick(4) ? (uint16_t)pick(32768) : TRANSPARENT;
                for (int k = 0; k < run_; k++) sBgLine[b][side ? 279 - k : 40 + k] = c;
            }
        static uint32_t keep[2][NARROW_SIDE_W];
        sNarrowSideFill = 0; BuildNarrowSides(y - 40, 0, on, prio);
        for (int side = 0; side < 2; side++) memcpy(keep[side], sNarrowSide[side][y - 40], sizeof keep[side]);
        memset(sNarrowSide, 0, sizeof sNarrowSide);
        sNarrowSideFill = 1; BuildNarrowSides(y - 40, 0, on, prio);
        for (int side = 0; side < 2; side++) {
            assert(!memcmp(keep[side], sNarrowSide[side][y - 40], sizeof keep[side]));
            fills += keep[side][0] == keep[side][95];
        }
    }
    assert(used[0] && used[1] && used[2] && used[3] && copies && fills);
    /* wide scenes: full rows, extensions above and below the picture */
    long wideUsed[WIDE_MODES] = {0};
    gPortScreen.wide = 1; sLeft = -40; sRight = 280;
    for (int t = 0; t < 60000; t++) {
        int on[4], prio[4], y = t % 3 == 0 ? pick(H) : 40 + pick(160), gy = y - 40;
        int objects = pick(3), layers = pick(3) ? pick(3) : pick(5);
        uint16_t dispcnt = pick(12) == 0 ? 0x2000 : 0;
        sJiminyCoverCapture = sJiminyDetailCapture = sJiminyPageCapture = sJiminyPaperCapture = 0;
        for (int b = 0; b < 4; b++) {
            on[b] = b < layers; prio[b] = pick(4); sBgDockedLatched[b] = pick(12) == 0;
            sWorldPadding[b] = pick(3) ? TRANSPARENT : (uint16_t)pick(32768);
        }
        IO16(0x50) = pick(2) ? 0 : (uint16_t)((pick(3) ? 0 : pick(4) << 6) | (pick(2) ? pick(64) : 0) | (pick(2) ? pick(64) << 8 : 0));
        IO16(0x52) = (uint16_t)(pick(32) | pick(32) << 8); IO16(0x54) = (uint16_t)pick(32);
        for (int i = 0; i < 512; i++) palette[i] = (uint16_t)pick(32768);
        sLineHasObjects = 0; sHudLayer = pick(8) == 0; sHdText = pick(2); sWorldBlendActive = 0;
        for (int x = 0; x < W; x++) {
            for (int b = 0; b < 4; b++) sBgLine[b][x] = pick(3) ? (uint16_t)pick(32768) : TRANSPARENT;
            sObjLine[x] = objects && !pick(objects == 1 ? 40 : 3) ? (uint16_t)pick(32768) : TRANSPARENT;
            if (sObjLine[x] != TRANSPARENT) sLineHasObjects = 1;
            sObjPrio[x] = (uint8_t)pick(4); sObjIndex[x] = (uint16_t)pick(128);
            sObjSemi[x] = objects == 2 && pick(t % 2 ? 400 : 3) == 0;
            sInWorld[x] = pick(4) != 0; sWinMask[x] = dispcnt ? (uint8_t)pick(64) : 0x3f;
        }
        sNeonOpaqueCompose = sNeonNarrowCompose = sJournalCentralVector = false;
        sJournalPageVector = sNarrowSecondVector = sNarrowPlainVector = sMenuBgVector = false;
        WIDE_OFF;
        run(&ref, y, dispcnt, on, prio);
        sNeonOpaqueCompose = true;
        memset(sNeonOpaqueRows, 0, sizeof sNeonOpaqueRows);
        run(&got, y, dispcnt, on, prio);
        assert(!memcmp(&ref, &got, sizeof ref));
        const bool before = sNeonOpaqueRows[y];
        for (int mode = 0; mode < WIDE_MODES; mode++) {
            WIDE_SET(mode);
            memset(sNeonOpaqueRows, 0, sizeof sNeonOpaqueRows);
            run(&got, y, dispcnt, on, prio);
            if (!before && sNeonOpaqueRows[y]) wideUsed[mode]++;
            if (memcmp(&ref, &got, sizeof ref)) {
                printf("MISMATCH wide case %d mode %d row %d bld %04x\n", t, mode, gy, IO16(0x50));
                return 1;
            }
        }
    }
    for (int mode = 0; mode < WIDE_MODES; mode++) { assert(wideUsed[mode]); printf("wide mode %d newly vector rows %ld\n", mode, wideUsed[mode]); }
    printf("PASS narrow menu composition: 60000 rows; newly vector rows: page %ld second %ld plain %ld background %ld; copies %ld\n",
           used[0], used[1], used[2], used[3], copies);
    return 0;
}
'''
cc = os.environ.get('CC') or shutil.which('cc') or str(
    root.parent.parent.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe')
if not Path(cc).exists() and not shutil.which(cc):
    cc = 'D:/01Claude/ReKHCoM/toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe'
with tempfile.TemporaryDirectory() as directory:
    c = Path(directory) / 'narrow.c'
    exe = Path(directory) / 'narrow.exe'
    c.write_text(fixture, encoding='utf8')
    subprocess.run([cc, '-std=gnu11', '-O1', '-w', str(c), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
