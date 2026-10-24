/*
 * Settings, stored as key=value lines in kh_com.ini next to the executable.
 */
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "port/port.h"
#include "port/port_aspect.h"

PortConfig gPortConfig = {
    PORT_VIEW_AUTO,     /* viewMode */
    PORT_ZOOM_DEFAULT,   /* zoom: x4 */
    PORT_SCALE_INTEGER, /* scaleMode */
    PORT_FILTER_NEAREST, /* filter */
    0,                  /* fullscreen */
    1,                  /* vsync */
    1,                  /* hudSize */
    1,                  /* narrowFit */
    0,                  /* scanlines */
    0,                  /* colorCorrection */
    1,                  /* frameSkip: a late frame is not drawn, the logic stays exact (test_frameskip.py) */
    0,                  /* showFps */
    4,                  /* windowScale */
    0,                  /* fullscreenWidth: 0 = the desktop's resolution */
    0,                  /* fullscreenHeight */
    100,                /* masterVolume */
    100,                /* musicVolume */
    100,                /* sfxVolume */
    1,                  /* dodgeButton */
    1,                  /* stockButton */
    0,                  /* hdText */
    8,                  /* cardHudRaise */
    0,                  /* experimental unstock: off by default */
    1,                  /* saveBank */
#ifdef PORT_HD2D
    1,                  /* hd2d: on in the alternative HD-2D build */
#else
    0,                  /* hd2d (experimental) */
#endif
    0,                  /* smoothAudio (experimental) */
    0,                  /* ps2Mode (experimental) */
    0,                  /* ps2Difficulty (experimental) */
    0,                  /* ps2FastText (experimental) */
    0,                  /* hdHud: pending, forced off (see PortConfigLoad) */
    0,                  /* aspect: fill the window */
    0,                  /* integerZoom: legacy key, not shown in the UI */
    {
        { SDLK_X, SDLK_K },             /* A */
        { SDLK_Z, SDLK_J },             /* B */
        { SDLK_A, 0 },                  /* L */
        { SDLK_S, 0 },                  /* R */
        { SDLK_RETURN, 0 },             /* Start */
        { SDLK_BACKSPACE, SDLK_RSHIFT }, /* Select */
        { SDLK_UP, SDLK_W },            /* Up */
        { SDLK_DOWN, 0 },               /* Down */
        { SDLK_LEFT, 0 },               /* Left */
        { SDLK_RIGHT, 0 },              /* Right */
        { SDLK_D, 0 },                  /* stock (L + R) */
        { SDLK_C, 0 },                  /* dodge roll */
        { SDLK_V, 0 },                  /* experimental return stocked cards */
    },
    {
        SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
        SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, SDL_GAMEPAD_BUTTON_START, SDL_GAMEPAD_BUTTON_BACK,
        SDL_GAMEPAD_BUTTON_DPAD_UP, SDL_GAMEPAD_BUTTON_DPAD_DOWN, SDL_GAMEPAD_BUTTON_DPAD_LEFT,
        SDL_GAMEPAD_BUTTON_DPAD_RIGHT, SDL_GAMEPAD_BUTTON_NORTH, SDL_GAMEPAD_BUTTON_WEST,
        SDL_GAMEPAD_BUTTON_RIGHT_STICK,
    },
};

#define CONFIG_VERSION PORT_ZOOM_CONFIG_VERSION

static int sConfigVersion;
/* a settings file from before the graphics modes (no graphics_mode line): the player's own settings, which "GBA
   enhanced" keeps as they are when the first start asks for a mode */
static int sUpgradedSettings;
static char sDataPath[768];

void PortConfigSetDataPath(const char* path) {
    SDL_snprintf(sDataPath, sizeof(sDataPath), "%s/", path);
}

typedef struct Key {
    const char* name;
    int* value;
    int min;
    int max;
} Key;

