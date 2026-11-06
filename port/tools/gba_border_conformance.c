/* Compile with the extractor's border_output_fixture.inc and production inc.
 * SDL is mocked: real geometry/cache/control flow, no game/GPU/ROM execution. */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "port/gba_border_layout.h"
#include "port/port_aspect.h"
#define BORDER_W 384
#define BORDER_H 216
#define GBA_SCREEN_W 240
#define GBA_SCREEN_H 160
#define SDL_min(a,b) ((a)<(b)?(a):(b))
#define SDL_clamp(v,a,b) ((v)<(a)?(a):(v)>(b)?(b):(v))
#define SDL_roundf roundf
#define SDL_PIXELFORMAT_RGBA32 0
#define SDL_SCALEMODE_NEAREST 0
#define SDL_BLENDMODE_BLEND 1
enum { PORT_VIEW_ORIGINAL, PORT_VIEW_WIDE, PORT_VIEW_EXTENDED, PORT_VIEW_MAXIMUM, PORT_VIEW_AUTO, PORT_VIEW_CRT };
enum { PORT_SCALE_INTEGER, PORT_SCALE_FIT, PORT_SCALE_STRETCH };
typedef struct { float x,y; } SDL_FPoint;
typedef struct { float r,g,b,a; } SDL_FColor;
typedef struct { SDL_FPoint position; SDL_FColor color; SDL_FPoint tex_coord; } SDL_Vertex;
typedef struct { float x,y,w,h; } SDL_FRect;
typedef struct { int w,h; const unsigned char* pixels; } SDL_Surface;
typedef struct { int w,h; } SDL_Texture;
static int renderer;
static int rendererHeight;
static int* sRenderer=&renderer;
static SDL_Texture *sBorderTex,*sBorderMaterialTex;
static int sBorderShown,sBorderAttempted,sRendererResetPending,sFitScale,sAutoScale;
static struct { int gbaBorder,viewMode,crtOutput,scaleMode; } gPortConfig;
static struct { int width,height; } gPortScreen;
float gPortAutoScaleF;
static unsigned char dummy;
static const unsigned char* kGbaBorders[]={&dummy,&dummy,&dummy};
static const unsigned kGbaBorderSizes[]={1,1,1};
static int decoded,created,alive,failDecode,failTexture,failGeometry,draws,cards;
static PortGbaBorderRect protectedArea;
static unsigned char* stbi_load_from_memory(const unsigned char* data,int bytes,int* w,int* h,int* channels,int desired) {
    int x,y;unsigned char* p;
    (void)data;(void)bytes;(void)desired;decoded++;
    if(failDecode)return NULL;
    *w=BORDER_W;*h=BORDER_H;*channels=4;p=malloc(BORDER_W*BORDER_H*4);
    for(y=0;y<BORDER_H;y++)for(x=0;x<BORDER_W;x++){
        unsigned char* q=p+(y*BORDER_W+x)*4;
        q[0]=y%32;q[1]=32;q[2]=64;q[3]=255;
        if(x==52)q[0]=q[1]=q[2]=255; /* isolated star must lose majority */
        if(x>=72&&x<312&&y>=28&&y<188)q[3]=0;
    }
    return p;
}
static void stbi_image_free(void* p){free(p);}
static void BorderMapCards(unsigned char* p,int w,int h){(void)p;(void)w;(void)h;cards++;}
static SDL_Surface* SDL_CreateSurfaceFrom(int w,int h,int format,unsigned char* p,int pitch){
    SDL_Surface* s=malloc(sizeof(*s));(void)format;assert(pitch==w*4);*s=(SDL_Surface){w,h,p};return s;
}
static SDL_Texture* SDL_CreateTextureFromSurface(int* r,SDL_Surface* surface){
    SDL_Texture* t;int y;(void)r;created++;
    if(failTexture==created)return NULL;
    if(surface->w==1)for(y=0;y<BORDER_H;y++){
        assert(surface->pixels[y*4]==y%32);assert(surface->pixels[y*4+3]==255);
    }
    t=malloc(sizeof(*t));*t=(SDL_Texture){surface->w,surface->h};alive++;return t;
}
static void SDL_DestroySurface(SDL_Surface* s){free(s);}
static void SDL_DestroyTexture(SDL_Texture* t){assert(t&&alive>0);alive--;free(t);}
static int SDL_SetTextureScaleMode(SDL_Texture* t,int mode){(void)t;(void)mode;return 1;}
static int SDL_SetTextureBlendMode(SDL_Texture* t,int mode){(void)t;(void)mode;return 1;}
static int SDL_RenderTexture(int* r,SDL_Texture* t,const SDL_FRect* src,const SDL_FRect* dst){
    (void)r;(void)t;(void)src;assert(dst->w>0&&dst->h>0);draws++;return 1;
}
static int SDL_RenderGeometry(int* r,SDL_Texture* t,const SDL_Vertex* v,int nv,const int* indices,int ni){
    int i,j;(void)r;assert(t&&nv<=32&&ni<=48&&nv%4==0);draws++;
    for(i=0;i<ni;i++)assert(indices[i]>=0&&indices[i]<nv);
    for(i=0;i<nv;i+=4){
        float x0=v[i].position.x,y0=v[i].position.y,x1=v[i+2].position.x,y1=v[i+2].position.y;
        assert(x0>=-.01f&&y0>=-.01f&&x1>=x0&&y1>=y0);
        assert(x1<=renderer+.01f); /* renderer holds current outer width */
        assert(y1<=rendererHeight+.01f);
        assert(x1<=protectedArea.x+.01f||x0>=protectedArea.x+protectedArea.w-.01f||
               y1<=protectedArea.y+.01f||y0>=protectedArea.y+protectedArea.h-.01f);
        for(j=i;j<i+4;j++)assert(v[j].tex_coord.x>=-.000001f&&v[j].tex_coord.x<=1.000001f&&v[j].tex_coord.y>=-.000001f&&v[j].tex_coord.y<=1.000001f);
    }
    return !failGeometry;
}
#include "../src/port_gba_border.inc"
#include "border_output_fixture.inc"

