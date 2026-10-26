/* Browser glyph rasterization; layout/occlusion is shared with the PC port. */
#include <SDL3/SDL.h>
#include <emscripten.h>
#include "port/port.h"
#define HD_TEXT_CAP 7.0f
#define HD_TEXT_BASELINE 11
#define HD_TEXT_MAX_GLYPHS 256
#define HD_TEXT_JP_CELL 10
#define HD_TEXT_JP_CACHE 1024
typedef struct HdGlyph {
    SDL_Texture* tex;
    int ready,w,h,left,top,advance;
} HdGlyph;
typedef struct HdFont {
    void* font;
    int key,style;
    float cap,scale;
    HdGlyph glyphs[256],jpGlyphs[HD_TEXT_JP_CACHE];
    uint16_t jpCodes[HD_TEXT_JP_CACHE];
} HdFont;
static HdFont fonts[4][2];
static int nextFont[4];
static uint8_t scratch[512*512];
int gPortWebHdReady;
const uint16_t* PortLatinGlyphWidths(void);
uint32_t PortFrameCount(void);
EMSCRIPTEN_KEEPALIVE void PortWebSetHdReady(int ready) {gPortWebHdReady=ready;}
EM_JS(int, WebRaster, (int style,float cap,float gameWidth,int code,int jp,void* pixels,int* metrics), {
    return Module.rasterGlyph ? Module.rasterGlyph(style,cap,gameWidth,code,jp,pixels,metrics) : 0;
});
static void Release(HdFont* f) {
    int i;
    for(i=0;i<256;i++)SDL_DestroyTexture(f->glyphs[i].tex);
    for(i=0;i<HD_TEXT_JP_CACHE;i++)SDL_DestroyTexture(f->jpGlyphs[i].tex);
    SDL_memset(f,0,sizeof(*f));
}
void PortWebHdClear(void) {
    int s,k;for(s=0;s<4;s++)for(k=0;k<2;k++)Release(&fonts[s][k]);
}
static HdFont* FontForScale(float scale,int style) {
    int key=(int)(scale*16+.5f),i;
    HdFont* f;
    for(i=0;i<2;i++)if(fonts[style][i].font && fonts[style][i].key==key)return &fonts[style][i];
    f=&fonts[style][nextFont[style]];nextFont[style]^=1;Release(f);
    f->font=gPortWebHdReady ? (void*)1 : NULL;
    f->key=key;f->style=style==3?0:style;f->scale=scale;f->cap=HD_TEXT_CAP*scale;
    return f;
}
static void Raster(SDL_Renderer* renderer,HdFont* f,HdGlyph* g,int code,int jp) {
    static const char sample[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const uint16_t* widths=PortLatinGlyphWidths();
    float gameWidth=0;int i,m[5];
    uint32_t* rgba;
    if(g->ready)return;g->ready=1;
    for(i=0;sample[i];i++)gameWidth+=widths[(unsigned char)sample[i]]*f->scale;
    if(!WebRaster(f->style,f->cap,gameWidth,code,jp,scratch,m)) {gPortWebHdReady=0;return;}
    g->w=m[0];g->h=m[1];g->advance=m[2];g->left=m[3];g->top=m[4];
    if(!g->w || !g->h)return;
    rgba=SDL_malloc((size_t)g->w*g->h*4);
    if(!rgba){gPortWebHdReady=0;return;}
    for(i=0;i<g->w*g->h;i++)rgba[i]=((uint32_t)scratch[i]<<24)|0xFFFFFF;
    g->tex=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STATIC,g->w,g->h);
    if(g->tex){SDL_UpdateTexture(g->tex,NULL,rgba,g->w*4);SDL_SetTextureBlendMode(g->tex,SDL_BLENDMODE_BLEND);SDL_SetTextureScaleMode(g->tex,SDL_SCALEMODE_NEAREST);}
    else gPortWebHdReady=0;
    SDL_free(rgba);
}
static HdGlyph* Glyph(SDL_Renderer* renderer,HdFont* f,int code) {
    HdGlyph* g=&f->glyphs[code&255];Raster(renderer,f,g,code&255,0);return g;
}
static HdGlyph* GlyphJp(SDL_Renderer* renderer,HdFont* f,uint16_t code) {
    static HdGlyph none;
    unsigned slot=(code*2654435761u)>>22,k;
    for(k=0;k<HD_TEXT_JP_CACHE;k++,slot=(slot+1)%HD_TEXT_JP_CACHE) {
        if(f->jpCodes[slot]==code || !f->jpCodes[slot]) {
            f->jpCodes[slot]=code;Raster(renderer,f,&f->jpGlyphs[slot],code,1);return &f->jpGlyphs[slot];
        }
    }
    return &none;
}
#include "port/port_hdtext_layout.inc"

void PortHdTextShutdown(void) {
    for (int style = 0; style < 4; style++)
        for (int slot = 0; slot < 2; slot++) Release(&fonts[style][slot]);
    SDL_memset(nextFont, 0, sizeof(nextFont));
}