static const Key sKeys[] = {
    { "config_version", &sConfigVersion, 0, 1000 },
    { "view_mode", &gPortConfig.viewMode, 0, PORT_VIEW_COUNT - 1 },
    { "zoom", &gPortConfig.zoom, 0, PORT_ZOOM_LEVELS },
    { "experimental_battle_zoom", &gPortConfig.battleZoom, 0, PORT_ZOOM_LEVELS },
    { "cheat_unlock_riku", &gPortConfig.cheatUnlockRiku, 0, 1 },
    { "cheat_one_hit", &gPortConfig.cheatOneHit, 0, 1 },
    { "cheat_infinite_hp", &gPortConfig.cheatInfiniteHp, 0, 1 },
    { "experimental_save_point_slot_select", &gPortConfig.savePointSlotSelect, 0, 1 },
    { "experimental_hide_touch_controls", &gPortConfig.hideTouchControls, 0, 1 },
    { "undub", &gPortConfig.undub, 0, 1 },
    { "integer_zoom", &gPortConfig.integerZoom, 0, 1 },
    { "scale_mode", &gPortConfig.scaleMode, 0, PORT_SCALE_COUNT - 1 },
    { "filter", &gPortConfig.filter, 0, PORT_FILTER_COUNT - 1 },
    { "fullscreen", &gPortConfig.fullscreen, 0, 1 },
    { "vsync", &gPortConfig.vsync, 0, 1 },
    { "hud_size", &gPortConfig.hudSize, 0, 2 },
    { "narrow_fit", &gPortConfig.narrowFit, 0, 1 },
    { "scanlines", &gPortConfig.scanlines, 0, 100 },
    { "color_correction", &gPortConfig.colorCorrection, 0, 1 },
    { "show_fps", &gPortConfig.showFps, 0, 1 },
    { "experimental_frame_skip", &gPortConfig.frameSkip, 0, 1 },
    { "window_scale", &gPortConfig.windowScale, 1, 8 },
    { "fullscreen_width", &gPortConfig.fullscreenWidth, 0, 16384 },
    { "fullscreen_height", &gPortConfig.fullscreenHeight, 0, 16384 },
    { "master_volume", &gPortConfig.masterVolume, 0, 100 },
    { "music_volume", &gPortConfig.musicVolume, 0, 100 },
    { "sfx_volume", &gPortConfig.sfxVolume, 0, 100 },
    { "dodge_button", &gPortConfig.dodgeButton, 0, 1 },
    { "stock_button", &gPortConfig.stockButton, 0, 1 },
    { "pad_layout", &gPortConfig.padLayout, 0, 2 },
    { "discord_presence", &gPortConfig.discordPresence, 0, 1 },
    { "net_stats", &gPortConfig.netStats, 0, 1 },
    { "net_delay", &gPortConfig.netDelay, 0, 8 },
    { "hd_movies", &gPortConfig.hdMovies, 0, 1 },
    { "hd_music", &gPortConfig.hdMusic, 0, 1 },
    { "hd_scenes", &gPortConfig.hdScenes, 0, 1 },
    { "hd_subtitles", &gPortConfig.hdSubtitles, 0, 1 },
    { "hd_cards", &gPortConfig.hdCards, 0, 1 },
    { "hd_title", &gPortConfig.hdTitle, 0, 1 },
    { "graphics_mode", &gPortConfig.graphicsMode, 0, PORT_MODE_COUNT - 1 },
    { "graphics_mode_applied", &gPortConfig.graphicsModeApplied, 0, PORT_MODE_COUNT - 1 },
    { "gba_border", &gPortConfig.gbaBorder, 0, 3 },
    { "invert_card_scroll", &gPortConfig.invertCardScroll, 0, 1 },
    { "game_language", &gPortConfig.gameLanguage, 0, 5 },
    { "experimental_moogle_notice", &gPortConfig.moogleNotice, 0, 1 },
    { "debug_gba_map_logic", &gPortConfig.gbaMapLogic, 0, 1 },
    { "experimental_unstock", &gPortConfig.unstockButton, 0, 1 },
    { "hd_text", &gPortConfig.hdText, 0, 1 },
    { "card_hud_raise", &gPortConfig.cardHudRaise, 0, 16 },
    { "save_bank", &gPortConfig.saveBank, 1, PORT_SAVE_BANKS },
    { "hd2d", &gPortConfig.hd2d, 0, 1 },
    { "smooth_audio", &gPortConfig.smoothAudio, 0, 1 },
    { "ps2_mode", &gPortConfig.ps2Mode, 0, 1 },
    { "ps2_difficulty", &gPortConfig.ps2Difficulty, 0, 2 },
    { "ps2_fast_text", &gPortConfig.ps2FastText, 0, 1 },
    { "hd_hud", &gPortConfig.hdHud, 0, 1 },
    { "aspect", &gPortConfig.aspect, 0, PORT_ASPECT_COUNT - 1 },
#define BIND(name, act)                                                           { "kb_" name, &gPortConfig.keys[act][0], 0, 0x7FFFFFFF },                     { "kb_" name "_2", &gPortConfig.keys[act][1], 0, 0x7FFFFFFF },                { "pad_" name, &gPortConfig.pad[act], -1, PORT_PAD_AXIS_BASE + SDL_GAMEPAD_AXIS_COUNT * 2 - 1 }
    BIND("a", PORT_ACT_A), BIND("b", PORT_ACT_B), BIND("l", PORT_ACT_L), BIND("r", PORT_ACT_R),
    BIND("start", PORT_ACT_START), BIND("select", PORT_ACT_SELECT), BIND("up", PORT_ACT_UP),
    BIND("down", PORT_ACT_DOWN), BIND("left", PORT_ACT_LEFT), BIND("right", PORT_ACT_RIGHT),
    BIND("stock", PORT_ACT_STOCK), BIND("dodge", PORT_ACT_DODGE),
    BIND("unstock", PORT_ACT_UNSTOCK),
#undef BIND
};