static void Clear(void){
    if(sBorderTex)SDL_DestroyTexture(sBorderTex);
    if(sBorderMaterialTex)SDL_DestroyTexture(sBorderMaterialTex);
    sBorderTex=sBorderMaterialTex=NULL;sBorderShown=sBorderAttempted=0;
    failDecode=failTexture=failGeometry=decoded=created=draws=cards=sRendererResetPending=0;
}
static void Equal(SDL_FRect a,SDL_FRect b){assert(memcmp(&a,&b,sizeof(a))==0);}
int main(void){
    static const int sizes[][2]={{1920,1080},{640,480},{3440,1440},{5120,1440},{1080,1920},{1620,1080},{240,160},{17,31}};
    int v,m,aspect,size,zoom,offCases=0,onCases=0;
    SDL_FRect actual,old;
    /* Actual old/new OutputRect must remain bit-identical with Border0. */
    for(v=0;v<6;v++)for(m=0;m<3;m++)for(size=0;size<8;size++)for(zoom=0;zoom<4;zoom++){
        gPortConfig.gbaBorder=0;gPortConfig.viewMode=v;gPortConfig.scaleMode=m;gPortConfig.crtOutput=zoom;
        PortConfigView(v,&gPortScreen.width,&gPortScreen.height);gPortAutoScaleF=2.5f;
        sAutoScale=v==4?2:0;sFitScale=zoom==3?2:0;
        OutputRect(sizes[size][0],sizes[size][1],&actual);OldOutputRect(sizes[size][0],sizes[size][1],&old);Equal(actual,old);offCases++;
    }
    sFitScale=sAutoScale=0;gPortConfig.gbaBorder=1;assert(GbaBorderReady()&&decoded==1&&created==2&&alive==2&&cards==1);
    assert(GbaBorderReady()&&decoded==1&&created==2);
    for(v=0;v<6;v++)for(m=0;m<3;m++)for(size=0;size<8;size++)for(aspect=0;aspect<PORT_ASPECT_COUNT;aspect++){
        PortAspectBounds outer=PortAspectFit(sizes[size][0],sizes[size][1],aspect);
        PortGbaBorderArea a=PortGbaBorderViewport(outer.w,outer.h,1);
        gPortConfig.viewMode=v;gPortConfig.scaleMode=m;gPortConfig.crtOutput=0;
        PortConfigView(v,&gPortScreen.width,&gPortScreen.height);
        sAutoScale=v==4?2:0;gPortAutoScaleF=2.5f;
        if(v==4){gPortScreen.width=(int)ceilf(a.w/2.5f);gPortScreen.height=(int)ceilf(a.h/2.5f);
            if(gPortScreen.width<240)gPortScreen.width=240;if(gPortScreen.height<160)gPortScreen.height=160;}
        OutputRect(a.w,a.h,&actual);
        assert(actual.x>=0&&actual.y>=0&&actual.x+actual.w<=a.w+.01f&&actual.y+actual.h<=a.h+.01f);
        assert(fabsf(actual.w/gPortScreen.width-actual.h/gPortScreen.height)<.001f);
        protectedArea=(PortGbaBorderRect){a.x,a.y,a.w,a.h};renderer=outer.w;rendererHeight=outer.h;draws=0;
        DrawGbaBorder(outer.w,outer.h,&protectedArea);assert(draws<=2);onCases++;
    }
    /* Exact legacy Original picture at1920x1080 and one full texture draw. */
    {PortGbaBorderArea a=PortGbaBorderViewport(1920,1080,1);gPortConfig.viewMode=0;gPortConfig.scaleMode=0;
     gPortScreen.width=240;gPortScreen.height=160;sAutoScale=0;OutputRect(a.w,a.h,&actual);
     protectedArea=(PortGbaBorderRect){a.x+actual.x,a.y+actual.y,actual.w,actual.h};
     assert(protectedArea.x==360&&protectedArea.y==140&&protectedArea.w==1200&&protectedArea.h==800);
     assert(PortGbaBorderLegacy(1920,1080,&protectedArea));draws=0;DrawGbaBorder(1920,1080,&protectedArea);assert(draws==1);}
    Clear();assert(alive==0);
    /* Failed decode/each texture creation preserves the normal full viewport. */
    for(m=0;m<3;m++){
        gPortConfig.gbaBorder=1;failDecode=m==0;failTexture=m;
        assert(!GbaBorderReady());assert(alive==0);
        {int c=created,d=decoded;assert(!GbaBorderReady()&&created==c&&decoded==d);}
        {PortGbaBorderArea a=PortGbaBorderViewport(1920,1080,GbaBorderReady());assert(a.x==0&&a.y==0&&a.w==1920&&a.h==1080);}
        Clear();
    }
    gPortConfig.gbaBorder=1;assert(GbaBorderReady());gPortConfig.gbaBorder=2;failTexture=created+2;
    assert(!GbaBorderReady()&&alive==2&&sBorderShown==1);gPortConfig.gbaBorder=1;assert(GbaBorderReady());
    gPortConfig.gbaBorder=2;failTexture=0;assert(GbaBorderReady()&&alive==2&&sBorderShown==2);
    Clear();assert(alive==0);
    printf("PASS actual border cache/geometry/OutputRect: off%d bit-identical, on%d uniform/no crop, legacy and allocation fallback\n",offCases,onCases);
    return 0;
}
