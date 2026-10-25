/* Options > Scenes (port_scene_gallery.c): the story's scenes, the ones not seen yet as "???" (as the journal does).
 * Two tabs: GBA (the cartridge's scene, replayed in the game's engine, from the title only) and HD (the Re:CoM scene,
 * when the HD mod's scenes are imported). Y or L/R changes the tab. */
int PortHdSceneCount(void);
int PortHdSceneAt(int i, const char** movie);
int PortHdSceneImported(const char* movie);
int PortHdMoviePlayName(const char* movie);
int PortSceneIsSeen(int eventId);
int PortSceneGalleryPlay(int eventId);
#include "evt/event_ids.h"
#include "port_scene_names.inc"

static int sSceneChoice, sSceneScroll;
static int sSceneOrder[sizeof(kSceneNames) / sizeof(kSceneNames[0])], sSceneOrderN;

static int SceneCount(void) {
    return (int)(sizeof(kSceneNames) / sizeof(kSceneNames[0]));
}

/* row i in story order (Sora's events, then Riku's: their ids), and its Re:CoM scene */
static int SceneAt(int i, const char** movie) {
    int k, j, eventId;
    if (!sSceneOrderN) {
        for (k = 0; k < SceneCount(); k++) {
            for (j = sSceneOrderN; j > 0 && kSceneNames[sSceneOrder[j - 1]].eventId > kSceneNames[k].eventId; j--)
                sSceneOrder[j] = sSceneOrder[j - 1];
            sSceneOrder[j] = k;
            sSceneOrderN++;
        }
    }
    if (i < 0 || i >= SceneCount())
        return -1;
    eventId = kSceneNames[sSceneOrder[i]].eventId;
    if (movie) {
        *movie = "";
        for (k = 0; k < PortHdSceneCount(); k++) {
            const char* m;
            if (PortHdSceneAt(k, &m) == eventId) {
                *movie = m;
                break;
            }
        }
    }
    return eventId;
}

static const char* SceneLabel(int i, int l) {
    return i >= 0 && i < SceneCount() ? kSceneNames[sSceneOrder[i]].name[l] : "";
}

static int SceneHdAvailable(void) {
    const char* movie;
    int i;
    for (i = 0; i < SceneCount(); i++)
        if (SceneAt(i, &movie) >= 0 && PortHdSceneImported(movie))
            return 1;
    return 0;
}

static void SceneMenuOpen(void) {
    sPage = PAGE_SCENES;
    sSceneChoice = sSceneScroll = 0;
    sSceneHd = 0;
    sMenuLightStart = MenuLightNow();
}

static int SceneMenuInput(const SDL_Event* ev) {
    int move = 0, tab = 0, back = 0, play = 0, count = SceneCount();
    if (ev->type == SDL_EVENT_KEY_DOWN) {
        switch (ev->key.key) {
        case SDLK_UP: case SDLK_W: move = -1; break;
        case SDLK_DOWN: case SDLK_S: move = 1; break;
        case SDLK_LEFT: case SDLK_A: move = -5; break;
        case SDLK_RIGHT: case SDLK_D: move = 5; break;
        case SDLK_Q: case SDLK_PAGEUP: case SDLK_E: case SDLK_PAGEDOWN: case SDLK_TAB: case SDLK_X: tab = 1; break;
        case SDLK_RETURN: case SDLK_KP_ENTER: case SDLK_SPACE: case SDLK_K: play = !ev->key.repeat; break;
        case SDLK_ESCAPE: case SDLK_BACKSPACE: case SDLK_F1: case SDLK_Z: case SDLK_J: back = !ev->key.repeat; break;
        }
    } else if (ev->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        switch (UiPadButton(ev)) {
        case SDL_GAMEPAD_BUTTON_DPAD_UP: move = -1; break;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN: move = 1; break;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT: move = -5; break;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: move = 5; break;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: case SDL_GAMEPAD_BUTTON_NORTH:
            tab = 1; break;
        case SDL_GAMEPAD_BUTTON_SOUTH: play = 1; break;
        case SDL_GAMEPAD_BUTTON_EAST: back = 1; break;
        case SDL_GAMEPAD_BUTTON_START: case SDL_GAMEPAD_BUTTON_BACK: case SDL_GAMEPAD_BUTTON_GUIDE: Close(); return 1;
        }
    }
    if (back) { sPage = PAGE_MAIN; sCursor = ITEM_SCENES_MENU; sScroll = 0; return 1; }
    if (tab && (sSceneHd || SceneHdAvailable())) { sSceneHd = !sSceneHd; }
    if (move && count) {
        if (move == 1 || move == -1) sSceneChoice = Wrap(sSceneChoice + move, count);
        else sSceneChoice = SDL_clamp(sSceneChoice + move, 0, count - 1);
    }
    if (play && count) {
        const char* movie;
        int eventId = SceneAt(sSceneChoice, &movie);
        if (eventId >= 0 && PortSceneIsSeen(eventId)) {
            if (sSceneHd && PortHdSceneImported(movie)) {
                PortHdMoviePlayName(movie); /* played here, over the paused game; back to this list after */
            } else if (!sSceneHd && PortSceneGalleryPlay(eventId)) {
                Close();
            }
        }
    }
    if (move || tab) sMenuLightStart = MenuLightNow();
    return 1;
}

