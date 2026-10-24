#include <SDL3/SDL.h>
#include <math.h>
#include "port/port.h"
#include "port/port_android.h"
#include "port/port_ui.h"
#include "touch_icons.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
static int touchEnabled;
EMSCRIPTEN_KEEPALIVE void PortWebTouchEnable(int enabled) {
    touchEnabled=enabled;
    gPortConfig.hideTouchControls=!enabled;
    PortAndroidTouchClear();
}
EMSCRIPTEN_KEEPALIVE void PortWebTouchPersist(void) { PortConfigSave(); }
#endif

typedef struct Finger { SDL_FingerID id; float x,y; int active; } Finger;
typedef struct Button { float x,y; uint16_t bits; const char* text; int icon; } Button;
static Finger fingers[10];
static float aspect=16.0f/9.0f;
#if defined(__EMSCRIPTEN__) || defined(PORT_TOUCH_CENTERED_LAYOUT)
static float layoutX,layoutY,layoutW=1,layoutH=1,outputW=1,outputH=1;
static void CenterLayout(int w,int h) {
    float margin=SDL_max(8.0f,SDL_min(w,h)*.025f);
    outputW=w;outputH=h;
    layoutW=SDL_min(w-2*margin,(h-2*margin)*16.0f/9.0f);
    layoutW=SDL_max(1.0f,layoutW);layoutH=layoutW*9.0f/16.0f;
    layoutX=(w-layoutW)*.5f;layoutY=(h-layoutH)*.5f;
    aspect=16.0f/9.0f;
}
#endif
static const Button buttons[]={
    {.13f,.34f,0x200,"L",0},{.87f,.34f,0x100,"R",0},
    {.91f,.62f,1,"A",0},{.83f,.79f,2,"B",0},
    {.73f,.71f,PORT_KEY_DODGE,"",1},{.81f,.49f,PORT_KEY_STOCK,"",2},
    {.45f,.93f,4,"SELECT",0},{.55f,.93f,8,"START",0},
    {.95f,.17f,12,"...",0}
};
static SDL_Texture* icons[2];
static uint16_t uiHeld;
/* Android: with the touch controls hidden and a gamepad connected, nothing is drawn, not even the options button or
   the pad in the menus (issue #38); with no gamepad the options button stays, the only way into the menu. */