#ifdef __EMSCRIPTEN__
/* The browser uses the same bounded settings and persistence as Options.
   SRAM banks are selected before starting, never switched beneath a game. */
EMSCRIPTEN_KEEPALIVE int PortWebOption(const char* name, int value, int write) {
    size_t i;
    for (i = 0; i < sizeof(sKeys) / sizeof(sKeys[0]); i++) {
        if (SDL_strcmp(name, sKeys[i].name)) continue;
        if (write) {
            if (!SDL_strcmp(name, "save_bank") || !SDL_strcmp(name, "config_version")) return -1;
            *sKeys[i].value = SDL_clamp(value, sKeys[i].min, sKeys[i].max);
            PortConfigSave();
        }
        return *sKeys[i].value;
    }
    return -1;
}
#endif

int PortSaveSlotSelectEnabled(void) {
    return gPortConfig.savePointSlotSelect != 0;
}

/* This is a title permission, not a fabricated story-completion flag. */
int PortRikuAvailable(int soraCleared) {
    return soraCleared != 0 || gPortConfig.cheatUnlockRiku != 0;
}

static char* Trim(char* s) {
    char* e;

    while (SDL_isspace((unsigned char)*s))
        s++;
    e = s + SDL_strlen(s);
    while (e > s && SDL_isspace((unsigned char)e[-1]))
        *--e = 0;
    return s;
}

const char* PortConfigPath(const char* name) {
    static char paths[4][1024];
    static int next;
    const char* base = sDataPath[0] ? sDataPath : SDL_GetBasePath();
    char* out = paths[next++ & 3];

    SDL_snprintf(out, 1024, "%s%s", base ? base : "", name);
    return out;
}

/* The badge atlas also lives in the executable, so the achievement badges show
   wherever the file is missing (development builds, console data folders). */
static const unsigned char sBadgesPng[] = {
#include "port/badges_png.inc"
};

/* A file shipped with the game (e.g. "assets/achievements/badges.png"): the data
   folder, then the executable's folder, then the package itself (Android's APK
   assets and the web build's embedded files, stored without "assets/"), then the
   copy built in for the badges. Free with SDL_free. */
