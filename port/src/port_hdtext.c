/*
 * HD text: the dialogue glyphs the game draws as 16x16 sprites from its pixel
 * font are left out of the GBA picture (port_ppu.c) and drawn here with an
 * outline font at the window's resolution, at the same places and in the
 * same colours (palette fades included).
 *
 * Each word starts where the game puts it, or after the previous word when
 * the outline font set that one wider; letters keep their natural spacing.
 * Glyphs are rasterised with GDI and cached as textures, drawn unscaled at
 * whole pixels.
 *
 * Fonts are distributed in assets/fonts and embedded as core resources, so
 * loose test executables retain KHMenu too. Letters a font lacks are built from it where possible:
 * accented letters are the base letter with the accent of the fallback font
 * (Segoe UI), and the Spanish opening marks are ? and ! turned over; anything
 * else comes from the fallback font.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <SDL3/SDL.h>

#include "port/port.h"
#include "port/port_font_resources.h"

/* The pixel font's capitals are 7 pixels tall, from row 4 to row 10 of the
   16x16 cell: the baseline is row 11. */
#define HD_TEXT_CAP 7.0f
#define HD_TEXT_BASELINE 11
#define HD_TEXT_MAX_GLYPHS 256
/* Japanese: glyphs on a 10-pixel grid, 10 pixels tall, on the same baseline. */
#define HD_TEXT_JP_CELL 10
#define HD_TEXT_JP_EM 11.0f
#define HD_TEXT_JP_CACHE 1024

const uint16_t* PortLatinGlyphWidths(void);
uint32_t PortFrameCount(void);

typedef struct HdGlyph {
    SDL_Texture* tex;
    int ready;
    int w, h;
    int left, top; /* bitmap position from the pen on the baseline */
    int advance;
} HdGlyph;

typedef struct HdFont {
    int key;
    float cap;      /* capital height in output pixels */
    HFONT font;     /* the HD font */
    HFONT fallback; /* for what the HD font lacks: Segoe UI, same capital height */
    HFONT jp;       /* Japanese version: Yu Gothic UI, the size of the game's 10-pixel glyphs */
    HdGlyph glyphs[256];
    uint16_t jpCodes[HD_TEXT_JP_CACHE]; /* Shift-JIS codes of jpGlyphs, 0 = free */
    HdGlyph jpGlyphs[HD_TEXT_JP_CACHE];
} HdFont;

/* A glyph bitmap in memory: 8-bit coverage, origin as GDI reports it. */
typedef struct Bitmap {
    uint8_t* a;
    int w, h, left, top, advance;
} Bitmap;

static HDC sDc;
static HdFont sFonts[4][2]; /* [style][cache slot]; style 2: numbers, menu face */
static int sNextFont[4];
static int sCustomTried;
static const WCHAR* sFace[2] = { L"Segoe UI", L"Segoe UI" }; /* menu / dialogue, movie subtitles */
static HANDLE sEmbeddedFonts[2]; /* process-private resources, kept for GDI's lifetime */
static WCHAR sFontFiles[2][1024];

static int LoadEmbeddedFont(int i) {
    static const int ids[2] = { PORT_FONT_KHMENU_RESOURCE, PORT_FONT_COMIC_HEARTS_RESOURCE };
    HMODULE module = GetModuleHandleW(NULL);
    HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(ids[i]), MAKEINTRESOURCEW(10)); /* RCDATA */
    HGLOBAL loaded;
    void* data;
    DWORD size, count = 0;
    if (!resource || !(size = SizeofResource(module, resource)) ||
        !(loaded = LoadResource(module, resource)) || !(data = LockResource(loaded)))
        return 0;
    sEmbeddedFonts[i] = AddFontMemResourceEx(data, size, NULL, &count);
    return sEmbeddedFonts[i] != NULL && count > 0;
}

