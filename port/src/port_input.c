/*
 * Keyboard and gamepad input, mapped to the GBA key bits.
 */
#include <SDL3/SDL.h>

#include "port/port.h"
#include "port/pad_layout.h"
#include "port/controller_prompts.h"
#include "port/controller_prompt_glyphs.h"
#include "port/port_ui.h"
#include "mode.h"
#include "text_types.h"
#include "msg.h"
#include "card_message_data.h"

extern Mode* gCurrentMode;
extern Mode gModeBattle;
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) || defined(PORT_ANDROID64) || defined(PORT_TOUCH)
#include "port/port_android.h"
#endif

#define KEY_A 0x001
#define KEY_B 0x002
#define KEY_SELECT 0x004
#define KEY_START 0x008
#define KEY_RIGHT 0x010
#define KEY_LEFT 0x020
#define KEY_UP 0x040
#define KEY_DOWN 0x080
#define KEY_R 0x100
#define KEY_L 0x200
#define KEY_STOCK PORT_KEY_STOCK
#define KEY_DODGE PORT_KEY_DODGE
#define KEY_UNSTOCK PORT_KEY_UNSTOCK

/* Frames a dodge press waits for the battle code to take it. */
#define DODGE_PENDING_FRAMES 6
/* Frames the return-stocked-cards button is held before it acts (0.5 s). */
#define UNSTOCK_HOLD_FRAMES 30

#define STICK_DEADZONE 12000
#define TRIGGER_THRESHOLD 16000

/* GBA key bit of each remappable action (port.h PORT_ACT_*). */
static const uint16_t sActionBits[PORT_ACT_COUNT] = {
    KEY_A,   KEY_B,    KEY_L,    KEY_R,     KEY_START, KEY_SELECT,
    KEY_UP,  KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_STOCK, KEY_DODGE, KEY_UNSTOCK,
};

static uint16_t sKeyboard;
static int sSuppress;
static int sBattleMenuKeys, sBattleMenuChord;
/* Select + Up/Down changes the zoom (Auto view): 0 Select up, 1 Select alone held back, 2 Select passed to the
   game, 3 the zoom chord; sZoomEat keeps Up/Down from the game until they are released after a chord. */
static int sZoomSelect, sZoomEat;
static uint16_t sZoomPrev;
static int sDodgeHeld;
static int sDodgePending;
static int sUnstockHeld;
static int sUnstockPending;
/* The deck editor's buttons (the dodge and stock bindings): a press is kept a few frames, as the dodge's. */
static int sDeckPlaceHeld, sDeckPlacePending;
static int sDeckCompactHeld, sDeckCompactPending;
/* Activity is event-driven: axis noise must not switch away from keyboard.
   Keep only an ID, never a gamepad pointer across hotplug. */
static SDL_JoystickID sPromptPad;
static int sPromptKeyboard = 1;

void PortPromptHandleEvent(const void* p) {
    const SDL_Event* ev=p;
    if (ev->type == SDL_EVENT_KEY_DOWN && !ev->key.repeat) sPromptKeyboard=1;
    else if (ev->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        sPromptKeyboard=0; sPromptPad=ev->gbutton.which;
    } else if (ev->type == SDL_EVENT_GAMEPAD_AXIS_MOTION && SDL_abs(ev->gaxis.value)>16000) {
        sPromptKeyboard=0; sPromptPad=ev->gaxis.which;
    }
}

static SDL_Gamepad* PromptPad(void) {
    return sPromptPad ? SDL_GetGamepadFromID(sPromptPad) : NULL;
}

static int PromptFamily(SDL_Gamepad* pad) {
    int selected=gPortConfig.controllerPrompts;
    if (selected != KH_PROMPT_AUTO) return selected;
    switch (pad ? SDL_GetGamepadType(pad) : SDL_GAMEPAD_TYPE_UNKNOWN) {
        case SDL_GAMEPAD_TYPE_PS3: case SDL_GAMEPAD_TYPE_PS4: case SDL_GAMEPAD_TYPE_PS5: return KH_PROMPT_PLAYSTATION;
        case SDL_GAMEPAD_TYPE_XBOX360: case SDL_GAMEPAD_TYPE_XBOXONE: return KH_PROMPT_XBOX;
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO: case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT: case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
            return KH_PROMPT_NINTENDO;
        default: return KH_PROMPT_GENERIC;
    }
}

