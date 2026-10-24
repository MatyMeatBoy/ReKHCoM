/**
 * port_ps2qol.c
 * PC port: an optional "Re:Chain of Memories" version of the game, with
 * features taken from Kingdom Hearts Re:Chain of Memories (PS2).
 *
 * Starting a new game always asks for the version (original or Re:CoM)
 * and, for Re:CoM, the difficulty; choosing Re:CoM turns on the switch
 * "Re:CoM mode" (ps2_mode). The original version, or any game while the
 * switch is off, plays exactly as the cartridge (preservation). Which version a game is follows its save file: the choice is
 * kept beside the saves in kh_com_recom.ini (bank, file), never inside the
 * cartridge's save data, and comes back when that file is loaded.
 */
#include <SDL3/SDL.h>
#include <stdio.h>
#include "port/port.h"

static int sActive; /* the game being played is the Re:CoM version */

/* The version belongs to the game being played (chosen at New Game,
   remembered per save file), not to a setting. */
int PortRecomActive(void) {
    return sActive;
}

void PortRecomSetActive(int active) {
    sActive = active != 0;
}

/* Re:Chain of Memories difficulty changes only damage: Beginner scales damage
   to Sora by 50% and damage to enemies by 125%; Proud uses 150% and 80%.
   It does not change HP, card rules, or AI. */
static int DamagePercent(int player) {
    switch (gPortConfig.ps2Difficulty) {
    case 1: return player ? 50 : 125;
    case 2: return player ? 150 : 80;
    default: return 100;
    }
}

static int PortScaleDifficulty(int damage, int player);

/* the damage a hit does: the difficulty, then the cheats */
int PortScaleDamage(int damage, int player) {
    extern int PortCheatDamage(int damage, int player);
    return PortCheatDamage(PortScaleDifficulty(damage, player), player);
}

static int PortScaleDifficulty(int damage, int player) {
    int scaled;
    /* the original version, or Standard: the cartridge's own value, untouched */
    if (!PortRecomActive() || gPortConfig.ps2Difficulty == 0 || damage <= 0)
        return damage;
    scaled = damage * DamagePercent(player) / 100;
    if (scaled < 1)
        scaled = 1;
    const char* damageLog;
    PORT_ENV_ONCE(damageLog, SDL_getenv, "KHCOM_DAMAGELOG");
    if (damageLog)
        PortLog("damage: %s %d -> %d (difficulty %d)", player ? "player" : "enemy", damage, scaled,
                gPortConfig.ps2Difficulty);
    return scaled;
}

int PortFastText(void) {
    return PortRecomActive() && gPortConfig.ps2FastText;
}

/* ---- which version each save file is ------------------------------------------ */

#define RECOM_FILE "kh_com_recom.ini"
#define RECOM_KEYS (PORT_SAVE_BANKS * 8)

/* key: bank and file (-1: the quick save) -> 0 when never recorded (a save
   from before: the current settings stay), else bit 0 set, bit 1 Re:CoM,
   bits 2-3 its difficulty, bit 4 HD text, bit 5 the legacy QoL bundle.
   Bit 6 marks individual controls: bits 7-8 dodge/stock, bit 9 retired,
   bit 10 fast dialogue. Old records retain their original interpretation. */
#define RECOM_INDIVIDUAL (1 << 6)
#define RECOM_DODGE (1 << 7)
#define RECOM_STOCK (1 << 8)
#define RECOM_FAST_TEXT (1 << 10)
static int sTable[RECOM_KEYS];
static int sLoaded;

static int Key(int file) {
    int bank = SDL_clamp(gPortConfig.saveBank, 1, PORT_SAVE_BANKS) - 1;
    return bank * 8 + SDL_clamp(file + 1, 0, 7);
}

static void Load(void) {
    FILE* f;
    char line[64];
    int k, v;

    if (sLoaded)
        return;
    sLoaded = 1;
    f = fopen(PortConfigPath(RECOM_FILE), "r");
    if (f == NULL)
        return;
    while (fgets(line, sizeof(line), f))
        if (SDL_sscanf(line, "slot%d=%d", &k, &v) == 2 && k >= 0 && k < RECOM_KEYS)
            sTable[k] = v;
    fclose(f);
}

static void Store(void) {
    FILE* f = fopen(PortConfigPath(RECOM_FILE), "w");
    int k;

    if (f == NULL)
        return;
    fprintf(f, "; ReKHCoM: how each save file's game was started (never inside the save itself)\n");
    fprintf(f, "; slot = (bank - 1) * 8 + file + 1; bits: 1 set, 2 Re:CoM, 4-8 difficulty, 16 HD text, 32 QoL buttons\n");
    fprintf(f, "; 64 individual options recorded, 128 dodge, 256 stock, 512 reserved, 1024 fast dialogue\n");
    for (k = 0; k < RECOM_KEYS; k++)
        if (sTable[k])
            fprintf(f, "slot%d=%d\n", k, sTable[k]);
    fclose(f);
}

int PortQolButtons(void) {
    return gPortConfig.dodgeButton && gPortConfig.stockButton;
}

void PortSetQolButtons(int on) {
    gPortConfig.dodgeButton = gPortConfig.stockButton = on != 0;
}

/* save.c: the game wrote save file `file` (-1: the quick save). */
void PortRecomSaved(int file) {
    int v;
    Load();
    v = 1 | (sActive ? 2 : 0) | (SDL_clamp(gPortConfig.ps2Difficulty, 0, 2) << 2) | (gPortConfig.hdText ? 16 : 0)
        | (PortQolButtons() ? 32 : 0) | RECOM_INDIVIDUAL
        | (gPortConfig.dodgeButton ? RECOM_DODGE : 0)
        | (gPortConfig.stockButton ? RECOM_STOCK : 0)
        | (gPortConfig.ps2FastText ? RECOM_FAST_TEXT : 0);
    if (sTable[Key(file)] != v) {
        sTable[Key(file)] = v;
        Store();
    }
}

/* save.c: the game loaded save file `file`: its version and the choices it
   was started with come back; files never recorded keep today's settings. */
void PortRecomLoaded(int file) {
    int v;
    Load();
    v = sTable[Key(file)];
    PortLog("recom: loaded file %d (bank %d): %s", file, gPortConfig.saveBank,
            !(v & 1) ? "not recorded, settings kept" : (v & 2) ? "Re:CoM" : "original");
    if (!(v & 1)) {
        sActive = 0;
        return;
    }
    sActive = (v & 2) != 0;
    gPortConfig.ps2Difficulty = SDL_clamp((v >> 2) & 3, 0, 2);
    gPortConfig.hdText = (v & 16) != 0;
    if (v & RECOM_INDIVIDUAL) {
        gPortConfig.dodgeButton = (v & RECOM_DODGE) != 0;
        gPortConfig.stockButton = (v & RECOM_STOCK) != 0;
        /* Legacy bit 512 must not enable the separate experimental option. */
        gPortConfig.ps2FastText = (v & RECOM_FAST_TEXT) != 0;
    } else {
        PortSetQolButtons((v & 32) != 0);
    }
    PortVideoApplyConfig();
}
