#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <SDL3/SDL.h>
#include "port/port.h"
#include "port/pad_layout.h"
#include "port/menu_input.h"
#include "port/controller_prompts.h"
#include "port/port_scene_gallery.h"
#include "evt/event_ids.h"
#include "../src/port_scene_names.inc"

PortConfig gPortConfig;
enum { PAGE_MAIN, PAGE_SCENES, PAGE_ACHIEVEMENTS, ITEM_SCENES_MENU, LANG_ES = 4, LANG_JA = 5, MENU_FADE = 8 };
static int sSceneHd, sPage, sCursor, sScroll, sOpen, sFade, sClosing, sReturn, sScenePause;
static int sFbW, sFbH, sHdUiCount, sMenuLightStart;
static uint32_t *sFb, *sHdUiFrame;
static int playCalls, imported = 1, allSeen = 1, clicks;
static int physicalAxes[2];
static SDL_JoystickID* FakeGamepads(int* count) {
    SDL_JoystickID* ids = SDL_malloc(sizeof(*ids)); assert(ids);
    *count = 1; ids[0] = 1; return ids;
}
static SDL_Gamepad* FakePad(SDL_JoystickID id) { (void)id; return (SDL_Gamepad*)(uintptr_t)1; }
static Sint16 FakeAxis(SDL_Gamepad* pad, SDL_GamepadAxis axis) {
    (void)pad; return physicalAxes[axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER];
}
#define SDL_GetGamepads FakeGamepads
#define SDL_GetGamepadFromID FakePad
#define SDL_GetGamepadAxis FakeAxis
static int SceneAt(int, const char**);
static int SceneViewCount(void);
static void SceneMenuOpen(void);
static int UiPadButton(const SDL_Event*);
void PortUiGalleryResume(void);
static int MenuLightNow(void) { return 123; }
static int Wrap(int value, int count) { return (value + count) % count; }
static int Lang(void) { return 0; }
static void Close(void) { sOpen = 0; }
void PortLog(const char* format, ...) { (void)format; }
int PortScenePosterRendering(void) { return 0; }
void PortInputSuppressUntilRelease(void) {}
void PortUiOpenOptions(void) { SceneMenuOpen(); }
void PortSceneGalleryPreviewCacheClear(void) {}
void PortSceneGalleryPreviewCancel(void) {}
int PortSceneBackgroundPreview(int id, uint32_t* out, int w, int h) { (void)id; (void)out; (void)w; (void)h; return 0; }
int PortScenePosterPreview(int id, uint32_t* out, int* w, int* h) { (void)id; (void)out; (void)w; (void)h; return 0; }
int PortSceneIsSeen(int id) { return allSeen || id == EVENT_149_RIKU_B12F_OPNING; }
int PortHdSceneImported(const char* name) { (void)name; return imported; }
const char* PortHdGalleryMovieName(int id) {
    if (id == EVENT_000_1F_ENTRANCE_PART1) return "sora0";
    if (id == EVENT_012_2F_ENTRANCE) return "sora12";
    if (id == EVENT_149_RIKU_B12F_OPNING || id == PORT_SCENE_MOVIE(4)) return "shared";
    if (id == EVENT_195_RIKU_ENDING || id == PORT_SCENE_MOVIE(5)) return "riku";
    return NULL;
}
int PortSceneGalleryPlay(int id) { (void)id; playCalls++; return 1; }
int PortHdGalleryPlayScene(int id, int hybrid) { (void)id; (void)hybrid; playCalls++; return 1; }
void PortMenuSoundBegin(void) {}
void PortMenuSoundEnd(void) {}
void PortMenuSoundClick(void) { clicks++; }
void PortUiForceClose(void) {}
void PortSceneGalleryAuditBegin(void) {}
void PortSceneGalleryAuditEnd(void) {}
void PortSceneGalleryHdBegin(int id) { (void)id; }
void PortSceneGalleryHdEnd(void) {}
void PortUiNotice(const char* text) { (void)text; }
int PortControllerBindingLabel(int binding, char* out, size_t size) {
    if (!gPortConfig.controllerPrompts) return 0;
    SDL_strlcpy(out, KhPromptButtonLabel(binding, gPortConfig.controllerPrompts, 0), size);
    return 1;
}
#include "gallery-consumer.inc"