int PortControllerBindingLabel(int binding, char* out, size_t size) {
    SDL_Gamepad* pad=PromptPad();
    const char* label;
    if (!gPortConfig.controllerPrompts) return 0;
    label=KhPromptButtonLabel(binding,PromptFamily(pad),KhPadIsNintendo(pad,gPortConfig.padLayout));
    if (label[0]=='?' && binding>=0 && binding<SDL_GAMEPAD_BUTTON_COUNT) {
        const char* generic=SDL_GetGamepadStringForButton((SDL_GamepadButton)binding);
        if(generic)label=generic;
    }
    SDL_strlcpy(out,label,size);
    return 1;
}

int PortControllerActionLabel(int action, char* out, size_t size) {
    SDL_Gamepad* pad=PromptPad();
    if (!gPortConfig.controllerPrompts || action<0 || action>=PORT_ACT_COUNT) return 0;
    if (sPromptKeyboard || !pad) {
        int key=gPortConfig.keys[action][0];
        if (!key) key=gPortConfig.keys[action][1];
        const char* label=key ? SDL_GetKeyName((SDL_Keycode)key) : "-";
        /* Tutorial buffers are game encoding, not UTF-8. Never insert a
           multi-byte host key name as independent Latin/Shift-JIS bytes. */
        for(const unsigned char* c=(const unsigned char*)label;*c;c++)
            if(*c>=128) { SDL_snprintf(out,size,"Key %u",(unsigned)key);return 1; }
        SDL_strlcpy(out,label,size);
        return 1;
    }
    return PortControllerBindingLabel(gPortConfig.pad[action],out,size);
}

int PortControllerActionGlyph(int action) {
    char label[32];
    if(action<0 || action>=PORT_ACT_COUNT)return 0;
    if(sPromptKeyboard || !PromptPad()) {
        if(!gPortConfig.keys[action][0] && !gPortConfig.keys[action][1])return 0;
    } else if(gPortConfig.pad[action]<0)return 0;
    if(!PortControllerActionLabel(action,label,sizeof(label)))return 0;
    return KhPromptGlyphId(label);
}

const unsigned char* PortControllerGlyphTiles(int glyph) {
    static unsigned char tiles[2][144][128],ready[2][144];
    int compact=glyph>=256; if(compact)glyph-=256;
    if(glyph<=0 || glyph>=144 || !(glyph<=8 || glyph>=128 || strchr(PORT_PROMPT_LETTERS,glyph)))return NULL;
    if(!ready[compact][glyph]) {
        unsigned char full[128];KhPromptGlyphPixels(glyph,full);
        if(!compact)memcpy(tiles[0][glyph],full,128);
        else for(int y=0;y<10;y++)for(int x=0;x<5;x++) {
            int color=0;
            for(int px=x*12/5;px<(x+1)*12/5;px++) {
                int at=((y/8)*2+px/8)*32+(y%8)*4+(px%8)/2;
                int sample=(full[at]>>((px&1)*4))&15;
                if(sample>color)color=sample;
            }
            KhPromptGlyphDot(tiles[1][glyph],x,y,color);
        }
        ready[compact][glyph]=1;
    }
    return tiles[compact][glyph];
}

int PortControllerPresetFamily(void) {
    int family=PromptFamily(PromptPad());
    return family>=KH_PROMPT_NINTENDO && family<=KH_PROMPT_PLAYSTATION?family:0;
}

int PortApplyControllerConfirmPreset(int family) {
    int confirm,cancel;
    if(!KhPromptPresetBindings(family,KhPadIsNintendo(PromptPad(),gPortConfig.padLayout),&confirm,&cancel))return 0;
    gPortConfig.pad[PORT_ACT_A]=confirm;gPortConfig.pad[PORT_ACT_B]=cancel;
    PortInputResetBindings();
    return 1;
}

uint64_t PortControllerPromptRevision(void) {
    static int previous[6+PORT_ACT_COUNT*3];
    static uint64_t revision;
    SDL_Gamepad* pad=PromptPad();
    int current[6+PORT_ACT_COUNT*3];
    current[0]=gPortConfig.controllerPrompts;current[1]=gPortConfig.padLayout;
    current[2]=gPortConfig.invertCardScroll;current[3]=sPromptKeyboard;
    current[4]=(int)sPromptPad;current[5]=pad?(int)SDL_GetGamepadType(pad):-1;
    memcpy(current+6,gPortConfig.pad,sizeof(gPortConfig.pad));
    memcpy(current+6+PORT_ACT_COUNT,gPortConfig.keys,sizeof(gPortConfig.keys));
    if(memcmp(previous,current,sizeof(current))) {
        memcpy(previous,current,sizeof(current));
        if(++revision==0)revision=1;
    }
    return revision;
}

