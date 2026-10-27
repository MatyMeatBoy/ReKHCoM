/* Options > Scenes (port_scene_gallery.c): the story's scenes, the ones not seen yet as "???" (as the journal does).
 * GBA, HD and ReKHCoM (the same HD/native selection as live gameplay with its HD switches on).
 * Y or L/R changes the tab. Replays preserve the original game and its logical Re:CoM version. */
int PortHdSceneImported(const char* movie);
int PortSceneGalleryPlay(int eventId);
#include "evt/event_ids.h"
#include "port/port_scene_gallery.h"
#include "port_scene_names.inc"

static int sSceneChoice, sSceneScroll, sSceneHarnessTick;
static int sSceneOrder[sizeof(kSceneNames) / sizeof(kSceneNames[0])], sSceneOrderN;
static int sSceneView[sizeof(kSceneNames) / sizeof(kSceneNames[0])], sSceneViewN, sSceneViewTab = -1;
static unsigned char sSceneImported[sizeof(kSceneNames) / sizeof(kSceneNames[0])];
static unsigned char sSceneBroken[sizeof(kSceneNames) / sizeof(kSceneNames[0])];
static int SceneAt(int i, const char** movie);
static int SceneViewCount(void);
int PortUiGalleryHarnessDone(void) {
    const char* at = SDL_getenv("KHCOM_GALLERY_QUIT_AFTER");
    return at && sSceneHarnessTick >= SDL_atoi(at) && !sOpen && !sScenePause;
}

void PortUiGalleryResume(void) {
    if (PortScenePosterRendering()) return; /* offline capture exits instead of opening a paused menu */
    int choice = sSceneChoice, scroll = sSceneScroll, hd = sSceneHd, tick = sSceneHarnessTick;
    int eventId = SceneAt(choice, NULL), i;
    unsigned char broken[sizeof(sSceneBroken)];
    memcpy(broken, sSceneBroken, sizeof(broken));
    sScenePause = 0;
    PortUiOpenOptions();
    sPage = PAGE_SCENES;
    sSceneChoice = choice; sSceneScroll = scroll; sSceneHd = hd; sSceneHarnessTick = tick;
    memcpy(sSceneBroken, broken, sizeof(broken));
    sSceneViewTab = -1;
    for (i = 0; i < SceneViewCount(); i++) if (SceneAt(i, NULL) == eventId) { sSceneChoice = i; break; }
    sMenuLightStart = MenuLightNow();
}

static int SceneKey(SDL_Keycode key, int action) {
    return (gPortConfig.keys[action][0] && key == (SDL_Keycode)gPortConfig.keys[action][0]) ||
           (gPortConfig.keys[action][1] && key == (SDL_Keycode)gPortConfig.keys[action][1]);
}

static void SceneFitText(char* text, int width) {
    while (TextWidth(text) > width && *text) {
        size_t n = strlen(text) - 1;
        while (n && ((unsigned char)text[n] & 0xC0) == 0x80) n--;
        text[n] = 0;
    }
}
static void SceneCopyText(char* out, size_t size, const char* text) {
    size_t length = strlen(text), n = length < size - 1 ? length : size - 1;
    if (n < length) while (n && ((unsigned char)text[n] & 0xC0) == 0x80) n--;
    memcpy(out, text, n); out[n] = 0;
}

static int SceneCount(void) { return (int)(sizeof(kSceneNames) / sizeof(kSceneNames[0])); }

static int SceneRawAt(int i, const char** movie) {
    int k, j, eventId;
    if (!sSceneOrderN) {
        for (k = 0; k < SceneCount(); k++) {
            for (j = sSceneOrderN; j > 0 && kSceneNames[sSceneOrder[j - 1]].storyOrder > kSceneNames[k].storyOrder; j--)
                sSceneOrder[j] = sSceneOrder[j - 1];
            sSceneOrder[j] = k;
            sSceneOrderN++;
        }
    }
    if (i < 0 || i >= SceneCount()) return -1;
    eventId = kSceneNames[sSceneOrder[i]].eventId;
    if (movie) { const char* name = PortHdGalleryMovieName(eventId); *movie = name ? name : ""; }
    return eventId;
}

