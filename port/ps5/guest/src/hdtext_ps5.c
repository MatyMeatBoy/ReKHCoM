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
 * The font is assets/fonts/KHMenu.otf next to the executable (Segoe UI when
 * it is missing). Letters a font lacks are built from it where possible:
 * accented letters are the base letter with the accent of the fallback font
 * (Segoe UI), and the Spanish opening marks are ? and ! turned over; anything
 * else comes from the fallback font.
 */
#include <SDL3/SDL.h>

#include "port/port.h"

/* The glyphs are rasterised with stb_truetype from the project's own fonts (assets/fonts), where the Windows build
   asks GDI. Japanese needs a CJK font and is not available here: that version keeps the cartridge's text. */
#define STBTT_ifloor(x) ((int)__builtin_floor(x))
#define STBTT_iceil(x) ((int)__builtin_ceil(x))
#define STBTT_sqrt(x) __builtin_sqrt(x)
#define STBTT_pow(x, y) pow(x, y)
#define STBTT_fmod(x, y) fmod(x, y)
#define STBTT_cos(x) cos(x)
#define STBTT_acos(x) (0.0)
#define STBTT_fabs(x) __builtin_fabs(x)
#define STBTT_malloc(x, u) ((void)(u), malloc(x))
#define STBTT_free(x, u) ((void)(u), free(x))
#define STBTT_assert(x) ((void)0)
#define STBTT_strlen(x) strlen(x)
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

typedef uint16_t WCHAR;
struct Inst;
typedef struct Inst* HFONT;


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

/* A font file and one size of it. */
typedef struct Face {
    stbtt_fontinfo info;
    uint8_t* data;
    int loaded;
} Face;

typedef struct Inst {
    Face* face;
    float scale;  /* pixels per font unit */
    float xk;     /* horizontal condensing, 1 = none */
} Inst;

static HdFont sFonts[4][2]; /* [style][cache slot]; style 2: numbers, menu face */
static int sNextFont[4];
static int sCustomTried;
static Face sFace[2]; /* KHMenu (menu, dialogue), Comic Hearts (movie subtitles) */

static const char* AssetsRoot(void) {
    const char* env = SDL_getenv("KHCOM_ASSETS");
    return env && *env ? env : "/app0/assets";
}

static void LoadCustomFont(void) {
    static const char* const files[2] = { "fonts/KHMenu.otf", "fonts/ComicHearts.otf" };
    int i;

    if (sCustomTried)
        return;
    sCustomTried = 1;
    for (i = 0; i < 2; i++) {
        char path[1024];
        FILE* f;
        long size;

        SDL_snprintf(path, sizeof(path), "%s/%s", AssetsRoot(), files[i]);
        f = fopen(path, "rb");
        if (f == NULL) {
            PortLog("HD text: unable to load %s", path);
            continue;
        }
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        fseek(f, 0, SEEK_SET);
        sFace[i].data = SDL_malloc((size_t)size);
        if (sFace[i].data != NULL && fread(sFace[i].data, 1, (size_t)size, f) == (size_t)size
            && stbtt_InitFont(&sFace[i].info, sFace[i].data, 0)) {
            sFace[i].loaded = 1;
            PortLog("HD text: font %s", path);
        } else {
            PortLog("HD text: %s is not a usable font", path);
        }
        fclose(f);
    }
}

/* port_ppu.c: HD text only with its face loaded; without it the cartridge's text stays (#277) */
int PortPortableHdReady(void) {
    LoadCustomFont();
    return sFace[0].loaded;
}

static HFONT MakeFont(Face* face, int px, float xk) {
    Inst* f;

    if (face == NULL || !face->loaded)
        return NULL;
    f = SDL_calloc(1, sizeof(*f));
    if (f != NULL) {
        f->face = face;
        f->scale = stbtt_ScaleForMappingEmToPixels(&face->info, (float)px);
        f->xk = xk;
    }
    return f;
}

static int GlyphBox(const Inst* f, int cp, int* x0, int* y0, int* x1, int* y1) {
    int g = stbtt_FindGlyphIndex(&f->face->info, cp);

    if (g == 0)
        return 0;
    stbtt_GetGlyphBitmapBox(&f->face->info, g, f->scale * f->xk, f->scale, x0, y0, x1, y1);
    return 1;
}

/* Font of the face whose capital H is `cap` pixels tall. A face set wider
   than the game's pixel font is condensed to its widths, so lines keep
   inside their windows. */