static void LoadCustomFont(void) {
    static const char* const files[2] = { "assets/fonts/KHMenu.otf", "assets/fonts/ComicHearts.otf" };
    static const WCHAR* const faces[2] = { L"KHMenu", L"Comic Hearts" };
    const char* base = SDL_GetBasePath();
    int i;

    if (sCustomTried)
        return;
    sCustomTried = 1;
    for (i = 0; i < 2; i++) {
        char path[1024];
        WCHAR wpath[1024];

        /* next to the executable, or in the data folder: the packaged game
           runs from bin/ with its assets at the root (--data-dir) */
        SDL_strlcpy(path, PortConfigPath(files[i]), sizeof(path));
        if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)
            SDL_snprintf(path, sizeof(path), "%s%s", base ? base : "", files[i]);
        if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES && base)
            SDL_snprintf(path, sizeof(path), "%s../%s", base, files[i]);
        if (MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, 1024) > 0 && AddFontResourceExW(wpath, FR_PRIVATE, NULL) > 0) {
            lstrcpynW(sFontFiles[i], wpath, 1024);
            sFace[i] = faces[i];
            PortLog("HD text: font %s", path);
        } else if (LoadEmbeddedFont(i)) {
            sFace[i] = faces[i];
            PortLog("HD text: font %s (embedded core resource)", i ? "Comic Hearts" : "KHMenu");
        } else {
            PortLog("HD text: unable to load %s; using fallback", path);
        }
    }
}

static HFONT MakeFont(const WCHAR* face, int px, int width, int weight) {
    return CreateFontW(-px, width, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS,
                       CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH | FF_SWISS, face);
}

/* Font of the face whose capital H is `cap` pixels tall. A face set wider
   than the game's pixel font is condensed to its widths, so lines keep
   inside their windows. */
static HFONT FontWithCap(const WCHAR* face, float cap, float scale, int weight) {
    static const MAT2 identity = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };
    static const char sample[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const uint16_t* widths = PortLatinGlyphWidths();
    GLYPHMETRICS gm;
    TEXTMETRICW tm;
    SIZE size;
    WCHAR wide[64];
    float game = 0;
    int px = (int)(cap * 1.5f + 0.5f), i, n = (int)sizeof(sample) - 1;
    HFONT f = MakeFont(face, px, 0, weight);

    SelectObject(sDc, f);
    if (GetGlyphOutlineW(sDc, L'H', GGO_METRICS, &gm, 0, NULL, &identity) != GDI_ERROR && gm.gmBlackBoxY > 0) {
        int fit = (int)(px * cap / (float)gm.gmBlackBoxY + 0.5f);
        if (fit != px && fit > 0) {
            DeleteObject(f);
            px = fit;
            f = MakeFont(face, px, 0, weight);
            SelectObject(sDc, f);
        }
    }
    for (i = 0; i < n; i++) {
        wide[i] = (WCHAR)sample[i];
        game += widths[(uint8_t)sample[i]] * scale;
    }
    if (GetTextExtentPoint32W(sDc, wide, n, &size) && GetTextMetricsW(sDc, &tm) && size.cx > game * 1.02f) {
        int w = (int)(tm.tmAveCharWidth * game / size.cx + 0.5f);
        DeleteObject(f);
        f = MakeFont(face, px, SDL_max(1, w), weight);
    }
    return f;
}

static void FontRelease(HdFont* f) {
    int i;

    for (i = 0; i < 256; i++) {
        if (f->glyphs[i].tex != NULL)
            SDL_DestroyTexture(f->glyphs[i].tex);
    }
    for (i = 0; i < HD_TEXT_JP_CACHE; i++) {
        if (f->jpGlyphs[i].tex != NULL)
            SDL_DestroyTexture(f->jpGlyphs[i].tex);
    }
    if (f->jp != NULL)
        DeleteObject(f->jp);
    if (f->fallback != NULL && f->fallback != f->font)
        DeleteObject(f->fallback);
    if (f->font != NULL)
        DeleteObject(f->font);
    SDL_memset(f, 0, sizeof(*f));
}

