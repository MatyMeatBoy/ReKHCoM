static int sAchievementChoice, sAchievementScroll, sAchievementView;
/* The set's badges: assets/achievements/badges.png, one atlas (10 per row in
 * set order, 64x64), decoded once when the list is first shown. */
#include <stdio.h>
#include <stdlib.h>
#include "../decompui/third_party/stb_image.h"
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
}
/* Read-only, portable list inside Options. Account controls stay in launcher. */
static int AchievementMenuRows(int* indices, int* earned) {
    PortAchievementInfo info;
    int i, count = 0;
    *earned = 0;
    for (i = 0; i < PortAchievementsListCount(); ++i) {
        if (!PortAchievementsListGet(i, &info)) continue;
        *earned += info.state == 1;
        if ((sAchievementView == 1 && info.state != 0) || (sAchievementView == 2 && info.state != 1)) continue;
        indices[count++] = i;
    }
    return count;
}
static void AchievementMenuOpen(void) {
    PortAchievementsListRefresh();
    sPage = PAGE_ACHIEVEMENTS;
    sAchievementChoice = sAchievementScroll = sAchievementView = 0;
    sMenuLightStart = MenuLightNow();
}
static int AchievementMenuInput(const SDL_Event* ev) {
    int move = 0, filter = 0, back = 0, indices[256], earned, count;
    if (ev->type == SDL_EVENT_KEY_DOWN) {
        switch (ev->key.key) {
        case SDLK_UP: case SDLK_W: move = -1; break;
        case SDLK_DOWN: case SDLK_S: move = 1; break;
        case SDLK_LEFT: case SDLK_A: move = -5; break;
        case SDLK_RIGHT: case SDLK_D: move = 5; break;
        case SDLK_Q: case SDLK_PAGEUP: filter = -1; break;
        case SDLK_E: case SDLK_PAGEDOWN: filter = 1; break;
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
        case SDL_GAMEPAD_BUTTON_EAST: back = 1; break;
        case SDL_GAMEPAD_BUTTON_START: case SDL_GAMEPAD_BUTTON_BACK: case SDL_GAMEPAD_BUTTON_GUIDE: Close(); return 1;
        }
    }
    if (back) { sPage = PAGE_MAIN; sCursor = ITEM_ACHIEVEMENTS_MENU; sScroll = 0; return 1; }
    if (filter) { sAchievementView = Wrap(sAchievementView + filter, 3); sAchievementChoice = sAchievementScroll = 0; }
    count = AchievementMenuRows(indices, &earned);
    if (move && count) {
        /* single steps wrap; a 5-row jump stops at the first or last row */
        if (move == 1 || move == -1) sAchievementChoice = Wrap(sAchievementChoice + move, count);
        else sAchievementChoice = SDL_clamp(sAchievementChoice + move, 0, count - 1);
    }
    if (move || filter) sMenuLightStart = MenuLightNow();
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
    DrawText(es ? "Logros" : "Achievements", x, MENU_TITLE_Y, MENU_INK, 0xFFD8D8D8u);
    SDL_snprintf(text, sizeof(text), "%d/%d    L < %s > R", earned, PortAchievementsListCount(), filters[es][sAchievementView]);
    AchievementMenuText(text, x, MENU_TOP + 2, right, MENU_RED_DARK);
    if (sAchievementChoice >= count) sAchievementChoice = SDL_max(0, count - 1);
    if (sAchievementChoice < sAchievementScroll) sAchievementScroll = sAchievementChoice;
    if (sAchievementChoice >= sAchievementScroll + rows) sAchievementScroll = sAchievementChoice - rows + 1;
    for (i = sAchievementScroll, y = top; i < count && i < sAchievementScroll + rows; ++i, y += 16) {
        int selected = i == sAchievementChoice;
        PortAchievementsListGet(indices[i], &info);
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
        PortAchievementsListGet(indices[sAchievementChoice], &info);
        AchievementMenuBadge(&info, indices[sAchievementChoice], right - 64, height - 88);
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
        const char* note = es ? "Se consiguen al jugar, con o sin cuenta de RetroAchievements" :
                                "Earned as you play, with or without a RetroAchievements account";
#else
        const char* note = es ? "El set oficial es de la versión europea" : "The official set is for the European version";
#endif
        AchievementMenuText(note, x, height - 22, right, MENU_DIM);
    }
    AchievementMenuText(es ? "Arriba/abajo: lista  L/R: filtro  B: volver" : "Up/down: list  L/R: filter  B: back",
        x, height - 11, right, MENU_DIM);
}