static HFONT FontWithCap(Face* face, float cap, float scale) {
    static const char sample[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const uint16_t* widths = PortLatinGlyphWidths();
    float game = 0, width = 0;
    int px = (int)(cap * 1.5f + 0.5f), i, n = (int)sizeof(sample) - 1;
    int x0, y0, x1, y1;
    Inst* f = MakeFont(face, px, 1.0f);

    if (f == NULL)
        return NULL;
    if (GlyphBox(f, 'H', &x0, &y0, &x1, &y1) && y1 - y0 > 0) {
        int fit = (int)(px * cap / (float)(y1 - y0) + 0.5f);
        if (fit != px && fit > 0) {
            SDL_free(f);
            px = fit;
            f = MakeFont(face, px, 1.0f);
        }
    }
    for (i = 0; i < n; i++) {
        int adv, lsb;
        stbtt_GetCodepointHMetrics(&face->info, sample[i], &adv, &lsb);
        width += adv * f->scale;
        game += widths[(uint8_t)sample[i]] * scale;
    }
    if (width > game * 1.02f)
        f->xk = game / width;
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
    if (f->fallback != NULL && f->fallback != f->font)
        SDL_free(f->fallback);
    if (f->font != NULL)
        SDL_free(f->font);
    SDL_memset(f, 0, sizeof(*f));
}

static HdFont* FontForScale(float scale, int style) {
    int key = (int)(scale * 16.0f + 0.5f);
    HdFont* f;
    int i;

    for (i = 0; i < 2; i++) {
        if (sFonts[style][i].font != NULL && sFonts[style][i].key == key)
            return &sFonts[style][i];
    }
    LoadCustomFont();
    f = &sFonts[style][sNextFont[style]];
    sNextFont[style] ^= 1;
    FontRelease(f);
    f->key = key;
    f->cap = HD_TEXT_CAP * scale;
    f->font = FontWithCap(&sFace[style == 1 ? 1 : 0], f->cap, scale);
    /* what the face lacks (the subtitle face has no accents) comes from the menu face */
    f->fallback = style == 1 ? FontWithCap(&sFace[0], f->cap, scale) : f->font;
    return f;
}

static int HasGlyph(HFONT font, WCHAR wc) {
    return font != NULL && stbtt_FindGlyphIndex(&font->face->info, wc) != 0;
}

static int Render(HFONT font, WCHAR wc, Bitmap* out) {
    int x0, y0, x1, y1, adv, lsb;

    SDL_memset(out, 0, sizeof(*out));
    if (font == NULL || !GlyphBox(font, wc, &x0, &y0, &x1, &y1))
        return 0;
    stbtt_GetCodepointHMetrics(&font->face->info, wc, &adv, &lsb);
    out->advance = (int)(adv * font->scale * font->xk + 0.5f);
    out->left = x0;
    out->top = -y0;
    out->w = x1 - x0;
    out->h = y1 - y0;
    if (out->w > 0 && out->h > 0) {
        out->a = SDL_calloc((size_t)out->w * out->h, 1);
        if (out->a == NULL) {
            out->w = out->h = 0;
            return 0;
        }
        stbtt_MakeCodepointBitmap(&font->face->info, out->a, out->w, out->h, out->w, font->scale * font->xk, font->scale, wc);
    } else {
        out->w = out->h = 0;
    }
    return 1;
}

/* The Windows-1252 byte as Unicode. */
static WCHAR Cp1252(uint8_t c) {
    static const uint16_t high[32] = {
        0x20AC, 0, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017D, 0,
        0, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0, 0x017E, 0x0178
    };
    return c >= 0x80 && c < 0xA0 ? high[c - 0x80] : c;
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

/* Japanese glyphs are not available here: nothing is drawn (that version runs with HD text off). */
static HdGlyph* GlyphJp(SDL_Renderer* renderer, HdFont* f, uint16_t code) {
    static HdGlyph sNone;
    (void)renderer; (void)f; (void)code;
    return &sNone;
}

static HdGlyph* Glyph(SDL_Renderer* renderer, HdFont* f, int code) {
    HdGlyph* g = &f->glyphs[code & 255];
    WCHAR wc = Cp1252((uint8_t)code);
    Bitmap bm;
    int ok;

    if (g->ready)
        return g;
    g->ready = 1;
    if (wc == 0)
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

static int CompareGlyphs(const void* a, const void* b) {
    const PortTextGlyph* p = a;
    const PortTextGlyph* q = b;

    if (p->y != q->y)
        return p->y - q->y;
    return p->x - q->x;
}

static void DrawGlyph(SDL_Renderer* r, HdGlyph* g, float x, float y, uint32_t color, int alpha) {
    SDL_FRect d;

    if (g->tex == NULL)
        return;
    d.x = SDL_floorf(x + 0.5f) + g->left;
    d.y = SDL_floorf(y + 0.5f) - g->top;
    d.w = (float)g->w;
    d.h = (float)g->h;
    SDL_SetTextureColorMod(g->tex, (color >> 16) & 255, (color >> 8) & 255, color & 255);
    SDL_SetTextureAlphaMod(g->tex, (Uint8)alpha);
    SDL_RenderTexture(r, g->tex, NULL, &d);
}

/* Experimental card digits: outline glyphs fitted to their native ink box,
   including the card's affine animation. Values still come from game code. */
void PortHdTextHudDigits(SDL_Renderer* renderer, float originX, float originY, float scale) {
    PortHudDigit digits[128];
    HdFont* font;
    int i, n;
    if (!gPortConfig.hdHud) return;
    n = PortPpuHudDigits(digits, 128);
    if (!n) return;
    font = FontForScale(8.0f, 2);
    if (!font->font) return;
    for (i = 0; i < n; i++) {
        HdGlyph* g = Glyph(renderer, font, digits[i].code);
        SDL_FRect box = {originX + digits[i].x * scale, originY + digits[i].y * scale,
                         digits[i].w * scale, digits[i].h * scale};
        uint32_t color = digits[i].color;
        if (!g->tex || !digits[i].w || !digits[i].h) continue;
        if (SDL_getenv("KHCOM_HDHUDLOG")) PortLog("HD HUD digit %c: slot %d box %d,%d %dx%d", digits[i].code,
                                                digits[i].slot, digits[i].x, digits[i].y, digits[i].w, digits[i].h);
        SDL_SetTextureScaleMode(g->tex, SDL_strcmp(SDL_GetRendererName(renderer), "software") == 0 ?
                               SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR);
        SDL_SetTextureColorMod(g->tex, color >> 16, color >> 8, color);
        SDL_SetTextureAlphaMod(g->tex, 255);
        {
            int x, y;
            for (y = 0; y < digits[i].h; y++) for (x = 0; x < digits[i].w; x++) {
                SDL_FRect source = {x * g->w / (float)digits[i].w, y * g->h / (float)digits[i].h,
                                    g->w / (float)digits[i].w, g->h / (float)digits[i].h};
                SDL_FRect cell = {box.x + x * scale, box.y + y * scale, scale, scale};
                if (PortPpuHudDigitVisible(i, digits[i].x + x, digits[i].y + y))
                    SDL_RenderTexture(renderer, g->tex, &source, &cell);
            }
        }
    }
}

static void DrawFlowing(SDL_Renderer* renderer, PortTextGlyph* glyphs, int n, float originX, float originY, float scale) {
    const uint16_t* widths = PortLatinGlyphWidths();
    HdFont* font;
    float shadow = SDL_max(1.0f, SDL_floorf(scale * 0.5f));
    int i, j, pass, lineY, lineGameEnd = 0;
    float space, lineEnd;

    font = FontForScale(scale, 0);
    if (font->font == NULL)
        return;

    /* Japanese glyphs keep their cells on the game's grid; the rest flow. */
    for (i = j = 0; i < n; i++) {
        if (glyphs[i].code > 0xFF) {
            HdGlyph* g = GlyphJp(renderer, font, glyphs[i].code);
            float x = originX + glyphs[i].x * scale + (HD_TEXT_JP_CELL * scale - g->advance) * 0.5f;
            float base = originY + (glyphs[i].y + HD_TEXT_BASELINE) * scale;
            int alpha = glyphs[i].semi ? glyphs[i].semi : 0xFF;
            DrawGlyph(renderer, g, x + shadow, base + shadow, glyphs[i].shadow, alpha);
            DrawGlyph(renderer, g, x, base, glyphs[i].face, alpha);
        } else {
            glyphs[j++] = glyphs[i];
        }
    }
    n = j;
    if (n == 0)
        return;

    /* Words: glyphs on one line, each where the previous one ends. */
    space = (float)Glyph(renderer, font, ' ')->advance;
    lineY = -1;
    lineEnd = 0;
    for (i = 0; i < n; i = j) {
        float start, pen, baseline;

        for (j = i + 1; j < n; j++) {
            const PortTextGlyph* p = &glyphs[j - 1];
            if (glyphs[j].y != p->y || glyphs[j].x != p->x + widths[p->code & 255])
                break;
        }
        start = originX + glyphs[i].x * scale;
        if (glyphs[i].y == lineY) {
            /* Words of a sentence follow each other with the font's own
               space; a wider gap is a column and keeps its place. */
            int gap = glyphs[i].x - lineGameEnd, gameSpace = SDL_max(1, widths[' ']);
            if (gap <= gameSpace * 2)
                start = lineEnd + space * SDL_max(1, (gap + gameSpace / 2) / gameSpace);
            else if (start < lineEnd + space)
                start = lineEnd + space;
        }
        baseline = originY + (glyphs[i].y + HD_TEXT_BASELINE) * scale;
        for (pass = 0; pass < 2; pass++) {
            int t;
            pen = start;
            for (t = i; t < j; t++) {
                HdGlyph* g = Glyph(renderer, font, glyphs[t].code);
                int alpha = glyphs[t].semi ? glyphs[t].semi : 0xFF;
                if (pass == 0)
                    DrawGlyph(renderer, g, pen + shadow, baseline + shadow, glyphs[t].shadow, alpha);
                else
                    DrawGlyph(renderer, g, pen, baseline, glyphs[t].face, alpha);
                pen += g->advance;
            }
        }
        lineY = glyphs[i].y;
        lineEnd = pen;
        lineGameEnd = glyphs[j - 1].x + widths[glyphs[j - 1].code & 255];
    }
}

/* Movie subtitles: each line is centred on the game's own line, so the
   outline font's other widths keep it where the cartridge puts it. */
static void DrawSubtitles(SDL_Renderer* renderer, PortTextGlyph* glyphs, int n, float originX, float originY,
                          float scale) {
    const uint16_t* widths = PortLatinGlyphWidths();
    HdFont* font = FontForScale(scale, 1);
    float shadow = SDL_max(1.0f, SDL_floorf(scale * 0.5f));
    float space;
    int i, j, gameSpace = SDL_max(1, widths[' ']);

    if (font->font == NULL)
        return;
    space = (float)Glyph(renderer, font, ' ')->advance;
    for (i = 0; i < n; i = j) {
        float hdWidth = 0, pen, baseline;
        int t, prevEnd = glyphs[i].x, gameStart = glyphs[i].x, gameEnd;

        for (j = i; j < n && glyphs[j].y == glyphs[i].y; j++) {
            int jp = glyphs[j].code > 0xFF, gap = glyphs[j].x - prevEnd;
            if (j > i && gap > 0)
                hdWidth += space * SDL_max(1, (gap + gameSpace / 2) / gameSpace);
            hdWidth += jp ? HD_TEXT_JP_CELL * scale : (float)Glyph(renderer, font, glyphs[j].code)->advance;
            prevEnd = glyphs[j].x + (jp ? HD_TEXT_JP_CELL : widths[glyphs[j].code == 0x97 ? 25 : glyphs[j].code & 255]);
        }
        gameEnd = prevEnd;
        pen = originX + ((gameStart + gameEnd) * 0.5f) * scale - hdWidth * 0.5f;
        baseline = originY + (glyphs[i].y + HD_TEXT_BASELINE) * scale;
        prevEnd = gameStart;
        for (t = i; t < j; t++) {
            int jp = glyphs[t].code > 0xFF, gap = glyphs[t].x - prevEnd, alpha = glyphs[t].semi ? glyphs[t].semi : 0xFF;
            if (t > i && gap > 0)
                pen += space * SDL_max(1, (gap + gameSpace / 2) / gameSpace);
            if (jp) {
                HdGlyph* g = GlyphJp(renderer, font, glyphs[t].code);
                float x = pen + (HD_TEXT_JP_CELL * scale - g->advance) * 0.5f;
                DrawGlyph(renderer, g, x + shadow, baseline + shadow, glyphs[t].shadow, alpha);
                DrawGlyph(renderer, g, x, baseline, glyphs[t].face, alpha);
                pen += HD_TEXT_JP_CELL * scale;
                prevEnd = glyphs[t].x + HD_TEXT_JP_CELL;
            } else {
                HdGlyph* g = Glyph(renderer, font, glyphs[t].code);
                DrawGlyph(renderer, g, pen + shadow, baseline + shadow, glyphs[t].shadow, alpha);
                DrawGlyph(renderer, g, pen, baseline, glyphs[t].face, alpha);
                pen += g->advance;
                prevEnd = glyphs[t].x + widths[glyphs[t].code == 0x97 ? 25 : glyphs[t].code & 255];
            }
        }
    }
}

/* Small numeric HUD cells (EXP and next-level counters). Keep each cell in
   place instead of applying dialogue word spacing to tabular numbers. */
static void DrawSmallNumbers(SDL_Renderer* renderer, PortTextGlyph* glyphs, int n,
                             float originX, float originY, float scale) {
    float shadow = SDL_max(1.0f, SDL_floorf(scale * 0.5f));
    int i;
    for (i = 0; i < n; i++) {
        /* each number at the height and baseline of its own pixel digit */
        int cap = glyphs[i].cap ? glyphs[i].cap : 5, base = glyphs[i].base ? glyphs[i].base : 7;
        HdFont* font = FontForScale(scale * (cap / HD_TEXT_CAP), 2);
        HdGlyph* glyph;
        float x, baseline;
        if (font->font == NULL)
            return;
        glyph = Glyph(renderer, font, glyphs[i].code);
        x = originX + glyphs[i].x * scale + (8.0f * scale - glyph->advance) * 0.5f;
        baseline = originY + (glyphs[i].y + base) * scale;
        int alpha = glyphs[i].semi ? glyphs[i].semi : 0xFF;
        if (glyphs[i].clip1) {
            /* part of the cell is under a window: draw only the rest */
            SDL_Rect clip;
            clip.x = (int)SDL_floorf(originX + (glyphs[i].x + glyphs[i].clip0) * scale);
            clip.w = (int)SDL_ceilf(originX + (glyphs[i].x + glyphs[i].clip1) * scale) - clip.x;
            clip.y = (int)SDL_floorf(originY + (glyphs[i].y - 4) * scale);
            clip.h = (int)SDL_ceilf(16 * scale);
            SDL_SetRenderClipRect(renderer, &clip);
        }
        DrawGlyph(renderer, glyph, x + shadow, baseline + shadow, glyphs[i].shadow, alpha);
        DrawGlyph(renderer, glyph, x, baseline, glyphs[i].face, alpha);
        if (glyphs[i].clip1)
            SDL_SetRenderClipRect(renderer, NULL);
    }
}

void PortHdTextDraw(SDL_Renderer* renderer, int hudLayer, float originX, float originY, float scale) {
    static PortTextGlyph glyphs[HD_TEXT_MAX_GLYPHS];
    int n, i, flowing = 0, subs, smallCount;

    if (!gPortConfig.hdText || scale <= 0.0f)
        return;
    n = PortPpuTextGlyphs(glyphs, HD_TEXT_MAX_GLYPHS, hudLayer);
    if (n == 0)
        return;
    SDL_qsort(glyphs, n, sizeof(glyphs[0]), CompareGlyphs);
    /* style 0 first (in place), then the subtitles after them */
    {
        static PortTextGlyph sub[HD_TEXT_MAX_GLYPHS];
        static PortTextGlyph small[HD_TEXT_MAX_GLYPHS];
        for (i = subs = smallCount = 0; i < n; i++) {
            if (glyphs[i].style == 2)
                small[smallCount++] = glyphs[i];
            else if (glyphs[i].style)
                sub[subs++] = glyphs[i];
            else
                glyphs[flowing++] = glyphs[i];
        }
        /* Opt-in evidence of the glyph classes actually emitted by the PPU. */
        {
            const char* trace = SDL_getenv("KHCOM_HD_TEXT_TRACE");
            while (trace && *trace) {
                const char* comma = SDL_strchr(trace, ',');
                if ((uint32_t)SDL_atoi(trace) == PortFrameCount()) {
                    PortLog("hd-text: frame=%u layer=%d flowing=%d subtitles=%d small=%d",
                            (unsigned)PortFrameCount(), hudLayer, flowing, subs, smallCount);
                    break;
                }
                trace = comma ? comma + 1 : NULL;
            }
        }
        if (flowing > 0)
            DrawFlowing(renderer, glyphs, flowing, originX, originY, scale);
        if (subs > 0)
            DrawSubtitles(renderer, sub, subs, originX, originY, scale);
        if (smallCount > 0)
            DrawSmallNumbers(renderer, small, smallCount, originX, originY, scale);
    }
}


/* Shift-JIS code of a Unicode character (two-byte codes only), 0 if none: the options overlay draws Japanese with the
   cartridge's own font. Only ASCII is known here (no code page tables). */
int PortCodepointToSjis(unsigned cp) {
    return cp < 128 ? (int)cp : 0;
}

#define PORT_HD_TEXT_SIMPLE_DRAW
#include "port/port_hdtext_ui.inc"
#undef PORT_HD_TEXT_SIMPLE_DRAW

void PortHdTextShutdown(void) {
    int style, slot;
    for (style = 0; style < 4; style++)
        for (slot = 0; slot < 2; slot++) FontRelease(&sFonts[style][slot]);
    for (slot = 0; slot < 2; slot++) {
        SDL_free(sFace[slot].data);
        SDL_memset(&sFace[slot], 0, sizeof(sFace[slot]));
    }
    SDL_memset(sNextFont, 0, sizeof(sNextFont));
    sCustomTried = 0;
}