int PortControllerSemanticGlyph(int token) {
    int action=(token&63)-1;
    if(action<0 || action>=PORT_ACT_COUNT)return 0;
    if((token&64) && gPortConfig.invertCardScroll && (action==PORT_ACT_L || action==PORT_ACT_R))
        action=action==PORT_ACT_L?PORT_ACT_R:PORT_ACT_L;
    if(!gPortConfig.controllerPrompts) {
        static const int gba[]={ 'A','B','L','R',140,141,5,6,7,8 };
        return action<10?gba[action]:'?';
    }
    int glyph=PortControllerActionGlyph(action);
    return glyph?glyph:'?'; /* Explicit unknown, never retain a stale prior button. */
}

/* After closing an overlay, ignore buttons still held until released so the
   confirming press does not reach the game. */
void PortInputSuppressUntilRelease(void) {
    sKeyboard = 0; /* the overlay may have consumed this key's release */
    sSuppress = 1;
    sBattleMenuKeys = sBattleMenuChord = 0;
    sZoomSelect = sZoomEat = 0;
    sDodgeHeld = sDodgePending = 0;
    sUnstockHeld = sUnstockPending = 0;
    sDeckPlaceHeld = sDeckPlacePending = sDeckCompactHeld = sDeckCompactPending = 0;
}

void PortInputResetBindings(void) {
    sKeyboard = 0;
    PortInputSuppressUntilRelease();
}

/* Test harness: KHCOM_VPAD="from-to:BUTTON,..." plugs in a virtual gamepad
   and holds its buttons on those frames (SOUTH, EAST, WEST, NORTH, START,
   BACK, LB, RB, LT, RT, UP, DOWN, LEFT, RIGHT, RS_LEFT, RS_RIGHT, RS_UP, RS_DOWN). */
static SDL_Joystick* sVirtualPad;

static void VirtualPadUpdate(void) {
    static const struct { const char* name; int button; int axis; int sign; } kNames[] = {
        { "SOUTH", SDL_GAMEPAD_BUTTON_SOUTH, -1, 0 }, { "EAST", SDL_GAMEPAD_BUTTON_EAST, -1, 0 },
        { "WEST", SDL_GAMEPAD_BUTTON_WEST, -1, 0 }, { "NORTH", SDL_GAMEPAD_BUTTON_NORTH, -1, 0 },
        { "START", SDL_GAMEPAD_BUTTON_START, -1, 0 }, { "BACK", SDL_GAMEPAD_BUTTON_BACK, -1, 0 },
        { "LB", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, -1, 0 }, { "RB", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, -1, 0 },
        { "LT", -1, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 1 }, { "RT", -1, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 1 },
        { "UP", SDL_GAMEPAD_BUTTON_DPAD_UP, -1, 0 }, { "DOWN", SDL_GAMEPAD_BUTTON_DPAD_DOWN, -1, 0 },
        { "LEFT", SDL_GAMEPAD_BUTTON_DPAD_LEFT, -1, 0 }, { "RIGHT", SDL_GAMEPAD_BUTTON_DPAD_RIGHT, -1, 0 },
        { "RS_LEFT", -1, SDL_GAMEPAD_AXIS_RIGHTX, -1 }, { "RS_RIGHT", -1, SDL_GAMEPAD_AXIS_RIGHTX, 1 },
        { "RS_UP", -1, SDL_GAMEPAD_AXIS_RIGHTY, -1 }, { "RS_DOWN", -1, SDL_GAMEPAD_AXIS_RIGHTY, 1 },
    };
    const char* spec = SDL_getenv("KHCOM_VPAD");
    int held[SDL_GAMEPAD_BUTTON_COUNT] = { 0 }, axes[SDL_GAMEPAD_AXIS_COUNT] = { 0 }, i;
    unsigned frame = PortFrameCount();
    if (sVirtualPad == NULL || spec == NULL)
        return;
    /* Joystick trigger axes use the full signed range; SDL maps their rest
       position to gamepad zero. Raw zero would mean a half-held trigger. */
    axes[SDL_GAMEPAD_AXIS_LEFT_TRIGGER] = -32768;
    axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER] = -32768;
    while (*spec) {
        unsigned a = 0, b = 0;
        char name[16] = { 0 };
        int n = 0;
        if (SDL_sscanf(spec, "%u-%u:%15[A-Z_]%n", &a, &b, name, &n) < 3 || n == 0)
            break;
        for (i = 0; i < (int)SDL_arraysize(kNames); i++)
            if (SDL_strcmp(name, kNames[i].name) == 0 && frame >= a && frame <= b) {
                if (kNames[i].button >= 0) held[kNames[i].button] = 1;
                else axes[kNames[i].axis] = kNames[i].sign * 32767;
            }
        spec += n;
        if (*spec == ',') spec++;
    }
    for (i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; i++)
        SDL_SetJoystickVirtualButton(sVirtualPad, i, held[i]);
    for (i = 0; i < SDL_GAMEPAD_AXIS_COUNT; i++)
        SDL_SetJoystickVirtualAxis(sVirtualPad, i, (Sint16)axes[i]);
    SDL_UpdateJoysticks();
}

