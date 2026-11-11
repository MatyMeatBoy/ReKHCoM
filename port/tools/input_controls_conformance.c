/* Production input path with a real SDL virtual gamepad; no cartridge needed. */
#include <assert.h>
#include <stdio.h>
#include "../src/port_input.c"
PortConfig gPortConfig = {
    .dodgeButton=1,.stockButton=1,.unstockButton=1,
    .keys={{SDLK_X,SDLK_K},{SDLK_Z,SDLK_J},{SDLK_A,0},{SDLK_S,0},
           {SDLK_RETURN,0},{SDLK_BACKSPACE,0},{SDLK_UP,SDLK_W},{SDLK_DOWN,0},
           {SDLK_LEFT,0},{SDLK_RIGHT,0},{SDLK_D,0},{SDLK_C,0},{SDLK_V,0}},
    .pad={SDL_GAMEPAD_BUTTON_SOUTH,SDL_GAMEPAD_BUTTON_EAST,
          SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
          SDL_GAMEPAD_BUTTON_START,SDL_GAMEPAD_BUTTON_BACK,
          SDL_GAMEPAD_BUTTON_DPAD_UP,SDL_GAMEPAD_BUTTON_DPAD_DOWN,
          SDL_GAMEPAD_BUTTON_DPAD_LEFT,SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
          SDL_GAMEPAD_BUTTON_NORTH,SDL_GAMEPAD_BUTTON_WEST,SDL_GAMEPAD_BUTTON_RIGHT_STICK}
};
uint32_t PortFrameCount(void){return 0;}
int PortRollbackResimulating(void){return 0;} /* netplay link is outside this input fixture */
int VsBattleLocalFacingLeft(void){return 0;}
/* the game's side port_input.c reaches (the battle's card scroll, the zoom hotkeys): outside a battle here */
Mode* gCurrentMode;
Mode gModeBattle, gModeVsbattle;
void PortUiZoomStep(int dir){(void)dir;}
void PortLog(const char* fmt,...){(void)fmt;}
static void key(SDL_Keycode code,int down){
    SDL_Event e={0};e.type=down?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP;
    e.key.key=code;PortInputHandleEvent(&e);
}
int main(void){
    assert(SDL_Init(SDL_INIT_GAMEPAD|SDL_INIT_EVENTS));
    const SDL_Keycode codes[]={SDLK_X,SDLK_Z,SDLK_A,SDLK_S,SDLK_RETURN,
        SDLK_BACKSPACE,SDLK_UP,SDLK_DOWN,SDLK_LEFT,SDLK_RIGHT,SDLK_D,SDLK_C,SDLK_V};
    const uint16_t bits[]={1,2,0x200,0x100,8,4,0x40,0x80,0x20,0x10,
        PORT_KEY_STOCK,PORT_KEY_DODGE,PORT_KEY_UNSTOCK};
    for(int i=0;i<13;i++){
        key(codes[i],1);assert(PortInputKeys()==bits[i]);
        key(codes[i],0);assert(!PortInputKeys());
    }
    gPortConfig.unstockButton=0;
    key(SDLK_V,1);assert(!PortInputKeys());key(SDLK_V,0);
    for(int i=0;i<60;i++) {PortInputResolve(PORT_KEY_UNSTOCK);assert(!PortTakeUnstock());}
    gPortConfig.unstockButton=1;PortInputResolve(0);
    key(SDLK_LEFT,1);key(SDLK_RIGHT,1);assert(!PortInputKeys());
    key(SDLK_RIGHT,0);assert(PortInputKeys()==0x20);key(SDLK_LEFT,0);
    assert((PortInputResolve(PORT_KEY_STOCK)&0x300)==0x300);
    PortInputResolve(PORT_KEY_DODGE);
    assert(PortTakeDodge(KEY_LEFT,0)==KEY_LEFT);
    PortInputResolve(0);PortInputResolve(PORT_KEY_DODGE);
    for(int i=0;i<29;i++){assert(!PortInputResolve(PORT_KEY_UNSTOCK));assert(!PortTakeUnstock());}
    PortInputResolve(PORT_KEY_UNSTOCK);assert(PortTakeUnstock());assert(!PortTakeUnstock());
    for(int i=0;i<60;i++){PortInputResolve(PORT_KEY_UNSTOCK);assert(!PortTakeUnstock());}
    PortInputResolve(0);
    for(int i=0;i<30;i++)PortInputResolve(PORT_KEY_UNSTOCK);
    key(SDLK_X,1);
    SDL_Event lost={0};lost.type=SDL_EVENT_WINDOW_FOCUS_LOST;PortInputHandleEvent(&lost);
    assert(!PortInputKeys() && !PortTakeDodge(0,0) && !PortTakeUnstock());
    for(int i=0;i<30;i++)PortInputResolve(PORT_KEY_UNSTOCK);
    PortInputSuppressUntilRelease();assert(!PortTakeUnstock());assert(!PortInputKeys());
    /* Rebinding while a key is down cannot leave the old action stuck: its
       later release no longer matches the new mapping. */
    key(SDLK_X,1);assert(PortInputKeys()==KEY_A);
    gPortConfig.keys[PORT_ACT_A][0]=SDLK_Q;
    key(SDLK_X,0);assert(PortInputKeys()==KEY_A);
    PortInputResetBindings();assert(!PortInputKeys());
    key(SDLK_Q,1);assert(PortInputKeys()==KEY_A);
    key(SDLK_Q,0);assert(!PortInputKeys());
    gPortConfig.keys[PORT_ACT_A][0]=SDLK_X;
    /* A key released while the overlay owns key-up must not keep suppression
       latched forever after closing. A fresh press must work immediately. */
    key(SDLK_X,1);assert(PortInputKeys()==KEY_A);
    PortInputSuppressUntilRelease();assert(!PortInputKeys());
    key(SDLK_Z,1);assert(PortInputKeys()==KEY_B);
    key(SDLK_Z,0);assert(!PortInputKeys());
    SDL_VirtualJoystickDesc desc;SDL_INIT_INTERFACE(&desc);
    desc.type=SDL_JOYSTICK_TYPE_GAMEPAD;desc.naxes=SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons=SDL_GAMEPAD_BUTTON_COUNT;desc.name="ReKHCoM controls test";
    SDL_JoystickID id=SDL_AttachVirtualJoystick(&desc);assert(id);
    SDL_Joystick* joystick=SDL_OpenJoystick(id);assert(joystick);
    SDL_Gamepad* pad=SDL_OpenGamepad(id);assert(pad);
    assert(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_SOUTH,true));
    SDL_UpdateJoysticks();key(SDLK_LEFT,1);assert(PortInputKeys()==(1|0x20));
    assert(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_SOUTH,false));
    SDL_UpdateJoysticks();key(SDLK_LEFT,0);assert(!PortInputKeys());
    assert(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_RIGHT_STICK,true));
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTY,32767));
    SDL_UpdateJoysticks();assert(PortInputKeys()==PORT_KEY_UNSTOCK);
    gPortConfig.unstockButton=0;assert(!PortInputKeys());
    assert(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_RIGHT_STICK,false));
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTY,0));
    gPortConfig.pad[PORT_ACT_L]=PORT_PAD_AXIS_BASE+SDL_GAMEPAD_AXIS_LEFT_TRIGGER*2;
    gPortConfig.pad[PORT_ACT_R]=PORT_PAD_AXIS_BASE+SDL_GAMEPAD_AXIS_RIGHT_TRIGGER*2;
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,-32768));
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,-32768));
    SDL_UpdateJoysticks();assert(!PortInputKeys());
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,-2000));
    SDL_UpdateJoysticks();assert(!PortInputKeys()); /* below gamepad threshold */
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,32767));
    SDL_UpdateJoysticks();assert(PortInputKeys()==KEY_L);
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,32767));
    SDL_UpdateJoysticks();assert(PortInputKeys()==(KEY_L|KEY_R));
    PortInputSuppressUntilRelease();assert(!PortInputKeys());
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,-32768));
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,-32768));
    SDL_UpdateJoysticks();assert(!PortInputKeys());
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,32767));
    SDL_UpdateJoysticks();assert(PortInputKeys()==KEY_R);
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,-32768));
    SDL_UpdateJoysticks();assert(!PortInputKeys());
    /* A disconnected controller cannot leave a remapped action held. */
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,32767));
    SDL_UpdateJoysticks();assert(PortInputKeys()==KEY_L);
    SDL_CloseGamepad(pad);SDL_CloseJoystick(joystick);SDL_DetachVirtualJoystick(id);
    SDL_UpdateJoysticks();assert(!PortInputKeys());
    /* A generic USB controller with no SDL gamepad mapping remains usable
       after assigning its physical button and axis in Controls. */
    SDL_INIT_INTERFACE(&desc);
    desc.type=SDL_JOYSTICK_TYPE_UNKNOWN;desc.naxes=2;desc.nbuttons=4;desc.nhats=1;desc.name="Raw N64-style joystick";
    id=SDL_AttachVirtualJoystick(&desc);assert(id && !SDL_IsGamepad(id));
    joystick=SDL_OpenJoystick(id);assert(joystick);
    gPortConfig.pad[PORT_ACT_A]=PORT_PAD_RAW_BUTTON_BASE+2;
    assert(SDL_SetJoystickVirtualButton(joystick,2,true));SDL_UpdateJoysticks();assert(PortInputKeys()==KEY_A);
    assert(SDL_SetJoystickVirtualButton(joystick,2,false));SDL_UpdateJoysticks();assert(!PortInputKeys());
    gPortConfig.pad[PORT_ACT_R]=PORT_PAD_RAW_AXIS_BASE+1*2;
    assert(SDL_SetJoystickVirtualAxis(joystick,1,32767));SDL_UpdateJoysticks();assert(PortInputKeys()==KEY_R);
    assert(SDL_SetJoystickVirtualAxis(joystick,1,0));SDL_UpdateJoysticks();assert(!PortInputKeys());
    /* Hat directions are independent bits, including diagonals. */
    gPortConfig.pad[PORT_ACT_UP]=3000;
    gPortConfig.pad[PORT_ACT_RIGHT]=3001;
    assert(SDL_SetJoystickVirtualHat(joystick,0,SDL_HAT_RIGHTUP));
    SDL_UpdateJoysticks();assert(PortInputKeys()==(KEY_UP|KEY_RIGHT));
    assert(SDL_SetJoystickVirtualHat(joystick,0,SDL_HAT_CENTERED));
    SDL_UpdateJoysticks();assert(!PortInputKeys());
    SDL_Event raw={0}, menu={0};
    raw.type=SDL_EVENT_JOYSTICK_HAT_MOTION;raw.jhat.which=id;raw.jhat.value=SDL_HAT_UP;
    assert(PortInputRawBinding(&raw)==PORT_PAD_RAW_HAT_BASE);
    raw.jhat.value=SDL_HAT_CENTERED;assert(PortInputRawBinding(&raw)==-1);
    raw.type=SDL_EVENT_JOYSTICK_AXIS_MOTION;raw.jaxis.which=id;raw.jaxis.axis=1;raw.jaxis.value=-17000;
    assert(PortInputRawBinding(&raw)==PORT_PAD_RAW_AXIS_BASE+3);
    raw.jaxis.value=8000;assert(PortInputRawBinding(&raw)==-1);
    gPortConfig.pad[PORT_ACT_START]=PORT_PAD_RAW_BUTTON_BASE;
    gPortConfig.pad[PORT_ACT_SELECT]=PORT_PAD_RAW_BUTTON_BASE+1;
    raw.type=SDL_EVENT_JOYSTICK_BUTTON_DOWN;raw.jbutton.which=id;raw.jbutton.button=1;
    assert(PortInputRawBinding(&raw)==PORT_PAD_RAW_BUTTON_BASE+1);
    assert(SDL_SetJoystickVirtualButton(joystick,1,true));SDL_UpdateJoysticks();
    assert(!PortInputRawMenuChord(&raw)); /* Select alone stays a game action. */
    assert(SDL_SetJoystickVirtualButton(joystick,0,true));SDL_UpdateJoysticks();
    raw.jbutton.button=0;assert(PortInputRawMenuChord(&raw));
    assert(SDL_SetJoystickVirtualButton(joystick,0,false));SDL_UpdateJoysticks();
    assert(!PortInputRawMenuChord(&raw));
    assert(SDL_SetJoystickVirtualButton(joystick,0,true));SDL_UpdateJoysticks();
    raw.jbutton.button=1;assert(PortInputRawMenuChord(&raw)); /* Either order. */
    PortInputResetRawMenu();assert(PortInputRawMenuEvent(&raw,&menu)==1); /* Held opener can't close. */
    assert(SDL_SetJoystickVirtualButton(joystick,0,false));
    assert(SDL_SetJoystickVirtualButton(joystick,1,false));SDL_UpdateJoysticks();
    raw.type=SDL_EVENT_JOYSTICK_BUTTON_UP;raw.jbutton.button=0;assert(PortInputRawMenuEvent(&raw,&menu)==1);
    raw.jbutton.button=1;assert(PortInputRawMenuEvent(&raw,&menu)==1);
    assert(SDL_SetJoystickVirtualButton(joystick,2,true));SDL_UpdateJoysticks();
    raw.type=SDL_EVENT_JOYSTICK_BUTTON_DOWN;raw.jbutton.button=2;
    assert(PortInputRawMenuEvent(&raw,&menu)==2 && menu.key.key==SDLK_RETURN);
    assert(PortInputRawMenuEvent(&raw,&menu)==1); /* One press, no repeats. */
    assert(SDL_SetJoystickVirtualButton(joystick,2,false));SDL_UpdateJoysticks();
    raw.type=SDL_EVENT_JOYSTICK_BUTTON_UP;assert(PortInputRawMenuEvent(&raw,&menu)==1);
    assert(SDL_SetJoystickVirtualHat(joystick,0,SDL_HAT_UP));SDL_UpdateJoysticks();
    raw.type=SDL_EVENT_JOYSTICK_HAT_MOTION;raw.jhat.which=id;raw.jhat.hat=0;raw.jhat.value=SDL_HAT_UP;
    assert(PortInputRawMenuEvent(&raw,&menu)==2 && menu.key.key==SDLK_UP);
    assert(SDL_SetJoystickVirtualHat(joystick,0,SDL_HAT_CENTERED));SDL_UpdateJoysticks();
    raw.type=SDL_EVENT_JOYSTICK_HAT_MOTION;raw.jhat.which=id;raw.jhat.value=SDL_HAT_CENTERED;
    assert(PortInputRawMenuEvent(&raw,&menu)==1);
    /* SDL can queue both edges before the menu consumes them. Preserve the
       event, even though polling already sees the released final state. */
    assert(SDL_SetJoystickVirtualButton(joystick,2,true));SDL_UpdateJoysticks();
    assert(SDL_SetJoystickVirtualButton(joystick,2,false));SDL_UpdateJoysticks();
    raw.type=SDL_EVENT_JOYSTICK_BUTTON_DOWN;raw.jbutton.which=id;raw.jbutton.button=2;
    assert(PortInputRawMenuEvent(&raw,&menu)==2 && menu.key.key==SDLK_RETURN);
    raw.type=SDL_EVENT_JOYSTICK_BUTTON_UP;assert(PortInputRawMenuEvent(&raw,&menu)==1);
    /* Two controls pumped together must still produce two ordered actions. */
    gPortConfig.pad[PORT_ACT_B]=PORT_PAD_RAW_BUTTON_BASE+3;
    assert(SDL_SetJoystickVirtualButton(joystick,2,true));
    assert(SDL_SetJoystickVirtualButton(joystick,3,true));SDL_UpdateJoysticks();
    raw.type=SDL_EVENT_JOYSTICK_BUTTON_DOWN;raw.jbutton.button=2;
    assert(PortInputRawMenuEvent(&raw,&menu)==2 && menu.key.key==SDLK_RETURN);
    raw.jbutton.button=3;assert(PortInputRawMenuEvent(&raw,&menu)==2 && menu.key.key==SDLK_ESCAPE);
    assert(SDL_SetJoystickVirtualButton(joystick,2,false));
    assert(SDL_SetJoystickVirtualButton(joystick,3,false));SDL_UpdateJoysticks();
    /* The input layer must retain an open handle so SDL delivers events. */
    SDL_CloseJoystick(joystick);
    PortInputInit();
    joystick=SDL_GetJoystickFromID(id);assert(joystick);
    PortInputInit(); /* Repeated initialization must not add open references. */
    assert(SDL_DetachVirtualJoystick(id));
    raw.type=SDL_EVENT_JOYSTICK_REMOVED;raw.jdevice.which=id;PortInputHandleEvent(&raw);
    assert(!SDL_GetJoystickFromID(id));SDL_UpdateJoysticks();assert(!PortInputKeys());

    /* A new raw device is opened through the hotplug event path too. */
    id=SDL_AttachVirtualJoystick(&desc);assert(id && !SDL_IsGamepad(id));
    raw.type=SDL_EVENT_JOYSTICK_ADDED;raw.jdevice.which=id;PortInputHandleEvent(&raw);
    joystick=SDL_GetJoystickFromID(id);assert(joystick);
    /* Import an SDL2-style mapping, then use normalized controls. */
    char guid[33], mapping[512];
    SDL_GUIDToString(SDL_GetJoystickGUID(joystick),guid,sizeof(guid));
    SDL_snprintf(mapping,sizeof(mapping),"%s,Generic USB test,a:b2,b:b3,start:b0,back:b1,leftx:a0,lefty:a1,dpup:h0.1,dpright:h0.2,dpdown:h0.4,dpleft:h0.8,platform:Windows,\n",guid);
    assert(SDL_SaveFile("gamecontrollerdb.txt",mapping,SDL_strlen(mapping)));
    PortInputInit();assert(SDL_IsGamepad(id));
    assert(SDL_RemovePath("gamecontrollerdb.txt"));
    raw.type=SDL_EVENT_GAMEPAD_ADDED;raw.gdevice.which=id;PortInputHandleEvent(&raw);
    pad=SDL_GetGamepadFromID(id);assert(pad);
    joystick=SDL_GetGamepadJoystick(pad);
    raw.type=SDL_EVENT_JOYSTICK_BUTTON_DOWN;raw.jbutton.which=id;raw.jbutton.button=2;
    assert(PortInputRawBinding(&raw)==-1); /* Don't capture duplicate raw gamepad events. */
    assert(PortInputRawMenuEvent(&raw,&menu)==0);
    gPortConfig.pad[PORT_ACT_A]=SDL_GAMEPAD_BUTTON_SOUTH;
    assert(SDL_SetJoystickVirtualButton(joystick,2,true));SDL_UpdateJoysticks();
    assert(PortInputKeys()==KEY_A);
    assert(SDL_SetJoystickVirtualButton(joystick,2,false));SDL_UpdateJoysticks();
    /* An explicitly selected environment file overrides a local mapping. */
    SDL_snprintf(mapping,sizeof(mapping),"%s,Override USB test,a:b3,start:b0,back:b1,platform:Windows,\n",guid);
    assert(SDL_SaveFile("mapping-override.txt",mapping,SDL_strlen(mapping)));
    assert(SDL_SetHint(SDL_HINT_GAMECONTROLLERCONFIG_FILE,"mapping-override.txt"));
    SDL_snprintf(mapping,sizeof(mapping),"%s,Local USB test,a:b2,start:b0,back:b1,platform:Windows,\n",guid);
    assert(SDL_SaveFile("gamecontrollerdb.txt",mapping,SDL_strlen(mapping)));
    KhLoadGamepadMappings();
    char* selected=SDL_GetGamepadMapping(pad);assert(selected && SDL_strstr(selected,"a:b3"));SDL_free(selected);
    assert(SDL_SetJoystickVirtualButton(joystick,3,true));SDL_UpdateJoysticks();assert(PortInputKeys()==KEY_A);
    SDL_ResetHint(SDL_HINT_GAMECONTROLLERCONFIG_FILE);
    assert(SDL_RemovePath("mapping-override.txt"));assert(SDL_RemovePath("gamecontrollerdb.txt"));
    assert(SDL_DetachVirtualJoystick(id));
    raw.type=SDL_EVENT_GAMEPAD_REMOVED;raw.gdevice.which=id;PortInputHandleEvent(&raw);
    SDL_UpdateJoysticks();assert(!PortInputKeys());
    SDL_Quit();puts("Input: keys, normalized pads/triggers, raw buttons/axes/hats, diagonals, capture, menu/chord, hotplug/disconnect and SDL2-format mapping import PASS");
    return 0;
}