static int AllHidden(void) {
#if defined(PORT_SWITCH)
    /* Switch: the Joy-Cons are always there and Minus + Plus held opens the options (issue #36) */
    return gPortConfig.hideTouchControls;
#elif defined(__EMSCRIPTEN__)
    return 0;
#else
    int count=0;
    SDL_JoystickID* ids;
    if(!gPortConfig.hideTouchControls) return 0;
    ids=SDL_GetGamepads(&count);
    SDL_free(ids);
    return count>0;
#endif
}
void PortAndroidTouchClear(void) { SDL_memset(fingers,0,sizeof(fingers)); uiHeld=0; }
void PortAndroidTouchSettingsChanged(void) {
    PortAndroidTouchClear();
#ifdef __EMSCRIPTEN__
    touchEnabled=1;
    EM_ASM({ if (Module.onTouchPanelsChanged) Module.onTouchPanelsChanged(!!$0); },
           !gPortConfig.hideTouchControls);
#endif
}
static void TouchUi(void) {
    uint16_t keys=PortAndroidTouchKeys(), pressed=keys & ~uiHeld;
    static const struct { uint16_t bit; SDL_Keycode key; } bindings[]={
        {0x40,SDLK_UP},{0x80,SDLK_DOWN},{0x20,SDLK_LEFT},{0x10,SDLK_RIGHT},
        {1,SDLK_RETURN},{2,SDLK_ESCAPE},{4,SDLK_ESCAPE},{8,SDLK_ESCAPE}
    };
    int i;
    uiHeld=keys;
    if(!PortUiActive()) {
        if((keys & 12)==12 && (pressed & 12)) {
            PortAndroidTouchClear();
            PortUiOpenOptions();
        }
        return;
    }
    /* The settings overlay consumes SDL keyboard/gamepad events, whereas
       gameplay reads GBA bits. Forward touch presses to that same UI route. */
    for(i=0;i<(int)SDL_arraysize(bindings);i++) if(pressed & bindings[i].bit) {
        SDL_Event event={0};event.type=SDL_EVENT_KEY_DOWN;
        event.key.key=bindings[i].key;
        PortUiHandleEvent(&event);
    }
}
void PortAndroidTouchEvent(const void* event) {
#ifdef __EMSCRIPTEN__
    if(!touchEnabled)return;
#endif
    const SDL_Event* e=event;
    int i, freeSlot=-1;
    if(e->type==SDL_EVENT_RENDER_DEVICE_RESET) {
        for(i=0;i<2;i++) { SDL_DestroyTexture(icons[i]); icons[i]=NULL; }
        return;
    }
    if(e->type==SDL_EVENT_WINDOW_FOCUS_LOST || e->type==SDL_EVENT_WILL_ENTER_BACKGROUND) {
        PortAndroidTouchClear(); return;
    }
    if(e->type!=SDL_EVENT_FINGER_DOWN && e->type!=SDL_EVENT_FINGER_MOTION &&
       e->type!=SDL_EVENT_FINGER_UP && e->type!=SDL_EVENT_FINGER_CANCELED) return;
    for(i=0;i<10;i++) {
        if(!fingers[i].active) { if(freeSlot<0) freeSlot=i; continue; }
        if(fingers[i].id!=e->tfinger.fingerID) continue;
        fingers[i].x=e->tfinger.x; fingers[i].y=e->tfinger.y;
        if(e->type==SDL_EVENT_FINGER_UP || e->type==SDL_EVENT_FINGER_CANCELED) fingers[i].active=0;
        TouchUi();
        return;
    }
    if(e->type==SDL_EVENT_FINGER_DOWN && freeSlot>=0) {
        fingers[freeSlot]=(Finger){e->tfinger.fingerID,e->tfinger.x,e->tfinger.y,1};
        TouchUi();
    }
}
uint16_t PortAndroidTouchKeys(void) {
#ifdef __EMSCRIPTEN__
    if(!touchEnabled)return 0;
#endif
    uint16_t keys=0;
    int i,b;
    if(AllHidden()) return 0;
    int panels = !gPortConfig.hideTouchControls || PortUiActive();
    for(i=0;i<10;i++) if(fingers[i].active) {
        float fx=fingers[i].x,fy=fingers[i].y;
#if defined(__EMSCRIPTEN__) || defined(PORT_TOUCH_CENTERED_LAYOUT)
        fx=(fx*outputW-layoutX)/layoutW;fy=(fy*outputH-layoutY)/layoutH;
#endif
        float dx=(fx-.13f)*aspect,dy=fy-.73f;
        if(panels && SDL_fabsf(dx)<.17f && SDL_fabsf(dy)<.17f) {
            if(dx<-.035f) keys|=0x20;
            if(dx>.035f) keys|=0x10;
            if(dy<-.035f) keys|=0x40;
            if(dy>.035f) keys|=0x80;
            continue;
        }
        for(b=0;b<(int)SDL_arraysize(buttons);b++) {
            if(!panels && b!=(int)SDL_arraysize(buttons)-1) continue;
            float x=(fx-buttons[b].x)*aspect,y=fy-buttons[b].y;
            float radius=b==6 || b==7 ? .055f : .075f;
            if(x*x+y*y<=radius*radius) { keys|=buttons[b].bits; break; }
        }
    }
    return keys;
}
static void Circle(SDL_Renderer* r,float x,float y,float radius,SDL_FColor color) {
    SDL_Vertex v[18]; int indices[48],i;
    v[0]=(SDL_Vertex){{x,y},color,{0,0}};
    for(i=0;i<=16;i++) {
        float a=i*6.28318530718f/16;
        v[i+1]=(SDL_Vertex){{x+SDL_cosf(a)*radius,y+SDL_sinf(a)*radius},color,{0,0}};
        if(i<16) {indices[i*3]=0;indices[i*3+1]=i+1;indices[i*3+2]=i+2;}
    }
    SDL_RenderGeometry(r,NULL,v,18,indices,48);
}
static void DrawButton(SDL_Renderer* r,int w,int h,float x,float y,float radius,const char* text,int icon,int active) {
    float px=x*w,py=y*h,rr=radius*h;
#if defined(__EMSCRIPTEN__) || defined(PORT_TOUCH_CENTERED_LAYOUT)
    px=layoutX+x*layoutW;py=layoutY+y*layoutH;rr=radius*layoutH;
    h=(int)layoutH;
#endif
    SDL_FColor edge={1,1,1,active?.9f:.5f},inside={.04f,.06f,.11f,active?.75f:.25f};
    Circle(r,px,py,rr,edge); Circle(r,px,py,rr-2,inside);
    if(icon) {
        SDL_FRect rect={px-rr*.66f,py-rr*.66f,rr*1.32f,rr*1.32f};
        SDL_SetTextureAlphaMod(icons[icon-1],active?240:150);
        SDL_RenderTexture(r,icons[icon-1],NULL,&rect);
    } else {
        float scale=SDL_max(1.0f,(radius*h)/(SDL_strlen(text)>2?25.0f:10.0f));
        float oldX,oldY; SDL_GetRenderScale(r,&oldX,&oldY);
        SDL_SetRenderScale(r,scale,scale);SDL_SetRenderDrawColor(r,255,255,255,active?240:150);
        SDL_RenderDebugText(r,px/scale-SDL_strlen(text)*4,py/scale-4,text);
        SDL_SetRenderScale(r,oldX,oldY);
    }
}
void PortAndroidTouchDraw(SDL_Renderer* r) {
#ifdef __EMSCRIPTEN__
    if(!touchEnabled)return;
#endif
    int w,h,b; uint16_t keys=PortAndroidTouchKeys();
    if(AllHidden())return;
    SDL_GetCurrentRenderOutputSize(r,&w,&h); if(w<=0 || h<=0)return;
    aspect=(float)w/h;
#if defined(__EMSCRIPTEN__) || defined(PORT_TOUCH_CENTERED_LAYOUT)
    CenterLayout(w,h);
#endif
    for(b=0;b<2;b++) if(!icons[b]) {
        icons[b]=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,32,32);
        if(!icons[b])return;
        SDL_UpdateTexture(icons[b],NULL,b?touch_stock:touch_roll,32*4);
        SDL_SetTextureBlendMode(icons[b],SDL_BLENDMODE_BLEND);
    }
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    for(b=0;b<(int)SDL_arraysize(buttons);b++) {
        if(gPortConfig.hideTouchControls && !PortUiActive() && b!=(int)SDL_arraysize(buttons)-1) continue;
        DrawButton(r,w,h,buttons[b].x,buttons[b].y,
            b==6 || b==7?.05f:.066f,buttons[b].text,buttons[b].icon,(keys&buttons[b].bits)==buttons[b].bits);
    }
    if(gPortConfig.hideTouchControls && !PortUiActive()) return;
    DrawButton(r,w,h,.13f,.73f-.10f,.063f,"^",0,keys&0x40);
    DrawButton(r,w,h,.13f,.73f+.10f,.063f,"v",0,keys&0x80);
    DrawButton(r,w,h,.13f-.10f/aspect,.73f,.063f,"<",0,keys&0x20);
    DrawButton(r,w,h,.13f+.10f/aspect,.73f,.063f,">",0,keys&0x10);
}