/* Resolve availability once per opening/tab, never stat dozens of movie files every render frame. */
static void SceneBuildView(void) {
    int i, j;
    if (sSceneViewTab == sSceneHd) return;
    sSceneViewTab = sSceneHd;
    sSceneViewN = 0;
    for (i = 0; i < SceneCount(); i++) {
        const char* movie;
        SceneRawAt(i, &movie);
        sSceneImported[i] = !sSceneBroken[i] && *movie && PortHdSceneImported(movie);
    }
    for (i = 0; i < SceneCount(); i++) {
        const char* movie;
        int id = SceneRawAt(i, &movie), duplicate = 0;
        if (sSceneHd == 1 && !*movie) continue;
        /* A complete HD event already contains its movie tail. Match the actual shared mapping, not file numbers. */
        if (sSceneHd && PORT_SCENE_IS_MOVIE(id) && sSceneImported[i])
            for (j = 0; j < SceneCount(); j++) {
                const char* other;
                int otherId = SceneRawAt(j, &other);
                if (!PORT_SCENE_IS_MOVIE(otherId) && sSceneImported[j] && PortSceneIsSeen(otherId) && !strcmp(movie, other)) {
                    duplicate = 1; break;
                }
            }
        if (!duplicate) sSceneView[sSceneViewN++] = i;
    }
}

static int SceneViewCount(void) { SceneBuildView(); return sSceneViewN; }
static int SceneAt(int i, const char** movie) {
    SceneBuildView();
    return i >= 0 && i < sSceneViewN ? SceneRawAt(sSceneView[i], movie) : -1;
}
static int SceneImportedAt(int i) {
    SceneBuildView();
    return i >= 0 && i < sSceneViewN && sSceneImported[sSceneView[i]];
}
static int SceneProviderHd(int i) { return sSceneHd == 1 || (sSceneHd == 2 && SceneImportedAt(i)); }
static int SceneRiku(int id) { return PORT_SCENE_IS_MOVIE(id) ? id == PORT_SCENE_MOVIE(5) : id >= EVENT_149_RIKU_B12F_OPNING; }
static const char* SceneLabel(int i, int l) {
    SceneBuildView();
    return i >= 0 && i < sSceneViewN ? kSceneNames[sSceneOrder[sSceneView[i]]].name[l] : "";
}
static void SceneMarkBroken(const char* movie) {
    int i, eventId = SceneAt(sSceneChoice, NULL);
    for (i = 0; i < SceneCount(); i++) {
        const char* name;
        SceneRawAt(i, &name);
        if (!strcmp(movie, name)) sSceneBroken[i] = 1;
    }
    sSceneViewTab = -1; /* restore native movie aliases too when their complete HD event could not be decoded */
    for (i = 0; i < SceneViewCount(); i++) if (SceneAt(i, NULL) == eventId) { sSceneChoice = i; break; }
    PortLog("scene gallery: HD resource failed; native aliases restored for %s", movie);
    if (SDL_getenv("KHCOM_GALLERY_AUDIT")) for (i = 0; i < SceneViewCount(); i++) {
        const char* name;
        int id = SceneAt(i, &name);
        if (PORT_SCENE_IS_MOVIE(id) && !strcmp(name, movie))
            PortLog("scene gallery native alias: id=%d available=%d", id, !SceneProviderHd(i));
    }
}

