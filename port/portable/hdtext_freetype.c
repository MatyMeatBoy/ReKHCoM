/* Portable outline rasterizer; the game's layout/occlusion is shared with
   Windows/Web. FreeType handles the existing CFF OpenType fonts directly. */
#include <SDL3/SDL.h>
#include <stdio.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "port/port.h"
#define HD_TEXT_CAP 7.0f
#define HD_TEXT_BASELINE 11
#define HD_TEXT_MAX_GLYPHS 256
#define HD_TEXT_JP_CELL 10
#define HD_TEXT_JP_CACHE 1024
typedef struct HdGlyph { SDL_Texture* tex; int ready,w,h,left,top,advance; } HdGlyph;
typedef struct HdFont {
    FT_Face font;
    int key,style;
    float cap,scale,condense;
    HdGlyph glyphs[256],jpGlyphs[HD_TEXT_JP_CACHE];
    uint16_t jpCodes[HD_TEXT_JP_CACHE];
} HdFont;
static FT_Library library;
static unsigned char* data[2];
static size_t sizes[2];
static HdFont fonts[4][2];
static int nextFont[4], tried, ready;
static FT_Face fallback, japanese;
static char sFallbackPath[1024]; /* the fallback face's file: Options can choose it for the menus (hd_font=2) */
const uint16_t* PortLatinGlyphWidths(void);
uint32_t PortFrameCount(void);
unsigned PortSjisToCodepoint(unsigned code);

/* First usable CJK face: KHCOM_CJK_FONT, common distro/Steam Deck paths, then
   whatever fontconfig maps to Japanese. */
static void OpenJapanese(void) {
    static const char* const paths[] = {
        "/system/fonts/NotoSansCJK-Regular.ttc", "/system/fonts/NotoSansCJKjp-Regular.otf",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/google-noto-cjk/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/google-noto-sans-cjk-fonts/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/OTF/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJKjp-Regular.otf",
        "/usr/share/fonts/truetype/takao-gothic/TakaoGothic.ttf",
        "/usr/share/fonts/truetype/fonts-japanese-gothic.ttf",
        "/usr/share/fonts/ipa-gothic/ipag.ttf", "/usr/share/fonts/TTF/ipag.ttf",
        "/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf",
        "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc"
    };
    const char* env=SDL_getenv("KHCOM_CJK_FONT");
    size_t i;
    if (env && *env) FT_New_Face(library,env,0,&japanese);
    for (i=0;!japanese && i<SDL_arraysize(paths);i++) FT_New_Face(library,paths[i],0,&japanese);
#if defined(__linux__) && !defined(__ANDROID__)
    if (!japanese) {
        FILE* pipe=popen("fc-match -f '%{file}' :lang=ja 2>/dev/null","r");
        if (pipe) {
            char path[1024]; size_t n=fread(path,1,sizeof(path)-1,pipe); path[n]=0;
            pclose(pipe);
            if (n && FT_New_Face(library,path,0,&japanese)) japanese=NULL;
        }
    }
#endif
    if (japanese) PortLog("HD text: CJK face found");
}