void PortInputInit(void) {
    SDL_SetGamepadEventsEnabled(true);
    if (SDL_getenv("KHCOM_VPAD")) {
        SDL_VirtualJoystickDesc desc;
        SDL_JoystickID id;
        SDL_INIT_INTERFACE(&desc);
        desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
        desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
        desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
        desc.name = "KHCOM virtual pad";
        id = SDL_AttachVirtualJoystick(&desc);
        sVirtualPad = id ? SDL_OpenJoystick(id) : NULL;
        PortLog("vpad: %s", sVirtualPad ? "attached" : SDL_GetError());
    }
}

void PortInputHandleEvent(const void* p) {
    const SDL_Event* ev = (const SDL_Event*)p;
    size_t i;
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) || defined(PORT_ANDROID64) || defined(PORT_TOUCH)
    PortAndroidTouchEvent(p);
#endif

    switch (ev->type) {
        case SDL_EVENT_GAMEPAD_ADDED:
            SDL_OpenGamepad(ev->gdevice.which);
            break;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            if (ev->key.repeat)
                break;
            for (i = 0; i < PORT_ACT_COUNT * 2; i++) {
                if (gPortConfig.keys[i / 2][i % 2] == 0 || (SDL_Keycode)gPortConfig.keys[i / 2][i % 2] != ev->key.key)
                    continue;
                if (ev->type == SDL_EVENT_KEY_DOWN)
                    sKeyboard |= sActionBits[i / 2];
                else
                    sKeyboard &= ~sActionBits[i / 2];
            }
            break;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            sKeyboard = 0;
            sBattleMenuKeys = sBattleMenuChord = sZoomSelect = sZoomEat = 0;
            sDodgeHeld = sDodgePending = 0;
            sUnstockHeld = sUnstockPending = 0;
            sDeckPlaceHeld = sDeckPlacePending = sDeckCompactHeld = sDeckCompactPending = 0;
            break;
    }
}

int PortInputKeyBound(int key) {
    size_t i;
    for (i = 0; i < PORT_ACT_COUNT * 2; i++)
        if (gPortConfig.keys[i / 2][i % 2] != 0 && gPortConfig.keys[i / 2][i % 2] == key)
            return 1;
    return 0;
}

static uint16_t GamepadKeys(SDL_Gamepad* pad) {
    uint16_t keys = 0;
    int lx, ly, i;

    for (i = 0; i < PORT_ACT_COUNT; i++) {
        uint16_t bit = sActionBits[i];
        int binding = gPortConfig.pad[i];
        if (binding < 0) continue;
        if (binding >= PORT_PAD_AXIS_BASE) {
            int encoded = binding - PORT_PAD_AXIS_BASE;
            int axis = encoded / 2, negative = encoded & 1;
            int value;
            if (axis >= SDL_GAMEPAD_AXIS_COUNT) continue;
            value = SDL_GetGamepadAxis(pad, (SDL_GamepadAxis)axis);
            if ((!negative && value <= TRIGGER_THRESHOLD) || (negative && value >= -TRIGGER_THRESHOLD)) continue;
        } else if (!KhPadButtonDown(pad, binding, gPortConfig.padLayout)) continue;
        /* with the one-button options off, their buttons are B and A again */
        if (i == PORT_ACT_DODGE && !gPortConfig.dodgeButton)
            bit = KEY_B;
        if (i == PORT_ACT_STOCK && !gPortConfig.stockButton)
            bit = KEY_A;
        keys |= bit;
    }
    lx = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX);
    ly = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY);
    if (lx < -STICK_DEADZONE)
        keys |= KEY_LEFT;
    if (lx > STICK_DEADZONE)
        keys |= KEY_RIGHT;
    if (ly < -STICK_DEADZONE)
        keys |= KEY_UP;
    if (ly > STICK_DEADZONE)
        keys |= KEY_DOWN;

    /* Right stick for the cards: left / right = L / R, up = L + R (stock). */
    lx = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHTX);
    ly = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHTY);
    if (lx < -STICK_DEADZONE * 2)
        keys |= KEY_L;
    if (lx > STICK_DEADZONE * 2)
        keys |= KEY_R;
    if (ly < -STICK_DEADZONE * 2)
        keys |= KEY_L | KEY_R;
    /* down held returns the stocked cards */
    if (ly > STICK_DEADZONE * 2)
        keys |= KEY_UNSTOCK;
    return keys;
}