static void DrawSceneMenu(int l, int width, int height) {
    static const char* const kTitle[LANG_COUNT] = { "Scenes", "Scènes", "Szenen", "Scene", "Escenas", "シーン" };
    static const char* const kLocked[LANG_COUNT] = { "???", "???", "???", "???", "???", "???" };
    static const char* const kTabs[2][LANG_COUNT] = {
        { "GBA", "GBA", "GBA", "GBA", "GBA", "GBA" }, { "HD", "HD", "HD", "HD", "HD", "HD" } };
    static const char* const kHelp[LANG_COUNT] = {
        "Up/down  A: watch  Y/L/R: GBA/HD  B: back", "Haut/bas  A : voir  Y/L/R : GBA/HD  B : retour",
        "Auf/ab  A: ansehen  Y/L/R: GBA/HD  B: zurück", "Su/giù  A: guarda  Y/L/R: GBA/HD  B: indietro",
        "Arriba/abajo  A: ver  Y/L/R: GBA/HD  B: volver", "うえ/した  A:みる  Y/L/R:GBA/HD  B:もどる" };
    static const char* const kTitleOnly[LANG_COUNT] = {
        "GBA scenes play from the title screen.", "Les scènes GBA se lancent depuis l'écran titre.",
        "GBA-Szenen starten vom Titelbildschirm.", "Le scene GBA partono dalla schermata del titolo.",
        "Las escenas GBA se ven desde la pantalla de título.", "GBAのシーンはタイトルからみられます。" };
    static const char* const kNoHd[LANG_COUNT] = {
        "Not imported (HD mod).", "Non importée (mod HD).", "Nicht importiert (HD-Mod).", "Non importata (mod HD).",
        "No importada (mod HD).", "HD MODにありません。" };
    int count = SceneCount(), seen = 0, x = 10, top = MENU_TOP + 23, right = width - 10, i, y;
    int rows = SDL_max(1, (height - top - 30) / 16);
    char text[160];
    for (i = 0; i < count; i++)
        seen += PortSceneIsSeen(SceneAt(i, NULL));
    Fill(0, 0, width, height, 0xFFFFFFFFu);
    SDL_snprintf(text, sizeof(text), "%s - %s", kTitle[l], kTabs[sSceneHd][l]);
    DrawText(text, x, MENU_TITLE_Y, MENU_INK, 0xFFD8D8D8u);
    SDL_snprintf(text, sizeof(text), "%d/%d", seen, count);
    DrawText(text, x, MENU_TOP + 2, MENU_RED_DARK, 0xFFD8D8D8u);
    if (sSceneChoice >= count) sSceneChoice = SDL_max(0, count - 1);
    if (sSceneChoice < sSceneScroll) sSceneScroll = sSceneChoice;
    if (sSceneChoice >= sSceneScroll + rows) sSceneScroll = sSceneChoice - rows + 1;
    for (i = sSceneScroll, y = top; i < count && i < sSceneScroll + rows; i++, y += 16) {
        const char* movie;
        int eventId = SceneAt(i, &movie), on = PortSceneIsSeen(eventId);
        int playable = on && (sSceneHd ? PortHdSceneImported(movie) : 1);
        char name[96];
        MenuBar(x, y, right - x, i == sSceneChoice);
        Fill(x + 2, y + 1, 7, 7, playable ? MENU_RED : MENU_GREY);
        if (on)
            SDL_strlcpy(name, SceneLabel(i, l), sizeof(name));
        SDL_snprintf(text, sizeof(text), "%02d  %s", i + 1, on ? name : kLocked[l]);
        DrawText(text, x + 16, y - 2, playable ? MENU_INK : MENU_DIM, 0xFFD8D8D8u);
    }
    {
        const char* movie;
        int eventId = count ? SceneAt(sSceneChoice, &movie) : -1;
        const char* note = NULL;
        if (eventId >= 0 && PortSceneIsSeen(eventId)) {
            if (sSceneHd && !PortHdSceneImported(movie))
                note = kNoHd[l];
            else if (!sSceneHd && !PortDebugModeAvailable())
                note = kTitleOnly[l];
        }
        if (note)
            DrawText(note, x, height - 22, MENU_DIM, 0xFFFFFFFFu);
    }
    DrawText(kHelp[l], x, height - 11, MENU_DIM, 0xFFFFFFFFu);
}