static void SceneMenuOpen(void) {
    extern void PortSceneGalleryPreviewCacheClear(void);
    PortSceneGalleryPreviewCacheClear(); /* a fresh visit can see updated poster assets */
    sPage = PAGE_SCENES;
    sSceneChoice = sSceneScroll = 0;
    sSceneHd = SDL_getenv("KHCOM_SCENES_SHOT") ? SDL_clamp(SDL_atoi(SDL_getenv("KHCOM_SCENES_SHOT")) - 1, 0, 2) : 0;
    memset(sSceneBroken, 0, sizeof(sSceneBroken));
    sSceneViewTab = -1;
    SceneBuildView();
    sSceneHarnessTick = 0;
    if (SDL_getenv("KHCOM_GALLERY_CHOICE"))
        sSceneChoice = SDL_clamp(SDL_atoi(SDL_getenv("KHCOM_GALLERY_CHOICE")), 0, SceneViewCount() - 1);
    if (SDL_getenv("KHCOM_GALLERY_EVENT")) {
        int i, found = 0, wanted = SDL_atoi(SDL_getenv("KHCOM_GALLERY_EVENT"));
        for (i = 0; i < SceneViewCount(); i++) if (SceneAt(i, NULL) == wanted) { sSceneChoice = i; found = 1; break; }
        PortLog("scene gallery selection: event=%d found=%d tab=%d", wanted, found, sSceneHd);
    }
    sMenuLightStart = MenuLightNow();
    if (SDL_getenv("KHCOM_GALLERY_PREVIEWTEST")) {
        static int checked;
        if (!checked) {
            extern int PortSceneBackgroundPreview(int eventId, uint32_t* out, int width, int height);
            uint32_t still[160 * 90];
            extern int PortScenePosterPreview(int, uint32_t*, int*, int*);
            uint32_t* poster = SDL_malloc(640 * 360 * sizeof(uint32_t));
            int i, ok = 0, posterOk = 0;
            checked = 1;
            for (i = 0; i < SceneCount(); i++) {
                int eventId = SceneRawAt(i, NULL), pw, ph;
                if (poster && PortScenePosterPreview(eventId, poster, &pw, &ph)) posterOk++;
                if (PortSceneBackgroundPreview(eventId, still, 160, 90)) ok++;
                else PortLog("scene gallery preview: missing GBA background event %d", eventId);
            }
            PortLog("scene gallery preview check: %d/%d cartridge backgrounds", ok, SceneCount());
            PortLog("scene gallery engine posters: %d/%d portable PNGs", posterOk, SceneCount());
            SDL_free(poster);
        }
    }
}