uint16_t PortInputKeys(void) {
    uint16_t keys = sKeyboard;
    SDL_JoystickID* ids;
    int count, i;
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) || defined(PORT_ANDROID64) || defined(PORT_TOUCH)
    keys |= PortAndroidTouchKeys();
#endif

    VirtualPadUpdate();
    ids = SDL_GetGamepads(&count);
    if (ids != NULL) {
        for (i = 0; i < count; i++) {
            SDL_Gamepad* pad = SDL_GetGamepadFromID(ids[i]);
            if (pad == NULL)
                pad = SDL_OpenGamepad(ids[i]);
            if (pad != NULL)
                keys |= GamepadKeys(pad);
        }
        SDL_free(ids);
    }
    if (!gPortConfig.unstockButton)
        keys &= ~KEY_UNSTOCK;
    /* The hardware cannot report opposite directions at once. */
    if ((keys & (KEY_LEFT | KEY_RIGHT)) == (KEY_LEFT | KEY_RIGHT))
        keys &= ~(KEY_LEFT | KEY_RIGHT);
    if ((keys & (KEY_UP | KEY_DOWN)) == (KEY_UP | KEY_DOWN))
        keys &= ~(KEY_UP | KEY_DOWN);
    if (sSuppress) {
        if (keys != 0)
            return 0;
        sSuppress = 0;
    }
    return keys;
}

/* Keep the original localized instructions, swapping only this tutorial's button labels. */
static const void* CardScrollTutorialText(int message, const void* source) {
    static TextChar inverted[256];
    const TextChar* text = source;
    size_t length, i;

    if (!gPortConfig.invertCardScroll || message != CARD_MSG_ROBE_TUTORIAL_18 || text == NULL)
        return source;
    for (length = 0; length < SDL_arraysize(inverted) - 1 && text[length]; length++) {}
    if (text[length])
        return source;
    SDL_memcpy(inverted, text, (length + 1) * sizeof(*text));
#ifdef VERSION_JP
    /* Japanese text uses two-byte Shift-JIS glyphs, including full-width L/R. */
    for (i = 0; i + 1 < length; i += 2) {
        if (inverted[i] == 0x82 && inverted[i + 1] == 0x6b) inverted[i + 1] = 0x71;
        else if (inverted[i] == 0x82 && inverted[i + 1] == 0x71) inverted[i + 1] = 0x6b;
    }
#else
    int highlighted = 0;
    for (i = 0; i < length; i++) {
        if (inverted[i] == MSG_CODE_HIGHLIGHT) highlighted = 1;
        else if (inverted[i] == MSG_CODE_PLAIN) highlighted = 0;
        else if (highlighted && inverted[i] == 'L') inverted[i] = 'R';
        else if (highlighted && inverted[i] == 'R') inverted[i] = 'L';
    }
#endif
    return inverted;
}

/* Only authored, highlighted button tokens in the tutorial range are
   resolved. Prose, names, ordinary A/B letters and unrelated messages stay
   untouched. This buffer is independent from the card-scroll inversion. */