static HdFont* FontForScale(float scale, int style) {
    int key = (int)(scale * 16.0f + 0.5f) + (gPortConfig.hdFont == 2 ? 2 : 0) * 100000; /* the face chosen too */
    const WCHAR* face;
    HdFont* f;
    int i;

    for (i = 0; i < 2; i++) {
        if (sFonts[style][i].font != NULL && sFonts[style][i].key == key)
            return &sFonts[style][i];
    }
    if (sDc == NULL)
        sDc = CreateCompatibleDC(NULL);
    LoadCustomFont();
    f = &sFonts[style][sNextFont[style]];
    sNextFont[style] ^= 1;
    FontRelease(f);
    f->key = key;
    f->cap = HD_TEXT_CAP * scale;
    /* menus and dialogue in the face chosen in Options (KHMenu, Comic Hearts or the system's); subtitles keep theirs */
    face = style == 1 ? sFace[1] : gPortConfig.hdFont == 2 ? L"Segoe UI" : sFace[0]; /* Comic Hearts: subtitles only */
    {
        f->font = FontWithCap(face, f->cap, scale, face[0] == L'S' ? FW_SEMIBOLD : FW_NORMAL);
        {
            WCHAR selected[128];
            char actual[256];
            SelectObject(sDc, f->font);
            if (GetTextFaceW(sDc, 128, selected) > 0 &&
                WideCharToMultiByte(CP_UTF8, 0, selected, -1, actual, sizeof(actual), NULL, NULL) > 0)
                PortLog("HD text: selected style=%d face=%s scale=%.3f key=%d", style, actual, scale, key);
        }
    }
    f->fallback = face[0] == L'S' ? f->font
                                          : FontWithCap(style && style != 3 ? L"Comic Sans MS" : L"Segoe UI", f->cap, scale,
                                                        style && style != 3 ? FW_NORMAL : FW_SEMIBOLD);
    return f;
}

static int HasGlyph(HFONT font, WCHAR wc) {
    WORD index = 0xFFFF;

    SelectObject(sDc, font);
    return GetGlyphIndicesW(sDc, &wc, 1, &index, GGI_MARK_NONEXISTING_GLYPHS) == 1 && index != 0xFFFF;
}

static int Render(HFONT font, WCHAR wc, Bitmap* out) {
    static const MAT2 identity = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };
    GLYPHMETRICS gm;
    DWORD size;

    SDL_memset(out, 0, sizeof(*out));
    SelectObject(sDc, font);
    size = GetGlyphOutlineW(sDc, wc, GGO_GRAY8_BITMAP, &gm, 0, NULL, &identity);
    if (size == GDI_ERROR)
        return 0;
    out->advance = gm.gmCellIncX;
    out->left = gm.gmptGlyphOrigin.x;
    out->top = gm.gmptGlyphOrigin.y;
    if (size > 0) {
        uint8_t* bits = SDL_malloc(size);
        int pitch = ((int)gm.gmBlackBoxX + 3) & ~3, x, y;

        out->w = (int)gm.gmBlackBoxX;
        out->h = (int)gm.gmBlackBoxY;
        out->a = SDL_calloc((size_t)out->w * out->h, 1);
        if (bits == NULL || out->a == NULL
            || GetGlyphOutlineW(sDc, wc, GGO_GRAY8_BITMAP, &gm, size, bits, &identity) == GDI_ERROR) {
            SDL_free(bits);
            SDL_free(out->a);
            out->a = NULL;
            out->w = out->h = 0;
            return 0;
        }
        for (y = 0; y < out->h; y++)
            for (x = 0; x < out->w; x++)
                out->a[y * out->w + x] = (uint8_t)(bits[y * pitch + x] * 255u / 64u);
        SDL_free(bits);
    }
    return 1;
}

