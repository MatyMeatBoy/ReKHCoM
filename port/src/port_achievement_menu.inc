static int sAchievementChoice, sAchievementScroll, sAchievementView;
static int sAchievementList; /* 0 official, 1 Re:CoM, 2 Rebel CoM, 3 Rebel Re:CoM (from Cheats) */
int PortRebelEarned(int index);
int PortRecomActive(void);
/* The set's badges: assets/achievements/badges.png, one atlas (10 per row in
 * set order, 64x64), decoded once when the list is first shown. */
#include <stdio.h>
#include <stdlib.h>
#include "../decompui/third_party/stb_image.h"
enum { kMenuRecomFrame = 97 }; /* the filigree's cell (recom_set.inc kRecomFrameCell) */
static unsigned char* sAchievementAtlas;
static int sAchievementAtlasRows, sAchievementAtlasTried;
static void AchievementMenuBadge(const PortAchievementInfo* info, int index, int x, int y) {
    int px, py;
    if (!sAchievementAtlasTried) {
        int w, h, channels;
        sAchievementAtlasTried = 1;
        size_t size;
        void* file = PortLoadAsset("assets/achievements/badges.png", &size);
        sAchievementAtlas = file ? stbi_load_from_memory(file, (int)size, &w, &h, &channels, 4) : NULL;
        SDL_free(file);
        if (sAchievementAtlas && (w != 640 || h % 64)) { stbi_image_free(sAchievementAtlas); sAchievementAtlas = NULL; }
        sAchievementAtlasRows = sAchievementAtlas ? h / 64 : 0;
    }
    if (!sAchievementAtlas || index < 0 || index / 10 >= sAchievementAtlasRows) {
        DrawText("?", x + 28, y + 24, MENU_DIM, 0xFFFFFFFFu);
        return;
    }
    for (py = 0; py < 64; ++py) for (px = 0; px < 64; ++px) {
        const unsigned char* p = sAchievementAtlas + (((index / 10) * 64 + py) * 640 + (index % 10) * 64 + px) * 4;
        unsigned r = p[0], g = p[1], b = p[2];
        if (info->state != 1) r = g = b = (r * 77 + g * 150 + b * 29) >> 8;
        Pixel(x + px, y + py, 0xFF000000u | r << 16 | g << 8 | b);
    }
    if (sAchievementList && kMenuRecomFrame / 10 < sAchievementAtlasRows) {
        /* the Re:CoM list's gold filigree around the badge: its cell drawn at 80 px (1.25x), solid where it shows */
        for (py = 0; py < 80; ++py) for (px = 0; px < 80; ++px) {
            const unsigned char* p = sAchievementAtlas +
                (((kMenuRecomFrame / 10) * 64 + py * 64 / 80) * 640 + (kMenuRecomFrame % 10) * 64 + px * 64 / 80) * 4;
            if (p[3] < 128) continue;
            if (sAchievementList >= 2) { /* Rebel mode: the same filigree in black, its light kept as dark greys */
                unsigned v = ((p[0] * 77 + p[1] * 150 + p[2] * 29) >> 8) / 4;
                Pixel(x - 8 + px, y - 8 + py, 0xFF000000u | v << 16 | v << 8 | v);
            } else {
                Pixel(x - 8 + px, y - 8 + py, 0xFF000000u | p[0] << 16 | p[1] << 8 | p[2]);
            }
        }
    }
}
/* Read-only, portable list inside Options. Account controls stay in launcher. */
/* the list shown, row i: its text and badge (-1 none) */
static int AchievementMenuCount(void) {
    return (sAchievementList == 1 || sAchievementList == 3) ? PortRecomListCount() : PortAchievementsListCount();
}
static int AchievementMenuGet(int i, PortAchievementInfo* info, int* badge) {
    if (sAchievementList == 1) return PortRecomListGet(i, info, badge);
    if (sAchievementList >= 2) { /* the official set, as Rebel mode earned it */
        int ok = sAchievementList == 3 ? PortRebelRecomListGet(i, info, badge) : PortAchievementsListGet(i, info);
        if (badge && sAchievementList == 2) *badge = i;
        if (ok) {
            static const char* const kIn[6] = { "In Rebel mode: ", "En mode rebelle : ", "Im Rebellen-Modus: ",
                "In modalità ribelle: ", "En modo rebelde: ", "反逆モードで: " };
            char text[sizeof(info->description)];
            int l = Lang();
            if (l < 0 || l > 5) l = 0;
            SDL_snprintf(text, sizeof(text), "%s%s", kIn[l], info->description);
            SDL_strlcpy(info->description, text, sizeof(info->description));
            if (sAchievementList == 2) info->state = PortRebelEarned(i);
        }
        return ok;
    }
    if (badge) *badge = i;
    return PortAchievementsListGet(i, info);
}
static int AchievementMenuRows(int* indices, int* earned) {
    PortAchievementInfo info;
    int i, count = 0;
    *earned = 0;
    for (i = 0; i < AchievementMenuCount(); ++i) {
        if (!AchievementMenuGet(i, &info, NULL)) continue;
        *earned += info.state == 1;
        if ((sAchievementView == 1 && info.state != 0) || (sAchievementView == 2 && info.state != 1)) continue;
        indices[count++] = i;
    }
    return count;
}
static int sAchievementTriggerHeld[2];
static void AchievementMenuOpen(void) {
    PortAchievementsListRefresh();
    sPage = PAGE_ACHIEVEMENTS;
    sAchievementChoice = sAchievementScroll = sAchievementView = 0;
    sAchievementList = PortRecomActive() && PortRecomListCount() > 0; /* a Re:CoM game opens its own list */
    sAchievementTriggerHeld[0] = sAchievementTriggerHeld[1] = 0;
    sMenuLightStart = MenuLightNow();
}
static void RebelMenuOpen(void) {
    AchievementMenuOpen();
    sAchievementList = PortRecomActive() && PortRecomListCount() > 0 ? 3 : 2;
}
static int AchievementMenuInput(const SDL_Event* ev) {
    int move = 0, filter = 0, back = 0, list = 0, indices[256], earned, count;
    if (ev->type == SDL_EVENT_KEY_DOWN) {
        switch (ev->key.key) {
        case SDLK_UP: case SDLK_W: move = -1; break;
        case SDLK_DOWN: case SDLK_S: move = 1; break;
        case SDLK_LEFT: case SDLK_A: move = -5; break;
        case SDLK_RIGHT: case SDLK_D: move = 5; break;
        case SDLK_Q: case SDLK_PAGEUP: filter = -1; break;
        case SDLK_E: case SDLK_PAGEDOWN: filter = 1; break;
        case SDLK_TAB: case SDLK_X: list = 1; break;
        case SDLK_ESCAPE: case SDLK_BACKSPACE: case SDLK_F1: case SDLK_Z: case SDLK_J: back = !ev->key.repeat; break;
        }
    } else if (ev->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        switch (UiPadButton(ev)) {
        case SDL_GAMEPAD_BUTTON_DPAD_UP: move = -1; break;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN: move = 1; break;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT: move = -5; break;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: move = 5; break;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: filter = -1; break;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: filter = 1; break;
        case SDL_GAMEPAD_BUTTON_NORTH: list = 1; break;
        case SDL_GAMEPAD_BUTTON_EAST: back = 1; break;
        case SDL_GAMEPAD_BUTTON_START: case SDL_GAMEPAD_BUTTON_BACK: case SDL_GAMEPAD_BUTTON_GUIDE: Close(); return 1;
        }
    } else if (ev->type == SDL_EVENT_GAMEPAD_AXIS_MOTION && (ev->gaxis.axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER ||
                                                             ev->gaxis.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)) {
        /* L2 / R2 change the list too (user 25/10): once per press */
        int t = ev->gaxis.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, was = sAchievementTriggerHeld[t];
        sAchievementTriggerHeld[t] = KhPadAxisHeld(ev->gaxis.value, 0, was);
        if (sAchievementTriggerHeld[t] && !was) list = 1;
    }
    if (back && sAchievementList >= 2) { sPage = PAGE_CHEATS; sCursor = ITEM_CHEAT_REBEL_LIST; sScroll = 0; return 1; }
    if (back) { sPage = PAGE_MAIN; sCursor = ITEM_ACHIEVEMENTS_MENU; sScroll = 0; return 1; }
    if (list && PortRecomListCount() > 0) {
        sAchievementList = sAchievementList >= 2 ? (sAchievementList == 2 ? 3 : 2) : !sAchievementList;
        sAchievementChoice = sAchievementScroll = 0;
    }
    if (filter) { sAchievementView = Wrap(sAchievementView + filter, 3); sAchievementChoice = sAchievementScroll = 0; }
    count = AchievementMenuRows(indices, &earned);
    if (move && count) {
        /* single steps wrap; a 5-row jump stops at the first or last row */
        if (move == 1 || move == -1) sAchievementChoice = Wrap(sAchievementChoice + move, count);
        else sAchievementChoice = SDL_clamp(sAchievementChoice + move, 0, count - 1);
    }
    if (move || filter || list) sMenuLightStart = MenuLightNow();
    return 1;
}
static void AchievementMenuText(const char* text, int x, int y, int right, uint32_t colour) {
    while (*text) {
        int code = NextChar(&text), advance = GlyphWidth(code);
        if (x + advance > right - 10 && *text) { DrawText("...", x, y, colour, 0xFFD8D8D8u); return; }
        DrawGlyph(code, x, y, colour, 0xFFD8D8D8u); x += advance;
    }
}
static void DrawAchievementMenu(int l, int width, int height) {
    static const char* const filters[2][3] = {{"All", "Locked", "Unlocked"}, {"Todos", "No conseguidos", "Conseguidos"}};
    int indices[256], earned, count = AchievementMenuRows(indices, &earned), es = l == LANG_ES;
    int x = 10, top = MENU_TOP + 23, rows = SDL_max(1, (height - top - 94) / 16), i, y;
    int right = width - 10;
    PortAchievementInfo info;
    char text[160];
    Fill(0, 0, width, height, 0xFFFFFFFFu);
    const char* title = sAchievementList == 3 ? (es ? "Logros · Rebelde Re:CoM" : "Achievements · Rebel Re:CoM")
             : sAchievementList == 2 ? (es ? "Logros · Rebelde CoM" : "Achievements · Rebel CoM")
             : sAchievementList ? (es ? "Logros · Re:Chain of Memories" : "Achievements · Re:Chain of Memories")
                              : (es ? "Logros · Chain of Memories" : "Achievements · Chain of Memories");
    if (PortRecomListCount() > 0) {
        int left = DrawText("L2 < ", x, MENU_TITLE_Y, MENU_RED_DARK, 0xFFD8D8D8u);
        int end = right - TextWidth(" > R2");
        AchievementMenuText(title, left, MENU_TITLE_Y, end, MENU_INK);
        DrawText(" > R2", end, MENU_TITLE_Y, MENU_RED_DARK, 0xFFD8D8D8u);
    } else {
        AchievementMenuText(title, x, MENU_TITLE_Y, right, MENU_INK);
    }
    SDL_snprintf(text, sizeof(text), "%d/%d    L < %s > R", earned, AchievementMenuCount(), filters[es][sAchievementView]);
    AchievementMenuText(text, x, MENU_TOP + 2, right, MENU_RED_DARK);
    if (sAchievementChoice >= count) sAchievementChoice = SDL_max(0, count - 1);
    if (sAchievementChoice < sAchievementScroll) sAchievementScroll = sAchievementChoice;
    if (sAchievementChoice >= sAchievementScroll + rows) sAchievementScroll = sAchievementChoice - rows + 1;
    for (i = sAchievementScroll, y = top; i < count && i < sAchievementScroll + rows; ++i, y += 16) {
        int selected = i == sAchievementChoice;
        AchievementMenuGet(indices[i], &info, NULL);
        MenuBar(x, y, right - x, selected);
        /* A filled red mark denotes earned; grey remains locked/unknown. */
        Fill(x + 2, y + 1, 7, 7, info.state == 1 ? MENU_RED : MENU_GREY);
        SDL_snprintf(text, sizeof(text), "%s", info.state ? (es ? "Conseguido" : "Unlocked") : (es ? "No conseguido" : "Locked"));
        DrawText(text, right - TextWidth(text) - 4, y - 2, info.state == 1 ? MENU_RED_DARK : MENU_DIM, 0xFFD8D8D8u);
        AchievementMenuText(info.title, x + 16, y - 2, right - TextWidth(text) - 12, info.state == 1 ? MENU_INK : MENU_DIM);
    }
    if (!count) DrawText(es ? "Sin resultados" : "No matches", x + 16, top, MENU_DIM, 0xFFFFFFFFu);
    if (count) {
        const char* p;
        int dx = x, dy = height - 88;
        int descriptionRight = right - 76;
        int badge;
        AchievementMenuGet(indices[sAchievementChoice], &info, &badge);
        AchievementMenuBadge(&info, badge, right - 64, height - 88);
        p = info.description;
        while (*p && dy < height - 25) {
            int code = NextChar(&p), advance = GlyphWidth(code);
            if (dx + advance > descriptionRight) { dx = x; dy += 11; }
            if (dy >= height - 25) break;
            DrawGlyph(code, dx, dy, MENU_INK, 0xFFD8D8D8u); dx += advance;
        }
    }
    {
#ifdef VERSION_EU
        const char* note = sAchievementList >= 2 ? (es ? "Con trucos; solo en este equipo, nunca en RetroAchievements" :
                                                       "With cheats; on this device only, never on RetroAchievements")
                         : sAchievementList ? (es ? "Partidas Re:CoM; solo en este equipo, nunca en línea" :
                                                    "Re:CoM games; kept on this device, never online")
                         : es ? "Se consiguen al jugar, con o sin cuenta de RetroAchievements" :
                                "Earned as you play, with or without a RetroAchievements account";
#else
        const char* note = es ? "El set oficial es de la versión europea" : "The official set is for the European version";
#endif
        AchievementMenuText(note, x, height - 22, right, MENU_DIM);
    }
    AchievementMenuText(PortRecomListCount() > 0 ? (es ? "Arriba/abajo  L/R: filtro  Y/L2/R2: CoM/Re:CoM  B: volver"
                                                        : "Up/down  L/R: filter  Y/L2/R2: CoM/Re:CoM  B: back")
                        : es ? "Arriba/abajo: lista  L/R: filtro  B: volver" : "Up/down: list  L/R: filter  B: back",
        x, height - 11, right, MENU_DIM);
}