static int ControllerTutorialMessage(int message) {
    return (message>=CARD_MSG_WORLDSELECT_TUTORIAL_0 && message<=CARD_MSG_LEON_TUTORIAL_14) ||
           message==CARD_MSG_WARP_POINT_TUTORIAL || message==CARD_MSG_MAP_STAIR_TUTORIAL || message==CARD_MSG_RIKU_DECK_TUTORIAL
#ifndef VERSION_EU
           || message==CARD_MSG_RIKU_BATTLE_TUTORIAL
#endif
           ;
}
static int ControllerTutorialSemanticToken(int message,int action) {
    int flag=message==CARD_MSG_ROBE_TUTORIAL_18 && (action==PORT_ACT_L || action==PORT_ACT_R);
    /* CardScrollTutorialText already exchanged the displayed L/R. Store the
       original semantic action so another inversion can refresh this page. */
    if(flag && gPortConfig.invertCardScroll)action=action==PORT_ACT_L?PORT_ACT_R:PORT_ACT_L;
    return (action+1) | (flag?64:0);
}
/* Metadata only while Original is displayed. Offsets refer to the returned
   source, never a transformed string length or VM/reveal offset. */
static const TextChar* sPromptOriginalSource;
static size_t sPromptOriginalLength;
static unsigned char sPromptOriginalTokens[512],sPromptOriginalFollowers[512];
static void ControllerTutorialOriginal(int message,const TextChar* text) {
    sPromptOriginalSource=NULL;sPromptOriginalLength=0;
    memset(sPromptOriginalTokens,0,sizeof(sPromptOriginalTokens));
    memset(sPromptOriginalFollowers,0,sizeof(sPromptOriginalFollowers));
    if(!text || gPortConfig.controllerPrompts || !ControllerTutorialMessage(message))return;
    size_t length=0;
    while(length<512 && text[length])length++;
    if(length==512)return;
    sPromptOriginalSource=text;sPromptOriginalLength=length;
#ifndef VERSION_JP
    for(size_t i=0;i<length;i++)if(text[i]==MSG_CODE_HIGHLIGHT) {
        size_t end=i+1;char token[48];int actions[2];
        while(end<length && end-i<sizeof(token) && text[end]!=MSG_CODE_PLAIN)end++;
        if(end>=length || text[end]!=MSG_CODE_PLAIN)continue;
        size_t n=end-i-1;
        for(size_t k=0;k<n;k++)token[k]=(char)text[i+1+k];
        int count=KhPromptTokenActions(token,n,actions);
        static const char* const names[]={"A","B","L","R","START","SELECT"};
        for(int j=0;j<count;j++) {
            const char* name=names[actions[j]];size_t keyLength=strlen(name),offset=n;
            for(size_t k=0;k+keyLength<=n;k++)if(!memcmp(token+k,name,keyLength))offset=k;
            if(offset==n)continue;
            size_t at=i+1+offset;
            sPromptOriginalTokens[at]=(unsigned char)ControllerTutorialSemanticToken(message,actions[j]);
            for(size_t k=1;k<keyLength;k++) { sPromptOriginalFollowers[at+k]=1;sPromptOriginalTokens[at+k]=sPromptOriginalTokens[at]; }
        }
        i=end;
    }
#else
    if(length&1) { sPromptOriginalSource=NULL;return; }
    for(size_t i=0;i<length;i+=2) {
        int actions[2];size_t bytes=0;
        int count=KhPromptSjisActions(text+i,length-i,actions,&bytes);
        for(int j=0;j<count;j++) {
            size_t at=i+(j?4:0);
            sPromptOriginalTokens[at]=(unsigned char)ControllerTutorialSemanticToken(message,actions[j]);
            if(count==1)for(size_t k=2;k<bytes;k+=2) { sPromptOriginalFollowers[at+k]=1;sPromptOriginalTokens[at+k]=sPromptOriginalTokens[at]; }
        }
        if(count)i+=bytes-2;
    }
#endif
}
int PortControllerOriginalPromptAt(const void* position,int sjis,int* follower) {
    uintptr_t base=(uintptr_t)sPromptOriginalSource,at=(uintptr_t)position;
#ifdef VERSION_JP
    const int expectedSjis=1;
#else
    const int expectedSjis=0;
#endif
    *follower=0;
    if(!base || sjis!=expectedSjis || at<base || (at-base)%sizeof(TextChar))return 0;
    size_t offset=(at-base)/sizeof(TextChar);
    if(offset>=sPromptOriginalLength)return 0;
    *follower=sPromptOriginalFollowers[offset];
    return sPromptOriginalTokens[offset];
}
const void* PortCardScrollTutorialText(int message, const void* source) {
    const TextChar* text=CardScrollTutorialText(message,source);
    ControllerTutorialOriginal(message,text);
#ifndef VERSION_JP
    static TextChar resolved[768];
    size_t i=0,n=0;
    int changed=0;
    if (!text || !gPortConfig.controllerPrompts || !ControllerTutorialMessage(message)) return text;
    while (text[i] && i<512) {
        if (text[i]==MSG_CODE_HIGHLIGHT) {
            size_t end=i+1;
            char token[48];
            while(end<i+sizeof(token) && text[end] && text[end]!=MSG_CODE_PLAIN)end++;
            if(text[end]==MSG_CODE_PLAIN && end-i-1<sizeof(token)) {
                size_t length=end-i-1;
                for(size_t k=0;k<length;k++)token[k]=(char)text[i+1+k];
                int actions[2],countActions=KhPromptTokenActions(token,length,actions);
                if(countActions) {
                    resolved[n++]=MSG_CODE_HIGHLIGHT;
                    for(int a=0;a<countActions;a++) {
                        if(n+4>=SDL_arraysize(resolved))return text;
                        if(a)resolved[n++]='+';
                        resolved[n++]=PORT_PROMPT_SEMANTIC_TOKEN;
                        resolved[n++]=(TextChar)ControllerTutorialSemanticToken(message,actions[a]);
                    }
                    resolved[n++]=MSG_CODE_PLAIN;
                    i=end+1;changed=1;continue;
                }
            }
        }
        if(n+1>=SDL_arraysize(resolved))return text;
        resolved[n++]=text[i++];
    }
    if(text[i])return text;
    resolved[n]=0;
    return changed ? resolved : text;
#else
    static unsigned char resolved[1536];
    size_t length=0,i=0,n=0;
    int changed=0;
    if(!text || !gPortConfig.controllerPrompts || !ControllerTutorialMessage(message))return text;
    while(length<512 && text[length])length++;
    if(length==512 || (length&1))return text;
    while(i<length) {
        int actions[2];size_t bytes=0;
        int count=KhPromptSjisActions(text+i,length-i,actions,&bytes);
        if(count) {
            for(int a=0;a<count;a++) {
                if(n+6>=sizeof(resolved))return text;
                if(a) { resolved[n++]=0x81;resolved[n++]=0x7b; }
                resolved[n++]=PORT_PROMPT_SEMANTIC_TOKEN;
                resolved[n++]=(unsigned char)ControllerTutorialSemanticToken(message,actions[a]);
            }
            i+=bytes;changed=1;
        } else {
            if(n+3>=sizeof(resolved))return text;
            resolved[n++]=text[i++];resolved[n++]=text[i++];
        }
    }
    resolved[n]=0;
    return changed ? resolved : text;
#endif
}