/* Accented letters of Windows-1252: base letter and spacing accent. */
static int Decompose(WCHAR wc, WCHAR* base, WCHAR* accent) {
    static const struct {
        WCHAR c, base, accent;
    } table[] = {
        { 0xC0, 'A', 0x60 }, { 0xC1, 'A', 0xB4 }, { 0xC2, 'A', 0x2C6 }, { 0xC3, 'A', 0x2DC }, { 0xC4, 'A', 0xA8 },
        { 0xC5, 'A', 0x2DA }, { 0xC7, 'C', 0xB8 }, { 0xC8, 'E', 0x60 }, { 0xC9, 'E', 0xB4 }, { 0xCA, 'E', 0x2C6 },
        { 0xCB, 'E', 0xA8 }, { 0xCC, 'I', 0x60 }, { 0xCD, 'I', 0xB4 }, { 0xCE, 'I', 0x2C6 }, { 0xCF, 'I', 0xA8 },
        { 0xD1, 'N', 0x2DC }, { 0xD2, 'O', 0x60 }, { 0xD3, 'O', 0xB4 }, { 0xD4, 'O', 0x2C6 }, { 0xD5, 'O', 0x2DC },
        { 0xD6, 'O', 0xA8 }, { 0xD9, 'U', 0x60 }, { 0xDA, 'U', 0xB4 }, { 0xDB, 'U', 0x2C6 }, { 0xDC, 'U', 0xA8 },
        { 0xDD, 'Y', 0xB4 }, { 0xE0, 'a', 0x60 }, { 0xE1, 'a', 0xB4 }, { 0xE2, 'a', 0x2C6 }, { 0xE3, 'a', 0x2DC },
        { 0xE4, 'a', 0xA8 }, { 0xE5, 'a', 0x2DA }, { 0xE7, 'c', 0xB8 }, { 0xE8, 'e', 0x60 }, { 0xE9, 'e', 0xB4 },
        { 0xEA, 'e', 0x2C6 }, { 0xEB, 'e', 0xA8 }, { 0xEC, 0x131, 0x60 }, { 0xED, 0x131, 0xB4 },
        { 0xEE, 0x131, 0x2C6 }, { 0xEF, 0x131, 0xA8 }, { 0xF1, 'n', 0x2DC }, { 0xF2, 'o', 0x60 },
        { 0xF3, 'o', 0xB4 }, { 0xF4, 'o', 0x2C6 }, { 0xF5, 'o', 0x2DC }, { 0xF6, 'o', 0xA8 }, { 0xF9, 'u', 0x60 },
        { 0xFA, 'u', 0xB4 }, { 0xFB, 'u', 0x2C6 }, { 0xFC, 'u', 0xA8 }, { 0xFD, 'y', 0xB4 }, { 0xFF, 'y', 0xA8 },
    };
    size_t i;

    for (i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (table[i].c == wc) {
            *base = table[i].base;
            *accent = table[i].accent;
            return 1;
        }
    }
    return 0;
}

static int RowHasInk(const Bitmap* b, int y) {
    int x;

    for (x = 0; x < b->w; x++) {
        if (b->a[y * b->w + x] > 24)
            return 1;
    }
    return 0;
}

/* The base letter for an accent: a dotless i is the i without its dot. */
static int RenderBase(HFONT font, WCHAR base, Bitmap* out) {
    int y = 0;

    if (base != 0x131 || HasGlyph(font, base))
        return Render(font, base, out);
    if (!Render(font, L'i', out) || out->a == NULL)
        return 0;
    while (y < out->h && RowHasInk(out, y))
        y++; /* the dot */
    while (y < out->h && !RowHasInk(out, y))
        y++; /* the gap under it */
    if (y > 0 && y < out->h) {
        SDL_memmove(out->a, out->a + y * out->w, (size_t)(out->h - y) * out->w);
        out->h -= y;
        out->top -= y;
    }
    return 1;
}