void* PortLoadAsset(const char* name, size_t* size) {
    const char* base = SDL_GetBasePath();
    char path[1024];
    void* data = SDL_LoadFile(PortConfigPath(name), size);

    if (!data && base) {
        SDL_snprintf(path, sizeof(path), "%s%s", base, name);
        data = SDL_LoadFile(path, size);
    }
    if (!data && SDL_strncmp(name, "assets/", 7) == 0)
        data = SDL_LoadFile(name + 7, size);
    if (!data && SDL_strcmp(name, "assets/achievements/badges.png") == 0 &&
        (data = SDL_malloc(sizeof(sBadgesPng))) != NULL) {
        SDL_memcpy(data, sBadgesPng, sizeof(sBadgesPng));
        *size = sizeof(sBadgesPng);
    }
    return data;
}

void PortConfigView(int mode, int* w, int* h) {
    switch (mode) {
        case PORT_VIEW_ORIGINAL:
            *w = 240;
            *h = 160;
            break;
        case PORT_VIEW_WIDE:
            *w = 284;
            *h = 160;
            break;
        case PORT_VIEW_MAXIMUM:
            *w = 384;
            *h = 216;
            break;
        default:
            *w = 320;
            *h = 180;
            break;
    }
}

/* Match the v1.4 numeric labels: xN means N output pixels per GBA pixel.
 * The classic preset uses v1.4's exact fourth-step formula. */
static int sZoomOutW = 1920, sZoomOutH = 1080;
#define ZOOM_PIXEL_BUDGET (640 * 360 * 5 / 4)
int PortConfigZoomId(int level) {
    return PortZoomNormalize(level, 1, sZoomOutW, sZoomOutH);
}
int PortConfigZoomNext(int level, int dir) {
    return PortZoomStep(level, dir, 1, sZoomOutW, sZoomOutH);
}
int PortConfigBattleZoomNext(int level, int dir) {
    int index = level ? PortZoomIndex(level, 1, sZoomOutW, sZoomOutH) + 1 : 0;
    index = SDL_clamp(index + dir, 0, PortZoomCount(1, sZoomOutW, sZoomOutH));
    return index ? PortZoomAt(index - 1, 1, sZoomOutW, sZoomOutH) : 0;
}
float PortConfigZoomScaleF(int level) {
    return PortConfigZoomId(level) * 0.5f;
}
int PortConfigZoomScale(int level) {
    return (int)PortConfigZoomScaleF(level);
}
int PortConfigZoomExact(int level) {
    return !(PortConfigZoomId(level) & 1);
}

int PortConfigZoomHeavy(int level) {
    float f = PortConfigZoomScaleF(level);
    return (sZoomOutW / f) * (sZoomOutH / f) > ZOOM_PIXEL_BUDGET;
}

float gPortAutoScaleF; /* the Auto view's exact pixel scale (may be fractional) */

void PortConfigAutoView(int outW, int outH, int battle, int* w, int* h, int* scale) {
    float f;
    extern int PortUiMenuCanvas(void);

    sZoomOutW = outW;
    sZoomOutH = outH;
    /* the options menu is drawn on the canvas: while it is open the canvas
       keeps the Enhanced size, so its text stays readable at any zoom */
    f = PortUiMenuCanvas() ? SDL_max(outH / 320.0f, SDL_max(outW / (float)PORT_MAX_W, outH / (float)PORT_MAX_H))
                       : PortConfigZoomScaleF(battle && gPortConfig.battleZoom ? gPortConfig.battleZoom : gPortConfig.zoom);
    while (SDL_ceilf(outW / f) > PORT_MAX_W || SDL_ceilf(outH / f) > PORT_MAX_H)
        f += 0.25f;
    *w = SDL_max((int)SDL_ceilf(outW / f), GBA_SCREEN_W);
    *h = SDL_max((int)SDL_ceilf(outH / f), GBA_SCREEN_H);
    *scale = SDL_max((int)f, 1);
    gPortAutoScaleF = f;
}

/* Test harness: KHCOM_CONFIG="key=value;key=value" sets settings over kh_com.ini (the batteries run with the HD
   options on without writing each case's file). */