/* Select + Up/Down: the main zoom, at any moment (Auto view). Select alone reaches the game when it is released,
   so the chord neither opens the floor map nor switches the card list; with any other button (soft reset
   Start+Select+A+B, debug shortcuts) it reaches the game as it is held. */
static uint16_t ZoomChord(uint16_t keys) {
    uint16_t raw = keys, others = keys & ~(KEY_SELECT | KEY_UP | KEY_DOWN);
    uint16_t pressed = keys & ~sZoomPrev & (KEY_UP | KEY_DOWN);

    sZoomPrev = raw;
    if (gPortConfig.viewMode != PORT_VIEW_AUTO) {
        sZoomSelect = sZoomEat = 0;
        return keys;
    }
    if (!(keys & KEY_SELECT)) {
        if (sZoomSelect == 1)
            keys |= KEY_SELECT; /* a tap: the game sees it now */
        if (sZoomSelect == 3)
            sZoomEat = 1;
        sZoomSelect = 0;
    } else {
        if (sZoomSelect == 0)
            sZoomSelect = (others || (keys & (KEY_UP | KEY_DOWN))) ? 2 : 1;
        else if (sZoomSelect == 1 && others)
            sZoomSelect = 2;
        if ((sZoomSelect == 1 || sZoomSelect == 3) && pressed) {
            sZoomSelect = 3;
            PortUiZoomStep(pressed & KEY_UP ? -1 : 1); /* up: closer */
        }
        if (sZoomSelect != 2)
            keys &= ~KEY_SELECT;
    }
    if (sZoomSelect == 3 || sZoomEat) {
        keys &= ~(KEY_UP | KEY_DOWN);
        if (!(raw & (KEY_UP | KEY_DOWN)))
            sZoomEat = 0;
    }
    return keys;
}