/* base + accent in one bitmap; the accent sits centred above (cedilla below). */
static int Compose(HdFont* f, WCHAR wc, Bitmap* out) {
    WCHAR base, accent;
    Bitmap b, a;
    int gap = SDL_max(1, (int)(f->cap / 7.0f + 0.5f)), ax, atop, x0, y0, x1, y1, x, y;

    if (!Decompose(wc, &base, &accent) || !RenderBase(f->font, base, &b))
        return 0;
    if (b.a == NULL || !Render(f->fallback, accent, &a) || a.a == NULL) {
        *out = b;
        return 1;
    }
    ax = b.left + (b.w - a.w) / 2;
    atop = accent == 0xB8 ? b.top - b.h + gap / 2 : b.top + a.h + gap;
    x0 = SDL_min(b.left, ax);
    x1 = SDL_max(b.left + b.w, ax + a.w);
    y0 = SDL_max(b.top, atop);
    y1 = SDL_min(b.top - b.h, atop - a.h);
    out->w = x1 - x0;
    out->h = y0 - y1;
    out->left = x0;
    out->top = y0;
    out->advance = b.advance;
    out->a = SDL_calloc((size_t)out->w * out->h, 1);
    if (out->a != NULL) {
        for (y = 0; y < b.h; y++)
            for (x = 0; x < b.w; x++)
                out->a[(y0 - b.top + y) * out->w + (b.left - x0 + x)] = b.a[y * b.w + x];
        for (y = 0; y < a.h; y++) {
            for (x = 0; x < a.w; x++) {
                uint8_t* d = &out->a[(y0 - atop + y) * out->w + (ax - x0 + x)];
                *d = (uint8_t)SDL_max(*d, a.a[y * a.w + x]);
            }
        }
    }
    SDL_free(b.a);
    SDL_free(a.a);
    return out->a != NULL;
}

/* Spanish opening marks: the closing one turned over, hanging below the line
   from about the height of the small letters. */
static int Turned(HdFont* f, WCHAR wc, Bitmap* out) {
    WCHAR src = wc == 0xBF ? L'?' : L'!';
    int i, n;

    if (!HasGlyph(f->font, src) || !Render(f->font, src, out) || out->a == NULL)
        return 0;
    n = out->w * out->h;
    for (i = 0; i < n / 2; i++) {
        uint8_t t = out->a[i];
        out->a[i] = out->a[n - 1 - i];
        out->a[n - 1 - i] = t;
    }
    out->top = (int)(f->cap * 0.72f + 0.5f);
    return 1;
}

static void GlyphTexture(SDL_Renderer* renderer, HdGlyph* g, Bitmap* bm) {
    g->advance = bm->advance;
    g->left = bm->left;
    g->top = bm->top;
    if (bm->a != NULL && bm->w > 0 && bm->h > 0) {
        uint32_t* argb = SDL_malloc((size_t)bm->w * bm->h * 4);
        int i;

        if (argb != NULL) {
            for (i = 0; i < bm->w * bm->h; i++)
                argb[i] = ((uint32_t)bm->a[i] << 24) | 0xFFFFFFu;
            g->tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, bm->w, bm->h);
            if (g->tex != NULL) {
                SDL_UpdateTexture(g->tex, NULL, argb, bm->w * 4);
                SDL_SetTextureBlendMode(g->tex, SDL_BLENDMODE_BLEND);
                SDL_SetTextureScaleMode(g->tex, SDL_SCALEMODE_NEAREST);
                g->w = bm->w;
                g->h = bm->h;
            }
            SDL_free(argb);
        }
    }
    SDL_free(bm->a);
    bm->a = NULL;
}