static void character(int riku) {
    assert(SceneViewCount() > 0);
    for (int i = 0; i < SceneViewCount(); i++) assert(SceneRiku(SceneAt(i, NULL)) == riku);
}
static void axis(int side, int value) {
    physicalAxes[side] = value;
    SDL_Event event = {0};
    event.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
    event.gaxis.axis = side ? SDL_GAMEPAD_AXIS_RIGHT_TRIGGER : SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
    event.gaxis.value = value;
    SceneMenuInput(&event);
}
static void key(SDL_Keycode key, int repeat) {
    SDL_Event event = {0}; event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = key; event.key.repeat = repeat;
    SceneMenuInput(&event);
}
int main(int argc, char** argv) {
    (void)argv;
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    gPortConfig.pad[PORT_ACT_A] = SDL_GAMEPAD_BUTTON_SOUTH;
    gPortConfig.pad[PORT_ACT_B] = SDL_GAMEPAD_BUTTON_EAST;
    PortConfig original = gPortConfig;
    SceneMenuOpen();
    if (argc > 1) {
        character(1); assert(SceneAt(sSceneChoice, NULL) == SDL_atoi(SDL_getenv("KHCOM_GALLERY_EVENT")));
        puts("PASS targeted Riku event keeps its character and scene"); return 0;
    }
    character(0);
    SDL_Event trigger = {0};
    trigger.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
    trigger.gaxis.axis = SDL_GAMEPAD_AXIS_LEFT_TRIGGER; trigger.gaxis.value = 17000;
    SDL_Event mapped; unsigned char held[2] = {0};
    int axisBindings[2] = {PORT_PAD_AXIS_BASE + SDL_GAMEPAD_AXIS_LEFT_TRIGGER * 2, -1};
    assert(KhPadMenuAxis(&trigger, axisBindings, held, &mapped) == 2); /* would be A without reservation */
    assert(UiListTriggerReserved(&trigger));
    sScenePause = 1; assert(!UiListTriggerReserved(&trigger)); sScenePause = 0;
    sPage = PAGE_MAIN; assert(!UiListTriggerReserved(&trigger));
    sPage = PAGE_ACHIEVEMENTS; assert(UiListTriggerReserved(&trigger)); sPage = PAGE_SCENES;
    int soraCount = SceneViewCount();
    sSceneChoice = 8; sSceneScroll = 4;
    int soraChoice = SceneAt(sSceneChoice, NULL);
    axis(0, 9000); character(0);
    axis(0, 17000); character(1);
    assert(clicks == 1 && playCalls == 0); /* reserved trigger never confirms/replays */
    int rikuCount = SceneViewCount();
    assert(soraCount + rikuCount == SceneCount());
    sSceneChoice = 6; sSceneScroll = 3;
    int rikuChoice = SceneAt(sSceneChoice, NULL);
    axis(0, 15000); axis(0, 9000); character(1);
    axis(0, 8000); axis(0, 9000); character(1);
    axis(0, 17000); character(0);
    assert(SceneAt(sSceneChoice, NULL) == soraChoice && sSceneScroll == 4);
    axis(1, 17000); character(1);
    assert(SceneAt(sSceneChoice, NULL) == rikuChoice && sSceneScroll == 3);
    physicalAxes[0] = physicalAxes[1] = 0; /* both released while the replay owned input */
    PortUiGalleryResume(); character(1);
    assert(SceneAt(sSceneChoice, NULL) == rikuChoice && sSceneScroll == 3);
    axis(1, 17000); character(0);
    axis(1, 0); axis(1, 17000); character(1);
    key(SDLK_F2, 1); character(1);
    key(SDLK_F2, 0); character(0);
    key(SDLK_TAB, 0); assert(sSceneHd == 1); character(0);
    int foundMovie = 0;
    for (int i = 0; i < SceneViewCount(); i++) foundMovie |= SceneAt(i, NULL) == PORT_SCENE_MOVIE(4);
    assert(foundMovie); /* A Riku import must not suppress a Sora movie alias. */
    key(SDLK_F2, 0); character(1);
    for (int i = 0; i < SceneViewCount(); i++) assert(SceneAt(i, NULL) != PORT_SCENE_MOVIE(5));
    key(SDLK_TAB, 0); assert(sSceneHd == 2); character(1);
    imported = 0; sSceneViewTab = -1; character(1);
    for (int i = 0; i < SceneViewCount(); i++) assert(!SceneProviderHd(i));
    key(SDLK_TAB, 0); assert(sSceneHd == 0);
    allSeen = 0; key(SDLK_F2, 0); character(0);
    key(SDLK_RETURN, 0); assert(playCalls == 0);
    assert(!memcmp(&original, &gPortConfig, sizeof(original)));
    char help[128];
    gPortConfig.controllerPrompts = KH_PROMPT_PLAYSTATION;
    SceneCharacterHelp(0, help, sizeof(help)); assert(strstr(help, "L2/R2") && strstr(help, "F2"));
    gPortConfig.controllerPrompts = KH_PROMPT_XBOX;
    SceneCharacterHelp(LANG_ES, help, sizeof(help)); assert(strstr(help, "LT/RT"));
    gPortConfig.controllerPrompts = KH_PROMPT_NINTENDO;
    SceneCharacterHelp(LANG_JA, help, sizeof(help)); assert(strstr(help, "ZL/ZR"));
    physicalAxes[0] = 17000; SceneMenuOpen(); character(0);
    axis(0, 18000); character(0); /* held before opening is not a new press */
    axis(0, 0); axis(0, 17000); character(1);
    puts("PASS character catalogs, hysteresis, selections, resume, HD aliases/fallback and locked scenes");
    return 0;
}