/* A press of one of the deck editor's buttons: pending for a few frames, until the editor takes it. */
static void DeckButton(int down, int* held, int* pending) {
    if (down && !*held)
        *pending = DODGE_PENDING_FRAMES;
    else if (*pending > 0)
        (*pending)--;
    *held = down;
}

uint16_t PortInputResolve(uint16_t keys) {
    int dodge;

    keys = ZoomChord(keys);
    dodge = (keys & KEY_DODGE) && gPortConfig.dodgeButton;

    /* Defer battle Start/Select until release: either can be the first half
       of the options shortcut, which must neither pause nor switch lists. */
    if (gCurrentMode == &gModeBattle || gCurrentMode == &gModeVsbattle) {
        int menuKeys = keys & (KEY_START | KEY_SELECT);
        keys &= ~(KEY_START | KEY_SELECT);
        if (menuKeys == (KEY_START | KEY_SELECT))
            sBattleMenuChord = 1;
        if (!sBattleMenuChord)
            keys |= sBattleMenuKeys & ~menuKeys;
        sBattleMenuKeys = menuKeys;
        if (!menuKeys)
            sBattleMenuChord = 0;
    } else {
        sBattleMenuKeys = sBattleMenuChord = 0;
    }

    /* Reverse local battle inputs once, before hold/chord detection and link transmission. */
    if (gPortConfig.invertCardScroll && (gCurrentMode == &gModeBattle || gCurrentMode == &gModeVsbattle))
        keys = (keys & ~(KEY_L | KEY_R)) | ((keys & KEY_L) >> 1) | ((keys & KEY_R) << 1);

    /* Stock button: L and R together, without pressing both shoulders. */
    if ((keys & KEY_STOCK) && gPortConfig.stockButton)
        keys |= KEY_L | KEY_R;
    /* Dodge button: a press is kept for a few frames for the battle code. */
    if (dodge && !sDodgeHeld)
        sDodgePending = DODGE_PENDING_FRAMES;
    else if (sDodgePending > 0)
        sDodgePending--;
    sDodgeHeld = dodge;
    /* The deck editor reads the same two bindings, with or without their battle options. */
    DeckButton((keys & KEY_DODGE) != 0, &sDeckPlaceHeld, &sDeckPlacePending);
    DeckButton((keys & KEY_STOCK) != 0, &sDeckCompactHeld, &sDeckCompactPending);
    /* Return-stocked-cards button: acts once when held long enough. */
    if ((keys & KEY_UNSTOCK) && gPortConfig.unstockButton) {
        if (sUnstockHeld <= UNSTOCK_HOLD_FRAMES && ++sUnstockHeld == UNSTOCK_HOLD_FRAMES)
            sUnstockPending = DODGE_PENDING_FRAMES;
    } else {
        sUnstockHeld = 0;
        if (!gPortConfig.unstockButton)
            sUnstockPending = 0;
    }
    if (sUnstockPending > 0 && sUnstockHeld != UNSTOCK_HOLD_FRAMES)
        sUnstockPending--;
    return keys & 0x3FF;
}

/* Battle code: the dodge button acts as the second tap of a d-pad double tap
   toward the held direction, or the way the character faces. Returns that
   direction's key, or 0 without a pending press. */
int PortTakeDodge(int held, int facingLeft) {
    if (sDodgePending == 0)
        return 0;
    sDodgePending = 0;
    if (held & KEY_LEFT)
        return KEY_LEFT;
    if (held & KEY_RIGHT)
        return KEY_RIGHT;
    return facingLeft ? KEY_LEFT : KEY_RIGHT;
}

/* Deck editor: 1 once per press of the dodge binding (place the held card). */
int PortTakeDeckPlace(void) {
    int pending = sDeckPlacePending > 0;

    sDeckPlacePending = 0;
    return pending;
}

/* Deck editor: 1 once per press of the stock binding (close the deck's gaps). */
int PortTakeDeckCompact(void) {
    int pending = sDeckCompactPending > 0;

    sDeckCompactPending = 0;
    return pending;
}

/* Battle code: 1 once after the return-stocked-cards button was held. */
int PortTakeUnstock(void) {
    int pending = gPortConfig.unstockButton && sUnstockPending > 0;

    sUnstockPending = 0;
    return pending;
}