/* A Japanese glyph by its Shift-JIS code, from a small open-addressed cache. */
static HdGlyph* GlyphJp(SDL_Renderer* renderer, HdFont* f, uint16_t code) {
    static HdGlyph sNone;
    unsigned slot = (code * 2654435761u) >> 22, k;
    char sjis[2] = { (char)(code >> 8), (char)code };
    WCHAR wc;
    Bitmap bm;

    for (k = 0; k < HD_TEXT_JP_CACHE; k++, slot = (slot + 1) % HD_TEXT_JP_CACHE) {
        if (f->jpCodes[slot] == code)
            return &f->jpGlyphs[slot];
        if (f->jpCodes[slot] == 0)
            break;
    }
    if (k == HD_TEXT_JP_CACHE)
        return &sNone; /* full: draw nothing rather than evict mid-frame */
    f->jpCodes[slot] = code;
    if (f->jp == NULL)
        f->jp = MakeFont(L"Yu Gothic UI", (int)(HD_TEXT_JP_EM * f->cap / HD_TEXT_CAP + 0.5f), 0, FW_BOLD);
    if (MultiByteToWideChar(932, 0, sjis, 2, &wc, 1) == 1 && Render(f->jp, wc, &bm))
        GlyphTexture(renderer, &f->jpGlyphs[slot], &bm);
    return &f->jpGlyphs[slot];
}

static HdGlyph* GlyphNow(SDL_Renderer* renderer, HdFont* f, int code);
/* a glyph rendered the first time, counted for OptiPort's stutter view while the frame log is on (gPortStutter) */
static HdGlyph* Glyph(SDL_Renderer* renderer, HdFont* f, int code) {
    uint64_t t0;
    HdGlyph* g = &f->glyphs[code & 255];
    if (g->ready)
        return g;
    if (!gPortStutter.on)
        return GlyphNow(renderer, f, code);
    t0 = SDL_GetPerformanceCounter();
    g = GlyphNow(renderer, f, code);
    gPortStutter.glyphs++;
    gPortStutter.glyphTicks += SDL_GetPerformanceCounter() - t0;
    return g;
}

static HdGlyph* GlyphNow(SDL_Renderer* renderer, HdFont* f, int code) {
    HdGlyph* g = &f->glyphs[code & 255];
    char c = (char)code;
    WCHAR wc;
    Bitmap bm;
    int ok;

    if (g->ready)
        return g;
    g->ready = 1;
    if (MultiByteToWideChar(1252, 0, &c, 1, &wc, 1) != 1)
        return g;
    if (HasGlyph(f->font, wc))
        ok = Render(f->font, wc, &bm);
    else if (wc == 0xBF || wc == 0xA1)
        ok = Turned(f, wc, &bm) || Render(f->fallback, wc, &bm);
    else
        ok = Compose(f, wc, &bm) || Render(f->fallback, wc, &bm);
    if (ok)
        GlyphTexture(renderer, g, &bm);
    return g;
}

#include "port/port_hdtext_layout.inc"

void PortHdTextShutdown(void) {
    int style, slot;
    if (sDc) SelectObject(sDc, GetStockObject(SYSTEM_FONT));
    for (style = 0; style < 4; style++)
        for (slot = 0; slot < 2; slot++) FontRelease(&sFonts[style][slot]);
    if (sDc) DeleteDC(sDc);
    sDc = NULL;
    for (slot = 0; slot < 2; slot++) {
        if (sEmbeddedFonts[slot]) RemoveFontMemResourceEx(sEmbeddedFonts[slot]);
        if (sFontFiles[slot][0]) RemoveFontResourceExW(sFontFiles[slot], FR_PRIVATE, NULL);
        sEmbeddedFonts[slot] = NULL;
        sFontFiles[slot][0] = 0;
        sFace[slot] = L"Segoe UI";
    }
    SDL_memset(sNextFont, 0, sizeof(sNextFont));
    sCustomTried = 0;
}

int PortCodepointToSjis(unsigned cp) {
    WCHAR w = (WCHAR)cp;
    char b[4];
    int n = WideCharToMultiByte(932, 0, &w, 1, b, sizeof(b), NULL, NULL);

    return n == 2 ? ((uint8_t)b[0] << 8) | (uint8_t)b[1] : 0;
}