int PortPortableHdReady(void) {
    static const char* names[2] = {"KHMenu.otf","ComicHearts.otf"};
    int i;
    if (tried) return ready;
    tried=1;
    if (FT_Init_FreeType(&library)) return 0;
    for (i=0;i<2;i++) {
        char path[1024]; FT_Face face;
        SDL_snprintf(path,sizeof(path),"assets/fonts/%s",names[i]);
        data[i]=SDL_LoadFile(PortConfigPath(path),&sizes[i]);
        if (!data[i] && SDL_GetBasePath()) { /* next to the executable, or bin/../assets in the packaged game */
            char full[1024];
            SDL_snprintf(full,sizeof(full),"%s%s",SDL_GetBasePath(),path);data[i]=SDL_LoadFile(full,&sizes[i]);
            if (!data[i]) {SDL_snprintf(full,sizeof(full),"%s../%s",SDL_GetBasePath(),path);data[i]=SDL_LoadFile(full,&sizes[i]);}
        }
        if (!data[i]) {SDL_snprintf(path,sizeof(path),"fonts/%s",names[i]);data[i]=SDL_LoadFile(path,&sizes[i]);}
        if (!data[i]) PortLog("HD text: %s not found; the game's own text stays",names[i]);
        if (!data[i] || FT_New_Memory_Face(library,data[i],sizes[i],0,&face)) return 0;
        if (!FT_Get_Char_Index(face,'H')) {FT_Done_Face(face);return 0;}
        FT_Done_Face(face);
    }
    /* Optional platform fallbacks; no platform fonts are redistributed. */
    if (!FT_New_Face(library,"/system/fonts/Roboto-Regular.ttf",0,&fallback))
        SDL_strlcpy(sFallbackPath,"/system/fonts/Roboto-Regular.ttf",sizeof(sFallbackPath));
    if (!fallback && !FT_New_Face(library,"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",0,&fallback))
        SDL_strlcpy(sFallbackPath,"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",sizeof(sFallbackPath));
    {
        /* Comic Hearts has no accents: without a fallback face the first accented letter turns HD text
           off (#77, Bazzite/Fedora keep DejaVu elsewhere) */
        static const char* const paths[] = {
            "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf", "/usr/share/fonts/TTF/DejaVuSans.ttf",
            "/usr/share/fonts/dejavu/DejaVuSans.ttf", "/usr/share/fonts/google-noto/NotoSans-Regular.ttf",
            "/usr/share/fonts/noto/NotoSans-Regular.ttf", "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
            "/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", "C:/Windows/Fonts/arial.ttf"
        };
        size_t k;
        for (k=0;!fallback && k<SDL_arraysize(paths);k++)
            if (!FT_New_Face(library,paths[k],0,&fallback)) SDL_strlcpy(sFallbackPath,paths[k],sizeof(sFallbackPath));
    }
#if defined(__linux__) && !defined(__ANDROID__)
    if (!fallback) {
        FILE* pipe=popen("fc-match -f '%{file}' sans:lang=es 2>/dev/null","r");
        if (pipe) {
            char path[1024]; size_t n=fread(path,1,sizeof(path)-1,pipe); path[n]=0;
            pclose(pipe);
            if (n && FT_New_Face(library,path,0,&fallback)) fallback=NULL;
            else if (n) SDL_strlcpy(sFallbackPath,path,sizeof(sFallbackPath));
        }
    }
#endif
    PortLog("HD text: fallback face %s",fallback?"found":"missing (accents fall back to the game's text)");
    OpenJapanese();
#ifdef VERSION_JP
    /* Never hide Japanese SD glyphs without a usable Japanese outline face. */
    if (!japanese) return 0;
#endif
    ready=1;PortLog("HD text: portable KHMenu / Comic Hearts ready");return 1;
}
static void Release(HdFont* f) {
    int i;
    for(i=0;i<256;i++)SDL_DestroyTexture(f->glyphs[i].tex);
    for(i=0;i<HD_TEXT_JP_CACHE;i++)SDL_DestroyTexture(f->jpGlyphs[i].tex);
    if(f->font)FT_Done_Face(f->font);
    SDL_memset(f,0,sizeof(*f));
}
static HdFont* FontForScale(float scale,int style) {
    static const char sample[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const uint16_t* widths=PortLatinGlyphWidths();
    int choice=gPortConfig.hdFont==2?2:0; /* KHMenu or the system's; Comic Hearts: subtitles only */
    int key=(int)(scale*16+.5f)+choice*100000,i,top,bottom,px;float natural=0,game=0; /* the face chosen too */
    HdFont* f;
    for(i=0;i<2;i++)if(fonts[style][i].font && fonts[style][i].key==key)return &fonts[style][i];
    f=&fonts[style][nextFont[style]];nextFont[style]^=1;Release(f);
    if(!PortPortableHdReady())return f;
    /* menus and dialogue in the face chosen in Options (KHMenu, Comic Hearts or the system's); subtitles keep theirs */
    if(style!=1 && choice==2 && sFallbackPath[0]) { if(FT_New_Face(library,sFallbackPath,0,&f->font))f->font=NULL; }
    else if(FT_New_Memory_Face(library,data[style==1 || choice==1],sizes[style==1 || choice==1],0,&f->font))f->font=NULL;
    if(!f->font)return f;
    f->key=key;f->style=style==3?0:style;f->scale=scale;f->cap=HD_TEXT_CAP*scale;
    FT_Load_Char(f->font,'H',FT_LOAD_NO_SCALE);
    top=(int)f->font->glyph->metrics.height;
    px=top>0?(int)(f->cap*f->font->units_per_EM/top+.5f):(int)(f->cap*1.5f+.5f);
    FT_Set_Pixel_Sizes(f->font,0,SDL_max(1,px));
    for(i=0;sample[i];i++) {
        if(!FT_Load_Char(f->font,sample[i],FT_LOAD_DEFAULT))natural+=f->font->glyph->advance.x/64.0f;
        game+=widths[(unsigned char)sample[i]]*scale;
    }
    f->condense=natural>game*1.02f?game/natural:1.0f;
    (void)bottom;return f;
}
static unsigned Unicode(int code,int jp) {
    static const uint16_t cp1252[32]={0x20AC,0x81,0x201A,0x192,0x201E,0x2026,0x2020,0x2021,
        0x2C6,0x2030,0x160,0x2039,0x152,0x8D,0x17D,0x8F,0x90,0x2018,0x2019,0x201C,
        0x201D,0x2022,0x2013,0x2014,0x2DC,0x2122,0x161,0x203A,0x153,0x9D,0x17E,0x178};
    return jp?PortSjisToCodepoint(code):code>=128&&code<160?cp1252[code-128]:(unsigned)code;
}
/* Warmup enumerates optional characters, including undefined CP1252 slots and
   arrows on systems without CJK fonts. Do not turn a speculative miss into a
   permanent loss of HD text. Actual requested glyph failures still take the
   existing original-text fallback in Raster. */
static int CanWarmGlyph(HdFont* font, int code, int jp) {
    unsigned cp = Unicode(code, jp);
    FT_Face face = jp ? japanese : font->font;
    if (!face || !cp) return 0;
    if (FT_Get_Char_Index(face, cp)) return 1;
    return !jp && fallback && FT_Get_Char_Index(fallback, cp);
}
#define PORT_HD_TEXT_CAN_WARM(font, code, jp) CanWarmGlyph(font, code, jp)
static void Raster(SDL_Renderer* renderer,HdFont* f,HdGlyph* g,int code,int jp) {
    FT_Face face=f->font;FT_Matrix matrix;FT_Bitmap* b;unsigned cp=Unicode(code,jp);uint32_t* rgba;int x,y;
    if(!ready || g->ready)return;g->ready=1;
    if(!face || !cp)return;
    if(jp)face=japanese;
    else if(!FT_Get_Char_Index(face,cp))face=fallback;
    if(!face || !FT_Get_Char_Index(face,cp)) {
        ready=0;
        PortLog("HD text: missing glyph U+%04X; original text resumes next frame", cp);
        return;
    }
    if(face!=f->font) {
        FT_Set_Pixel_Sizes(face,0,SDL_max(1,(int)(f->cap*(jp?11.0f/7.0f:1.4f)+.5f)));
    }
    matrix.xx=(FT_Fixed)((jp?1:f->condense)*65536);matrix.xy=matrix.yx=0;matrix.yy=65536;
    FT_Set_Transform(face,&matrix,NULL);
    if(FT_Load_Char(face,cp,FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL)){ready=0;return;}
    b=&face->glyph->bitmap;g->w=b->width;g->h=b->rows;g->left=face->glyph->bitmap_left;
    g->top=face->glyph->bitmap_top;g->advance=(face->glyph->advance.x+32)>>6;
    if(!g->w || !g->h)return;
    if(b->pixel_mode!=FT_PIXEL_MODE_GRAY || g->w>512 || g->h>512){ready=0;return;}
    rgba=SDL_malloc((size_t)g->w*g->h*4);if(!rgba){ready=0;return;}
    for(y=0;y<g->h;y++)for(x=0;x<g->w;x++) {
        const unsigned char* row=b->buffer+(b->pitch<0?(g->h-1-y)*-b->pitch:y*b->pitch);
        rgba[y*g->w+x]=((uint32_t)row[x]<<24)|0xFFFFFF;
    }
    g->tex=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STATIC,g->w,g->h);
    if(g->tex && SDL_UpdateTexture(g->tex,NULL,rgba,g->w*4)) {
        SDL_SetTextureBlendMode(g->tex,SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(g->tex,SDL_SCALEMODE_NEAREST);
    } else {
        if(g->tex)SDL_DestroyTexture(g->tex);
        g->tex=NULL;g->ready=0;ready=0;
        /* The current PPU pass may already have omitted SD glyphs. */
        PortLog("HD text: glyph texture rejected; original text resumes next frame");
    }
    SDL_free(rgba);
}
static HdGlyph* Glyph(SDL_Renderer* r,HdFont* f,int code) {HdGlyph* g=&f->glyphs[code&255];Raster(r,f,g,code&255,0);return g;}
static HdGlyph* GlyphJp(SDL_Renderer* r,HdFont* f,uint16_t code) {
    static HdGlyph none;unsigned slot=(code*2654435761u)>>22,k;
    for(k=0;k<HD_TEXT_JP_CACHE;k++,slot=(slot+1)%HD_TEXT_JP_CACHE)
        if(!f->jpCodes[slot] || f->jpCodes[slot]==code){f->jpCodes[slot]=code;Raster(r,f,&f->jpGlyphs[slot],code,1);return &f->jpGlyphs[slot];}
    return &none;
}
#include "port/port_hdtext_layout.inc"

void PortHdTextShutdown(void) {
    int style, slot;
    for (style = 0; style < 4; style++)
        for (slot = 0; slot < 2; slot++) Release(&fonts[style][slot]);
    if (japanese && japanese != fallback) FT_Done_Face(japanese);
    if (fallback) FT_Done_Face(fallback);
    japanese = fallback = NULL;
    if (library) FT_Done_FreeType(library);
    library = NULL;
    for (slot = 0; slot < 2; slot++) {
        SDL_free(data[slot]); data[slot] = NULL; sizes[slot] = 0;
    }
    SDL_memset(nextFont, 0, sizeof(nextFont));
    tried = ready = 0;
    sFallbackPath[0] = 0;
}

/* A movie subtitle (port_hdmovie.c, Linux): the UTF-8 text at px pixels in KHMenu (the system's face, then the
   Japanese one, for letters it lacks), its "\n" lines centred, with pad blank pixels around it. Coverage, w*h bytes;
   the caller SDL_frees it. NULL: no font. */
unsigned char* PortPortableHdTextCover(const char* text, int px, int pad, int* w, int* h) {
    FT_Face face, faces[3];
    int lines = 1, line, widths[32], ascent, lineH, x, n = 0;
    const char* p;
    unsigned char* cover;
    if (!text || !PortPortableHdReady() || FT_New_Memory_Face(library, data[0], sizes[0], 0, &face)) return NULL;
    faces[n++] = face;
    if (fallback) faces[n++] = fallback;
    if (japanese) faces[n++] = japanese;
    for (x = 0; x < n; x++) FT_Set_Pixel_Sizes(faces[x], 0, (FT_UInt)px);
    ascent = (int)(face->size->metrics.ascender >> 6);
    lineH = (int)(face->size->metrics.height >> 6);
    for (p = text; *p; p++) lines += *p == '\n';
    if (lines > 32) lines = 32;
    SDL_zero(widths);
    for (line = 0, p = text; *p && line < 32;) { /* measure each line */
        Uint32 cp = SDL_StepUTF8(&p, NULL);
        if (cp == '\n') { line++; continue; }
        for (x = 0; x < n; x++)
            if (FT_Get_Char_Index(faces[x], cp) || x == n - 1) {
                if (!FT_Load_Char(faces[x], cp, FT_LOAD_DEFAULT)) widths[line] += (int)(faces[x]->glyph->advance.x >> 6);
                break;
            }
    }
    *w = 0;
    for (line = 0; line < lines; line++) *w = SDL_max(*w, widths[line]);
    *w += pad * 2 + 2;
    *h = lines * lineH + pad * 2 + 2;
    cover = SDL_calloc((size_t)*w * *h, 1);
    if (!cover) { FT_Done_Face(face); return NULL; }
    for (line = 0, p = text, x = (*w - widths[0]) / 2; *p && line < 32;) {
        Uint32 cp = SDL_StepUTF8(&p, NULL);
        int f;
        if (cp == '\n') { line++; x = line < 32 ? (*w - widths[line]) / 2 : 0; continue; }
        for (f = 0; f < n; f++)
            if (FT_Get_Char_Index(faces[f], cp) || f == n - 1) break;
        if (FT_Load_Char(faces[f], cp, FT_LOAD_RENDER)) continue;
        {
            FT_GlyphSlot g = faces[f]->glyph;
            int gx, gy, top = pad + 1 + line * lineH + ascent - g->bitmap_top, left = x + g->bitmap_left;
            for (gy = 0; gy < (int)g->bitmap.rows; gy++)
                for (gx = 0; gx < (int)g->bitmap.width; gx++) {
                    int yy = top + gy, xx = left + gx;
                    if (yy >= 0 && xx >= 0 && yy < *h && xx < *w) {
                        unsigned char v = g->bitmap.buffer[gy * g->bitmap.pitch + gx];
                        if (v > cover[yy * *w + xx]) cover[yy * *w + xx] = v;
                    }
                }
            x += (int)(g->advance.x >> 6);
        }
    }
    FT_Done_Face(face);
    return cover;
}