static void ApplyEnvConfig(void) {
    const char* env = SDL_getenv("KHCOM_CONFIG");
    char buf[512], *item, *next;
    size_t i;
    if (!env)
        return;
    SDL_strlcpy(buf, env, sizeof(buf));
    for (item = buf; item; item = next) {
        char* eq;
        next = SDL_strchr(item, ';');
        if (next)
            *next++ = ' ';
        eq = SDL_strchr(item, '=');
        if (!eq)
            continue;
        *eq = ' ';
        for (i = 0; i < sizeof(sKeys) / sizeof(sKeys[0]); i++)
            if (SDL_strcmp(Trim(item), sKeys[i].name) == 0)
                *sKeys[i].value = SDL_clamp(SDL_atoi(eq + 1), sKeys[i].min, sKeys[i].max);
    }
}

static void PortConfigLoadFile(void);

void PortConfigLoad(void) {
    PortConfigLoadFile();
    ApplyEnvConfig();
}

static void PortConfigLoadFile(void) {
    FILE* f = fopen(PortConfigPath("kh_com.ini"), "r");
    char line[256];

    gPortConfig.discordPresence = 1; /* on unless the file says otherwise */
    gPortConfig.netStats = 1;        /* the online alpha: the network line shows unless turned off */
#ifdef PORT_SWITCH
    gPortConfig.hideTouchControls = 1; /* the console has buttons; Minus + Plus held opens the options (issue #36) */
#endif
    gPortConfig.hdSubtitles = 1;
    gPortConfig.hdMovies = 1;        /* the HD mod is used once imported (HD_MOD.md); the full scenes mod is opt-in */
    gPortConfig.hdMusic = 1;

    /* a package preset (the Steam Deck's fullscreen) ships as
       kh_com.default.ini, so an update extracted over an install never
       replaces the player's own kh_com.ini */
    sUpgradedSettings = f != NULL; /* the player's own file (not the package's preset) */
    if (f == NULL)
        f = fopen(PortConfigPath("kh_com.default.ini"), "r");
    if (f == NULL)
        return;
    while (fgets(line, sizeof(line), f)) {
        char* eq = strchr(line, '=');
        size_t i;

        if (SDL_strncmp(line, "graphics_mode=", 14) == 0)
            sUpgradedSettings = 0;
        if (line[0] == '#' || line[0] == ';' || eq == NULL)
            continue;
        *eq = '\0';
        if (SDL_strcmp(Trim(line), "online_name") == 0) { /* the one text setting */
            SDL_strlcpy(gPortConfig.onlineName, Trim(eq + 1), sizeof(gPortConfig.onlineName));
            continue;
        }
        for (i = 0; i < sizeof(sKeys) / sizeof(sKeys[0]); i++) {
            if (SDL_strcmp(Trim(line), sKeys[i].name) == 0) {
                int v = SDL_atoi(eq + 1);
                *sKeys[i].value = SDL_clamp(v, sKeys[i].min, sKeys[i].max);
            }
        }
    }
    fclose(f);
    /* Settings files from before the auto view switch to it once. */
    if (sConfigVersion < 2)
        gPortConfig.viewMode = PORT_VIEW_AUTO;
    /* ...and from before v1.0 to filling the height in GBA-size scenes,
       whose margins are now drawn on the picture's own pixel grid. */
    if (sConfigVersion < 3)
        gPortConfig.narrowFit = 1;
    /* ...and from before v1.5 (fullscreen 1920x1080 by default) to the
       monitor's own resolution, so ultrawide monitors fill at 32:9. */
    /* ...and from before v1.6.2 back to the Normal zoom (x3 at 1080p): the
       steps were renumbered while testing */
    if (sConfigVersion < 5)
        gPortConfig.zoom = 1;
    /* ...and to filling the window (the aspect limit was left on by tests;
       it is an opt-in for very wide monitors) */
    if (sConfigVersion < 6)
        gPortConfig.aspect = 0;
    /* ...and from the named zoom steps to Enhanced (0) or a scale (xN) */
    if (sConfigVersion < 8) {
        gPortConfig.zoom = 2; /* x2, Enhanced */
        gPortConfig.integerZoom = 1;
    }
    /* Replace the ambiguous v1.7 zoom/pixel-rounding combination once. */
    if (sConfigVersion < 9) {
        gPortConfig.zoom = PORT_ZOOM_DEFAULT;
        gPortConfig.integerZoom = 1;
    }
    /* Only numbered, whole-pixel steps remain; migrate old half steps up. */
    if (gPortConfig.zoom & 1)
        gPortConfig.zoom++;
    if (gPortConfig.battleZoom & 1)
        gPortConfig.battleZoom++;
    if (gPortConfig.zoom == PORT_ZOOM_CLASSIC) gPortConfig.zoom = PORT_ZOOM_DEFAULT;
    gPortConfig.integerZoom = 0;
    if (sConfigVersion < 4 && gPortConfig.fullscreenWidth == 1920 && gPortConfig.fullscreenHeight == 1080) {
        gPortConfig.fullscreenWidth = 0;
        gPortConfig.fullscreenHeight = 0;
    }
    /* HD HUD: pending, off for players until it is finished, whatever an
       older kh_com.ini says. Development builds turn it on with
       KHCOM_HD_HUD_DEV=1 (then kh_com.ini's hd_hud applies). */
    if (!SDL_getenv("KHCOM_HD_HUD_DEV"))
        gPortConfig.hdHud = 0;
    /* the launcher changed the graphics mode: its presets now */
    if (gPortConfig.graphicsMode != PORT_MODE_CHOOSE && gPortConfig.graphicsMode != gPortConfig.graphicsModeApplied)
        PortApplyGraphicsMode(gPortConfig.graphicsMode);
#ifndef PORT_HD2D
    /* HD-2D look: pending as well (too many scenes still wrong); forced off
       outside its own build unless KHCOM_HD2D_DEV=1. */
    if (!SDL_getenv("KHCOM_HD2D_DEV"))
        gPortConfig.hd2d = 0;
#endif
}