static int SceneMenuInput(const SDL_Event* ev) {
    int move = 0, tab = 0, back = 0, play = 0, count = SceneViewCount();
    if (ev->type == SDL_EVENT_KEY_DOWN) {
        if (SceneKey(ev->key.key, PORT_ACT_A)) play = !ev->key.repeat;
        else if (SceneKey(ev->key.key, PORT_ACT_B)) back = !ev->key.repeat;
        else if (SceneKey(ev->key.key, PORT_ACT_L) || SceneKey(ev->key.key, PORT_ACT_R)) tab = !ev->key.repeat;
        else
        switch (ev->key.key) {
        case SDLK_UP: case SDLK_W: move = -1; break;
        case SDLK_DOWN: case SDLK_S: move = 1; break;
        case SDLK_LEFT: case SDLK_A: move = -5; break;
        case SDLK_RIGHT: case SDLK_D: move = 5; break;
        case SDLK_Q: case SDLK_PAGEUP: case SDLK_E: case SDLK_PAGEDOWN: case SDLK_TAB: tab = 1; break;
        case SDLK_RETURN: case SDLK_SPACE: case SDLK_K: play = !ev->key.repeat; break;
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
    if (back) { extern void PortSceneGalleryPreviewCancel(void); PortSceneGalleryPreviewCancel(); sPage = PAGE_MAIN; sCursor = ITEM_SCENES_MENU; sScroll = 0; return 1; }
    if (tab) {
        int oldId = SceneAt(sSceneChoice, NULL), i;
        sSceneHd = (sSceneHd + 1) % 3;
        count = SceneViewCount();
        sSceneChoice = 0;
        for (i = 0; i < count; i++) if (SceneAt(i, NULL) == oldId) { sSceneChoice = i; break; }
        sSceneScroll = 0;
    }
    if (move && count) {
        if (move == 1 || move == -1) sSceneChoice = Wrap(sSceneChoice + move, count);
        else sSceneChoice = SDL_clamp(sSceneChoice + move, 0, count - 1);
    }
    if (play && count) {
        const char* movie;
        int eventId = SceneAt(sSceneChoice, &movie);
        if (eventId >= 0 && PortSceneIsSeen(eventId)) {
            if (SceneProviderHd(sSceneChoice) && SceneImportedAt(sSceneChoice)) {
                extern void PortMenuSoundEnd(void);
                extern void PortMenuSoundBegin(void);
                extern void PortSceneGalleryHdBegin(int eventId);
                extern void PortSceneGalleryHdEnd(void);
                uint32_t *canvas = sFb, *hdCanvas = sHdUiFrame;
                int canvasW = sFbW, canvasH = sFbH;
                int played;
                /* A movie's own pause must not inherit the gallery's open overlay or mixer snapshot. */
                PortMenuSoundEnd();
                PortUiForceClose();
                PortSceneGalleryAuditBegin();
                PortSceneGalleryHdBegin(eventId);
                played = PortHdGalleryPlayScene(eventId, sSceneHd == 2);
                PortSceneGalleryHdEnd();
                PortSceneGalleryAuditEnd();
                PortUiGalleryResume();
                /* MoviePause draws into and frees its own temporary canvas. Never resume this draw on that pointer. */
                sFb = canvas; sFbW = canvasW; sFbH = canvasH;
                sHdUiFrame = hdCanvas;
                sHdUiCount = 0;
                PortMenuSoundBegin();
                PortLog("scene gallery: HD %s -> %d; returning to the list choice=%d tab=%d", movie, played, sSceneChoice, sSceneHd);
                if (!played && sSceneHd == 2) SceneMarkBroken(movie);
                if (!played && sSceneHd == 1) PortUiNotice(Lang() == LANG_ES ? "No se pudo abrir el vídeo HD" : "The HD video could not be opened");
                if (!played && sSceneHd == 2 && PortSceneGalleryPlay(eventId)) Close();
            } else if (sSceneHd != 1 && PortSceneGalleryPlay(eventId)) {
                Close();
            }
        }
    }
    if (move || tab) sMenuLightStart = MenuLightNow();
    return 1;
}

static void SceneHarness(void) {
    const char* route = SDL_getenv("KHCOM_GALLERY_KEYS");
    const char* shots = SDL_getenv("KHCOM_GALLERY_SHOTS");
    if (!route && !shots) return;
    ++sSceneHarnessTick;
    while (route && *route) {
        const char* colon = SDL_strchr(route, ':');
        const char* comma = SDL_strchr(route, ',');
        if (colon && (!comma || colon < comma) && SDL_atoi(route) == sSceneHarnessTick) {
            int key = SDL_atoi(colon + 1);
            SDL_Event ev;
            SDL_memset(&ev, 0, sizeof(ev));
            ev.type = SDL_EVENT_KEY_DOWN;
            ev.key.key = key;
            if (key < 0) Close(); else SceneMenuInput(&ev);
            PortLog("scene gallery test: tick=%d key=%d choice=%d HD=%d", sSceneHarnessTick, key, sSceneChoice, sSceneHd);
        }
        route = comma ? comma + 1 : NULL;
    }
    while (shots && *shots) {
        if (SDL_atoi(shots) == sSceneHarnessTick) {
            extern void PortHarnessShot(const char* name);
            char name[64];
            SDL_snprintf(name, sizeof(name), "gallery_%04d.bmp", sSceneHarnessTick);
            PortHarnessShot(name);
        }
        shots = SDL_strchr(shots, ',');
        if (shots) shots++;
    }
}

static void DrawSceneMenu(int l, int width, int height) {
    SceneHarness();
    static const char* const kTitle[LANG_COUNT] = { "Scenes", "Scènes", "Szenen", "Scene", "Escenas", "シーン" };
    static const char* const kUnlocked[LANG_COUNT] = {
        "Unlocked scenes: %d/%d", "Scènes débloquées : %d/%d", "Freigeschaltete Szenen: %d/%d",
        "Scene sbloccate: %d/%d", "Escenas desbloqueadas: %d/%d", "かいほうしたシーン: %d/%d" };
    static const char* const kLocked[LANG_COUNT] = { "???", "???", "???", "???", "???", "???" };
    static const char* const kTabs[3][LANG_COUNT] = {
        { "GBA", "GBA", "GBA", "GBA", "GBA", "GBA" }, { "HD", "HD", "HD", "HD", "HD", "HD" },
        { "ReKHCoM", "ReKHCoM", "ReKHCoM", "ReKHCoM", "ReKHCoM", "ReKHCoM" } };
    static const char* const kHelp[LANG_COUNT] = {
        "Up/down  A: watch  Y/L/R: tab  B: back", "Haut/bas  A : voir  Y/L/R : GBA/HD  B : retour",
        "Auf/ab  A: ansehen  Y/L/R: tab  B: zurück", "Su/giù  A: guarda  Y/L/R: tab  B: indietro",
        "Arriba/abajo  A: ver  Y/L/R: tab  B: volver", "うえ/した  A:みる  Y/L/R:タブ  B:もどる" };
    static const char* const kNoHd[LANG_COUNT] = {
        "Not imported (HD mod).", "Non importée (mod HD).", "Nicht importiert (HD-Mod).", "Non importata (mod HD).",
        "No importada (mod HD).", "HD MODにありません。" };
    static const char* const kTitleOnly[LANG_COUNT] = {
        "On this platform GBA replay starts from the title.", "Sur cette plateforme, les scènes GBA partent du titre.",
        "GBA-Szenen starten hier vom Titelbildschirm.", "Qui le scene GBA partono dal titolo.",
        "En esta plataforma las escenas GBA parten desde el título.", "GBAのシーンはタイトルからみられます。" };
    extern int PortSceneGalleryAvailable(void);
    int nativeAvailable = PortSceneGalleryAvailable();
    int count = SceneViewCount(), seen = 0, x = 10, top = MENU_TOP + 23, right = width - 10, i, y;
    int cardW = SDL_min(170, width * 2 / 5), cardX = right - cardW;
    int listRight = cardX - 10;
    int rows = SDL_max(1, (height - top - 30) / 16);
    char text[160];
    for (i = 0; i < count; i++)
        seen += PortSceneIsSeen(SceneAt(i, NULL));
    Fill(0, 0, width, height, 0xFFFFFFFFu);
    SDL_snprintf(text, sizeof(text), "%s - %s", kTitle[l], kTabs[sSceneHd][l]);
    DrawText(text, x, MENU_TITLE_Y, MENU_INK, 0xFFD8D8D8u);
    SDL_snprintf(text, sizeof(text), kUnlocked[l], seen, count);
    DrawText(text, x, MENU_TOP + 2, MENU_RED_DARK, 0xFFD8D8D8u);
    if (sSceneChoice >= count) sSceneChoice = SDL_max(0, count - 1);
    if (sSceneChoice < sSceneScroll) sSceneScroll = sSceneChoice;
    if (sSceneChoice >= sSceneScroll + rows) sSceneScroll = sSceneChoice - rows + 1;
    for (i = sSceneScroll, y = top; i < count && i < sSceneScroll + rows; i++, y += 16) {
        const char* movie;
        int eventId = SceneAt(i, &movie), on = PortSceneIsSeen(eventId);
        int playable = on && (SceneProviderHd(i) ? SceneImportedAt(i) : nativeAvailable);
        char name[96];
        MenuBar(x, y, listRight - x, i == sSceneChoice);
        Fill(x + 2, y + 1, 7, 7, playable ? MENU_RED : MENU_GREY);
        if (on)
            SceneCopyText(name, sizeof(name), SceneLabel(i, l));
        SDL_snprintf(text, sizeof(text), "%02d  %s", i + 1, on ? name : kLocked[l]);
        /* Long names stay on their own row; the full selected name is repeated on the card below its still. */
        SceneFitText(text, listRight - x - 20);
        DrawText(text, x + 16, y - 2, playable ? MENU_INK : MENU_DIM, 0xFFD8D8D8u);
    }
    {
        const char* movie;
        int eventId = count ? SceneAt(sSceneChoice, &movie) : -1;
        const char* note = NULL;
        if (eventId >= 0 && PortSceneIsSeen(eventId)) {
            extern const uint32_t* PortSceneGalleryPreview(int eventId, int hd);
            const uint32_t* preview = PortSceneGalleryPreview(eventId, SceneProviderHd(sSceneChoice));
            int pw = cardW - 12, ph = SDL_min(pw * 9 / 16, height - top - 72), px, py;
            char label[160];
            /* Keep the existing white menu card; no grey frame around the still. */
            Fill(cardX, top, cardW, ph + 60, 0xFFFFFFFFu);
            MenuBar(cardX + 4, top + 2, cardW - 8, 1);
            extern int PortSceneGalleryPreviewIsFallback(void);
            int previewHd = SceneProviderHd(sSceneChoice) && !PortSceneGalleryPreviewIsFallback();
            DrawText(SceneRiku(eventId) ? (previewHd ? "Riku - HD" : "Riku - GBA") : (previewHd ? "Sora - HD" : "Sora - GBA"),
                     cardX + 16, top, MENU_INK, 0xFFD8D8D8u);
            Fill(cardX + 6, top + 16, pw, ph, 0xFFFFFFFFu);
            if (preview) {
                extern void PortSceneGalleryPreviewSize(int*, int*);
                int sourceW, sourceH, drawW, drawH, drawX, drawY;
                PortSceneGalleryPreviewSize(&sourceW, &sourceH);
                drawW = pw; drawH = pw * sourceH / sourceW;
                if (drawH > ph) { drawH = ph; drawW = ph * sourceW / sourceH; }
                drawX = cardX + 6 + (pw - drawW) / 2; drawY = top + 16 + (ph - drawH) / 2;
                if (sHdUiFrame && sFb == sHdUiFrame && sHdUiCount < 2048) {
                    int n = sHdUiCount++;
                    sHdUiGlyphs[n].code = -2; sHdUiGlyphs[n].alive = 1;
                    sHdUiGlyphs[n].x = drawX; sHdUiGlyphs[n].y = drawY;
                    sHdUiGlyphs[n].pw = drawW; sHdUiGlyphs[n].run = drawH;
                    sHdUiGlyphs[n].face = 0xFFFFFFFFu; sHdUiGlyphs[n].shadow = 0xFF000000u;
                } else for (py = 0; py < drawH; py++) for (px = 0; px < drawW; px++)
                    Pixel(drawX + px, drawY + py, preview[(py * sourceH / drawH) * sourceW + px * sourceW / drawW]);
            } else {
                const char* empty = l == LANG_ES ? "Sin vista previa" : "No preview available";
                while (TextWidth(empty) > pw) { empty = l == LANG_ES ? "Sin captura" : "No still yet"; break; }
                DrawText(empty, cardX + 7, top + 18 + ph / 2, MENU_DIM, MENU_GREY);
            }
            SceneCopyText(label, sizeof(label), SceneLabel(sSceneChoice, l));
            /* Wrap at spaces to keep the scene's name separate from its picture. */
            {
                char *line = label, *end = label;
                int yy = top + 20 + ph, lines = 0;
                while (*line && lines++ < 3) {
                    char *last = NULL;
                    end = line;
                    while (*end) {
                        char saved = *end;
                        *end = 0;
                        if (TextWidth(line) > cardW - 12) { *end = saved; break; }
                        *end = saved;
                        if (*end == ' ') last = end;
                        end++;
                        while (((unsigned char)*end & 0xC0) == 0x80) end++;
                    }
                    if (*end && last && last > line) end = last;
                    {
                        char saved = *end;
                        *end = 0;
                        DrawText(line, cardX + 6, yy, MENU_INK, 0xFFD8D8D8u);
                        *end = saved;
                        line = saved == ' ' ? end + 1 : end;
                    }
                    yy += 10;
                }
            }
            if (sSceneHd == 1 && !SceneImportedAt(sSceneChoice))
                note = kNoHd[l];
            else if (!SceneProviderHd(sSceneChoice) && !nativeAvailable)
                note = kTitleOnly[l];
        }
        if (note)
            DrawText(note, x, height - 22, MENU_DIM, 0xFFFFFFFFu);
    }
    DrawText(kHelp[l], x, height - 11, MENU_DIM, 0xFFFFFFFFu);
}