/* A graphics mode's presets (the player can change any of them afterwards in Options or the launcher):
   HD: every HD asset of the imported mod and every improvement on; GBA enhanced: the port's improvements (wide
   view, one-button controls) with the HD extras off; GBA classic: everything off, the GBA picture at a whole scale
   with a decorative border, almost an emulator but native. */
void PortApplyGraphicsMode(int mode) {
    extern void PortSetQolButtons(int on);
    int hd = mode == PORT_MODE_HD, classic = mode == PORT_MODE_GBA_CLASSIC;
    if (mode <= PORT_MODE_CHOOSE || mode >= PORT_MODE_COUNT)
        return;
    gPortConfig.graphicsMode = gPortConfig.graphicsModeApplied = mode;
    if (mode == PORT_MODE_GBA_ENHANCED && sUpgradedSettings) { /* an update: the player's settings are that mode */
        sUpgradedSettings = 0;
        return;
    }
    sUpgradedSettings = 0;
    gPortConfig.viewMode = classic ? PORT_VIEW_ORIGINAL : PORT_VIEW_AUTO;
    gPortConfig.hdText = hd;
    gPortConfig.hdCards = hd;
    gPortConfig.hdTitle = hd;
    gPortConfig.hdMusic = hd;
    gPortConfig.hdMovies = !classic; /* HD intros (opening, credits, endings): on unless GBA Original */
    gPortConfig.hdSubtitles = !classic;
    gPortConfig.hdScenes = hd;
    PortSetQolButtons(!classic);
    gPortConfig.gbaBorder = classic ? (gPortConfig.gbaBorder ? gPortConfig.gbaBorder : 1) : 0;
}

void PortConfigSave(void) {
    FILE* f = fopen(PortConfigPath("kh_com.ini"), "w");
    size_t i;

    if (f == NULL)
        return;
    fprintf(f, "# Kingdom Hearts: Chain of Memories (PC) settings\n");
    sConfigVersion = CONFIG_VERSION;
    for (i = 0; i < sizeof(sKeys) / sizeof(sKeys[0]); i++)
        fprintf(f, "%s=%d\n", sKeys[i].name, *sKeys[i].value);
    if (gPortConfig.onlineName[0])
        fprintf(f, "online_name=%s\n", gPortConfig.onlineName);
    fclose(f);
#ifdef __EMSCRIPTEN__
    EM_ASM({ if(Module.persistSoon)Module.persistSoon(); });
#endif
}
