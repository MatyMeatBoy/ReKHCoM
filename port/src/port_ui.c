/*
 * In-game options overlay (graphics, display and sound settings).
 *
 * Drawn into the frame at the internal resolution with the game's own Latin
 * message font, so it matches the pixel art. Opened from the title screen
 * (SELECT), with F1 / Esc on the keyboard, or Start + Select (keyboard or
 * gamepad, as bound) / Guide anywhere. The game is paused while it is open.
 */
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

#include "port/port.h"
#include "port/port_aspect.h"
#include "port/port_ui.h"
#include "port/port_achievement_list.h"
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) || defined(PORT_ANDROID64) || defined(PORT_TOUCH)
#include "port/port_android.h"
#endif
#include "port/pad_layout.h"

/* A pad button in the menus: whatever is bound to the game's A confirms and to B goes back
   (as the in-game controls), so the menu follows the player's mapping; the face buttons
   bound to something else do nothing there. */
static int UiPadButton(const SDL_Event* ev) {
    int b = ev->gbutton.button;
    if (b == gPortConfig.pad[PORT_ACT_A]) return SDL_GAMEPAD_BUTTON_SOUTH;
    if (b == gPortConfig.pad[PORT_ACT_B]) return SDL_GAMEPAD_BUTTON_EAST;
    if (b == SDL_GAMEPAD_BUTTON_SOUTH || b == SDL_GAMEPAD_BUTTON_EAST) return -1;
    return b;
}

extern void* gMsgLatinFontFrames[];
extern const uint8_t gMsgLatinFontTiles[];
extern int PortDebugModeAvailable(void);
extern int PortCheatActionsAvailable(void);
extern int PortCheatMaxLevel(void);
extern int PortCheatMapCards(void);
extern int PortCheatJournal(void);
extern unsigned PortGameFlags(void);
extern int PortRematchAvailable(int i);
extern int PortRematchStart(int i);
extern void PortDebugModeEnter(void);
#ifdef VERSION_EU
extern uint32_t gLanguage;
#endif
const uint16_t* PortLatinGlyphWidths(void);
void PortInputSuppressUntilRelease(void);

enum { LANG_EN, LANG_FR, LANG_DE, LANG_IT, LANG_ES, LANG_JA, LANG_COUNT };

enum {
    /* the game's settings first, the picture's after: on a touch screen the first rows were the resolution
       and going down changed it (24/10) */
    ITEM_DODGE,
    ITEM_STOCK,
    ITEM_MUSIC,
    ITEM_SFX,
    ITEM_MASTER,
    ITEM_SAVEBANK,
    ITEM_UNDUB,
    ITEM_DISCORD,
    ITEM_FPS,
    ITEM_VIEW,
    ITEM_ASPECT,
    ITEM_ZOOM,
    ITEM_INTZOOM,
    ITEM_SCALE,
    ITEM_HUD,
    ITEM_NARROW,
    ITEM_FILTER,
    ITEM_FULLSCREEN,
    ITEM_FS_RES,
    ITEM_VSYNC,
    ITEM_SCANLINES,
    ITEM_LCD,
    ITEM_CARDS,
    ITEM_EXPERIMENTAL, /* section heading, not selectable */
    ITEM_BATTLE_ZOOM,
    ITEM_HDTEXT,
    ITEM_HDHUD,
    ITEM_HD2D,
    ITEM_SMOOTHAUDIO,
    ITEM_SAVE_SLOT_EXPERIMENTAL,
    ITEM_MOOGLE_NOTICE,
    ITEM_INVERT_CARD_SCROLL,
    ITEM_HIDE_TOUCH,
    ITEM_TOUCH_THEME,  /* the touch buttons' look: Clear, Dark, Color */
    ITEM_TOUCH_EDIT,   /* the layout editor: drag the touch buttons where the player wants them */
    ITEM_FRAMESKIP,
    ITEM_RECOM_EXTRAS, /* section heading, not selectable */
    ITEM_HD_MOVIES,    /* the HD mod (HD_MOD.md): shown only once imported */
    ITEM_HD_MUSIC,
    ITEM_HD_SCENES,
    ITEM_HD_SUBTITLES,
    ITEM_HD_CARDS,     /* HD cards, faces and battle words (port_hdsprites.c), once imported */
    ITEM_HD_TITLE,     /* the title's HD logo */
    ITEM_MODE_RESET,   /* a graphics mode's defaults (PortApplyGraphicsMode), confirmed twice */
    ITEM_GBA_BORDER,   /* GBA classic: the decorative border around the GBA picture (Original view) */
    ITEM_UNSTOCK,
    ITEM_PS2_MODE,
    ITEM_PS2_MENU,     /* opens the Re:CoM page */
    ITEM_CONTROLS_MENU,
    ITEM_CHEATS_MENU,
    ITEM_ACHIEVEMENTS_MENU,
    ITEM_CLOSE,
    ITEM_TITLE,        /* back to the title screen, as the game's own Continue screen does: last, asked twice */
    /* the Re:Chain of Memories page */
    ITEM_PS2_DIFFICULTY,
    ITEM_PS2_FASTTEXT,
    ITEM_PS2_HDSCENES, /* the HD mod's scenes in place of the events (only once imported) */
    ITEM_PS2_BACK,
    /* the new game page: how to start it */
    ITEM_NG_VERSION,
    ITEM_NG_DIFFICULTY,
    ITEM_NG_HDTEXT,
    ITEM_NG_QOL,
    ITEM_NG_START,
    ITEM_CHEAT_DEBUG,
    ITEM_CHEAT_RIKU,
    ITEM_CHEAT_ONEHIT,
    ITEM_CHEAT_INFHP,
    ITEM_CHEAT_STATES,
    ITEM_REMATCH_MENU, /* boss rematches: they keep the field in a state, as the quick states */
    ITEM_CHEAT_MAXLEVEL,
    ITEM_CHEAT_MAPCARDS,
    ITEM_CHEAT_JOURNAL,
    ITEM_CHEAT_DEBUG_HEADING, /* section heading, not selectable */
    ITEM_CHEAT_GBA_MAP,
    ITEM_CHEATS_BACK,
    ITEM_REMATCH_0, /* PortRematchCount() rows: the bosses, then the two tutorials */
    ITEM_REMATCH_LAST = ITEM_REMATCH_0 + 13,
    ITEM_REMATCH_BACK,
    ITEM_CONTROL_A,
    ITEM_CONTROL_B,
    ITEM_CONTROL_L,
    ITEM_CONTROL_R,
    ITEM_CONTROL_START,
    ITEM_CONTROL_SELECT,
    ITEM_CONTROL_UP,
    ITEM_CONTROL_DOWN,
    ITEM_CONTROL_LEFT,
    ITEM_CONTROL_RIGHT,
    ITEM_CONTROL_STOCK,
    ITEM_CONTROL_DODGE,
    ITEM_CONTROL_UNSTOCK,
    ITEM_CONTROL_RESET,
    ITEM_CONTROL_NINTENDO_HEADING, /* section heading, not selectable */
    ITEM_PAD_LAYOUT,               /* force the Nintendo layout */
    ITEM_CONTROLS_BACK,
    ITEM_COUNT
};

/* Two pages, each a run of items: the options and the Re:CoM settings. */
enum { PAGE_MAIN, PAGE_PS2, PAGE_NEWGAME, PAGE_CHEATS, PAGE_CONTROLS, PAGE_REMATCH, PAGE_ACHIEVEMENTS, PAGE_LOBBY };
static int sPage;
static int sTitleRequest; /* ITEM_TITLE: due once the menu has closed */
/* a big window over Options that asks twice, as the scene pause asks before a skip: back to the title (ITEM_TITLE),
   or which mode's presets to go back to (ITEM_MODE_RESET) */
enum { CONFIRM_NONE, CONFIRM_TITLE, CONFIRM_MODE, CONFIRM_STATES };
static int sConfirm, sConfirmStep, sConfirmCursor, sConfirmMode;
static void ConfirmOpen(int what);
static void ConfirmKey(int key);
static const char* const sModeNames[PORT_MODE_COUNT][LANG_COUNT] = {
    { "Choose", "Choisir", "Wählen", "Scegli", "Escoger", "選ぶ" },
    { "HD mode", "Mode HD", "HD-Modus", "Modalità HD", "Modo HD", "HDモード" },
    { "GBA enhanced", "GBA amélioré", "GBA verbessert", "GBA migliorato", "GBA mejorado", "GBA強化" },
    { "GBA classic", "GBA classique", "GBA klassisch", "GBA classico", "GBA clásico", "GBAクラシック" },
};
static void AchievementMenuOpen(void);
static int AchievementMenuInput(const SDL_Event* ev);
static int sControlPad, sControlCapture;
static void ControlBindingName(int binding, int l, char* out, size_t size);
static int sNgRecom; /* new game page: the Re:CoM version chosen */
/* WIZ_MODE and WIZ_MODE_GBA: the graphics mode on the first start; WIZ_ASSETS: something new imported since the last
   start, offered once */
enum { WIZ_VERSION, WIZ_DIFFICULTY, WIZ_OPTIONS, WIZ_MODE, WIZ_MODE_GBA, WIZ_ASSETS };
static int sWizStep, sWizCursor; /* the new game window's step and row */
static void WizKey(int key);
static int PageFirst(void) {
    return sPage == PAGE_REMATCH ? ITEM_REMATCH_0 : sPage == PAGE_CHEATS ? ITEM_CHEAT_DEBUG : sPage == PAGE_CONTROLS ? ITEM_CONTROL_A : sPage == PAGE_PS2 ? ITEM_PS2_DIFFICULTY : sPage == PAGE_NEWGAME ? ITEM_NG_VERSION : 0;
}
static int PageEnd(void) {
    return sPage == PAGE_REMATCH ? ITEM_REMATCH_BACK + 1 : sPage == PAGE_CHEATS ? ITEM_CHEATS_BACK + 1 : sPage == PAGE_CONTROLS ? ITEM_CONTROLS_BACK + 1 : sPage == PAGE_PS2 ? ITEM_PS2_BACK + 1 : sPage == PAGE_NEWGAME ? ITEM_NG_START + 1 : ITEM_TITLE + 1;
}

/* rows shown only where they apply */
static int ItemVisible(int item) {
    switch (item) {
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__) && !defined(PORT_ANDROID64) && !defined(PORT_TOUCH)
        case ITEM_HIDE_TOUCH:
        case ITEM_TOUCH_THEME:
        case ITEM_TOUCH_EDIT: return 0;
#endif
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) || defined(PORT_ANDROID64)
        case ITEM_REMATCH_MENU: return 0; /* rematches need states, not on these yet */
#endif
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) /* the console images hide them through build_ps5.PORT_PATCHES */
        case ITEM_FULLSCREEN:
        case ITEM_FS_RES: return 0;
#endif
        case ITEM_INTZOOM:
        case ITEM_SCALE: return 0;
        case ITEM_DISCORD: return PortDiscordAvailable();
        /* the HD mod's rows appear once the launcher imported its files */
        case ITEM_HD_MOVIES: return PortAssetPresent(PORT_ASSET_HD_MOVIES);
        case ITEM_HD_MUSIC: return PortAssetPresent(PORT_ASSET_HD_MUSIC);
        case ITEM_HD_SCENES:
        case ITEM_PS2_HDSCENES: return PortAssetPresent(PORT_ASSET_HD_SCENES);
        case ITEM_HD_SUBTITLES: return PortAssetPresent(PORT_ASSET_HD_SUBS);
        case ITEM_HD_CARDS: return PortAssetPresent(PORT_ASSET_HD_CARDS);
        case ITEM_HD_TITLE: return 0; /* part of HD HUD (ITEM_HD_CARDS) since 23/10 */
        case ITEM_GBA_BORDER: return gPortConfig.viewMode == PORT_VIEW_ORIGINAL;
        /* the version and its difficulty are chosen at New Game and stay
           with the save, as in the original: no switch for them here */
        case ITEM_PS2_MODE:
        case ITEM_PS2_MENU: return 0;
        /* settings of our own, not the player's to tune */
        case ITEM_FILTER:
        case ITEM_CARDS: return 0;
        case ITEM_NG_VERSION: return 1;
        case ITEM_NG_DIFFICULTY: return sNgRecom;
        case ITEM_HDHUD: return 0; /* HD HUD pending: hidden and forced off (KHCOM_HD_HUD_DEV=1 for development) */
#ifndef PORT_HD2D
        case ITEM_HD2D: return 0; /* HD-2D pending: hidden and forced off (KHCOM_HD2D_DEV=1 for development) */
#endif
        default: return 1;
    }
}

/* how many rows the items [from, to) take: hidden ones take none */
static int VisibleBetween(int from, int to) {
    int n = 0;
    for (; from < to; from++)
        n += ItemVisible(from) != 0;
    return n;
}

/* Strings: UTF-8, one row per item, columns in LANG_* order. */
static const char* const sMainZoom[LANG_COUNT] = {
    "Same as main zoom", "Comme le zoom principal", "Wie Hauptzoom",
    "Come zoom principale", "Igual al zoom principal", "通常ズームと同じ"
};
static const char* const sLabels[ITEM_COUNT][LANG_COUNT] = {
    { "One-button dodge roll", "Roulade en un bouton", "Ausweichrolle mit einer Taste", "Capriola con un tasto",
      "Voltereta con un botón", "ワンボタンドッジ" },
    { "One-button stock (L+R)", "Réserve en un bouton (L+R)", "Vorrat mit einer Taste (L+R)",
      "Accumulo con un tasto (L+R)", "Acumular con un botón (L+R)", "ワンボタンストック (L+R)" },
    { "Music volume", "Volume musique", "Musiklautstärke", "Volume musica", "Volumen música", "音楽の音量" },
    { "Effects volume", "Volume effets", "Effektlautstärke", "Volume effetti", "Volumen efectos", "効果音の音量" },
    { "Master volume", "Volume général", "Gesamtlautstärke", "Volume generale", "Volumen general", "全体の音量" },
    { "Save bank", "Banque de sauvegarde", "Speicherbank", "Banco di salvataggio", "Banco de partidas",
      "セーブバンク" },
    { "Japanese voices (restart)", "Voix japonaises (relancer)", "Japanische Stimmen (Neustart)",
      "Voci giapponesi (riavvia)", "Voces japonesas (reiniciar)", "日本語ボイス (再起動)" },
    { "Discord status", "Statut Discord", "Discord-Status", "Stato Discord", "Estado en Discord", "Discordステータス" },
    { "Show FPS", "Afficher FPS", "FPS anzeigen", "Mostra FPS", "Mostrar FPS", "FPS表示" },
    { "View", "Vue", "Ansicht", "Visuale", "Vista", "表示" },
    { "Aspect ratio", "Format d'image", "Seitenverhältnis", "Proporzioni", "Relación de aspecto", "画面の形" },
    { "Zoom", "Zoom", "Zoom", "Zoom", "Zoom", "ズーム" },
    { "Integer scales only", "Échelles entières seules", "Nur ganze Pixelskalen", "Solo scale intere", "Solo escalas enteras", "せいすうスケールのみ" },
    { "Scaling", "Mise à l'échelle", "Skalierung", "Scala", "Escalado", "スケール" },
    { "HUD size", "Taille du HUD", "HUD-Größe", "Dimensione HUD", "Tamaño del HUD", "HUDサイズ" },
    { "GBA-size scenes", "Scènes format GBA", "Szenen im GBA-Format", "Scene formato GBA", "Escenas tamaño GBA", "GBAサイズの場面" },
    { "Filter", "Filtre", "Filter", "Filtro", "Filtro", "フィルター" },
    { "Fullscreen", "Plein écran", "Vollbild", "Schermo intero", "Pantalla completa", "フルスクリーン" },
    { "Fullscreen resolution", "Résolution plein écran", "Vollbild-Auflösung", "Risoluzione a schermo intero",
      "Resolución completa", "フルスクリーン解像度" },
    { "VSync", "VSync", "VSync", "VSync", "VSync", "VSync" },
    { "Scanlines", "Lignes de balayage", "Scanlines", "Linee di scansione", "Scanlines", "スキャンライン" },
    { "GBA LCD colours", "Couleurs LCD GBA", "GBA-LCD-Farben", "Colori LCD GBA", "Colores LCD GBA", "GBAカラー" },
    { "Battle cards raised", "Cartes de combat relevées", "Kampfkarten angehoben", "Carte di battaglia rialzate",
      "Subir cartas en combate", "カードを上げる" },
    { "Experimental", "Expérimental", "Experimentell", "Sperimentale", "Experimentales", "実験的機能" },
    { "Battle zoom", "Zoom en combat", "Kampfzoom", "Zoom battaglia", "Zoom en combate", "バトルズーム" },
    { "HD text", "Texte HD", "HD-Text", "Testo HD", "Texto HD", "HDテキスト" },
    { "HD HUD (pilot)", "HUD HD (pilote)", "HD-HUD (Pilot)", "HUD HD (pilota)", "HUD HD (piloto)", "HD HUD (試験)" },
    { "HD-2D look", "Style HD-2D", "HD-2D-Optik", "Stile HD-2D", "Estilo HD-2D", "HD-2D表示" },
    { "Audio filter", "Filtre audio", "Audiofilter", "Filtro audio", "Filtro de audio", "オーディオフィルター" },
    { "Choose save slot at save points", "Choisir un emplacement aux points de sauvegarde",
      "Speicherplatz am Speicherpunkt wählen", "Scegli uno slot ai punti di salvataggio",
      "Elegir ranura en puntos de guardado", "セーブポイントでスロットを選択" },
    { "Moogle points notice", "Avis des points mog", "Mogry-Punkte anzeigen", "Avviso punti moguri",
      "Aviso de puntos moguri", "モーグリポイント通知" },
    { "Invert card scrolling", "Inverser le défilement", "Kartenauswahl umkehren", "Inverti scorrimento carte",
      "Invertir selección de cartas", "カード送りを反転" },
    { "Hide touch controls", "Masquer les commandes tactiles", "Touch-Steuerung ausblenden",
      "Nascondi controlli touch", "Ocultar controles táctiles", "タッチボタンを非表示" },
    { "Touch button theme", "Thème des boutons tactiles", "Design der Touch-Tasten", "Tema dei tasti touch",
      "Tema de botones", "タッチボタンのテーマ" },
    { "Move touch buttons", "Déplacer les boutons tactiles", "Touch-Tasten verschieben", "Sposta i tasti touch",
      "Mover botones táctiles", "タッチボタンを移動" },
    { "Skip frames when slow", "Sauter des images si lent", "Bilder überspringen wenn langsam",
      "Salta fotogrammi se lento", "Saltar cuadros si va lento", "遅い時はフレームを飛ばす" },
    { "Re:CoM improvements", "Améliorations Re:CoM", "Re:CoM-Verbesserungen", "Miglioramenti Re:CoM", "Mejoras de Re:CoM", "Re:CoMの改善" },
    { "HD intros", "Intros HD", "HD-Intros", "Intro HD", "Intros HD", "HDイントロ" }, /* opening, credits, endings */
    { "HD music", "Musique HD", "HD-Musik", "Musica HD", "Música HD", "HDミュージック" },
    { "HD scenes", "Scènes HD", "HD-Szenen", "Scene HD", "Escenas HD", "HDシーン" },
    { "HD subtitles", "Sous-titres HD", "HD-Untertitel", "Sottotitoli HD", "Subtítulos HD", "HDじまく" },
    { "HD HUD", "ATH HD", "HD-HUD", "HUD HD", "HUD HD", "HD HUD" }, /* cards, battle HUD and title logo together */
    { "HD title logo", "Logo de titre HD", "HD-Titellogo", "Logo del titolo HD", "Logo del título HD", "HDタイトルロゴ" },
    { "Reset to a mode's defaults", "Réglages d'un mode", "Standard eines Modus", "Predefiniti di una modalità",
      "Restablecer según el modo", "モードの初期値に戻す" },
    { "Border (Original view)", "Bordure (vue originale)", "Rahmen (Originalansicht)", "Cornice (vista originale)",
      "Borde (vista Original)", "わく（オリジナル表示）" },
    { "Return stocked cards", "Rendre les cartes en réserve", "Vorratskarten zurücknehmen",
      "Riprendi carte accumulate", "Devolver cartas acumuladas", "ストックをてふだに戻す" },
    { "Re:CoM mode", "Mode Re:CoM", "Re:CoM-Modus", "Modalità Re:CoM", "Modo Re:CoM", "Re:CoMモード" },
    { "Re:CoM settings", "Réglages Re:CoM", "Re:CoM-Einstellungen", "Impostazioni Re:CoM", "Ajustes Re:CoM",
      "Re:CoMオプション" },
    { "Controls", "Commandes", "Steuerung", "Controlli", "Controles", "コントロール" },
    { "Cheats", "Codes", "Cheats", "Trucchi", "Trucos", "チート" },
    { "Achievements", "Succès", "Erfolge", "Obiettivi", "Logros", "じっせき" },
    { "Back", "Retour", "Zurück", "Indietro", "Volver", "戻る" },
    { "Return to title screen", "Retour à l'écran titre", "Zurück zum Titelbildschirm",
      "Torna alla schermata del titolo", "Volver a la pantalla de título", "タイトル画面に戻る" },
    { "Difficulty", "Difficulté", "Schwierigkeit", "Difficoltà", "Dificultad", "難しさ" },
    { "Fast dialogue", "Dialogues rapides", "Schnelle Dialoge", "Dialoghi veloci", "Diálogos rápidos",
      "会話を早く" },
    { "HD scenes in place of the events", "Scènes HD à la place des évènements", "HD-Szenen statt der Ereignisse",
      "Scene HD al posto degli eventi", "Reemplazar escenas por las HD", "イベントをHDシーンに置き換え" },
    { "Back", "Retour", "Zurück", "Indietro", "Volver", "戻る" },
    { "Version", "Version", "Version", "Versione", "Versión", "バージョン" },
    { "Difficulty", "Difficulté", "Schwierigkeit", "Difficoltà", "Dificultad", "難しさ" },
    { "HD text", "Texte HD", "HD-Text", "Testo HD", "Texto HD", "HDテキスト" },
    { "One-button dodge and stock", "Roulade et réserve en un bouton", "Ausweichen und Vorrat mit einer Taste",
      "Capriola e accumulo con un tasto", "Rodar y acumular con un botón", "ワンボタンそうさ" },
    { "Start", "Commencer", "Beginnen", "Inizia", "Comenzar", "はじめる" },
    { "Enter debug mode", "Entrer en mode debug", "Debug-Modus starten", "Entra in modalità debug", "Entrar al modo debug", "デバッグモード" },
    { "Unlock Riku", "Débloquer Riku", "Riku freischalten", "Sblocca Riku", "Desbloquear a Riku", "リクを解放" },
    { "Enemies fall in one hit", "Ennemis vaincus d'un coup", "Gegner fallen mit einem Treffer",
      "Nemici sconfitti con un colpo", "Enemigos caen de un golpe", "敵を一撃で倒す" },
    { "Infinite HP", "PV infinis", "Unendliche KP", "PS infiniti", "Vida infinita", "HP無限" },
    { "Quick states (F5 save, F8 load)", "États rapides (F5 sauver, F8 charger)", "Schnellstände (F5 sichern, F8 laden)",
      "Stati rapidi (F5 salva, F8 carica)", "Estados rápidos (F5 guarda, F8 carga)", "クイックステート (F5セーブ/F8ロード)" },
    { "Boss rematches", "Revanches de boss", "Boss-Revanchen", "Rivincite coi boss", "Revancha de jefes", "ボス再戦" },
    { "Level 99 with every bonus", "Niveau 99 avec tous les bonus", "Stufe 99 mit allen Boni",
      "Livello 99 con tutti i bonus", "Nivel 99 con todos los bonos", "全ボーナスつきレベル99" },
    { "Every map card (9 each)", "Toutes les cartes de salle (9)", "Alle Raumkarten (je 9)",
      "Tutte le carte stanza (9)", "Todas las cartas de mapa (9 c/u)", "全マップカード (9枚ずつ)" },
    { "Complete the journal", "Compléter le journal", "Tagebuch vervollständigen",
      "Completa il diario", "Completar el diario", "日記をすべてうめる" },
    { "Debug mode", "Mode debug", "Debug-Modus", "Modalità debug", "Modo debug", "デバッグモード" },
    { "[ReKHCoM] GBA map logic", "[ReKHCoM] Logique carte GBA", "[ReKHCoM] GBA-Kartenlogik",
      "[ReKHCoM] Logica mappa GBA", "[ReKHCoM] Lógica Mapa GBA", "[ReKHCoM] GBAマップロジック" },
    { "Back", "Retour", "Zurück", "Indietro", "Volver", "戻る" },
    { "Guard Armor", "Guard Armor", "Guard Armor", "Guard Armor", "Guard Armor", "Guard Armor" },
    { "Trickmaster", "Trickmaster", "Trickmaster", "Trickmaster", "Trickmaster", "Trickmaster" },
    { "Jafar", "Jafar", "Jafar", "Jafar", "Jafar", "Jafar" },
    { "Hades", "Hades", "Hades", "Hades", "Hades", "Hades" },
    { "Ursula", "Ursula", "Ursula", "Ursula", "Ursula", "Ursula" },
    { "Parasite Cage", "Parasite Cage", "Parasite Cage", "Parasite Cage", "Parasite Cage", "Parasite Cage" },
    { "Oogie Boogie", "Oogie Boogie", "Oogie Boogie", "Oogie Boogie", "Oogie Boogie", "Oogie Boogie" },
    { "Hook", "Hook", "Hook", "Hook", "Hook", "Hook" },
    { "Darkside", "Darkside", "Darkside", "Darkside", "Darkside", "Darkside" },
    { "Dragon Maleficent", "Dragon Maleficent", "Dragon Maleficent", "Dragon Maleficent", "Dragon Maleficent", "Dragon Maleficent" },
    { "Riku", "Riku", "Riku", "Riku", "Riku", "Riku" },
    { "Vexen", "Vexen", "Vexen", "Vexen", "Vexen", "Vexen" },
    { "Battle tutorial 1", "Tutoriel de combat 1", "Kampf-Tutorial 1", "Tutorial di battaglia 1", "Tutorial de combate 1", "バトルチュートリアル1" },
    { "Battle tutorial 2", "Tutoriel de combat 2", "Kampf-Tutorial 2", "Tutorial di battaglia 2", "Tutorial de combate 2", "バトルチュートリアル2" },
    { "Back", "Retour", "Zurück", "Indietro", "Volver", "戻る" },
    { "A", "A", "A", "A", "A", "A" },
    { "B", "B", "B", "B", "B", "B" },
    { "L", "L", "L", "L", "L", "L" },
    { "R", "R", "R", "R", "R", "R" },
    { "Start", "Start", "Start", "Start", "Start", "Start" },
    { "Select", "Select", "Select", "Select", "Select", "Select" },
    { "Arriba", "Haut", "Oben", "Su", "Arriba", "上" },
    { "Abajo", "Bas", "Unten", "Giù", "Abajo", "下" },
    { "Izquierda", "Gauche", "Links", "Sinistra", "Izquierda", "左" },
    { "Derecha", "Droite", "Rechts", "Destra", "Derecha", "右" },
    { "Acumular", "Stock", "Stock", "Accumula", "Acumular", "ストック" },
    { "Esquivar", "Esquive", "Ausweichen", "Schiva", "Esquivar", "ドッジ" },
    { "Devolver cartas", "Rendre les cartes", "Karten zurückgeben", "Restituisci carte", "Devolver cartas", "カードを戻す" },
    { "Restaurar valores predeterminados", "Restaurer les valeurs par défaut", "Standardwerte wiederherstellen", "Ripristina valori predefiniti", "Restaurar valores predeterminados", "初期値に戻す" },
    { "Nintendo controller", "Manette Nintendo", "Nintendo-Controller", "Controller Nintendo", "Mando Nintendo", "ニンテンドーのコントローラー" },
    { "Force Nintendo layout", "Forcer la disposition Nintendo", "Nintendo-Belegung erzwingen", "Forza disposizione Nintendo",
      "Forzar mando Nintendo", "ニンテンドーの配置にこてい" },
    { "Volver", "Retour", "Zurück", "Indietro", "Volver", "戻る" },
};
static const char* const sCheatsTitle[LANG_COUNT] = { "Cheats", "Codes", "Cheats", "Trucchi", "Trucos", "チート" };
static const char* const sCheatsHelp[LANG_COUNT] = {
    "Debug: title screen only. Level, cards, journal: on the map; then save.",
    "Debug : écran titre. Niveau, cartes, journal : sur la carte.",
    "Debug: nur im Titel. Stufe, Karten, Tagebuch: auf der Karte.",
    "Debug: solo dal titolo. Livello, carte, diario: sulla mappa.",
    "Debug: solo en el título. Nivel, cartas y diario: en el mapa; luego guarda.",
    "デバッグはタイトルで。レベル・カード・日記はマップ上で。セーブすると有効になります。",
};
static const char* const sRematchTitle[LANG_COUNT] = { "Boss rematches", "Revanches de boss", "Boss-Revanchen",
    "Rivincite coi boss", "Revancha de jefes", "ボス再戦" };
static const char* const sRematchHelp[LANG_COUNT] = {
    "Bosses already beaten, from a room. You come back here; nothing from the fight is kept.",
    "Boss déjà vaincus, depuis une salle. Retour ici ; rien du combat n'est gardé.",
    "Besiegte Bosse, aus einem Raum. Danach zurück hierher; nichts vom Kampf bleibt.",
    "Boss già sconfitti, da una stanza. Si torna qui; nulla del combattimento resta.",
    "Jefes ya vencidos, desde una sala. Vuelves aquí; no se conserva nada del combate.",
    "倒したボスと再戦（部屋から）。終了後ここへ戻り、戦闘の結果は残りません。",
};
/* why a rematch cannot be fought now (PortRematchWhyNot, main.c): 1 playing as Riku, 2 not walking a room,
   3 that boss not beaten yet in the story */
static const char* const sRematchWhy[3][LANG_COUNT] = {
    { "Only in Sora's story.", "Seulement dans l'histoire de Sora.", "Nur in Soras Geschichte.",
      "Solo nella storia di Sora.", "Solo en la historia de Sora.", "ソラの物語でのみ。" },
    { "Available while walking a room (not in a battle, menu or scene).",
      "Disponible en marchant dans une salle (pas en combat, menu ou scène).",
      "Verfügbar beim Gehen in einem Raum (nicht im Kampf, Menü oder in Szenen).",
      "Disponibile camminando in una stanza (non in battaglia, menu o scena).",
      "Disponible caminando en una sala (no en combate, menú ni escena).",
      "へやを歩いているときに使えます（戦闘・メニュー・イベント中は不可）。" },
    { "Beat this boss in the story first.", "Bats d'abord ce boss dans l'histoire.",
      "Besiege diesen Boss zuerst in der Geschichte.", "Sconfiggi prima questo boss nella storia.",
      "Vence primero a este jefe en la historia.", "まず物語でこのボスを倒してください。" },
};
static int Lang(void);
static const char* RematchWhy(int i) {
    extern int PortRematchWhyNot(int i);
    int why = PortRematchWhyNot(i);
    return sRematchWhy[SDL_clamp(why, 1, 3) - 1][Lang()];
}
static const char* const sCheatDone[LANG_COUNT] = {
    "Cheat applied", "Code appliqué", "Cheat angewendet", "Trucco applicato", "Truco aplicado", "チートを使う",
};
static const char* const sNgTitle[LANG_COUNT] = { "New game", "Nouvelle partie", "Neues Spiel", "Nuova partita",
    "Nueva partida", "ニューゲーム" };
static const char* const sVersions[2][LANG_COUNT] = {
    { "Original", "Originale", "Original", "Originale", "Original", "オリジナル" },
    { "Re:Chain of Memories", "Re:Chain of Memories", "Re:Chain of Memories", "Re:Chain of Memories",
      "Re:Chain of Memories", "Re:チェイン オブ メモリーズ" },
};
/* New game page: what the selected row means, under the list. */
static const char* const sNgDescOriginal[LANG_COUNT] = {
    "The original Game Boy Advance version, exactly as on the cartridge.",
    "La version Game Boy Advance originale, exactement comme sur la cartouche.",
    "Die originale Game-Boy-Advance-Version, genau wie auf dem Modul.",
    "La versione originale per Game Boy Advance, esattamente come sulla cartuccia.",
    "La versión original de Game Boy Advance, exactamente como en el cartucho.",
    "ゲームボーイアドバンスそのままのオリジナル。",
};
static const char* const sNgDescRecom[LANG_COUNT] = {
    "With improvements from Re:Chain of Memories (PS2): difficulty levels and faster dialogue.",
    "Avec les améliorations de Re:Chain of Memories (PS2) : niveaux de difficulté et dialogues rapides.",
    "Mit Verbesserungen aus Re:Chain of Memories (PS2): Schwierigkeitsgrade und schnellere Dialoge.",
    "Con i miglioramenti di Re:Chain of Memories (PS2): livelli di difficoltà e dialoghi più rapidi.",
    "Con mejoras de Re:Chain of Memories (PS2): niveles de dificultad y diálogos más rápidos.",
    "Re:チェイン オブ メモリーズ (PS2) の改良入り：難しさの選択と会話の高速化。",
};
static const char* const sNgDescDifficulty[3][LANG_COUNT] = {
    { "As the original: the cartridge's damage.", "Comme l'original : les dégâts de la cartouche.",
      "Wie das Original: der Schaden des Moduls.", "Come l'originale: i danni della cartuccia.",
      "Como el original: el daño del cartucho.", "オリジナルと同じダメージ。" },
    { "Sora takes 50% damage; enemies take 125%.", "Sora subit 50 % des dégâts ; les ennemis en subissent 125 %.",
      "Sora erleidet 50 % Schaden; Gegner 125 %.", "Sora subisce il 50% dei danni; i nemici il 125%.",
      "Sora recibe un 50 % de daño; los enemigos, un 125 %.", "ソラの被ダメージは50%、敵は125%。" },
    { "Sora takes 150% damage; enemies take 80%.", "Sora subit 150 % des dégâts ; les ennemis en subissent 80 %.",
      "Sora erleidet 150 % Schaden; Gegner 80 %.", "Sora subisce il 150% dei danni; i nemici l'80%.",
      "Sora recibe un 150 % de daño; los enemigos, un 80 %.", "ソラの被ダメージは150%、敵は80%。" },
};
static const char* const sNgDescHd[LANG_COUNT] = {
    "Dialogue drawn with a crisp font at the window's resolution.",
    "Dialogues tracés avec une police nette à la résolution de la fenêtre.",
    "Dialoge in einer scharfen Schrift in Fensterauflösung.",
    "Dialoghi disegnati con un carattere nitido alla risoluzione della finestra.",
    "Diálogos con una fuente nítida a la resolución de la ventana.",
    "会話をウィンドウ解像度のくっきりしたフォントで表示。",
};
static const char* const sNgDescHdMusic[LANG_COUNT] = {
    "Re:Chain of Memories' soundtrack from your KH HD 1.5+2.5 in place of the cartridge's music.",
    "La bande-son de Re:Chain of Memories de votre KH HD 1.5+2.5 à la place de la musique de la cartouche.",
    "Der Soundtrack von Re:Chain of Memories aus deinem KH HD 1.5+2.5 statt der Musik des Moduls.",
    "La colonna sonora di Re:Chain of Memories del tuo KH HD 1.5+2.5 al posto della musica della cartuccia.",
    "La banda sonora de Re:Chain of Memories de tu KH HD 1.5+2.5 en lugar de la música del cartucho.",
    "カートリッジの音楽の代わりにKH HD 1.5+2.5のRe:チェイン オブ メモリーズのサウンドトラック。",
};
static const char* const sNgDescHdMovies[LANG_COUNT] = {
    "The opening, the credits and the endings as the HD movies of your KH HD 1.5+2.5.",
    "L'ouverture, le générique et les fins avec les vidéos HD de votre KH HD 1.5+2.5.",
    "Intro, Abspann und Enden als HD-Videos aus deinem KH HD 1.5+2.5.",
    "L'introduzione, i titoli di coda e i finali con i video HD del tuo KH HD 1.5+2.5.",
    "La introducción, los créditos y los finales con los videos HD de tu KH HD 1.5+2.5.",
    "オープニング、スタッフロール、エンディングをKH HD 1.5+2.5のHDムービーで。",
};
static const char* const sNgDescHdScenes[LANG_COUNT] = {
    "Story scenes that Re:CoM has as movies play them instead (English voices, subtitles in your language).",
    "Les scènes que Re:CoM a en vidéo les jouent à la place (voix anglaises, sous-titres dans votre langue).",
    "Szenen, die Re:CoM als Video hat, laufen stattdessen so (englische Stimmen, Untertitel in deiner Sprache).",
    "Le scene che Re:CoM ha come video vengono riprodotte così (voci inglesi, sottotitoli nella tua lingua).",
    "Las escenas que Re:CoM tiene en video se ven así (voces en inglés, subtítulos en tu idioma).",
    "Re:CoMでムービーになっている場面はそれで再生（英語音声、じまくはゲームの言語）。",
};
static const char* const sNgDescHdHud[LANG_COUNT] = {
    "The HUD is what is drawn over the game: cards, HP, battle words and the title's logo, in Re:CoM's HD art.",
    "L'ATH est ce qui s'affiche par-dessus le jeu : cartes, PV, mots de combat et logo du titre, en art HD de Re:CoM.",
    "Das HUD ist, was über dem Spiel liegt: Karten, KP, Kampfwörter und das Titellogo, in Re:CoMs HD-Grafik.",
    "L'HUD è ciò che appare sopra il gioco: carte, PV, parole di battaglia e logo del titolo, con la grafica HD di Re:CoM.",
    "El HUD es lo que se dibuja sobre el juego: cartas, vida, palabras de combate y el logo del título, con el arte HD de Re:CoM.",
    "HUDはゲームの上に表示されるもの（カード、HP、戦闘の文字、タイトルロゴ）で、Re:CoMのHDの絵になります。",
};
static const char* const sNgDescQol[LANG_COUNT] = {
    "One button for the dodge roll and for stocking cards (L+R).",
    "Un bouton pour la roulade et pour la réserve (L+R).",
    "Eine Taste für Ausweichen und Vorrat (L+R).",
    "Un tasto per la capriola e per l'accumulo (L+R).",
    "Un botón para rodar y para acumular cartas (L+R).",
    "ドッジロールとカードストック（L+R）を別のボタンで使う。",
};
static const char* const sNgDescStart[LANG_COUNT] = {
    "Everything here can be changed later with START + SELECT.",
    "Tout peut être changé plus tard avec START + SELECT.",
    "Alles lässt sich später mit START + SELECT ändern.",
    "Tutto si può cambiare in seguito con START + SELECT.",
    "Todo se puede cambiar después con START + SELECT.",
    "あとから START + SELECT で変更できます。",
};

static const char* NewGameDescription(int item, int l) {
    switch (item) {
        case ITEM_NG_VERSION: return sNgRecom ? sNgDescRecom[l] : sNgDescOriginal[l];
        case ITEM_NG_DIFFICULTY: return sNgDescDifficulty[SDL_clamp(gPortConfig.ps2Difficulty, 0, 2)][l];
        case ITEM_NG_HDTEXT: return sNgDescHd[l];
        case ITEM_NG_QOL: return sNgDescQol[l];
        default: return sNgDescStart[l];
    }
}

static const char* const sPs2Title[LANG_COUNT] = { "Re:Chain of Memories", "Re:Chain of Memories",
    "Re:Chain of Memories", "Re:Chain of Memories", "Re:Chain of Memories", "Re:チェイン オブ メモリーズ" };

static const char* const sTitle[LANG_COUNT] = { "Options", "Options", "Optionen", "Opzioni", "Opciones",
    "オプション",
};
static const char* const sHelp[LANG_COUNT] = {
    "\xE2\x86\x91\xE2\x86\x93 Select   \xE2\x86\x90\xE2\x86\x92 Change   B Back",
    "\xE2\x86\x91\xE2\x86\x93 Choisir   \xE2\x86\x90\xE2\x86\x92 Modifier   B Retour",
    "\xE2\x86\x91\xE2\x86\x93 Wählen   \xE2\x86\x90\xE2\x86\x92 Ändern   B Zurück",
    "\xE2\x86\x91\xE2\x86\x93 Scegli   \xE2\x86\x90\xE2\x86\x92 Cambia   B Indietro",
    "\xE2\x86\x91\xE2\x86\x93 Elegir   \xE2\x86\x90\xE2\x86\x92 Cambiar   B Volver",
    "\xE2\x86\x91\xE2\x86\x93 選択   \xE2\x86\x90\xE2\x86\x92 変更   B 戻る",
};
static const char* const sHelpNg[LANG_COUNT] = {
    "ââ Select   ââ Change   A Start",
    "ââ Choisir   ââ Modifier   A Commencer",
    "ââ Wählen   ââ Ändern   A Beginnen",
    "ââ Scegli   ââ Cambia   A Inizia",
    "ââ Elegir   ââ Cambiar   A Comenzar",
    "ââ 選択   ââ 変更   A はじめる",
};
static const char* const sTitleHint[LANG_COUNT] = { "SELECT  Options", "SELECT  Options", "SELECT  Optionen",
                                                    "SELECT  Opzioni", "SELECT  Opciones",
    "SELECT  オプション",
};
static const char* const sDifficulty[3][LANG_COUNT] = {
    { "Standard", "Standard", "Standard", "Standard", "Estándar", "スタンダード" },
    { "Beginner", "Débutant", "Anfänger", "Principiante", "Principiante", "ビギナー" },
    { "Proud", "Fier", "Stolz", "Orgoglioso", "Orgulloso", "プラウド" },
};
static const char* const sOnOff[2][LANG_COUNT] = {
    { "Off", "Non", "Aus", "No", "No", "オフ" },
    { "On", "Oui", "An", "Sì", "Sí", "オン" },
};
static const char* const sViews[PORT_VIEW_COUNT][LANG_COUNT] = {
    { "Original 240x160", "Originale 240x160", "Original 240x160", "Originale 240x160", "Original 240x160", "オリジナル 240x160" },
    { "Wide 284x160", "Large 284x160", "Breit 284x160", "Panoramica 284x160", "Panorámica 284x160", "ワイド 284x160" },
    { "Extended 320x180", "Étendue 320x180", "Erweitert 320x180", "Estesa 320x180", "Ampliada 320x180", "ワイド＋ 320x180" },
    { "Maximum 384x216", "Maximale 384x216", "Maximal 384x216", "Massima 384x216", "Máxima 384x216", "いちばん広い 384x216" },
    { "Auto (fill window)", "Auto (remplir)", "Auto (füllen)", "Auto (riempi)", "Auto (llenar ventana)", "自動 (画面に合わせる)" },
};
static const char* const sZoomHeavy[LANG_COUNT] = {
    "Demanding: needs a fast PC for 60 fps", "Exigeant : un PC rapide pour 60 i/s",
    "Anspruchsvoll: schneller PC für 60 fps", "Impegnativo: serve un PC veloce per 60 fps",
    "Exigente: requiere un PC potente para 60 fps", "高負荷：60fpsには高性能なPCが必要",
};
static const char* const sScales[PORT_SCALE_COUNT][LANG_COUNT] = {
    { "Integer", "Entière", "Ganzzahlig", "Intera", "Entero", "ドットばい" },
    { "Fit", "Ajuster", "Einpassen", "Adatta", "Ajustar", "合わせる" },
    { "Stretch", "Étirer", "Strecken", "Allunga", "Estirar", "引きのばし" },
};
static const char* const sFilters[PORT_FILTER_COUNT][LANG_COUNT] = {
    { "Sharp", "Net", "Scharf", "Nitido", "Nítido", "シャープ" },
    { "Pixel art", "Pixel art", "Pixel-Art", "Pixel art", "Pixel art", "ドット絵" },
    { "Smooth", "Lisse", "Weich", "Morbido", "Suave", "なめらか" },
};
static const char* const sHudSizes[3][LANG_COUNT] = {
    { "Original", "Original", "Original", "Originale", "Original", "オリジナル" },
    { "Large", "Grand", "Groß", "Grande", "Grande", "おおきい" },
    { "Larger", "Plus grand", "Größer", "Più grande", "Más grande", "もっとおおきい" },
};
static const char* const sAspect[PORT_ASPECT_COUNT][LANG_COUNT] = {
    { "Fill window", "Remplir la fenêtre", "Fenster füllen", "Riempi finestra", "Llenar ventana", "ウィンドウに合わせる" },
    { "16:9", "16:9", "16:9", "16:9", "16:9", "16:9" },
    { "21:9", "21:9", "21:9", "21:9", "21:9", "21:9" },
    { "Force 21:9", "Forcer 21:9", "21:9 erzwingen", "Forza 21:9", "Forzar 21:9", "21:9にこてい" },
    { "Force 32:9", "Forcer 32:9", "32:9 erzwingen", "Forza 32:9", "Forzar 32:9", "32:9にこてい" },
    { "4:3", "4:3", "4:3", "4:3", "4:3", "4:3" },
    { "GBA 3:2", "GBA 3:2", "GBA 3:2", "GBA 3:2", "GBA 3:2", "GBA 3:2" },
};
static const char* const sPressA[LANG_COUNT] = { "A: apply", "A : appliquer", "A: übernehmen", "A: applica",
    "A: aplicar", "A: 決定" };
static const char* const sIntegralReference[LANG_COUNT] = { "integer", "entière", "ganzzahlig", "intera", "íntegra", "せいすう" };
static const char* const sNarrow[2][LANG_COUNT] = {
    { "Exact pixels", "Pixels exacts", "Exakte Pixel", "Pixel esatti", "Píxel exacto", "1:1ピクセル" },
    { "Fill height", "Remplir la hauteur", "Höhe füllen", "Riempi altezza", "Llenar altura", "高さに合わせる" },
};
static const char* const sDesktop[LANG_COUNT] = { "Desktop", "Bureau", "Desktop", "Desktop", "Escritorio",
    "デスクトップ",
};
static const char* const sAutoOnly[LANG_COUNT] = { "(Auto view)", "(vue Auto)", "(Auto-Ansicht)", "(vista Auto)",
                                                   "(vista Auto)",
    "(自動表示のみ)",
};

static const int sViewOrder[PORT_VIEW_COUNT] = { PORT_VIEW_AUTO, PORT_VIEW_ORIGINAL, PORT_VIEW_WIDE,
                                                 PORT_VIEW_EXTENDED, PORT_VIEW_MAXIMUM };

static int sOpen;
static int sTouchEdit; /* the touch layout editor is open (PortUiTouchEdit) */
/* the game's own menu sounds as the overlay opens and closes (taken by
   port_main once the menu's sound context is set up / the game's is back):
   its field menu opens with SYS_CANSEL and closes with SYS_CLOSE */
static int sOpenSound, sCloseSound;
static int sCursor;
static int sScroll;
static int sTitleHintFrames;
static char sNotice[96];
static int sNoticeFrames;
static int sSkipPrompt;
static int sModeW[64], sModeH[64], sModeCount;

/* ---- drawing ------------------------------------------------------------------ */

static uint32_t* sFb;
static int sFbW, sFbH;

static int Lang(void) {
#ifdef VERSION_EU
    return gLanguage < LANG_COUNT ? (int)gLanguage : LANG_EN;
#elif defined(VERSION_JP)
    return LANG_JA;
#else
    return LANG_EN;
#endif
}

#ifdef VERSION_JP
/* Japanese characters: drawn from the cartridge's Shift-JIS font, 10 pixels apart. */
#define JP_CHAR 0x1000000
#define JP_ADVANCE 10
int PortCodepointToSjis(unsigned cp);
void GetSjisGlyph(uint16_t code, uint16_t* glyph, uint8_t* bank);
extern void* gMsgFontBank0Frames[];
extern void* gMsgFontBank1Frames[];
extern void* gMsgFontBank2Frames[];
extern void* gMsgFontBank3Frames[];
extern const uint8_t gMsgFontBank0Tiles[];
extern const uint8_t gMsgFontBank1Tiles[];
extern const uint8_t gMsgFontBank2Tiles[];
extern const uint8_t gMsgFontBank3Tiles[];
#endif

/* Next character of a UTF-8 string as a code of the game's Latin font. */
static int NextChar(const char** s) {
    const unsigned char* p = (const unsigned char*)*s;
    uint32_t c = *p++;

    if (c >= 0xE0 && p[0] && p[1]) {
        c = ((c & 15) << 12) | ((p[0] & 63) << 6) | (p[1] & 63);
        p += 2;
    } else if (c >= 0xC0 && p[0]) {
        c = ((c & 31) << 6) | (p[0] & 63);
        p += 1;
    }
    *s = (const char*)p;
    switch (c) {
        case 0x2191:
            return 0x0A;
        case 0x2193:
            return 0x0B;
        case 0x2190:
            return 0x0C;
        case 0x2192:
            return 0x0D;
        case 0x25CB:
            return 0x0E;
        case 0xFF1A:
            return ':'; /* the Japanese font has no full-width colon */
    }
#ifdef VERSION_JP
    if (c >= 0x3000)
        return JP_CHAR | (int)c; /* kana, kanji and full-width forms */
#endif
    return c < 256 ? (int)c : '?';
}

static void Pixel(int x, int y, uint32_t c) {
    if (x >= 0 && y >= 0 && x < sFbW && y < sFbH)
        sFb[y * sFbW + x] = c;
}

static uint32_t Mix(uint32_t a, uint32_t b, int t) {
    uint32_t r = (((a >> 16) & 255) * (256 - t) + ((b >> 16) & 255) * t) >> 8;
    uint32_t g = (((a >> 8) & 255) * (256 - t) + ((b >> 8) & 255) * t) >> 8;
    uint32_t bl = ((a & 255) * (256 - t) + (b & 255) * t) >> 8;
    return 0xFF000000u | (r << 16) | (g << 8) | bl;
}

static void Shade(int x0, int y0, int w, int h, uint32_t color, int alpha) {
    int x, y;

    for (y = SDL_max(y0, 0); y < SDL_min(y0 + h, sFbH); y++)
        for (x = SDL_max(x0, 0); x < SDL_min(x0 + w, sFbW); x++)
            sFb[y * sFbW + x] = Mix(sFb[y * sFbW + x], color, alpha);
}

static void Rect(int x0, int y0, int w, int h, uint32_t c) {
    int i;

    for (i = 0; i < w; i++) {
        Pixel(x0 + i, y0, c);
        Pixel(x0 + i, y0 + h - 1, c);
    }
    for (i = 0; i < h; i++) {
        Pixel(x0, y0 + i, c);
        Pixel(x0 + w - 1, y0 + i, c);
    }
}

static int GlyphWidth(int code) {
#ifdef VERSION_JP
    if (code & JP_CHAR)
        return JP_ADVANCE;
#endif
    return PortLatinGlyphWidths()[code & 255];
}

static int TextWidth(const char* s) {
    int w = 0;

    while (*s)
        w += GlyphWidth(NextChar(&s));
    return w;
}

/* The font's glyphs occupy rows 3..12 of their 16x16 cell; y is the top of
   that band. Colour 1 is the face, colour 2 the shadow. */
static void DrawGlyph(int code, int x, int y, uint32_t face, uint32_t shadow) {
    const uint16_t* frame;
    const uint8_t* tiles;
    int px, py, top = 3, bottom = 13;

    if (code == ' ' || code <= 0)
        return;
#ifdef VERSION_JP
    if (code & JP_CHAR) {
        static void** const frames[4] = { gMsgFontBank0Frames, gMsgFontBank1Frames, gMsgFontBank2Frames,
                                          gMsgFontBank3Frames };
        static const uint8_t* const banks[4] = { gMsgFontBank0Tiles, gMsgFontBank1Tiles, gMsgFontBank2Tiles,
                                                 gMsgFontBank3Tiles };
        int sjis = PortCodepointToSjis((unsigned)(code & 0xFFFF));
        uint16_t glyph = 0;
        uint8_t bank = 0;

        if ((sjis >> 8) == 0x81) {
            /* punctuation: the table LayoutMsgGlyphsSjis uses (msg.c) */
            static const uint16_t row81[][2] = {
                { 0x41, 0xF5 }, { 0x42, 0xF6 }, { 0x43, 0xF8 }, { 0x44, 0xF7 }, { 0x45, 0xF9 }, { 0x48, 0xF1 },
                { 0x49, 0xF0 }, { 0x5B, 0xFD }, { 0x5C, 0xFC }, { 0x5E, 0xF3 }, { 0x60, 0xFE }, { 0x63, 0xFB },
                { 0x66, 0xFF }, { 0x69, 0xEC }, { 0x6A, 0xED }, { 0x75, 0xE8 }, { 0x76, 0xE9 }, { 0x77, 0xEA },
                { 0x78, 0xEB }, { 0x7B, 0xDF }, { 0x7C, 0xFC }, { 0x81, 0xEF }, { 0x93, 0xF2 }, { 0x96, 0xF4 },
            };
            size_t k;
            for (k = 0; k < sizeof(row81) / sizeof(row81[0]); k++)
                if (row81[k][0] == (sjis & 0xFF))
                    break;
            if (k == sizeof(row81) / sizeof(row81[0]))
                return;
            glyph = row81[k][1];
        } else {
            GetSjisGlyph((uint16_t)sjis, &glyph, &bank);
        }
        if (bank > 3)
            return;
        frame = (const uint16_t*)frames[bank][glyph];
        if (frame == NULL)
            return;
        tiles = banks[bank] + (frame[3] & 0x3FF) * 32;
        top = 0;
        bottom = 15;
    } else
#endif
    {
        frame = (const uint16_t*)gMsgLatinFontFrames[code & 255];
        if (frame == NULL || frame[0] == 0)
            return;
        tiles = gMsgLatinFontTiles + (frame[3] & 0x3FF) * 32;
    }
    /* Latin glyphs use rows 3..12 of the cell, Japanese ones nearly all of it */
    for (py = top; py < bottom; py++) {
        for (px = 0; px < 16; px++) {
            int t = (py >> 3) * 2 + (px >> 3);
            uint8_t b = tiles[t * 32 + (py & 7) * 4 + ((px & 7) >> 1)];
            int v = (px & 1) ? (b >> 4) : (b & 15);
            if (v == 1)
                Pixel(x + px, y + py - 3, face);
            else if (v >= 2)
                Pixel(x + px, y + py - 3, shadow);
        }
    }
}

static int DrawText(const char* s, int x, int y, uint32_t face, uint32_t shadow) {
    while (*s) {
        int c = NextChar(&s);
        DrawGlyph(c, x, y, face, shadow);
        x += GlyphWidth(c);
    }
    return x;
}

int PortUiTextWidth(const char* text) { return TextWidth(text); }

int PortUiRenderTextLine(uint32_t* pixels, int width, int height, const char* text, uint32_t color) {
    uint32_t* previous = sFb;
    uint32_t shadow = Mix(color, 0xFFFCFBFBu, 170);
    int previousW = sFbW, previousH = sFbH, x = 0;
    if (!pixels || width <= 0 || height <= 0) return 0;
    memset(pixels, 0, (size_t)width * height * sizeof(*pixels));
    sFb = pixels; sFbW = width; sFbH = height;
    while (*text) {
        int code = NextChar(&text), advance = GlyphWidth(code);
        if (x + advance > width - 10 && *text) {
            DrawText("...", x, 0, color, shadow);
            x += 9; break;
        }
        DrawGlyph(code, x, 0, color, shadow); x += advance;
    }
    sFb = previous; sFbW = previousW; sFbH = previousH;
    return x;
}

/* ---- values ------------------------------------------------------------------ */

static void RefreshModes(void) {
    sModeCount = PortVideoListModes(sModeW, sModeH, 64);
}

static int ViewIndex(void) {
    int i;

    for (i = 0; i < PORT_VIEW_COUNT; i++)
        if (sViewOrder[i] == gPortConfig.viewMode)
            return i;
    return 0;
}

static int sFsPending = -1; /* resolution row: the entry being browsed, -1 none */

static int ModeIndex(void) {
    int i;

    for (i = 0; i < sModeCount; i++)
        if (sModeW[i] == gPortConfig.fullscreenWidth && sModeH[i] == gPortConfig.fullscreenHeight)
            return i + 1;
    return 0;
}

static int ItemEnabled(int item) {
    if (item >= ITEM_REMATCH_0 && item <= ITEM_REMATCH_LAST)
        return PortRematchAvailable(item - ITEM_REMATCH_0);
    switch (item) {
        case ITEM_UNDUB: return PortUndubAvailable();
        case ITEM_CHEAT_DEBUG: return PortDebugModeAvailable();
        case ITEM_REMATCH_MENU: return 1;
        case ITEM_CHEAT_MAXLEVEL:
        case ITEM_CHEAT_MAPCARDS:
        case ITEM_CHEAT_JOURNAL: return PortCheatActionsAvailable();
        case ITEM_BATTLE_ZOOM:
        case ITEM_ZOOM:
        case ITEM_INTZOOM:
            return gPortConfig.viewMode == PORT_VIEW_AUTO;
        case ITEM_SCALE:
            return gPortConfig.viewMode != PORT_VIEW_AUTO;
        case ITEM_PS2_MENU:
            return gPortConfig.ps2Mode != 0;
        case ITEM_TOUCH_THEME: /* greyed while the touch buttons are hidden */
        case ITEM_TOUCH_EDIT:
            return !gPortConfig.hideTouchControls;
        default:
            return 1;
    }
}

static void ValueText(int item, char* out, size_t size) {
    int l = Lang();

    if (item >= ITEM_REMATCH_0 && item <= ITEM_REMATCH_LAST) {
        SDL_strlcpy(out, "→", size);
        return;
    }

    switch (item) {
        case ITEM_VIEW:
            SDL_strlcpy(out, sViews[gPortConfig.viewMode][l], size);
            break;
        case ITEM_BATTLE_ZOOM:
        case ITEM_ZOOM: {
            int zoom = item == ITEM_BATTLE_ZOOM ? gPortConfig.battleZoom : gPortConfig.zoom;
            if (item == ITEM_BATTLE_ZOOM && !zoom && ItemEnabled(item))
                SDL_strlcpy(out, sMainZoom[l], size);
            else if (ItemEnabled(item)) /* the step and the pixel scale it gives here */
            {
                int z = PortConfigZoomId(zoom);
                SDL_snprintf(out, size, "x%d", z / 2);
                if (PortZoomReference(z))
                    SDL_snprintf(out, size, "x%d (%s x%d)", z / 2, sIntegralReference[l], PortZoomReference(z));
                if (PortConfigZoomHeavy(zoom) || z == 12)
                    SDL_strlcat(out, "  !", size);
            }
            else
                SDL_strlcpy(out, sAutoOnly[l], size);
            break;
        }
        case ITEM_SCALE:
            SDL_strlcpy(out, sScales[gPortConfig.scaleMode][l], size);
            break;
        case ITEM_HUD:
            SDL_strlcpy(out, sHudSizes[SDL_clamp(gPortConfig.hudSize, 0, 2)][l], size);
            break;
        case ITEM_ASPECT:
            SDL_strlcpy(out, sAspect[SDL_clamp(gPortConfig.aspect, 0, PORT_ASPECT_COUNT - 1)][l], size);
            break;
        case ITEM_NARROW:
            SDL_strlcpy(out, sNarrow[gPortConfig.narrowFit != 0][l], size);
            break;
        case ITEM_FILTER:
            SDL_strlcpy(out, sFilters[gPortConfig.filter][l], size);
            break;
        case ITEM_FULLSCREEN:
            SDL_strlcpy(out, sOnOff[gPortConfig.fullscreen != 0][l], size);
            break;
        case ITEM_FS_RES: {
            /* the resolution being browsed; A applies it */
            int idx = sFsPending >= 0 ? sFsPending : ModeIndex();
            if (idx == 0)
                SDL_strlcpy(out, sDesktop[l], size);
            else
                SDL_snprintf(out, size, "%dx%d", sModeW[idx - 1], sModeH[idx - 1]);
            if (sFsPending >= 0 && sFsPending != ModeIndex()) {
                SDL_strlcat(out, "  ", size);
                SDL_strlcat(out, sPressA[l], size);
            }
            break;
        }
        case ITEM_INTZOOM:
            SDL_strlcpy(out, sOnOff[gPortConfig.integerZoom != 0][l], size);
            break;
        case ITEM_VSYNC:
            SDL_strlcpy(out, sOnOff[gPortConfig.vsync != 0][l], size);
            break;
        case ITEM_SCANLINES:
            if (gPortConfig.scanlines == 0)
                SDL_strlcpy(out, sOnOff[0][l], size);
            else
                SDL_snprintf(out, size, "%d%%", gPortConfig.scanlines);
            break;
        case ITEM_LCD:
            SDL_strlcpy(out, sOnOff[gPortConfig.colorCorrection != 0][l], size);
            break;
        case ITEM_HDTEXT:
            SDL_strlcpy(out, sOnOff[gPortConfig.hdText != 0][l], size);
            break;
        case ITEM_CARDS:
            if (gPortConfig.cardHudRaise == 0)
                SDL_strlcpy(out, sOnOff[0][l], size);
            else
                SDL_snprintf(out, size, "%d px", gPortConfig.cardHudRaise);
            break;
        case ITEM_FPS:
            SDL_strlcpy(out, sOnOff[gPortConfig.showFps != 0][l], size);
            break;
        case ITEM_FRAMESKIP:
            SDL_strlcpy(out, sOnOff[gPortConfig.frameSkip != 0][l], size);
            break;
        case ITEM_DISCORD:
            SDL_strlcpy(out, sOnOff[gPortConfig.discordPresence != 0][l], size);
            break;
        case ITEM_HD_MOVIES:
            SDL_strlcpy(out, sOnOff[gPortConfig.hdMovies != 0][l], size);
            break;
        case ITEM_HD_MUSIC:
            SDL_strlcpy(out, sOnOff[gPortConfig.hdMusic != 0][l], size);
            break;
        case ITEM_HD_SCENES:
        case ITEM_PS2_HDSCENES:
            SDL_strlcpy(out, sOnOff[gPortConfig.hdScenes != 0][l], size);
            break;
        case ITEM_HD_SUBTITLES:
            SDL_strlcpy(out, sOnOff[gPortConfig.hdSubtitles != 0][l], size);
            break;
        case ITEM_HD_CARDS:
            SDL_strlcpy(out, sOnOff[gPortConfig.hdCards != 0][l], size);
            break;
        case ITEM_HD_TITLE:
            SDL_strlcpy(out, sOnOff[gPortConfig.hdTitle != 0][l], size);
            break;
        case ITEM_GBA_BORDER:
            if (gPortConfig.gbaBorder)
                SDL_snprintf(out, size, "%d", gPortConfig.gbaBorder);
            else
                SDL_strlcpy(out, sOnOff[0][l], size);
            break;
        case ITEM_MODE_RESET:
            SDL_strlcpy(out, "\xE2\x86\x92", size);
            break;
        case ITEM_DODGE:
            SDL_strlcpy(out, sOnOff[gPortConfig.dodgeButton != 0][l], size);
            break;
        case ITEM_STOCK:
            SDL_strlcpy(out, sOnOff[gPortConfig.stockButton != 0][l], size);
            break;
        case ITEM_PAD_LAYOUT:
            SDL_strlcpy(out, sOnOff[gPortConfig.padLayout == 2][l], size);
            break;
        case ITEM_INVERT_CARD_SCROLL:
            SDL_strlcpy(out, sOnOff[gPortConfig.invertCardScroll != 0][l], size);
            break;
        case ITEM_MOOGLE_NOTICE:
            SDL_strlcpy(out, sOnOff[gPortConfig.moogleNotice != 0][l], size);
            break;
        case ITEM_UNSTOCK:
            SDL_strlcpy(out, sOnOff[gPortConfig.unstockButton != 0][l], size);
            break;
        case ITEM_SAVEBANK:
            SDL_snprintf(out, size, "%d / %d", gPortConfig.saveBank, PORT_SAVE_BANKS);
            break;
        case ITEM_UNDUB:
            SDL_strlcpy(out, sOnOff[gPortConfig.undub != 0][l], size);
            break;
        case ITEM_HD2D:
            SDL_strlcpy(out, sOnOff[gPortConfig.hd2d != 0][l], size);
            break;
        case ITEM_HDHUD:
            SDL_strlcpy(out, sOnOff[gPortConfig.hdHud != 0][l], size);
            break;
        case ITEM_SMOOTHAUDIO:
            SDL_strlcpy(out, sOnOff[gPortConfig.smoothAudio != 0][l], size);
            break;
        case ITEM_SAVE_SLOT_EXPERIMENTAL:
            SDL_strlcpy(out, sOnOff[gPortConfig.savePointSlotSelect != 0][l], size);
            break;
        case ITEM_HIDE_TOUCH:
            SDL_strlcpy(out, sOnOff[gPortConfig.hideTouchControls != 0][l], size);
            break;
        case ITEM_TOUCH_THEME: {
            static const char* const names[3] = { "Clear", "Dark", "Color" };
            SDL_strlcpy(out, names[SDL_clamp(gPortConfig.touchTheme, 0, 2)], size);
            break;
        }
        case ITEM_PS2_MODE:
            SDL_strlcpy(out, sOnOff[gPortConfig.ps2Mode != 0][l], size);
            break;
        case ITEM_CHEAT_RIKU:
            SDL_strlcpy(out, sOnOff[gPortConfig.cheatUnlockRiku != 0][l], size);
            break;
        case ITEM_CHEAT_ONEHIT:
            SDL_strlcpy(out, sOnOff[gPortConfig.cheatOneHit != 0][l], size);
            break;
        case ITEM_CHEAT_INFHP:
            SDL_strlcpy(out, sOnOff[gPortConfig.cheatInfiniteHp != 0][l], size);
            break;
        case ITEM_CHEAT_STATES:
            SDL_strlcpy(out, sOnOff[gPortConfig.cheatSaveStates != 0][l], size);
            break;
        case ITEM_CHEAT_GBA_MAP:
            SDL_strlcpy(out, sOnOff[gPortConfig.gbaMapLogic != 0][l], size);
            break;
        case ITEM_CHEAT_MAXLEVEL:
        case ITEM_CHEAT_MAPCARDS:
        case ITEM_CHEAT_JOURNAL:
            SDL_strlcpy(out, "→", size);
            break;
        case ITEM_CHEAT_DEBUG:
        case ITEM_CONTROL_RESET:
            SDL_strlcpy(out, "\xE2\x86\x92", size);
            break;
        case ITEM_CHEATS_MENU:
        case ITEM_REMATCH_MENU:
        case ITEM_ACHIEVEMENTS_MENU:
        case ITEM_CONTROLS_MENU:
        case ITEM_PS2_MENU:
        case ITEM_TOUCH_EDIT:
            SDL_strlcpy(out, "\xE2\x86\x92", size);
            break;
        case ITEM_TITLE:
            SDL_strlcpy(out, "\xE2\x86\x92", size);
            break;
        case ITEM_PS2_DIFFICULTY:
        case ITEM_NG_DIFFICULTY:
            SDL_strlcpy(out, sDifficulty[SDL_clamp(gPortConfig.ps2Difficulty, 0, 2)][l], size);
            break;
        case ITEM_NG_VERSION:
            SDL_strlcpy(out, sVersions[sNgRecom != 0][l], size);
            break;
        case ITEM_NG_HDTEXT:
            SDL_strlcpy(out, sOnOff[gPortConfig.hdText != 0][l], size);
            break;
        case ITEM_NG_QOL:
            SDL_strlcpy(out, sOnOff[PortQolButtons() != 0][l], size);
            break;
        case ITEM_PS2_FASTTEXT:
            SDL_strlcpy(out, sOnOff[gPortConfig.ps2FastText != 0][l], size);
            break;
        case ITEM_MUSIC:
            SDL_snprintf(out, size, "%d%%", gPortConfig.musicVolume);
            break;
        case ITEM_SFX:
            SDL_snprintf(out, size, "%d%%", gPortConfig.sfxVolume);
            break;
        case ITEM_MASTER:
            SDL_snprintf(out, size, "%d%%", gPortConfig.masterVolume);
            break;
        default:
            if (item >= ITEM_CONTROL_A && item <= ITEM_CONTROL_UNSTOCK) {
                int action = item - ITEM_CONTROL_A;
                if (sControlCapture) SDL_strlcpy(out, "Press...", size);
                else if (sControlPad) {
                    ControlBindingName(gPortConfig.pad[action], l, out, size);
                } else {
                    int b = gPortConfig.keys[action][0];
                    SDL_strlcpy(out, b ? SDL_GetKeyName((SDL_Keycode)b) : "-", size);
                }
            } else {
                out[0] = 0;
            }
            break;
    }
}

static int Wrap(int v, int n) {
    return ((v % n) + n) % n;
}

static void Close(void);

static void ResetControlBindings(void) {
    static const int keys[PORT_ACT_COUNT][2] = {
        { SDLK_X, SDLK_K }, { SDLK_Z, SDLK_J }, { SDLK_A, 0 }, { SDLK_S, 0 },
        { SDLK_RETURN, 0 }, { SDLK_BACKSPACE, SDLK_RSHIFT }, { SDLK_UP, SDLK_W },
        { SDLK_DOWN, 0 }, { SDLK_LEFT, 0 }, { SDLK_RIGHT, 0 }, { SDLK_D, 0 },
        { SDLK_C, 0 }, { SDLK_V, 0 },
    };
    static const int pad[PORT_ACT_COUNT] = {
        SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
        SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, SDL_GAMEPAD_BUTTON_START, SDL_GAMEPAD_BUTTON_BACK,
        SDL_GAMEPAD_BUTTON_DPAD_UP, SDL_GAMEPAD_BUTTON_DPAD_DOWN, SDL_GAMEPAD_BUTTON_DPAD_LEFT,
        SDL_GAMEPAD_BUTTON_DPAD_RIGHT, SDL_GAMEPAD_BUTTON_NORTH, SDL_GAMEPAD_BUTTON_WEST,
        SDL_GAMEPAD_BUTTON_RIGHT_STICK,
    };
    memcpy(gPortConfig.keys, keys, sizeof(keys));
    memcpy(gPortConfig.pad, pad, sizeof(pad));
    PortConfigSave();
}

static void Change(int item, int dir) {
    if (item >= ITEM_REMATCH_0 && item <= ITEM_REMATCH_LAST) {
        if (dir > 0 && !ItemEnabled(item)) /* not now: why, and when it can be fought */
            PortUiNotice(RematchWhy(item - ITEM_REMATCH_0));
        else if (dir > 0 && PortRematchStart(item - ITEM_REMATCH_0))
            Close();
        return;
    }
    if (!ItemEnabled(item))
        return;
    switch (item) {
        case ITEM_VIEW:
            gPortConfig.viewMode = sViewOrder[Wrap(ViewIndex() + dir, PORT_VIEW_COUNT)];
            break;
        case ITEM_BATTLE_ZOOM:
            gPortConfig.battleZoom = PortConfigBattleZoomNext(gPortConfig.battleZoom, dir);
            break;
        case ITEM_ZOOM:
            gPortConfig.zoom = PortConfigZoomNext(gPortConfig.zoom, dir);
            break;
        case ITEM_INTZOOM:
            gPortConfig.integerZoom = !gPortConfig.integerZoom;
            gPortConfig.zoom = PortConfigZoomId(gPortConfig.zoom);
            break;
        case ITEM_SCALE:
            gPortConfig.scaleMode = Wrap(gPortConfig.scaleMode + dir, PORT_SCALE_COUNT);
            break;
        case ITEM_HUD:
            gPortConfig.hudSize = Wrap(gPortConfig.hudSize + dir, 3);
            break;
        case ITEM_ASPECT:
            gPortConfig.aspect = Wrap(gPortConfig.aspect + dir, PORT_ASPECT_COUNT);
            break;
        case ITEM_NARROW:
            gPortConfig.narrowFit = !gPortConfig.narrowFit;
            break;
        case ITEM_FILTER:
            gPortConfig.filter = Wrap(gPortConfig.filter + dir, PORT_FILTER_COUNT);
            break;
        case ITEM_FULLSCREEN:
            gPortConfig.fullscreen = !gPortConfig.fullscreen;
            break;
        case ITEM_FS_RES:
            /* browse only; ConfirmFsRes (A) applies */
            sFsPending = Wrap((sFsPending >= 0 ? sFsPending : ModeIndex()) + dir, sModeCount + 1);
            return;
        case ITEM_VSYNC:
            gPortConfig.vsync = !gPortConfig.vsync;
            break;
        case ITEM_SCANLINES:
            gPortConfig.scanlines = Wrap(gPortConfig.scanlines / 25 + dir, 5) * 25;
            break;
        case ITEM_LCD:
            gPortConfig.colorCorrection = !gPortConfig.colorCorrection;
            break;
        case ITEM_HDTEXT:
            gPortConfig.hdText = !gPortConfig.hdText;
            break;
        case ITEM_CARDS:
            gPortConfig.cardHudRaise = Wrap(gPortConfig.cardHudRaise / 4 + dir, 5) * 4;
            break;
        case ITEM_FPS:
            gPortConfig.showFps = !gPortConfig.showFps;
            break;
        case ITEM_FRAMESKIP:
            gPortConfig.frameSkip = !gPortConfig.frameSkip;
            break;
        case ITEM_DISCORD:
            gPortConfig.discordPresence = !gPortConfig.discordPresence;
            break;
        case ITEM_HD_MOVIES:
            gPortConfig.hdMovies = !gPortConfig.hdMovies;
            break;
        case ITEM_HD_MUSIC:
            gPortConfig.hdMusic = !gPortConfig.hdMusic;
            break;
        case ITEM_HD_SCENES:
        case ITEM_PS2_HDSCENES:
            gPortConfig.hdScenes = !gPortConfig.hdScenes;
            break;
        case ITEM_HD_SUBTITLES:
            gPortConfig.hdSubtitles = !gPortConfig.hdSubtitles;
            break;
        case ITEM_HD_CARDS: /* HD HUD: the cards, the battle HUD and the title's logo together */
            gPortConfig.hdCards = !gPortConfig.hdCards;
            gPortConfig.hdTitle = gPortConfig.hdCards;
            break;
        case ITEM_HD_TITLE:
            gPortConfig.hdTitle = !gPortConfig.hdTitle;
            break;
        case ITEM_GBA_BORDER:
            gPortConfig.gbaBorder = Wrap(gPortConfig.gbaBorder + (dir ? dir : 1), 4);
            break;
        case ITEM_MODE_RESET: /* a window asks which mode, then twice: its presets replace the current settings */
            if (dir > 0)
                ConfirmOpen(CONFIRM_MODE);
            return;
        case ITEM_DODGE:
            gPortConfig.dodgeButton = !gPortConfig.dodgeButton;
            break;
        case ITEM_STOCK:
            gPortConfig.stockButton = !gPortConfig.stockButton;
            break;
        case ITEM_PAD_LAYOUT:
            gPortConfig.padLayout = gPortConfig.padLayout == 2 ? 0 : 2; /* off: detect the pad */
            break;
        case ITEM_INVERT_CARD_SCROLL:
            gPortConfig.invertCardScroll = !gPortConfig.invertCardScroll;
            break;
        case ITEM_MOOGLE_NOTICE:
            gPortConfig.moogleNotice = !gPortConfig.moogleNotice;
            break;
        case ITEM_UNSTOCK:
            gPortConfig.unstockButton = !gPortConfig.unstockButton;
            break;
        case ITEM_SAVEBANK:
            gPortConfig.saveBank = Wrap(gPortConfig.saveBank - 1 + dir, PORT_SAVE_BANKS) + 1;
            PortSramSwitchBank();
            break;
        case ITEM_UNDUB:
            gPortConfig.undub = !gPortConfig.undub;
            break;
        case ITEM_HD2D:
            gPortConfig.hd2d = !gPortConfig.hd2d;
            break;
        case ITEM_HDHUD:
            gPortConfig.hdHud = !gPortConfig.hdHud;
            break;
        case ITEM_SMOOTHAUDIO:
            gPortConfig.smoothAudio = !gPortConfig.smoothAudio;
            break;
        case ITEM_SAVE_SLOT_EXPERIMENTAL:
            gPortConfig.savePointSlotSelect = !gPortConfig.savePointSlotSelect;
            break;
        case ITEM_HIDE_TOUCH:
            gPortConfig.hideTouchControls = !gPortConfig.hideTouchControls;
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) || defined(PORT_ANDROID64) || defined(PORT_TOUCH)
            PortAndroidTouchSettingsChanged();
#endif
            break;
        case ITEM_TOUCH_THEME:
            gPortConfig.touchTheme = Wrap(gPortConfig.touchTheme + dir, 3);
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) || defined(PORT_ANDROID64) || defined(PORT_TOUCH)
            PortAndroidTouchSettingsChanged();
#endif
            break;
        case ITEM_PS2_MODE:
            gPortConfig.ps2Mode = !gPortConfig.ps2Mode;
            break;
        case ITEM_CHEATS_MENU:
            if (dir > 0) { sPage = PAGE_CHEATS; sCursor = sScroll = ITEM_CHEAT_DEBUG; }
            break;
        case ITEM_TITLE: /* asked twice in a window; unsaved progress is lost, as with A+B+Start+Select */
            if (dir > 0)
                ConfirmOpen(CONFIRM_TITLE);
            return;
        case ITEM_ACHIEVEMENTS_MENU:
            if (dir > 0) AchievementMenuOpen();
            return;
        case ITEM_TOUCH_EDIT:
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) || defined(PORT_ANDROID64) || defined(PORT_TOUCH)
            if (dir > 0 && !gPortConfig.hideTouchControls) {
                Close();
                sTouchEdit = 1; /* the game stays held until the editor's tick (PortUiTouchEdit(0)) */
                PortAndroidTouchEditStart();
            }
#endif
            return;
        case ITEM_REMATCH_MENU:
            if (dir > 0) { sPage = PAGE_REMATCH; sCursor = sScroll = ITEM_REMATCH_0; }
            break;
        case ITEM_REMATCH_BACK:
            sPage = PAGE_CHEATS; sCursor = ITEM_REMATCH_MENU; sScroll = ITEM_CHEAT_DEBUG;
            break;
        case ITEM_CONTROLS_MENU:
            if (dir > 0) {
                sPage = PAGE_CONTROLS;
                sControlPad = 0;
                sCursor = sScroll = ITEM_CONTROL_A;
            }
            break;
        case ITEM_CHEAT_RIKU:
            gPortConfig.cheatUnlockRiku = !gPortConfig.cheatUnlockRiku;
            break;
        case ITEM_CHEAT_ONEHIT:
            gPortConfig.cheatOneHit = !gPortConfig.cheatOneHit;
            break;
        case ITEM_CHEAT_INFHP:
            gPortConfig.cheatInfiniteHp = !gPortConfig.cheatInfiniteHp;
            break;
        case ITEM_CHEAT_STATES: /* off at once; on only through the warning */
            if (gPortConfig.cheatSaveStates) {
                gPortConfig.cheatSaveStates = 0;
                break;
            }
            ConfirmOpen(CONFIRM_STATES);
            return;
        case ITEM_CHEAT_GBA_MAP:
            gPortConfig.gbaMapLogic = !gPortConfig.gbaMapLogic;
            break;
        case ITEM_CHEAT_MAXLEVEL:
        case ITEM_CHEAT_MAPCARDS:
        case ITEM_CHEAT_JOURNAL:
            /* (the battle card collection is filled from the launcher's save editor) */
            if (dir > 0 && (item == ITEM_CHEAT_MAXLEVEL ? PortCheatMaxLevel() :
                            item == ITEM_CHEAT_MAPCARDS ? PortCheatMapCards() : PortCheatJournal())) {
                Close();
                PortUiNotice(sCheatDone[Lang()]);
            }
            break;
        case ITEM_CHEAT_DEBUG:
            if (PortDebugModeAvailable()) {
                PortDebugModeEnter();
                Close();
            }
            break;
        case ITEM_CHEATS_BACK:
            sPage = PAGE_MAIN; sCursor = ITEM_CHEATS_MENU; sScroll = 0;
            break;
        case ITEM_CONTROL_A: case ITEM_CONTROL_B: case ITEM_CONTROL_L: case ITEM_CONTROL_R:
        case ITEM_CONTROL_START: case ITEM_CONTROL_SELECT: case ITEM_CONTROL_UP: case ITEM_CONTROL_DOWN:
        case ITEM_CONTROL_LEFT: case ITEM_CONTROL_RIGHT: case ITEM_CONTROL_STOCK: case ITEM_CONTROL_DODGE:
        case ITEM_CONTROL_UNSTOCK:
            if (dir) { sControlPad = !sControlPad; sControlCapture = 0; } else sControlCapture = 1;
            break;
        case ITEM_CONTROL_RESET:
            if (dir > 0) ResetControlBindings();
            break;
        case ITEM_CONTROLS_BACK:
            PortConfigSave(); sPage = PAGE_MAIN; sCursor = ITEM_CONTROLS_MENU; sScroll = 0;
            break;
        case ITEM_PS2_MENU:
            if (dir > 0 && gPortConfig.ps2Mode) {
                sPage = PAGE_PS2;
                sCursor = ITEM_PS2_DIFFICULTY;
                sScroll = ITEM_PS2_DIFFICULTY;
            }
            break;
        case ITEM_PS2_BACK:
            sPage = PAGE_MAIN;
            sCursor = ITEM_PS2_MENU;
            break;
        case ITEM_NG_VERSION:
            /* HD text and the one-button controls stay as the start's mode set them */
            sNgRecom = !sNgRecom;
            break;
        case ITEM_NG_HDTEXT:
            gPortConfig.hdText = !gPortConfig.hdText;
            break;
        case ITEM_NG_QOL:
            PortSetQolButtons(!PortQolButtons());
            break;
        case ITEM_NG_START:
            PortRecomSetActive(sNgRecom);
            Close();
            sCloseSound = 0; /* the new game begins */
            break;
        case ITEM_NG_DIFFICULTY:
        case ITEM_PS2_DIFFICULTY:
            gPortConfig.ps2Difficulty = Wrap(gPortConfig.ps2Difficulty + dir, 3);
            break;
        case ITEM_PS2_FASTTEXT:
            gPortConfig.ps2FastText = !gPortConfig.ps2FastText;
            break;
        case ITEM_MUSIC:
            gPortConfig.musicVolume = SDL_clamp(gPortConfig.musicVolume + dir * 10, 0, 100);
            break;
        case ITEM_SFX:
            gPortConfig.sfxVolume = SDL_clamp(gPortConfig.sfxVolume + dir * 10, 0, 100);
            break;
        case ITEM_MASTER:
            gPortConfig.masterVolume = SDL_clamp(gPortConfig.masterVolume + dir * 10, 0, 100);
            break;
    }
    PortVideoApplyConfig();
}

/* A on the resolution row: the browsed resolution takes effect */
static void ConfirmFsRes(void) {
    int i = sFsPending >= 0 ? sFsPending : ModeIndex();
    gPortConfig.fullscreenWidth = i ? sModeW[i - 1] : 0;
    gPortConfig.fullscreenHeight = i ? sModeH[i - 1] : 0;
    sFsPending = -1;
    PortVideoApplyConfig();
}

/* ---- public ------------------------------------------------------------------ */

void PortUiInit(void) {
}

static int sScenePause;
int PortUiActive(void) {
    return sOpen || sScenePause || sTouchEdit; /* the scene pause and the touch layout editor hold the game too */
}

void PortUiTouchEdit(int on) {
    sTouchEdit = on;
}

/* Fades: the picture goes to black, the menu comes in from black, and the
   reverse on closing (MENU_FADE frames each way). */
#define MENU_FADE 8
#define MENU_FADE_COLOR 0xFF000000u
static int sFade;    /* frames since the menu opened */
static int sClosing; /* frames left of the menu fading to black */
static int sReturn;  /* frames left of the black fading back to the game */
static int sHarnessTick;

/* Options > Return to title screen, once the menu has closed (PortFrame resets the game). */
int PortUiTakeTitleRequest(void) {
    if (!sTitleRequest || sOpen)
        return 0;
    sTitleRequest = 0;
    return 1;
}

int PortUiMenuCanvas(void) {
    return sOpen && sFade >= MENU_FADE;
}

int PortUiSceneFade(void) {
    if (sOpen && sFade < MENU_FADE) return (sFade + 1) * 255 / MENU_FADE;
    if (!sOpen && sReturn > 0) return sReturn * 255 / MENU_FADE;
    return 0;
}

int PortUiTakeMenuSound(int open) {
    int* flag = open ? &sOpenSound : &sCloseSound;
    int take = *flag;
    *flag = 0;
    return take;
}

/* Counts every opening of the options: an event reading a START tap ignores one held across an opening (START +
   SELECT opens them; the START released when they close is not a tap of the scene's). */
static unsigned sOpenSerial;
unsigned PortUiOpenSerial(void) {
    return sOpenSerial;
}

void PortUiOpenOptions(void) {
    if (sOpen)
        return;
    sOpen = 1;
    sOpenSerial++;
    sConfirm = CONFIRM_NONE;
    sOpenSound = !SDL_getenv("KHCOM_OPTIONS_SHOT");
    sCloseSound = 0;
    sFade = SDL_getenv("KHCOM_OPTIONS_SHOT") ? MENU_FADE * 2 : 0; /* test shots: no fade */
    sClosing = 0;
    sReturn = 0;
    sCursor = 0;
    sScroll = 0;
    sPage = PAGE_MAIN;
    sHarnessTick = 0;
    if (SDL_getenv("KHCOM_OPTIONS_CURSOR")) /* test harness: shot of a lower row */
        sCursor = SDL_clamp(SDL_atoi(SDL_getenv("KHCOM_OPTIONS_CURSOR")), 0, ITEM_COUNT - 1);
    if (sCursor >= ITEM_CHEAT_DEBUG && sCursor <= ITEM_CHEATS_BACK) /* a row of the Cheats page */
        sPage = PAGE_CHEATS;
    if (sCursor >= ITEM_CONTROL_A && sCursor <= ITEM_CONTROLS_BACK) /* a row of the Controls page */
        sPage = PAGE_CONTROLS;
    RefreshModes();
    if (SDL_getenv("KHCOM_ACHIEVEMENTS_SHOT")) AchievementMenuOpen();
}

/* map.c: a new game was confirmed. Original preselected; its options start as the graphics mode chosen on the first
   start (or Options since) left them, not the cartridge's (24/10: HD text turned itself off on Android). */
void PortUiOpenNewGame(void) {
    /* unattended runs (KHCOM_FAST batteries, the bot, replays) start as the
       original unless a test drives this page */
    if ((SDL_getenv("KHCOM_FAST") || SDL_getenv("KHCOM_BOT")) && !SDL_getenv("KHCOM_OPTIONS_ACTIONS") && !SDL_getenv("KHCOM_WIZARD_KEYS")) {
        PortRecomSetActive(0);
        return;
    }
    PortUiOpenOptions();
    sOpenSound = 0; /* the game's own confirm has just sounded */
    sPage = PAGE_NEWGAME;
    sNgRecom = 0;
    /* the version is always the first question */
    sWizStep = WIZ_VERSION;
    sWizCursor = 0;
    PortVideoApplyConfig();
}

/* The first start (graphics_mode=0 in kh_com.ini): which mode, before the title. Unattended runs keep the settings
   as they are unless a test drives this window (KHCOM_MODE_CHOICE). */
static int sNewAssets = -1; /* PORT_ASSET_* bits imported since the last start whose option is off (-1 not looked) */
static int sAssetPick; /* the new assets' switches: the ones to turn on */

static int AssetOptionOn(int asset) {
    switch (asset) {
        case PORT_ASSET_HD_CARDS: return gPortConfig.hdCards;
        case PORT_ASSET_HD_MUSIC: return gPortConfig.hdMusic;
        case PORT_ASSET_HD_MOVIES: return gPortConfig.hdMovies;
        case PORT_ASSET_HD_SCENES: return gPortConfig.hdScenes;
        case PORT_ASSET_HD_SUBS: return gPortConfig.hdSubtitles;
        default: return gPortConfig.undub;
    }
}

/* Once per run, after the first start's mode: what the launcher brought since the last start (the HD mod's parts, the
   Japanese ROM of the undub) is offered once; an older kh_com.ini knows nothing yet, so what is there counts as known. */
static void CheckNewAssets(void) {
    int now, i;
    if (sNewAssets >= 0)
        return;
    now = PortAssetsPresentMask();
    sNewAssets = 0;
    if (gPortConfig.assetsSeen >= 0)
        for (i = 0; i < PORT_ASSET_COUNT; i++)
            if ((now & ~gPortConfig.assetsSeen) >> i & 1 && !AssetOptionOn(i))
                sNewAssets |= 1 << i;
    if (gPortConfig.assetsSeen != now) {
        gPortConfig.assetsSeen = now;
        PortConfigSave();
    }
}

int PortUiNeedsModeChoice(void) {
    int unattended = (SDL_getenv("KHCOM_FAST") || SDL_getenv("KHCOM_BOT")) && !SDL_getenv("KHCOM_MODE_CHOICE");
    if (sOpen)
        return 0;
    if (gPortConfig.graphicsMode == PORT_MODE_CHOOSE)
        return !unattended;
    CheckNewAssets();
    return sNewAssets > 0 && !unattended;
}

static int HdAnyImported(void) {
    return PortAssetPresent(PORT_ASSET_HD_CARDS) || PortAssetPresent(PORT_ASSET_HD_MOVIES) ||
           PortAssetPresent(PORT_ASSET_HD_MUSIC);
}

static int sModeNoHd; /* nothing of the HD mod imported: the HD mode is not offered (24/10, seen on Android) */
void PortUiOpenModeChoice(void) {
    PortUiOpenOptions();
    sOpenSound = 0;
    sPage = PAGE_NEWGAME;
    sModeNoHd = !HdAnyImported();
    sWizStep = gPortConfig.graphicsMode != PORT_MODE_CHOOSE ? WIZ_ASSETS : sModeNoHd ? WIZ_MODE_GBA : WIZ_MODE;
    sWizCursor = 0;
    sAssetPick = sNewAssets; /* every switch on: the player turns off what they do not want */
}

/* The online lobby (port_lobby.c), before the link battle's menu: the public rooms, open one, enter one, or the
   cable. The game waits under it as under the options. */
static int sLobbyCursor;
static void LobbyKey(int key);
void PortUiOpenLobby(void) {
    extern void PortLobbyRefresh(void);
    PortUiOpenOptions();
    sOpenSound = 0; /* the game's own confirm has just sounded */
    sPage = PAGE_LOBBY;
    sLobbyCursor = 0;
    PortLobbyRefresh();
}

void PortUiForceClose(void) {
    sOpenSound = sCloseSound = 0;
    sOpen = 0;
    sClosing = 0;
    sReturn = 0;
}

static void Close(void);

void PortUiHarnessToggle(void) {
    if (!sOpen)
        PortUiOpenOptions();
    else
        Close();
}

/* The game stays paused (sOpen) until the menu has faded out. */
static void Close(void) {
    if (sClosing > 0)
        return;
    sClosing = MENU_FADE;
    sCloseSound = 1;
    PortConfigSave();
    PortInputSuppressUntilRelease();
}

/* A step of the main zoom from the keyboard (+/-) or Select + Up/Down, with a notice. dir < 0: closer. */
void PortUiZoomStep(int dir) {
    char text[64];
    int l = Lang();
    if (gPortConfig.viewMode != PORT_VIEW_AUTO) {
        SDL_snprintf(text, sizeof(text), "Zoom %s", sAutoOnly[l]);
    } else {
        int z = PortConfigZoomNext(gPortConfig.zoom, dir);
        if (z != gPortConfig.zoom) {
            gPortConfig.zoom = z;
            PortConfigSave();
        }
        SDL_snprintf(text, sizeof(text), "Zoom x%d", PortConfigZoomId(z) / 2);
    }
    PortUiNotice(text);
}

void PortUiNotice(const char* text) {
    static const char* const prefix[LANG_COUNT][4] = {
        { "State", "saved", "loaded", "Slot" },
        { "État", "sauvegardé", "chargé", "Emplacement" },
        { "Zustand", "gespeichert", "geladen", "Platz" },
        { "Stato", "salvato", "caricato", "Slot" },
        { "Estado", "guardado", "cargado", "Ranura" },
        { "ステート", "セーブ", "ロード", "スロット" },
    };
    int l = Lang(), n;

    /* Translate the few messages the platform layer emits. */
    if (SDL_sscanf(text, "State %d saved", &n) == 1 && SDL_strstr(text, "saved"))
        SDL_snprintf(sNotice, sizeof(sNotice), "%s %d %s", prefix[l][0], n, prefix[l][1]);
    else if (SDL_sscanf(text, "State %d loaded", &n) == 1 && SDL_strstr(text, "loaded"))
        SDL_snprintf(sNotice, sizeof(sNotice), "%s %d %s", prefix[l][0], n, prefix[l][2]);
    else if (SDL_sscanf(text, "Slot %d", &n) == 1)
        SDL_snprintf(sNotice, sizeof(sNotice), "%s %d", prefix[l][3], n);
    else
        SDL_strlcpy(sNotice, text, sizeof(sNotice));
    sNoticeFrames = 120;
}

/* src/sio/sio.c over the network: the other player has not been heard for a while; the game waits (nothing is
   cut) and goes on when the packets come back. */
void PortUiLinkWaitingNotice(int seconds, int canGiveUp) {
    static const char* const text[LANG_COUNT] = {
        "Waiting for the other player... %d s", "En attente de l'autre joueur... %d s",
        "Warte auf den anderen Spieler... %d s", "In attesa dell'altro giocatore... %d s",
        "Esperando al otro jugador... %d s", "Waiting for the other player... %d s",
    };
    static const char* const giveUp[LANG_COUNT] = { "   (hold B: leave)", "   (B maintenu : quitter)",
                                                    "   (B halten: verlassen)", "   (tieni B: esci)",
                                                    "   (mantén B: salir)", "   (hold B: leave)" };
    SDL_snprintf(sNotice, sizeof(sNotice), text[Lang()], seconds);
    if (canGiveUp)
        SDL_strlcat(sNotice, giveUp[Lang()], sizeof(sNotice));
    sNoticeFrames = 90;
}

/* port_steamlink.c's state on screen: Spanish for the Spanish game, English otherwise */
void PortUiLinkNotice(const char* en, const char* es, const char* name) {
    SDL_snprintf(sNotice, sizeof(sNotice), Lang() == LANG_ES ? es : en, name ? name : "");
    sNoticeFrames = 240;
}

/* map_tasks.c / poo.c: moogle points picked up on the field. Pickups close
   together add up in one notice: "+30 moogle points (total 1250)". */
void PortMoogleNotice(unsigned amount, unsigned total) {
    static const char* const text[LANG_COUNT] = {
        "+%u moogle points (total %u)", "+%u points mog (total %u)", "+%u Mogry-Punkte (gesamt %u)",
        "+%u punti moguri (totale %u)", "+%u puntos moguri (total %u)", "モーグリポイント +%u (合計 %u)",
    };
    static unsigned sum;
    static uint32_t last;
    uint32_t now = PortFrameCount();
    char line[96];

    if (!gPortConfig.moogleNotice)
        return;
    sum = (sNoticeFrames > 0 && now - last < 120) ? sum + amount : amount;
    last = now;
    SDL_snprintf(line, sizeof(line), text[Lang()], sum, total);
    PortUiNotice(line);
}

void PortUiTitleHint(void) {
    sTitleHintFrames = 2;
}

static void Move(int dir) {
    extern void PortMenuSoundClick(void);
    PortMenuSoundClick(); /* the debug menu's click, as its light moves */
    sFsPending = -1; /* leaving the row drops a resolution not applied */
    do {
        int first = PageFirst(), n = PageEnd() - first;
        sCursor = first + Wrap(sCursor - first + dir, n);
        if (sCursor == ITEM_EXPERIMENTAL || sCursor == ITEM_RECOM_EXTRAS || sCursor == ITEM_CHEAT_DEBUG_HEADING ||
            sCursor == ITEM_CONTROL_NINTENDO_HEADING)
            sCursor = first + Wrap(sCursor - first + dir, n);
    } while ((!ItemEnabled(sCursor) && !(sCursor >= ITEM_REMATCH_0 && sCursor <= ITEM_REMATCH_LAST)) ||
             !ItemVisible(sCursor));
}

static int ScenePauseEvent(const SDL_Event* ev);
int PortUiHandleEvent(const void* p) {
    const SDL_Event* ev = (const SDL_Event*)p;

    if (sScenePause)
        return ScenePauseEvent(ev);

    if (sOpen && (sClosing > 0 || sFade < MENU_FADE)) /* fading: the menu takes no input */
        return ev->type == SDL_EVENT_KEY_DOWN || ev->type == SDL_EVENT_KEY_UP ||
               ev->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || ev->type == SDL_EVENT_GAMEPAD_BUTTON_UP;
    if (!sOpen) {
        if (ev->type == SDL_EVENT_KEY_DOWN && !ev->key.repeat && (ev->key.key == SDLK_F1 || ev->key.key == SDLK_ESCAPE)) {
            PortUiOpenOptions();
            return 1;
        }
        /* Start + Select anywhere (the bound buttons and keys): the other one
           is read as it is now, so a release the overlay swallowed never
           leaves a half combo behind. */
        if (ev->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            SDL_Gamepad* pad = SDL_GetGamepadFromID(ev->gbutton.which);
            int start = gPortConfig.pad[PORT_ACT_START], select = gPortConfig.pad[PORT_ACT_SELECT];
            int b = ev->gbutton.button;
            if (b == SDL_GAMEPAD_BUTTON_GUIDE
                || (pad != NULL && b == start && select >= 0 && KhPadButtonDown(pad, select, gPortConfig.padLayout))
                || (pad != NULL && b == select && start >= 0 && KhPadButtonDown(pad, start, gPortConfig.padLayout))) {
                PortUiOpenOptions();
                return 1;
            }
        }
        if (ev->type == SDL_EVENT_KEY_DOWN && !ev->key.repeat) {
            const bool* held = SDL_GetKeyboardState(NULL);
            int i, isStart = 0, isSelect = 0, startHeld = 0, selectHeld = 0;
            for (i = 0; i < 2; i++) {
                SDL_Keycode ks = (SDL_Keycode)gPortConfig.keys[PORT_ACT_START][i];
                SDL_Keycode kl = (SDL_Keycode)gPortConfig.keys[PORT_ACT_SELECT][i];
                if (ks != 0) {
                    isStart |= ev->key.key == ks;
                    startHeld |= held[SDL_GetScancodeFromKey(ks, NULL)];
                }
                if (kl != 0) {
                    isSelect |= ev->key.key == kl;
                    selectHeld |= held[SDL_GetScancodeFromKey(kl, NULL)];
                }
            }
            if ((isStart && selectHeld) || (isSelect && startHeld)) {
                PortUiOpenOptions();
                return 1;
            }
        }
        return 0;
    }

    if (sConfirm) {
        int key = -1;
        if (ev->type == SDL_EVENT_KEY_DOWN) {
            switch (ev->key.key) {
                case SDLK_UP: case SDLK_W: key = 0; break;
                case SDLK_DOWN: case SDLK_S: key = 1; break;
                case SDLK_RETURN: case SDLK_SPACE: case SDLK_X: case SDLK_K: key = ev->key.repeat ? -1 : 2; break;
                case SDLK_ESCAPE: case SDLK_BACKSPACE: case SDLK_F1: case SDLK_Z: case SDLK_J:
                    key = ev->key.repeat ? -1 : 3;
                    break;
            }
        } else if (ev->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            switch (UiPadButton(ev)) {
                case SDL_GAMEPAD_BUTTON_DPAD_UP: key = 0; break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN: key = 1; break;
                case SDL_GAMEPAD_BUTTON_SOUTH: key = 2; break;
                case SDL_GAMEPAD_BUTTON_EAST: case SDL_GAMEPAD_BUTTON_START: case SDL_GAMEPAD_BUTTON_BACK:
                case SDL_GAMEPAD_BUTTON_GUIDE:
                    key = 3;
                    break;
            }
        }
        if (key >= 0)
            ConfirmKey(key);
        return ev->type == SDL_EVENT_KEY_DOWN || ev->type == SDL_EVENT_KEY_UP ||
               ev->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || ev->type == SDL_EVENT_GAMEPAD_BUTTON_UP;
    }
    if (sPage == PAGE_ACHIEVEMENTS) {
        int click = 0;
        if (ev->type == SDL_EVENT_KEY_DOWN) {
            SDL_Keycode key = ev->key.key;
            click = key == SDLK_UP || key == SDLK_DOWN || key == SDLK_LEFT || key == SDLK_RIGHT ||
                    key == SDLK_W || key == SDLK_S || key == SDLK_A || key == SDLK_D || key == '\t';
        } else if (ev->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            int button = ev->gbutton.button;
            click = button == SDL_GAMEPAD_BUTTON_DPAD_UP || button == SDL_GAMEPAD_BUTTON_DPAD_DOWN ||
                    button == SDL_GAMEPAD_BUTTON_DPAD_LEFT || button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT ||
                    button == SDL_GAMEPAD_BUTTON_NORTH;
        }
        if (click) { extern void PortMenuSoundClick(void); PortMenuSoundClick(); }
        return AchievementMenuInput(ev);
    }
    if (sPage == PAGE_LOBBY && (ev->type == SDL_EVENT_KEY_DOWN || ev->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)) {
        int key = -1;
        if (ev->type == SDL_EVENT_KEY_DOWN) {
            switch (ev->key.key) {
                case SDLK_UP: case SDLK_W: key = 0; break;
                case SDLK_DOWN: case SDLK_S: key = 1; break;
                case SDLK_RETURN: case SDLK_SPACE: case SDLK_X: case SDLK_K: key = ev->key.repeat ? -1 : 3; break;
                case SDLK_ESCAPE: case SDLK_BACKSPACE: case SDLK_Z: case SDLK_J: key = ev->key.repeat ? -1 : 4; break;
            }
        } else {
            switch (UiPadButton(ev)) {
                case SDL_GAMEPAD_BUTTON_DPAD_UP: key = 0; break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN: key = 1; break;
                case SDL_GAMEPAD_BUTTON_SOUTH: case SDL_GAMEPAD_BUTTON_START: key = 3; break;
                case SDL_GAMEPAD_BUTTON_EAST: key = 4; break;
            }
        }
        if (key >= 0)
            LobbyKey(key);
        return 1;
    }
    if (sPage == PAGE_NEWGAME && (ev->type == SDL_EVENT_KEY_DOWN || ev->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)) {
        int key = -1;
        if (ev->type == SDL_EVENT_KEY_DOWN) {
            switch (ev->key.key) {
                case SDLK_UP: case SDLK_W: key = 0; break;
                case SDLK_DOWN: case SDLK_S: key = 1; break;
                case SDLK_LEFT: case SDLK_RIGHT: case SDLK_A: case SDLK_D: key = 2; break;
                case SDLK_RETURN: case SDLK_SPACE: case SDLK_X: case SDLK_K: key = 3; break;
                case SDLK_ESCAPE: case SDLK_BACKSPACE: case SDLK_Z: case SDLK_J: key = ev->key.repeat ? -1 : 4; break;
            }
        } else {
            switch (UiPadButton(ev)) {
                case SDL_GAMEPAD_BUTTON_DPAD_UP: key = 0; break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN: key = 1; break;
                case SDL_GAMEPAD_BUTTON_DPAD_LEFT: case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: key = 2; break;
                case SDL_GAMEPAD_BUTTON_SOUTH: case SDL_GAMEPAD_BUTTON_START: key = 3; break;
                case SDL_GAMEPAD_BUTTON_EAST: key = 4; break;
            }
        }
        if (key >= 0)
            WizKey(key);
        return 1;
    }
    if (sPage == PAGE_CONTROLS && sControlCapture && sCursor >= ITEM_CONTROL_A && sCursor <= ITEM_CONTROL_UNSTOCK) {
        int action = sCursor - ITEM_CONTROL_A;
        if (ev->type == SDL_EVENT_KEY_DOWN && !ev->key.repeat) {
            if (ev->key.key == SDLK_ESCAPE || ev->key.key == SDLK_AC_BACK) sControlCapture = 0;
            else if (!sControlPad) { gPortConfig.keys[action][0] = ev->key.key == SDLK_DELETE ? 0 : (int)ev->key.key; sControlCapture = 0; PortConfigSave(); }
            else if (ev->key.key == SDLK_DELETE) { gPortConfig.pad[action] = -1; sControlCapture = 0; PortConfigSave(); }
            return 1;
        }
        if (sControlPad && ev->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) { gPortConfig.pad[action] = ev->gbutton.button; sControlCapture = 0; PortConfigSave(); return 1; }
        if (sControlPad && ev->type == SDL_EVENT_GAMEPAD_AXIS_MOTION && SDL_abs(ev->gaxis.value) > 16000) { gPortConfig.pad[action] = PORT_PAD_AXIS_BASE + ev->gaxis.axis * 2 + (ev->gaxis.value < 0); sControlCapture = 0; PortConfigSave(); return 1; }
        /* waiting for a key with no keyboard (issue #37): a gamepad button, a tap or a click cancels it */
        if (!sControlPad && (ev->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || ev->type == SDL_EVENT_MOUSE_BUTTON_DOWN || ev->type == SDL_EVENT_FINGER_DOWN))
            sControlCapture = 0;
        return 1;
    }
    if (ev->type == SDL_EVENT_KEY_DOWN) {
        switch (ev->key.key) {
            case SDLK_UP:
            case SDLK_W:
                Move(-1);
                break;
            case SDLK_DOWN:
            case SDLK_S:
                Move(1);
                break;
            case SDLK_LEFT:
            case SDLK_A:
                Change(sCursor, -1);
                break;
            case SDLK_RIGHT:
            case SDLK_D:
                Change(sCursor, 1);
                break;
            case SDLK_RETURN:
            case SDLK_SPACE:
            case SDLK_X:
            case SDLK_K:
                if (sCursor == ITEM_CLOSE)
                    Close();
                else if (sCursor == ITEM_FS_RES)
                    ConfirmFsRes();
                else if (sPage == PAGE_CONTROLS)
                    Change(sCursor, sCursor == ITEM_CONTROL_RESET ? 1 : 0);
                else
                    Change(sCursor, 1);
                break;
            case SDLK_ESCAPE:
            case SDLK_BACKSPACE:
            case SDLK_F1:
            case SDLK_Z:
            case SDLK_J:
                if (!ev->key.repeat) {
                    if (sPage == PAGE_NEWGAME)
                        Change(ITEM_NG_START, 1);
                    else if (sPage == PAGE_CHEATS)
                        Change(ITEM_CHEATS_BACK, 1);
                    else if (sPage == PAGE_REMATCH)
                        Change(ITEM_REMATCH_BACK, 1);
                    else if (sPage == PAGE_PS2)
                        Change(ITEM_PS2_BACK, 1);
                    else if (sPage == PAGE_CONTROLS)
                        Change(ITEM_CONTROLS_BACK, 1);
                    else
                        Close();
                }
                break;
        }
        return 1;
    }
    if (ev->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        switch (UiPadButton(ev)) {
            case SDL_GAMEPAD_BUTTON_DPAD_UP:
                Move(-1);
                break;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
                Move(1);
                break;
            case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
                Change(sCursor, -1);
                break;
            case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
                Change(sCursor, 1);
                break;
            case SDL_GAMEPAD_BUTTON_SOUTH:
                if (sCursor == ITEM_CLOSE)
                    Close();
                else if (sCursor == ITEM_FS_RES)
                    ConfirmFsRes();
                else if (sPage == PAGE_CONTROLS)
                    Change(sCursor, sCursor == ITEM_CONTROL_RESET ? 1 : 0);
                else
                    Change(sCursor, 1);
                break;
            case SDL_GAMEPAD_BUTTON_EAST:
                if (sPage == PAGE_NEWGAME) {
                    Change(ITEM_NG_START, 1);
                    break;
                }
                if (sPage == PAGE_CHEATS) { Change(ITEM_CHEATS_BACK, 1); break; }
                if (sPage == PAGE_REMATCH) { Change(ITEM_REMATCH_BACK, 1); break; }
                if (sPage == PAGE_PS2) { Change(ITEM_PS2_BACK, 1); break; }
                if (sPage == PAGE_CONTROLS) { Change(ITEM_CONTROLS_BACK, 1); break; }
                Close();
                break;
            case SDL_GAMEPAD_BUTTON_START:
            case SDL_GAMEPAD_BUTTON_BACK:
            case SDL_GAMEPAD_BUTTON_GUIDE:
                Close();
                break;
        }
        return 1;
    }
    if (ev->type == SDL_EVENT_KEY_UP || ev->type == SDL_EVENT_GAMEPAD_BUTTON_UP)
        return 1;
    return 0;
}

#define COL_TEXT 0xFFF0F4FFu
#define COL_SHADOW 0xFF202848u
#define COL_TITLE 0xFFF8D870u
#define COL_VALUE 0xFFA8D8FFu
#define COL_DIM 0xFF7880A0u
#define COL_PANEL 0xFF0C1030u
#define COL_BORDER 0xFFD8C080u
#define COL_SELECT 0xFF3050A8u

static const char* const sSkipText[LANG_COUNT] = {
    "Press again to skip the intro", "Appuyez encore pour passer l'intro", "Erneut drücken: Intro überspringen",
    "Premi di nuovo per saltare l'intro", "Pulsa otra vez para saltar la intro",
    "もう一度押すとイントロをスキップ",
};

void PortUiSkipPrompt(int visible) {
    sSkipPrompt = visible;
}

/* Copyright and logo screens (#24): A shows the prompt for 4 seconds, A again
   while it is up confirms. Called once per game frame; reset when leaving. */
static int sIntroSkipTimer;

int PortUiIntroSkip(int pressed) {
    if (pressed) {
        if (sIntroSkipTimer > 0) {
            PortUiIntroSkipReset();
            return 1;
        }
        sIntroSkipTimer = 4 * 60;
        PortUiSkipPrompt(1);
    } else if (sIntroSkipTimer > 0 && --sIntroSkipTimer == 0) {
        PortUiSkipPrompt(0);
    }
    return 0;
}

void PortUiIntroSkipReset(void) {
    sIntroSkipTimer = 0;
    PortUiSkipPrompt(0);
}

static void MenuButton(int x, int y, const char* text, int key);

/* Bottom right of the GBA picture: a debug-menu button with the text. */
static void DrawSkipPrompt(int l) {
    const char* text = sSkipText[l];
    int x = gPortScreen.originX + GBA_SCREEN_W - TextWidth(text) - 38;
    int y = gPortScreen.originY + GBA_SCREEN_H - 15;

    MenuButton(x, y, text, 'A');
}

static int sSkipOwnLayer;

void PortUiSkipOwnLayer(int on) {
    sSkipOwnLayer = on;
}

int PortUiDrawSkipLayer(uint32_t* frame, int width, int height, int rect[4]) {
    const char* text;
    int l = Lang();

    if (!sSkipPrompt || sOpen)
        return 0;
    text = sSkipText[l];
    sFb = frame;
    sFbW = width;
    sFbH = height;
    DrawSkipPrompt(l);
    /* MenuButton's shade: from x - 3, y - 2, the text and the A key */
    rect[0] = gPortScreen.originX + GBA_SCREEN_W - TextWidth(text) - 41;
    rect[1] = gPortScreen.originY + GBA_SCREEN_H - 17;
    rect[2] = TextWidth(text) + 26 + GlyphWidth('A') + 4;
    rect[3] = 14;
    return 1;
}

/* The state / slot notice, on a layer of its own above everything else (the
   presenter draws it last, at the window's scale): like the app's own HUD.
   Returns the width in GBA pixels, 0 when there is no notice. */
static char sFpsText[48];

void PortUiSetFps(float fps, float logic) {
    SDL_snprintf(sFpsText, sizeof(sFpsText), "%.1f FPS   %.1f Hz", fps, logic);
}

/* the online link's line (net_stats): round trip, input delay, the last rollback's frames */
static const char* NetLine(void) {
    extern int NetLinkStats(int* rttMs, int* delay);
    extern int PortRollbackLastDepth(void);
    static char text[64];
    int rtt, delay;
    if (!gPortConfig.netStats || !NetLinkStats(&rtt, &delay))
        return NULL;
    SDL_snprintf(text, sizeof(text), Lang() == LANG_ES ? "Red %d ms  retardo %d  rollback %d" :
                                                         "Net %d ms  delay %d  rollback %d",
                 rtt, delay, PortRollbackLastDepth());
    return text;
}

int PortUiDrawNotice(uint32_t* frame, int width, int height) {
    const char* text = sNotice;

    if (sNoticeFrames > 0)
        sNoticeFrames--;
    else if (gPortConfig.showFps)
        text = sFpsText;
    else if (!(text = NetLine()))
        return 0;
    sFb = frame;
    sFbW = width;
    sFbH = height;
    MenuButton(3, 2, text, 0);
    return SDL_min(TextWidth(text) + 26, width);
}

/* ---- the options screen, in the style of the cartridge's debug menu --------
   White, a list of bars on the left (a grey square and an underline; the
   current one black with a red square), and the debug menu's own picture of
   Sora (gDebugMenuBg*, decoded once from the ROM) at the bottom right. */

extern uint8_t gDebugMenuBgTiles[];
extern uint16_t gDebugMenuBgMap[];
extern uint16_t gDebugMenuBgPalette[];
void LZ77UnCompWram(const void* src, void* dst);

#define MENU_ART_X 114 /* the picture starts right of the debug menu's bars */
#define MENU_INK 0xFF080808u
#define MENU_GREY 0xFFB5ADADu
#define MENU_SHADE 0xFFB5B5B5u
#define MENU_RED 0xFFFF1000u
#define MENU_RED_DARK 0xFF9C1000u
#define MENU_DIM 0xFF9C9494u
/* Japanese glyphs fill the whole 16-pixel cell (Latin ones 10 rows): the
   title and the list start a little lower. */
#ifdef VERSION_JP
#define MENU_TITLE_Y 5
#define MENU_TOP 18
#else
#define MENU_TITLE_Y 2
#define MENU_TOP 14
#endif

static uint32_t sMenuArt[GBA_SCREEN_W * GBA_SCREEN_H];
static int sMenuArtReady;

static void DecodeMenuArt(void) {
    static uint8_t tiles[32768];
    static uint16_t map[1024];
    int x, y;

#ifdef VERSION_EU
    LZ77UnCompWram(gDebugMenuBgTiles, tiles);
    LZ77UnCompWram(gDebugMenuBgMap, map);
#else
    memcpy(tiles, gDebugMenuBgTiles, 23360);
    memcpy(map, gDebugMenuBgMap, sizeof(map));
#endif
    for (y = 0; y < GBA_SCREEN_H; y++) {
        for (x = 0; x < GBA_SCREEN_W; x++) {
            uint16_t e = map[(y >> 3) * 32 + (x >> 3)];
            int tx = (e & 0x400) ? 7 - (x & 7) : (x & 7);
            int ty = (e & 0x800) ? 7 - (y & 7) : (y & 7);
            uint8_t v = tiles[(e & 0x3FF) * 64 + ty * 8 + tx];
            uint16_t c = gDebugMenuBgPalette[v];
            uint32_t r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
            sMenuArt[y * GBA_SCREEN_W + x] = v == 0 ? 0xFFFFFFFFu
                : 0xFF000000u | ((r << 3 | r >> 2) << 16) | ((g << 3 | g >> 2) << 8) | (b << 3 | b >> 2);
        }
    }
    sMenuArtReady = 1;
}

static void Fill(int x0, int y0, int w, int h, uint32_t c) {
    int x, y;

    for (y = y0; y < y0 + h; y++)
        for (x = x0; x < x0 + w; x++)
            Pixel(x, y, c);
}

extern const uint16_t gDebugMenuCursorAnim0[];
extern const uint16_t* gDebugMenuCursorFrames[];
extern const uint8_t gDebugMenuCursorTiles[];
extern const uint16_t gDebugMenuCursorPalette[];
static Uint64 sMenuLightStart, sMenuLightDrawClock;
static int sMenuLightFocus = -1;

static Uint64 MenuLightNow(void) {
    return SDL_getenv("KHCOM_MENU_FRAMECLOCK") ? sMenuLightDrawClock : SDL_GetTicks() * 60 / 1000;
}

/* The cartridge's actual six-step cursor cycle (8 ticks per step), sampled
 * from its four sprite frames. Only the light's 5x5 face is copied: existing
 * square/bar/text dimensions stay unchanged. Frames and colours remain in ROM. */
static void MenuLight(int x, int y) {
    const uint16_t* anim = gDebugMenuCursorAnim0;
    int count = anim[2], total = 0, frame = 0;
    for (int i = 0; i < count; ++i) total += anim[4 + i * 2];
    int tick = total > 0 ? (int)((MenuLightNow() - sMenuLightStart) % (unsigned)total) : 0;
    for (int i = 0; i < count; ++i) {
        frame = anim[3 + i * 2];
        if (tick < anim[4 + i * 2]) break;
        tick -= anim[4 + i * 2];
    }
    if (frame < 0 || frame > 3) return;
    const uint16_t* record = gDebugMenuCursorFrames[frame];
    for (int i = 0; i < record[0]; ++i) {
        const uint16_t* obj = record + 1 + i * 3;
        if ((obj[0] & 0xC0FF) != 0 || (obj[1] & 0xC1FF) != 0x4000) continue;
        int tile = obj[2] & 1023;
        for (int yy = 0; yy < 5; ++yy)
            for (int xx = 0; xx < 5; ++xx) {
                int sx = xx + 4, sy = yy + 3;
                if (obj[1] & 0x1000) sx = 15 - sx;
                if (obj[1] & 0x2000) sy = 15 - sy;
                int address = (tile + (sy / 8) * 2 + sx / 8) * 32 + (sy & 7) * 4 + (sx & 7) / 2;
                int index = (gDebugMenuCursorTiles[address] >> ((sx & 1) * 4)) & 15;
                uint16_t c = gDebugMenuCursorPalette[index];
                unsigned r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
                Pixel(x + xx, y + yy, 0xFF000000u | ((r << 3 | r >> 2) << 16) |
                      ((g << 3 | g >> 2) << 8) | (b << 3 | b >> 2));
            }
        return;
    }
}

/* One row: t is the top of its square, the bar runs from x to x + w. */
static void MenuBar(int x, int t, int w, int selected) {
    if (!selected) {
        Fill(x, t, 10, 10, MENU_GREY);
        Fill(x, t + 8, w, 2, MENU_GREY);
        return;
    }
    Fill(x, t, 11, 10, MENU_INK);
    Fill(x + 11, t, 1, 10, MENU_SHADE);
    MenuLight(x + 3, t + 3);
    Fill(x, t + 8, w, 2, MENU_INK);
    Fill(x + 1, t + 10, w, 1, MENU_SHADE);
}

/* One selected row on its own (the skip prompt, notices, hints): the bar
   under the text on a white plate, as the debug menu shows it. */
static void MenuButton(int x, int y, const char* text, int key) {
    int keyW = key ? GlyphWidth(key) + 4 : 0;
    int w = TextWidth(text) + 20 + keyW;

    Shade(x - 3, y - 2, w + 6, 14, 0xFFFFFFFFu, 224);
    MenuBar(x, y, w, 1);
    if (key)
        DrawGlyph(key, x + 15, y - 2, MENU_RED_DARK, 0xFFD8D8D8u);
    DrawText(text, x + 16 + keyW, y - 2, MENU_INK, 0xFFD8D8D8u);
}

/* A value without its explanation in brackets, for the narrow list. */
static void ShortValue(int item, char* out, size_t size) {
    char* paren;

    ValueText(item, out, size);
    if (item == ITEM_ZOOM) return; /* Reference annotations are meaningful. */
    paren = SDL_strstr(out, " (");
    if (paren != NULL && paren != out)
        *paren = 0;
}

/* ---- the new game window: a few steps, as modern games ask on first start - */

static const char* const sWizAsk[6][LANG_COUNT] = {
    { "Which version do you want to play?", "Quelle version veux-tu jouer ?", "Welche Version möchtest du spielen?",
      "Quale versione vuoi giocare?", "¿Qué versión quieres jugar?", "どのバージョンで遊びますか？" },
    { "Difficulty", "Difficulté", "Schwierigkeit", "Difficoltà", "Dificultad", "難しさ" },
    { "Before you start", "Avant de commencer", "Bevor es losgeht", "Prima di iniziare", "Antes de empezar",
      "はじめる前に" },
    { "How do you want to play?", "Comment veux-tu jouer ?", "Wie möchtest du spielen?", "Come vuoi giocare?",
      "¿Cómo quieres jugar?", "どの見た目で遊びますか？" },
    { "Which GBA?", "Quel GBA ?", "Welcher GBA?", "Quale GBA?", "¿Qué GBA?", "どちらのGBA？" },
    { "New content ready!", "Nouveau contenu prêt !", "Neue Inhalte bereit!", "Nuovi contenuti pronti!",
      "¡Contenido nuevo listo!", "新しいコンテンツの準備ができました！" },
};
/* the graphics mode, asked once on the first start (PortUiOpenModeChoice); Options and the launcher change it later */
static const char* const sModeDescHd[LANG_COUNT] = {
    "Re:Chain of Memories' cards, logo, faces, music and movies (imported by the launcher from your Kingdom Hearts HD "
    "1.5+2.5), HD text and every improvement.",
    "Cartes, logo, portraits, musique et vidéos de Re:Chain of Memories (importés par le lanceur depuis ton Kingdom "
    "Hearts HD 1.5+2.5), texte HD et toutes les améliorations.",
    "Karten, Logo, Porträts, Musik und Videos aus Re:Chain of Memories (vom Launcher aus deinem Kingdom Hearts HD "
    "1.5+2.5 importiert), HD-Text und alle Verbesserungen.",
    "Carte, logo, ritratti, musica e video di Re:Chain of Memories (importati dal launcher dal tuo Kingdom Hearts HD "
    "1.5+2.5), testo HD e tutti i miglioramenti.",
    "Cartas, logo, retratos, música y videos de Re:Chain of Memories (los importa el launcher desde tu Kingdom Hearts "
    "HD 1.5+2.5), texto HD y todas las mejoras.",
    "Re:Chain of Memoriesのカード・ロゴ・顔・音楽・ムービー（ランチャーがKingdom Hearts HD 1.5+2.5から取り込み）、"
    "HDテキストとすべての改善。",
};
static const char* const sModeDescHdMissing[LANG_COUNT] = {
    "Not loaded yet: import it from the launcher (HD mod) and it turns on here.",
    "Pas encore chargé : importe-le depuis le lanceur (mod HD) et il s'active ici.",
    "Noch nicht geladen: im Launcher importieren (HD-Mod), dann hier einschaltbar.",
    "Non ancora caricato: importalo dal launcher (mod HD) e si attiva qui.",
    "Aún no está cargado: impórtalo desde el launcher (Mod HD) y se activa aquí.",
    "まだ読みこまれていません：ランチャー（HD MOD）で取りこむと、ここでオンにできます。",
};
static const char* const sModeDescGba[LANG_COUNT] = {
    "The cartridge's graphics. Then choose enhanced or classic.",
    "Les graphismes de la cartouche. Puis choisis amélioré ou classique.",
    "Die Grafik des Moduls. Danach verbessert oder klassisch wählen.",
    "La grafica della cartuccia. Poi scegli migliorato o classico.",
    "Los gráficos del cartucho. Después eliges mejorado o clásico.",
    "カートリッジのグラフィック。次に強化かクラシックを選びます。",
};
static const char* const sModeDescEnhanced[LANG_COUNT] = {
    "ReKHCoM's wide view and improvements, with the HD extras off.",
    "La vue large et les améliorations de ReKHCoM, sans les extras HD.",
    "ReKHCoMs Breitbild und Verbesserungen, ohne die HD-Extras.",
    "La vista larga e i miglioramenti di ReKHCoM, senza gli extra HD.",
    "La vista amplia y las mejoras de ReKHCoM, con los extras HD apagados.",
    "ReKHCoMのワイド表示と改善（HDの追加要素はオフ）。",
};
static const char* const sModeDescClassic[LANG_COUNT] = {
    "As on the console: the 240x160 picture at a whole scale with a decorative border, every improvement off. "
    "Almost an emulator, but native.",
    "Comme sur la console : l'image 240x160 à une échelle entière avec une bordure décorative, toutes les "
    "améliorations désactivées. Presque un émulateur, mais natif.",
    "Wie auf der Konsole: das 240x160-Bild in ganzzahliger Größe mit einem Zierrahmen, alle Verbesserungen aus. "
    "Fast ein Emulator, aber nativ.",
    "Come sulla console: l'immagine 240x160 a scala intera con una cornice decorativa, tutti i miglioramenti "
    "disattivati. Quasi un emulatore, ma nativo.",
    "Como en la consola: la imagen de 240x160 a escala entera con un borde decorativo y todas las mejoras apagadas. "
    "Casi un emulador, pero nativo.",
    "本体と同じ：240x160の画面をせいすう倍で表示し、まわりにかざりのわく。改善はすべてオフ。ほぼエミュレーター、でもネイティブ。",
};
static const char* const sWizWide[LANG_COUNT] = { "Wide view", "Vue large", "Breitbild", "Vista larga",
                                                  "Vista amplia", "ワイド表示" };
static const char* const sWizWideDesc[LANG_COUNT] = {
    "More of the world at the sides, as ReKHCoM shows it. Off: the GBA's 240x160 picture.",
    "Plus du monde sur les côtés, comme ReKHCoM le montre. Désactivé : l'image 240x160 du GBA.",
    "Mehr von der Welt an den Seiten, wie ReKHCoM sie zeigt. Aus: das 240x160-Bild des GBA.",
    "Più mondo ai lati, come lo mostra ReKHCoM. Disattivata: l'immagine 240x160 del GBA.",
    "Más mundo a los lados, como lo muestra ReKHCoM. Apagada: la imagen de 240x160 del GBA.",
    "ReKHCoMのように左右に世界を広く表示。オフ：GBAの240x160の画面。",
};
static const char* const sModeLater[LANG_COUNT] = {
    "You can change it later in Options or the launcher.", "Modifiable plus tard dans les Options ou le lanceur.",
    "Später in den Optionen oder im Launcher änderbar.", "Puoi cambiarlo poi nelle Opzioni o nel launcher.",
    "Puedes cambiarlo luego en Opciones o en el launcher.", "あとでオプションやランチャーで変更できます。",
};
static const char* const sWizConfirm[LANG_COUNT] = { "Confirm", "Confirmer", "Bestätigen", "Conferma", "Confirmar",
    "決定" };
static const char* const sWizHelp[LANG_COUNT] = {
    "\xE2\x86\x91\xE2\x86\x93 Select   A OK   B Back",
    "\xE2\x86\x91\xE2\x86\x93 Choisir   A OK   B Retour",
    "\xE2\x86\x91\xE2\x86\x93 Wählen   A OK   B Zurück",
    "\xE2\x86\x91\xE2\x86\x93 Scegli   A OK   B Indietro",
    "\xE2\x86\x91\xE2\x86\x93 Elegir   A Aceptar   B Atrás",
    "\xE2\x86\x91\xE2\x86\x93 選択   A 決定   B 戻る",
};

/* The first question has nothing before it: no "B Back". */
static const char* const sWizHelpFirst[LANG_COUNT] = {
    "\xE2\x86\x91\xE2\x86\x93 Select   A OK",
    "\xE2\x86\x91\xE2\x86\x93 Choisir   A OK",
    "\xE2\x86\x91\xE2\x86\x93 Wählen   A OK",
    "\xE2\x86\x91\xE2\x86\x93 Scegli   A OK",
    "\xE2\x86\x91\xE2\x86\x93 Elegir   A Aceptar",
    "\xE2\x86\x91\xE2\x86\x93 選択   A 決定",
};

/* the options step: HD text, the one-button controls, then the HD mod's music, movies, scenes and HUD, then Start.
   The HD mod's rows are always there so the player knows they exist; greyed and off until the launcher imports
   them (24/10). */
enum { WOPT_WIDE, WOPT_HDTEXT, WOPT_QOL, WOPT_HDMUSIC, WOPT_HDMOVIES, WOPT_HDSCENES, WOPT_HDHUD, WOPT_START };
static int WizOptions(int* kinds) {
    int n = 0;
    /* GBA classic starts with everything off: a new game still offers the wide view */
    if (gPortConfig.graphicsMode == PORT_MODE_GBA_CLASSIC)
        kinds[n++] = WOPT_WIDE;
    kinds[n++] = WOPT_HDTEXT;
    kinds[n++] = WOPT_QOL;
    kinds[n++] = WOPT_HDMUSIC;
    kinds[n++] = WOPT_HDMOVIES;
    kinds[n++] = WOPT_HDSCENES;
    kinds[n++] = WOPT_HDHUD;
    kinds[n++] = WOPT_START;
    return n;
}

/* 0: a row of the HD mod whose files are not imported, it cannot be turned on */
static int WizOptionAvailable(int kind) {
    switch (kind) {
        case WOPT_HDMUSIC: return PortAssetPresent(PORT_ASSET_HD_MUSIC);
        case WOPT_HDMOVIES: return PortAssetPresent(PORT_ASSET_HD_MOVIES);
        case WOPT_HDSCENES: return PortAssetPresent(PORT_ASSET_HD_SCENES);
        case WOPT_HDHUD: return PortAssetPresent(PORT_ASSET_HD_CARDS);
        default: return 1;
    }
}

/* The window after something new arrived (the HD mod's parts, the Japanese ROM of the undub): its name in the title,
   what it does, and the choice; several at once get a switch each, any combination (24/10). */
static const char* const sAssetTitle[PORT_ASSET_COUNT][LANG_COUNT] = {
    { "HD HUD loaded!", "ATH HD chargé !", "HD-HUD geladen!", "HUD HD caricato!", "¡HUD HD cargado!",
      "HD HUDを読みこみました！" },
    { "HD music loaded!", "Musique HD chargée !", "HD-Musik geladen!", "Musica HD caricata!", "¡Música HD cargada!",
      "HDミュージックを読みこみました！" },
    { "HD intros loaded!", "Intros HD chargées !", "HD-Intros geladen!", "Intro HD caricate!", "¡Intros HD cargadas!",
      "HDイントロを読みこみました！" },
    { "HD scenes loaded!", "Scènes HD chargées !", "HD-Szenen geladen!", "Scene HD caricate!", "¡Escenas HD cargadas!",
      "HDシーンを読みこみました！" },
    { "HD subtitles loaded!", "Sous-titres HD chargés !", "HD-Untertitel geladen!", "Sottotitoli HD caricati!",
      "¡Subtítulos HD cargados!", "HDじまくを読みこみました！" },
    { "Undub loaded!", "Undub chargé !", "Undub geladen!", "Undub caricato!", "¡Undub cargado!",
      "Undubを読みこみました！" },
};
static const char* const sAssetsTitleMany[LANG_COUNT] = { /* %d: how many */
    "%d new extras loaded!", "%d nouveautés chargées !", "%d neue Extras geladen!", "%d novità caricate!",
    "¡%d mejoras nuevas cargadas!", "新しい追加要素を%dつ読みこみました！",
};
static const char* const sAssetName[PORT_ASSET_COUNT][LANG_COUNT] = {
    { "HD HUD", "ATH HD", "HD-HUD", "HUD HD", "HUD HD", "HD HUD" },
    { "HD music", "Musique HD", "HD-Musik", "Musica HD", "Música HD", "HDミュージック" },
    { "HD intros", "Intros HD", "HD-Intros", "Intro HD", "Intros HD", "HDイントロ" },
    { "HD scenes", "Scènes HD", "HD-Szenen", "Scene HD", "Escenas HD", "HDシーン" },
    { "HD subtitles", "Sous-titres HD", "HD-Untertitel", "Sottotitoli HD", "Subtítulos HD", "HDじまく" },
    { "Japanese voices (Undub)", "Voix japonaises (Undub)", "Japanische Stimmen (Undub)", "Voci giapponesi (Undub)",
      "Voces japonesas (Undub)", "日本語ボイス (Undub)" },
};
static const char* const sAssetDesc[PORT_ASSET_COUNT][LANG_COUNT] = {
    { "Re:Chain of Memories' cards, faces, battle words and title logo, in HD.",
      "Les cartes, visages, mots de combat et le logo du titre de Re:Chain of Memories, en HD.",
      "Karten, Gesichter, Kampfwörter und Titellogo von Re:Chain of Memories, in HD.",
      "Carte, volti, parole di battaglia e logo del titolo di Re:Chain of Memories, in HD.",
      "Las cartas, caras, palabras de combate y el logo del título de Re:Chain of Memories, en HD.",
      "Re:チェイン オブ メモリーズのカード、顔、バトルの文字、タイトルロゴをHDで。" },
    { "Re:Chain of Memories' soundtrack in place of the cartridge's music.",
      "La bande-son de Re:Chain of Memories à la place de la musique de la cartouche.",
      "Der Soundtrack von Re:Chain of Memories statt der Musik des Moduls.",
      "La colonna sonora di Re:Chain of Memories al posto della musica della cartuccia.",
      "La banda sonora de Re:Chain of Memories en lugar de la música del cartucho.",
      "カートリッジの音楽の代わりにRe:チェイン オブ メモリーズのサウンドトラック。" },
    { "The opening, credits and endings as Re:Chain of Memories' HD movies.",
      "L'ouverture, le générique et les fins en vidéos HD de Re:Chain of Memories.",
      "Intro, Abspann und Enden als HD-Videos von Re:Chain of Memories.",
      "Apertura, titoli di coda e finali come video HD di Re:Chain of Memories.",
      "La intro, los créditos y los finales con los videos HD de Re:Chain of Memories.",
      "オープニング、クレジット、エンディングをRe:チェイン オブ メモリーズのHDムービーで。" },
    { "The story scenes as Re:Chain of Memories' HD movies, where one matches.",
      "Les scènes de l'histoire en vidéos HD de Re:Chain of Memories, quand il y en a une.",
      "Die Story-Szenen als HD-Videos von Re:Chain of Memories, wo es eins gibt.",
      "Le scene della storia come video HD di Re:Chain of Memories, dove ce n'è uno.",
      "Las escenas de la historia con los videos HD de Re:Chain of Memories, donde hay uno.",
      "ストーリーの場面を、対応するRe:チェイン オブ メモリーズのHDムービーで。" },
    { "Subtitles in your language for the HD movies and scenes.",
      "Des sous-titres dans ta langue pour les vidéos et scènes HD.",
      "Untertitel in deiner Sprache für die HD-Videos und -Szenen.",
      "Sottotitoli nella tua lingua per i video e le scene HD.",
      "Subtítulos en tu idioma para los videos y escenas HD.",
      "HDムービーと場面に、あなたの言語のじまく。" },
    { "The original Japanese voices, with the text in your language. From the next start of the game.",
      "Les voix japonaises d'origine, avec le texte dans ta langue. Au prochain lancement du jeu.",
      "Die originalen japanischen Stimmen, mit Text in deiner Sprache. Ab dem nächsten Spielstart.",
      "Le voci giapponesi originali, con il testo nella tua lingua. Dal prossimo avvio del gioco.",
      "Las voces japonesas originales, con el texto en tu idioma. Desde el próximo inicio del juego.",
      "オリジナルの日本語ボイスに、あなたの言語の文字。次にゲームを起動したときから。" },
};
static const char* const sAssetsWellDone[2][LANG_COUNT] = { /* one, several */
    { "Well done, it's ready!", "Bravo, c'est prêt !", "Gut gemacht, alles bereit!", "Ben fatto, è pronto!",
      "¡Bien hecho, ya está listo!", "おつかれさまでした、準備完了です！" },
    { "Well done, all ready! The ones set to On turn on now.",
      "Bravo, tout est prêt ! Ceux sur Activé s'activent maintenant.",
      "Gut gemacht, alles bereit! Die auf An gestellten werden jetzt eingeschaltet.",
      "Ben fatto, è tutto pronto! Si attivano quelli impostati su Sì.",
      "¡Bien hecho, ya está todo listo! Se activan los que dejes en Sí.",
      "おつかれさまでした、準備完了です！オンにしたものが今オンになります。" },
};
static const char* const sAssetsRows[3][LANG_COUNT] = {
    { "Turn it on now", "L'activer maintenant", "Jetzt einschalten", "Attivalo ora", "Activar ahora",
      "今すぐオンにする" },
    { "Not now", "Pas maintenant", "Jetzt nicht", "Non ora", "Ahora no", "今はしない" },
    { "Confirm", "Confirmer", "Bestätigen", "Conferma", "Confirmar", "決定" },
};
static const char* const sAssetsOffDesc[2][LANG_COUNT] = { /* one, several */
    { "It stays off; turn it on whenever you like in Options.",
      "Ça reste désactivé ; active-le quand tu veux dans les Options.",
      "Bleibt aus; jederzeit in den Optionen einschaltbar.",
      "Resta spento; attivalo quando vuoi nelle Opzioni.",
      "Queda apagado; actívalo cuando quieras en Opciones.",
      "オフのままです。オプションでいつでもオンにできます。" },
    { "They all stay off; turn them on whenever you like in Options.",
      "Tout reste désactivé ; active-les quand tu veux dans les Options.",
      "Alles bleibt aus; jederzeit in den Optionen einschaltbar.",
      "Restano tutti spenti; attivali quando vuoi nelle Opzioni.",
      "Quedan todos apagados; actívalos cuando quieras en Opciones.",
      "すべてオフのままです。オプションでいつでもオンにできます。" },
};

static int AssetCount(int mask) {
    int n = 0;
    for (; mask; mask >>= 1)
        n += mask & 1;
    return n;
}

/* the i-th new asset (several: one row each, then Confirm and Not now) */
static int AssetAt(int row) {
    int i;
    for (i = 0; i < PORT_ASSET_COUNT; i++)
        if (sNewAssets >> i & 1 && row-- == 0)
            return i;
    return -1;
}

/* the new assets' window answered: the chosen ones on, never asked again for any of them */
static void AssetsChosen(int mask) {
    if (mask) {
        if (mask >> PORT_ASSET_HD_CARDS & 1)
            gPortConfig.hdCards = gPortConfig.hdTitle = 1;
        if (mask >> PORT_ASSET_HD_MUSIC & 1)
            gPortConfig.hdMusic = 1;
        if (mask >> PORT_ASSET_HD_MOVIES & 1)
            gPortConfig.hdMovies = 1;
        if (mask >> PORT_ASSET_HD_SCENES & 1)
            gPortConfig.hdScenes = 1;
        if (mask >> PORT_ASSET_HD_SUBS & 1)
            gPortConfig.hdSubtitles = 1;
        if (mask >> PORT_ASSET_UNDUB & 1)
            gPortConfig.undub = 1; /* the Japanese voices from the next start, as in Options */
        PortConfigSave();
        PortVideoApplyConfig();
    }
    sNewAssets = 0;
    Close();
}

static int WizRows(void) {
    int kinds[9];
    if (sWizStep == WIZ_ASSETS && AssetCount(sNewAssets) > 1)
        return AssetCount(sNewAssets) + 2;
    return sWizStep == WIZ_VERSION || sWizStep >= WIZ_MODE ? 2 : sWizStep == WIZ_DIFFICULTY ? 3 : WizOptions(kinds);
}

/* the first start: the graphics mode's presets, then the game goes on */
static void ModeChosen(int mode) {
    PortApplyGraphicsMode(mode);
    PortConfigSave();
    PortVideoApplyConfig();
    Close();
}

static void WizFinish(void) {
    PortRecomSetActive(sNgRecom);
    Close();
    sCloseSound = 0; /* the new game begins: not a return from a menu */
}

/* key: 0 up, 1 down, 2 left/right (toggle), 3 A, 4 B */
static void WizKey(int key) {
    extern void PortMenuSoundClick(void);
    extern void PortMenuSoundDecide(int confirm);
    int rows = WizRows();
    /* the menus' own sounds, as in the options: the cursor's click, a toggle's click, A and B (23/10) */
    if (key == 0 || key == 1 || (key == 2 && (sWizStep == WIZ_OPTIONS || sWizStep == WIZ_ASSETS)))
        PortMenuSoundClick();
    else if (key == 3 || (key == 4 && (sWizStep == WIZ_MODE_GBA || sWizStep == WIZ_OPTIONS || sWizStep == WIZ_DIFFICULTY ||
                                         sWizStep == WIZ_ASSETS)))
        PortMenuSoundDecide(key == 3);
    switch (key) {
        case 0: sWizCursor = (sWizCursor + rows - 1) % rows; return;
        case 1: sWizCursor = (sWizCursor + 1) % rows; return;
        case 4:
            if (sWizStep == WIZ_ASSETS) { /* not now */
                AssetsChosen(0);
                return;
            }
            if (sWizStep == WIZ_MODE_GBA && !sModeNoHd) {
                sWizStep = WIZ_MODE;
                sWizCursor = 1;
            }
            if (sWizStep == WIZ_OPTIONS) {
                sWizStep = sNgRecom ? WIZ_DIFFICULTY : WIZ_VERSION;
                sWizCursor = sNgRecom ? (gPortConfig.ps2Difficulty == 1 ? 0 : gPortConfig.ps2Difficulty == 0 ? 1 : 2) : 0;
            } else if (sWizStep == WIZ_DIFFICULTY) {
                sWizStep = WIZ_VERSION;
                sWizCursor = 1;
            }
            return;
        default:
            break;
    }
    if (sWizStep == WIZ_MODE && key == 3) {
        if (sWizCursor == 0) {
            ModeChosen(PORT_MODE_HD);
        } else {
            sWizStep = WIZ_MODE_GBA;
            sWizCursor = 0;
        }
        return;
    }
    if (sWizStep == WIZ_MODE_GBA && key == 3) {
        ModeChosen(sWizCursor == 0 ? PORT_MODE_GBA_ENHANCED : PORT_MODE_GBA_CLASSIC);
        return;
    }
    if (sWizStep == WIZ_ASSETS) {
        int asset = AssetCount(sNewAssets) > 1 ? AssetAt(sWizCursor) : -1;
        if (asset >= 0) /* a switch: left/right or A */
            sAssetPick ^= 1 << asset;
        else if (key == 3 && AssetCount(sNewAssets) > 1) /* Confirm, then Not now */
            AssetsChosen(sWizCursor == rows - 2 ? sAssetPick : 0);
        else if (key == 3)
            AssetsChosen(sWizCursor == 0 ? sNewAssets : 0);
        return;
    }
    if (sWizStep == WIZ_VERSION && key == 3) {
        sNgRecom = sWizCursor == 1;
        /* choosing Re:CoM here is what turns the Re:CoM rules on; the options below stay the start's presets */
        if (sNgRecom)
            gPortConfig.ps2Mode = 1;
        sWizStep = sNgRecom ? WIZ_DIFFICULTY : WIZ_OPTIONS;
        sWizCursor = sNgRecom ? 1 : 0; /* Standard first */
    } else if (sWizStep == WIZ_DIFFICULTY && key == 3) {
        static const int pick[3] = { 1, 0, 2 }; /* rows: Beginner, Standard, Proud */
        gPortConfig.ps2Difficulty = pick[sWizCursor];
        sWizStep = WIZ_OPTIONS;
        sWizCursor = 0;
    } else if (sWizStep == WIZ_OPTIONS) {
        int kinds[9], toggle = key == 2 || key == 3;
        WizOptions(kinds);
        if (!WizOptionAvailable(kinds[sWizCursor]))
            toggle = 0; /* not imported: greyed, stays off */
        switch (kinds[sWizCursor]) {
            case WOPT_WIDE:
                if (toggle) {
                    gPortConfig.viewMode = gPortConfig.viewMode == PORT_VIEW_ORIGINAL ? PORT_VIEW_AUTO : PORT_VIEW_ORIGINAL;
                    PortVideoApplyConfig();
                }
                break;
            case WOPT_HDTEXT:
                if (toggle) {
                    gPortConfig.hdText = !gPortConfig.hdText;
                    PortVideoApplyConfig();
                }
                break;
            case WOPT_QOL:
                if (toggle)
                    PortSetQolButtons(!PortQolButtons());
                break;
            case WOPT_HDMUSIC:
                if (toggle)
                    gPortConfig.hdMusic = !gPortConfig.hdMusic;
                break;
            case WOPT_HDMOVIES:
                if (toggle)
                    gPortConfig.hdMovies = !gPortConfig.hdMovies;
                break;
            case WOPT_HDSCENES:
                if (toggle)
                    gPortConfig.hdScenes = !gPortConfig.hdScenes;
                break;
            case WOPT_HDHUD:
                if (toggle)
                    gPortConfig.hdTitle = gPortConfig.hdCards = !gPortConfig.hdCards;
                break;
            default:
                if (key == 3)
                    WizFinish();
                break;
        }
    }
}

/* A three-byte UTF-8 character (Japanese): a line may break before and after it, it has no spaces. */
static int WideChar(const char* s) {
    return ((unsigned char)*s & 0xF0) == 0xE0;
}

/* Japanese punctuation that never starts a line (、。）」): kept at the end of the line before. */
static int ClosingMark(const char* s) {
    return !SDL_strncmp(s, "\xE3\x80\x81", 3) || !SDL_strncmp(s, "\xE3\x80\x82", 3) ||
           !SDL_strncmp(s, "\xEF\xBC\x89", 3) || !SDL_strncmp(s, "\xE3\x80\x8D", 3);
}

/* The end of the next unit that is not broken: a word, or one Japanese character with its closing marks. */
static const char* WrapUnit(const char* q) {
    if (WideChar(q)) {
        q += 3;
        while (WideChar(q) && ClosingMark(q)) q += 3;
        return q;
    }
    while (*q && *q != ' ' && !WideChar(q)) q++;
    return q;
}

/* Text wrapped to maxW; returns the lines drawn (draw = 0: only measures). */
static int DrawWrapped(const char* text, int x, int y, int maxW, uint32_t color, int draw) {
    char line[200];
    const char* p = text;
    int lines = 0;
    while (*p) {
        const char* best = NULL;
        const char* q = p;
        int n;
        while (1) {
            const char* e = WrapUnit(q);
            n = (int)(e - p);
            if (n >= (int)sizeof(line)) break;
            SDL_memcpy(line, p, (size_t)n);
            line[n] = 0;
            if (TextWidth(line) > maxW && best != NULL) break;
            best = e;
            if (!*e) break;
            q = *e == ' ' ? e + 1 : e;
        }
        if (best == NULL) best = p + SDL_strlen(p);
        n = SDL_min((int)(best - p), (int)sizeof(line) - 1);
        SDL_memcpy(line, p, (size_t)n);
        line[n] = 0;
        if (draw) DrawText(line, x, y + lines * 11, color, 0xFFD8D8D8u);
        lines++;
        p = *best == ' ' ? best + 1 : best;
    }
    return lines;
}

static void DrawWizard(int l, int width, int height) {
    const char* labels[9];
    const char* subs[9];
    const char* values[9] = { NULL };
    int dim[9] = { 0 };
    static char missing[9][400];
    int rows = WizRows(), i, panelW = SDL_min(width - 24, 250), x0, y0, h, y, textW, later, compact;
    static const int order[3] = { 1, 0, 2 };

    const char* ask = sWizAsk[sWizStep][l];
    static char askBuf[120];
    if (sWizStep == WIZ_ASSETS) {
        static char on[600];
        int n = AssetCount(sNewAssets), one = AssetAt(0);
        if (n == 1) {
            ask = sAssetTitle[one][l];
            SDL_snprintf(on, sizeof(on), "%s %s", sAssetsWellDone[0][l], sAssetDesc[one][l]);
            labels[0] = sAssetsRows[0][l]; subs[0] = on;
            labels[1] = sAssetsRows[1][l]; subs[1] = sAssetsOffDesc[0][l];
        } else {
            SDL_snprintf(askBuf, sizeof(askBuf), sAssetsTitleMany[l], n);
            ask = askBuf;
            for (i = 0; i < n && i < 6; i++) {
                labels[i] = sAssetName[AssetAt(i)][l]; subs[i] = sAssetDesc[AssetAt(i)][l];
                values[i] = sOnOff[sAssetPick >> AssetAt(i) & 1][l];
            }
            labels[i] = sAssetsRows[2][l]; subs[i] = sAssetsWellDone[1][l];
            labels[i + 1] = sAssetsRows[1][l]; subs[i + 1] = sAssetsOffDesc[1][l];
        }
    } else if (sWizStep == WIZ_MODE) {
        static char hd[600];
        int imported = ItemVisible(ITEM_HD_CARDS);
        SDL_snprintf(hd, sizeof(hd), "%s%s%s", sModeDescHd[l], imported ? "" : " ", imported ? "" : sModeDescHdMissing[l]);
        labels[0] = sModeNames[PORT_MODE_HD][l]; subs[0] = hd;
        static const char* const gbaMode[LANG_COUNT] = { "GBA mode", "Mode GBA", "GBA-Modus", "Modalità GBA",
                                                         "Modo GBA", "GBAモード" };
        labels[1] = gbaMode[l]; subs[1] = sModeDescGba[l];
    } else if (sWizStep == WIZ_MODE_GBA) {
        labels[0] = sModeNames[PORT_MODE_GBA_ENHANCED][l]; subs[0] = sModeDescEnhanced[l];
        labels[1] = sModeNames[PORT_MODE_GBA_CLASSIC][l]; subs[1] = sModeDescClassic[l];
    } else if (sWizStep == WIZ_VERSION) {
        labels[0] = sVersions[0][l]; subs[0] = sNgDescOriginal[l];
        labels[1] = sVersions[1][l]; subs[1] = sNgDescRecom[l];
    } else if (sWizStep == WIZ_DIFFICULTY) {
        for (i = 0; i < 3; i++) {
            labels[i] = sDifficulty[order[i]][l];
            subs[i] = sNgDescDifficulty[order[i]][l];
        }
    } else {
        int kinds[9], n = WizOptions(kinds);
        for (i = 0; i < n; i++) {
            switch (kinds[i]) {
                case WOPT_WIDE:
                    labels[i] = sWizWide[l]; subs[i] = sWizWideDesc[l];
                    values[i] = sOnOff[gPortConfig.viewMode != PORT_VIEW_ORIGINAL][l];
                    break;
                case WOPT_HDTEXT:
                    labels[i] = sLabels[ITEM_NG_HDTEXT][l]; subs[i] = sNgDescHd[l];
                    values[i] = sOnOff[gPortConfig.hdText != 0][l];
                    break;
                case WOPT_QOL:
                    labels[i] = sLabels[ITEM_NG_QOL][l]; subs[i] = sNgDescQol[l];
                    values[i] = sOnOff[PortQolButtons() != 0][l];
                    break;
                case WOPT_HDMUSIC:
                    labels[i] = sLabels[ITEM_HD_MUSIC][l]; subs[i] = sNgDescHdMusic[l];
                    values[i] = sOnOff[gPortConfig.hdMusic != 0][l];
                    break;
                case WOPT_HDMOVIES:
                    labels[i] = sLabels[ITEM_HD_MOVIES][l]; subs[i] = sNgDescHdMovies[l];
                    values[i] = sOnOff[gPortConfig.hdMovies != 0][l];
                    break;
                case WOPT_HDSCENES:
                    labels[i] = sLabels[ITEM_HD_SCENES][l]; subs[i] = sNgDescHdScenes[l];
                    values[i] = sOnOff[gPortConfig.hdScenes != 0][l];
                    break;
                case WOPT_HDHUD:
                    labels[i] = sLabels[ITEM_HD_CARDS][l]; subs[i] = sNgDescHdHud[l];
                    values[i] = sOnOff[gPortConfig.hdCards != 0][l];
                    break;
                default:
                    labels[i] = sWizConfirm[l]; subs[i] = sNgDescStart[l];
                    break;
            }
            if (!WizOptionAvailable(kinds[i])) { /* greyed and off, with how to get it */
                dim[i] = 1;
                values[i] = sOnOff[0][l];
                SDL_snprintf(missing[i], sizeof(missing[i]), "%s %s", subs[i], sModeDescHdMissing[l]);
                subs[i] = missing[i];
            }
        }
    }
    textW = panelW - 30;
    /* measure, then a white window in the middle of a darkened screen; on the GBA's 160 lines (Original view) a
       window too tall shows only the chosen row's description */
    later = sWizStep >= WIZ_MODE ? DrawWrapped(sModeLater[l], 0, 0, panelW - 16, 0, 0) * 11 + 2 : 0;
    for (compact = 0; compact < 2; compact++) {
        h = 22;
        for (i = 0; i < rows; i++)
            h += 14 + (compact && i != sWizCursor ? 0 : DrawWrapped(subs[i], 0, 0, textW, 0, 0) * 11 + 4);
        h += 14 + later;
        if (h <= height - 4)
            break;
    }
    compact = compact > 0;
    x0 = (width - panelW) / 2;
    y0 = SDL_max(2, (height - h) / 2);
    {
        /* on the wide view's row the picture shows behind the window, so the change is seen */
        int kinds[9];
        /* what the screen will look like, behind the window: 1 the wide view (HD mode, GBA enhanced, the wide view's
           row), 2 the GBA's own 240x160 screen in the middle (GBA classic) */
        int preview = sWizStep == WIZ_OPTIONS && WizOptions(kinds) > sWizCursor && kinds[sWizCursor] == WOPT_WIDE ? 1
                    : sWizStep == WIZ_MODE ? 1 : sWizStep == WIZ_MODE_GBA ? 1 + sWizCursor : 0;
        if (preview) {
            /* a night sky fills the whole screen at any shape (user 24/10), so the width shows; on GBA classic it is the
               decorative border, dimmed, around the GBA's screen where the game puts it: the hole at (72, 28) of the
               384x216 border (port_main.c), the border as large as the screen lets it */
            int yy, xx, sx = 0, sy = 0, sw = width, sh = height, dim = preview == 2;
            if (dim) {
                int bw = SDL_min(width, height * 384 / 216), bh = bw * 216 / 384;
                sw = bw * GBA_SCREEN_W / 384; sh = bh * GBA_SCREEN_H / 216;
                sx = (width - bw) / 2 + bw * 72 / 384; sy = (height - bh) / 2 + bh * 28 / 216;
            }
            for (yy = 0; yy < height; yy++) {
                int t = yy * 255 / SDL_max(1, height - 1), in = yy >= sy && yy < sy + sh;
                uint32_t c = 0xFF000000u | (uint32_t)(16 + t * 32 / 255) << 16 | (uint32_t)(16 + t * 16 / 255) << 8 |
                             (uint32_t)(56 + t * 40 / 255);
                uint32_t d = 0xFF000000u | (c >> 1 & 0x7F7F7Fu); /* the border: the same sky at half light */
                if (!dim) { Fill(0, yy, width, 1, c); continue; }
                Fill(0, yy, width, 1, d);
                if (in) Fill(sx, yy, sw, 1, c);
            }
            /* stars on a fixed grid of 24-pixel cells, one at a hashed place in each: the same density at any size */
            for (yy = 0; yy < height; yy += 24)
                for (xx = 0; xx < width; xx += 24) {
                    uint32_t hsh = (uint32_t)(xx * 73856093u ^ yy * 19349663u);
                    int px = xx + (int)(hsh % 24), py = yy + (int)(hsh / 24 % 24);
                    int in = px >= sx && px < sx + sw && py >= sy && py < sy + sh;
                    if (hsh % 3 || px >= width || py >= height) continue; /* a third of the cells */
                    Pixel(px, py, !dim || in ? 0xFFC0C8F0u : 0xFF606478u);
                }
            if (dim) { /* the frame around the GBA's screen */
                Fill(sx - 2, sy - 2, sw + 4, 2, MENU_GREY); Fill(sx - 2, sy + sh, sw + 4, 2, MENU_GREY);
                Fill(sx - 2, sy, 2, sh, MENU_GREY); Fill(sx + sw, sy, 2, sh, MENU_GREY);
            }
        } else {
            Fill(0, 0, width, height, 0xFF000000u);
        }
    }
    Fill(x0 - 2, y0 - 2, panelW + 4, h + 4, MENU_GREY);
    Fill(x0, y0, panelW, h, 0xFFFFFFFFu);
    DrawText(ask, x0 + 8, y0 + 4, MENU_RED_DARK, 0xFFD8D8D8u);
    Fill(x0 + 8, y0 + 17, panelW - 16, 1, MENU_GREY);
    y = y0 + 22;
    for (i = 0; i < rows; i++) {
        int sel = i == sWizCursor;
        MenuBar(x0 + 8, y + 2, panelW - 16, sel);
        DrawText(labels[i], x0 + 24, y, dim[i] ? MENU_DIM : MENU_INK, 0xFFD8D8D8u);
        if (values[i]) {
            int vx = x0 + panelW - 12 - TextWidth(values[i]);
            if (sel && !dim[i]) {
                vx -= 8;
                DrawText("\xE2\x86\x90", vx - 10, y, MENU_RED_DARK, 0xFFD8D8D8u);
                DrawText("\xE2\x86\x92", vx + TextWidth(values[i]) + 2, y, MENU_RED_DARK, 0xFFD8D8D8u);
            }
            DrawText(values[i], vx, y, dim[i] ? MENU_DIM : MENU_INK, 0xFFD8D8D8u);
        }
        y += 14;
        if (!compact || sel)
            y += DrawWrapped(subs[i], x0 + 24, y, textW, sel && !dim[i] ? MENU_INK : MENU_DIM, 1) * 11 + 4;
    }
    if (sWizStep >= WIZ_MODE)
        DrawWrapped(sModeLater[l], x0 + 8, y0 + h - 14 - later, panelW - 16, MENU_DIM, 1);
    DrawText(sWizStep == WIZ_VERSION || sWizStep == WIZ_MODE || sWizStep == WIZ_ASSETS ? sWizHelpFirst[l] : sWizHelp[l],
             x0 + 8, y0 + h - 12,
             MENU_DIM, 0xFFFFFFFFu);
}

#include "port_control_diagram.inc"

/* Cheats > Debug mode > GBA map logic: while it is the current row a note
   over the picture explains the ReKHCoM change it turns off, with a diagram
   of each: the wide view, the GBA's 240x160 screen dashed inside it, and a
   field Heartless in the margin (kept by ReKHCoM, gone with the GBA logic). */
static const char* const sGbaMapNote[LANG_COUNT] = {
    "ReKHCoM keeps field Heartless active while the wide view shows them. On: the GBA logic is back and they "
    "vanish once past the original 240x160 screen, so in the wide view they pop out of sight. For purists.",
    "ReKHCoM garde les Heartless du terrain actifs tant que la vue large les montre. Activé : la logique GBA "
    "revient et ils disparaissent hors de l'écran 240x160 d'origine ; dans la vue large, ils s'effacent d'un coup. Pour les puristes.",
    "ReKHCoM lässt Herzlose auf dem Feld aktiv, solange die Breitbild-Ansicht sie zeigt. An: die GBA-Logik kehrt "
    "zurück und sie verschwinden außerhalb des 240x160-Bildschirms, in der Breitbild-Ansicht schlagartig. Für Puristen.",
    "ReKHCoM mantiene attivi gli Heartless sul campo finché la vista larga li mostra. Attivo: torna la logica GBA "
    "e spariscono fuori dallo schermo 240x160 originale, quindi nella vista larga svaniscono di colpo. Per i puristi.",
    "ReKHCoM mantiene activos a los sincorazón del campo mientras la vista ancha los muestra. Activado: vuelve la "
    "lógica del GBA y desaparecen al salir de la pantalla original de 240x160, así que en la vista ancha se esfuman "
    "de golpe. Para puristas.",
    "ReKHCoMでは ワイド表示に見える間 フィールドのハートレスが 動き続けます。 オンにすると GBAのロジックに戻り "
    "元の240x160画面の外で 消えるため ワイド表示では 突然消えて見えます。 こだわりたい人に。",
};
static const char* const sGbaMapNoteCaption[2][LANG_COUNT] = {
    { "ReKHCoM", "ReKHCoM", "ReKHCoM", "ReKHCoM", "ReKHCoM", "ReKHCoM" },
    { "GBA", "GBA", "GBA", "GBA", "GBA", "GBA" },
};

/* A field Heartless, 5x7 drawn at 2x: black, two yellow eyes; only a faint
   outline once it popped. */
static void NoteHeartless(int x, int y, int gone) {
    static const char* const rows[7] = { " # # ", "#####", "#o#o#", "#####", " ### ", "## ##", "#   #" };
    int r, c;
    for (r = 0; r < 7; r++)
        for (c = 0; c < 5; c++) {
            char k = rows[r][c];
            if (k == ' ')
                continue;
            if (gone)
                Shade(x + c * 2, y + r * 2, 2, 2, 0xFFFFFFFFu, 120);
            else
                Fill(x + c * 2, y + r * 2, 2, 2, k == 'o' ? 0xFFF8D020u : 0xFF181018u);
        }
}

static void NoteDiagram(int x, int y, int w, int h, int gba, int l) {
    int gw = w * 240 / 320, gh = h * 160 / 180, gx = x + (w - gw) / 2, gy = y + (h - gh) / 2, i;
    int hx = gx + gw + (x + w - gx - gw - 10) / 2, hy = gy + gh / 2 - 7;
    Fill(x - 1, y - 1, w + 2, h + 2, MENU_GREY);
    Fill(x, y, w, h, 0xFF987050u);      /* the wide view's margins */
    Fill(gx, gy, gw, gh, 0xFFC8A078u);  /* the GBA's own picture */
    for (i = 0; i < gw; i += 4) {       /* its edge, dashed red */
        Fill(gx + i, gy, 2, 1, MENU_RED);
        Fill(gx + i, gy + gh - 1, 2, 1, MENU_RED);
    }
    for (i = 0; i < gh; i += 4) {
        Fill(gx, gy + i, 1, 2, MENU_RED);
        Fill(gx + gw - 1, gy + i, 1, 2, MENU_RED);
    }
    Fill(gx + gw / 2 - 3, gy + gh / 2 - 6, 6, 12, 0xFFC82818u); /* Sora */
    NoteHeartless(hx, hy, gba);
    if (gba) { /* the pop: a small burst where it was */
        Fill(hx + 4, hy - 3, 2, 2, 0xFFFFFFFFu);
        Fill(hx - 3, hy + 6, 2, 2, 0xFFFFFFFFu);
        Fill(hx + 11, hy + 6, 2, 2, 0xFFFFFFFFu);
        Fill(hx + 4, hy + 15, 2, 2, 0xFFFFFFFFu);
    }
    DrawText(sGbaMapNoteCaption[gba][l], x + (w - TextWidth(sGbaMapNoteCaption[gba][l])) / 2, y + h + 2,
             MENU_INK, 0xFFD8D8D8u);
}

static void DrawGbaMapNote(int l, int width, int height, int left) {
    int panelW = SDL_min(190, width - left - 6), x0, y0, h, textW, dw, dh, lines;
    if (panelW < 120)
        return; /* no room beside the list: the label says enough */
    x0 = width - panelW - 6;
    textW = panelW - 16;
    lines = DrawWrapped(sGbaMapNote[l], 0, 0, textW, 0, 0);
    dw = (panelW - 24) / 2;
    dh = dw * 9 / 16;
    h = 22 + lines * 11 + 8 + dh + 16;
    y0 = SDL_max(MENU_TOP, (height - h) / 2 - 6);
    Fill(x0 - 2, y0 - 2, panelW + 4, h + 4, MENU_GREY);
    Fill(x0, y0, panelW, h, 0xFFFFFFFFu);
    DrawText(sLabels[ITEM_CHEAT_GBA_MAP][l], x0 + 8, y0 + 4, MENU_RED_DARK, 0xFFD8D8D8u);
    Fill(x0 + 8, y0 + 17, panelW - 16, 1, MENU_GREY);
    DrawWrapped(sGbaMapNote[l], x0 + 8, y0 + 21, textW, MENU_INK, 1);
    NoteDiagram(x0 + 8, y0 + 22 + lines * 11 + 8, dw, dh, 0, l);
    NoteDiagram(x0 + panelW - 8 - dw, y0 + 22 + lines * 11 + 8, dw, dh, 1, l);
}
/* Beside "Force Nintendo layout": what it changes (issue #23). */
static const char* const sNintendoNote[LANG_COUNT] = {
    "Nintendo pads (Switch Pro, Joy-Con, 8BitDo in Switch mode) have A and B, X and Y the other way round from Xbox "
    "pads. ReKHCoM detects them; turn this on if yours is taken for an Xbox pad (e.g. through Steam or Lutris). On: "
    "the button labelled A is the game's A, B is B, X is X and Y is Y. Your bindings above stay as they are.",
    "Les manettes Nintendo (Switch Pro, Joy-Con, 8BitDo en mode Switch) ont A et B, X et Y inversés par rapport à "
    "Xbox. ReKHCoM les détecte ; activez ceci si la vôtre est prise pour une manette Xbox (via Steam ou Lutris). "
    "Activé : le bouton marqué A est le A du jeu, B est B, X est X et Y est Y. Vos assignations ne changent pas.",
    "Nintendo-Controller (Switch Pro, Joy-Con, 8BitDo im Switch-Modus) haben A und B, X und Y umgekehrt zu Xbox. "
    "ReKHCoM erkennt sie; schalte dies ein, wenn deiner als Xbox-Controller gilt (z. B. über Steam oder Lutris). "
    "An: die Taste mit A ist das A des Spiels, B ist B, X ist X und Y ist Y. Deine Belegung bleibt gleich.",
    "I controller Nintendo (Switch Pro, Joy-Con, 8BitDo in modalità Switch) hanno A e B, X e Y invertiti rispetto "
    "a Xbox. ReKHCoM li riconosce; attivalo se il tuo viene preso per un controller Xbox (es. con Steam o Lutris). "
    "Attivo: il tasto con la A è la A del gioco, B è B, X è X e Y è Y. Le tue assegnazioni restano uguali.",
    "Los mandos de Nintendo (Switch Pro, Joy-Con, 8BitDo en modo Switch) tienen A y B, X e Y al revés que los de "
    "Xbox. ReKHCoM los detecta; actívalo si el tuyo se toma por un mando de Xbox (por ejemplo con Steam o Lutris). "
    "Activado: el botón rotulado A es la A del juego, B es B, X es X e Y es Y. Tus asignaciones no cambian.",
    "ニンテンドーのコントローラー（Switch Pro、ジョイコン、Switchモードの8BitDo）は Xboxとくらべて AとB、XとYが逆です。 "
    "ReKHCoMは自動で判別します。 Xboxのパッドと判定される場合（SteamやLutrisを通してなど）はオンに。 "
    "オン: Aと書かれたボタンがゲームのA、BはB、XはX、YはYになります。 わりあては変わりません。",
};

static void DrawNintendoNote(int l, int width, int height, int left) {
    int panelW = SDL_min(190, width - left - 6), x0, y0, h, textW, lines;
    if (panelW < 120)
        return;
    x0 = width - panelW - 6;
    textW = panelW - 16;
    lines = DrawWrapped(sNintendoNote[l], 0, 0, textW, 0, 0);
    h = 22 + lines * 11 + 6;
    y0 = SDL_max(MENU_TOP, (height - h) / 2 - 6);
    Fill(x0 - 2, y0 - 2, panelW + 4, h + 4, MENU_GREY);
    Fill(x0, y0, panelW, h, 0xFFFFFFFFu);
    DrawText(sLabels[ITEM_PAD_LAYOUT][l], x0 + 8, y0 + 4, MENU_RED_DARK, 0xFFD8D8D8u);
    Fill(x0 + 8, y0 + 17, panelW - 16, 1, MENU_GREY);
    DrawWrapped(sNintendoNote[l], x0 + 8, y0 + 21, textW, MENU_INK, 1);
}
#include "port_achievement_menu.inc"

/* ---- the online lobby ---------------------------------------------------------------------------------------- */
int PortLobbyBackend(void);
int PortLobbyReady(void);
void PortLobbyRefresh(void);
void PortLobbyFrame(void);
int PortLobbyRoomCount(void);
const char* PortLobbyRoomName(int i);
int PortLobbyRoomRegion(int i);
int PortLobbyRegion(void);
const char* PortLobbyPlayerName(void);
const char* PortLobbyMyCode(void);
void PortLobbyCreate(void);
void PortLobbyJoin(int i);
void PortLobbyCable(void);

enum { LROW_CREATE = -1, LROW_REFRESH = -2, LROW_RETRY = -3, LROW_CABLE = -4 }; /* >= 0: a room */
static const char* const sLobbyTitle[LANG_COUNT] = { "Online battle", "Combat en ligne", "Online-Kampf",
                                                     "Battaglia online", "Batalla en línea", "Online battle" };
static const char* const sLobbyCreate[LANG_COUNT] = { "Open a public room", "Ouvrir une salle publique",
                                                      "Öffentlichen Raum öffnen", "Apri una stanza pubblica",
                                                      "Abrir una sala pública", "Open a public room" };
static const char* const sLobbyRefresh[LANG_COUNT] = { "Refresh", "Actualiser", "Aktualisieren", "Aggiorna",
                                                       "Actualizar", "Refresh" };
static const char* const sLobbyRetry[LANG_COUNT] = { "Try again", "Réessayer", "Erneut versuchen", "Riprova",
                                                     "Reintentar", "Try again" };
static const char* const sLobbyCable[LANG_COUNT] = { "Play by cable (local)", "Jouer par câble (local)",
                                                     "Per Kabel spielen (lokal)", "Gioca via cavo (locale)",
                                                     "Jugar por cable (local)", "Play by cable (local)" };
static const char* const sLobbyRoom[LANG_COUNT] = { "%s's room", "Salle de %s", "Raum von %s", "Stanza di %s",
                                                    "Sala de %s", "%s's room" };
static const char* const sLobbyLogin[LANG_COUNT] = {
    "Open Steam and sign in to play online.", "Ouvrez Steam et connectez-vous pour jouer en ligne.",
    "Öffne Steam und melde dich an, um online zu spielen.", "Apri Steam e accedi per giocare online.",
    "Abre Steam e inicia sesión para jugar en línea.", "Open Steam and sign in to play online." };
static const char* const sLobbyNoServer[LANG_COUNT] = {
    "The room server does not answer.", "Le serveur de salles ne répond pas.", "Der Raumserver antwortet nicht.",
    "Il server delle stanze non risponde.", "El servidor de salas no responde.", "The room server does not answer." };
static const char* const sLobbyAsking[LANG_COUNT] = { "Looking for rooms...", "Recherche de salles...",
                                                      "Suche nach Räumen...", "Ricerca di stanze...",
                                                      "Buscando salas...", "Looking for rooms..." };
static const char* const sLobbyEmpty[LANG_COUNT] = {
    "No public rooms right now: open one.", "Aucune salle publique : ouvrez-en une.",
    "Gerade keine öffentlichen Räume: öffne einen.", "Nessuna stanza pubblica: aprine una.",
    "No hay salas públicas ahora: abre una.", "No public rooms right now: open one." };
static const char* const sLobbyAs[LANG_COUNT] = { "Playing as %s", "Joueur : %s", "Spieler: %s", "Giocatore: %s",
                                                  "Jugando como %s", "Playing as %s" };
static const char* const sLobbyOther[LANG_COUNT] = { "another version", "autre version", "andere Version",
                                                     "altra versione", "otra versión", "another version" };
static const char* const sLobbyHelp[LANG_COUNT] = { "Confirm: choose   Back: cable", "Valider : choisir   Retour : câble",
                                                    "Bestätigen: wählen   Zurück: Kabel", "Conferma: scegli   Indietro: cavo",
                                                    "Confirmar: elegir   Atrás: cable", "Confirm: choose   Back: cable" };
static const char* const sLobbyOpened[LANG_COUNT] = { "Room open: waiting for a rival", "Salle ouverte : en attente",
                                                      "Raum offen: warte auf Gegner", "Stanza aperta: in attesa",
                                                      "Sala abierta: esperando rival", "Room open: waiting for a rival" };

/* the rows: open a room, the rooms, refresh, the cable; signed out or no server: try again, the cable */
static int LobbyRows(int* rows) {
    int n = 0, i, ready = PortLobbyReady(), count = PortLobbyRoomCount();
    if (ready < 0) {
        rows[n++] = LROW_RETRY;
        rows[n++] = LROW_CABLE;
        return n;
    }
    rows[n++] = LROW_CREATE;
    for (i = 0; i < count && n < 10; i++)
        rows[n++] = i;
    rows[n++] = LROW_REFRESH;
    rows[n++] = LROW_CABLE;
    return n;
}

static void LobbyKey(int key) {
    int rows[16], n = LobbyRows(rows), row;
    sLobbyCursor = SDL_clamp(sLobbyCursor, 0, n - 1);
    if (key == 0 || key == 1) {
        sLobbyCursor = (sLobbyCursor + (key == 0 ? n - 1 : 1)) % n;
        return;
    }
    if (key == 4) {
        PortLobbyCable();
        Close();
        return;
    }
    if (key != 3)
        return;
    row = rows[sLobbyCursor];
    if (row == LROW_CABLE) {
        PortLobbyCable();
        Close();
    } else if (row == LROW_REFRESH || row == LROW_RETRY) {
        PortLobbyRefresh();
    } else if (row == LROW_CREATE) {
        PortLobbyCreate();
        if (PortLobbyMyCode()[0])
            PortUiLinkNotice("Room %s open: waiting for a rival", "Sala %s abierta: esperando rival", PortLobbyMyCode());
        else
            PortUiLinkNotice(sLobbyOpened[0], sLobbyOpened[4], NULL);
        Close();
    } else if (row >= 0 && PortLobbyRoomRegion(row) == PortLobbyRegion()) {
        PortLobbyJoin(row);
        Close();
    }
}

static void DrawLobby(int l, int width, int height) {
    int rows[16], n, i, panelW = SDL_min(width - 24, 250), x0, y0, h, y, ready, count;
    char status[160], label[96];
    PortLobbyFrame();
    n = LobbyRows(rows);
    sLobbyCursor = SDL_clamp(sLobbyCursor, 0, n - 1);
    ready = PortLobbyReady();
    count = PortLobbyRoomCount();
    if (ready == -1)
        SDL_strlcpy(status, sLobbyLogin[l], sizeof(status));
    else if (ready == -2)
        SDL_strlcpy(status, sLobbyNoServer[l], sizeof(status));
    else if (count < 0)
        SDL_strlcpy(status, sLobbyAsking[l], sizeof(status));
    else if (count == 0)
        SDL_strlcpy(status, sLobbyEmpty[l], sizeof(status));
    else
        SDL_snprintf(status, sizeof(status), sLobbyAs[l], PortLobbyPlayerName());
    h = 22 + DrawWrapped(status, 0, 0, panelW - 16, 0, 0) * 11 + 4 + n * 14 + 16;
    x0 = (width - panelW) / 2;
    y0 = SDL_max(2, (height - h) / 2);
    Fill(0, 0, width, height, 0xFF000000u);
    Fill(x0 - 2, y0 - 2, panelW + 4, h + 4, MENU_GREY);
    Fill(x0, y0, panelW, h, 0xFFFFFFFFu);
    DrawText(sLobbyTitle[l], x0 + 8, y0 + 4, MENU_RED_DARK, 0xFFD8D8D8u);
    Fill(x0 + 8, y0 + 17, panelW - 16, 1, MENU_GREY);
    y = y0 + 22;
    y += DrawWrapped(status, x0 + 8, y, panelW - 16, ready < 0 ? MENU_RED_DARK : MENU_DIM, 1) * 11 + 4;
    for (i = 0; i < n; i++) {
        int sel = i == sLobbyCursor, row = rows[i], other = 0;
        const char* text = label;
        if (row == LROW_CREATE) text = sLobbyCreate[l];
        else if (row == LROW_REFRESH) text = sLobbyRefresh[l];
        else if (row == LROW_RETRY) text = sLobbyRetry[l];
        else if (row == LROW_CABLE) text = sLobbyCable[l];
        else {
            static const char* const regions[3] = { "EU", "US", "JP" };
            int region = PortLobbyRoomRegion(row);
            other = region != PortLobbyRegion();
            SDL_snprintf(label, sizeof(label), sLobbyRoom[l], PortLobbyRoomName(row));
            if (other)
                SDL_snprintf(label + SDL_strlen(label), sizeof(label) - SDL_strlen(label), " (%s, %s)",
                             region >= 0 && region < 3 ? regions[region] : "?", sLobbyOther[l]);
        }
        MenuBar(x0 + 8, y + 2, panelW - 16, sel);
        DrawText(text, x0 + 24, y, other ? MENU_DIM : MENU_INK, 0xFFD8D8D8u);
        y += 14;
    }
    DrawText(sLobbyHelp[l], x0 + 8, y0 + h - 12, MENU_DIM, 0xFFFFFFFFu);
}

/* ---- the big confirmation over Options: back to the title, or a mode's presets (asked twice) ---- */
static const char* const sConfirmTitleAsk[LANG_COUNT] = {
    "Return to the title screen?", "Retour à l'écran titre ?", "Zurück zum Titelbildschirm?",
    "Tornare alla schermata del titolo?", "¿Volver a la pantalla de título?", "タイトル画面に戻りますか？",
};
static const char* const sConfirmSure[LANG_COUNT] = {
    "Are you sure?", "Tu es sûr ?", "Bist du sicher?", "Sei sicuro?", "¿Seguro?", "本当によいですか？",
};
static const char* const sConfirmTitleWarn[2][LANG_COUNT] = {
    { "Anything not saved since your last save is lost.",
      "Tout ce qui n'a pas été sauvegardé depuis ta dernière sauvegarde sera perdu.",
      "Alles, was seit dem letzten Speichern nicht gesichert wurde, geht verloren.",
      "Tutto ciò che non hai salvato dall'ultimo salvataggio andrà perso.",
      "Se pierde todo lo que no guardaste desde tu última partida guardada.",
      "最後にセーブしてからの進行は失われます。" },
    { "Last confirmation: unsaved progress cannot be recovered.",
      "Dernière confirmation : la progression non sauvegardée ne pourra pas être récupérée.",
      "Letzte Bestätigung: nicht gespeicherter Fortschritt lässt sich nicht wiederherstellen.",
      "Ultima conferma: i progressi non salvati non si potranno recuperare.",
      "Confirmación final: lo que no guardaste no se puede recuperar.",
      "最終確認：セーブしていない進行はもどせません。" },
};
static const char* const sConfirmTitleRows[2][LANG_COUNT] = {
    { "No, keep playing", "Non, continuer", "Nein, weiterspielen", "No, continua a giocare", "No, seguir jugando",
      "いいえ、つづける" },
    { "Yes, return to the title", "Oui, retour au titre", "Ja, zum Titel", "Sì, torna al titolo", "Sí, volver al título",
      "はい、タイトルへ" },
};
static const char* const sConfirmModeAsk[LANG_COUNT] = {
    "Which preset do you want to go back to?", "Revenir à quel préréglage ?", "Zu welcher Voreinstellung zurück?",
    "A quale preimpostazione tornare?", "¿A qué preset quieres volver?", "どのプリセットに戻しますか？",
};
static const char* const sConfirmModeReset[LANG_COUNT] = { /* %s: the mode */
    "Reset the options to %s?", "Remettre les options en %s ?", "Optionen auf %s zurücksetzen?",
    "Riportare le opzioni a %s?", "¿Restablecer las opciones a %s?", "オプションを%sに戻しますか？",
};
static const char* const sConfirmModeWarn[3][LANG_COUNT] = {
    { "The same choices as on the first start.", "Les mêmes choix qu'au premier démarrage.",
      "Dieselbe Auswahl wie beim ersten Start.", "Le stesse scelte del primo avvio.",
      "Las mismas opciones del primer inicio.", "初回起動と同じえらびかたです。" },
    { "Your current settings are replaced by this mode's presets. Your saves are not touched.",
      "Tes réglages actuels sont remplacés par ceux de ce mode. Tes sauvegardes ne sont pas touchées.",
      "Deine Einstellungen werden durch die dieses Modus ersetzt. Spielstände bleiben unberührt.",
      "Le impostazioni attuali vengono sostituite da quelle di questa modalità. I salvataggi restano intatti.",
      "Tus ajustes actuales se reemplazan por los de este modo. Tus partidas no se tocan.",
      "今のオプションはこのモードの初期値に置き換わります。セーブデータはそのままです。" },
    { "Last confirmation: your current settings cannot be brought back.",
      "Dernière confirmation : tes réglages actuels ne pourront pas être rétablis.",
      "Letzte Bestätigung: die aktuellen Einstellungen lassen sich nicht zurückholen.",
      "Ultima conferma: le impostazioni attuali non si potranno ripristinare.",
      "Confirmación final: tus ajustes actuales no se pueden recuperar.",
      "最終確認：今のオプションにはもどせません。" },
};
static const char* const sConfirmModeRows[2][LANG_COUNT] = {
    { "No", "Non", "Nein", "No", "No", "いいえ" },
    { "Yes, reset", "Oui, rétablir", "Ja, zurücksetzen", "Sì, ripristina", "Sí, restablecer", "はい、戻す" },
};
/* Cheats > Quick states: one window, "No" first */
static const char* const sConfirmStatesAsk[LANG_COUNT] = {
    "Turn on quick states?", "Activer les états rapides ?", "Schnellstände einschalten?", "Attivare gli stati rapidi?",
    "¿Activar los estados rápidos?", "クイックステートをオンにしますか？",
};
static const char* const sConfirmStatesWarn[LANG_COUNT] = {
    "Only for testing or reproducing bugs. While they are on, no achievements unlock until you turn them off.",
    "Uniquement pour tester ou reproduire des bugs. Tant qu'ils sont actifs, aucun succès ne se débloque.",
    "Nur zum Testen oder Nachstellen von Fehlern. Solange sie an sind, werden keine Erfolge freigeschaltet.",
    "Solo per test o per riprodurre bug. Finché sono attivi non si sblocca nessun obiettivo.",
    "Solo para pruebas o para replicar errores. Mientras estén activos no se desbloquean logros.",
    "テストやバグのさいげんのためだけです。オンのあいだはじっせきがかいじょされません。",
};
static const char* const sConfirmStatesRows[2][LANG_COUNT] = {
    { "No", "Non", "Nein", "No", "No", "いいえ" },
    { "Yes, turn them on", "Oui, activer", "Ja, einschalten", "Sì, attiva", "Sí, activarlos", "はい、オンにする" },
};
static const char* const sConfirmCancel[LANG_COUNT] = { "Cancel", "Annuler", "Abbrechen", "Annulla", "Cancelar",
                                                        "やめる" };

/* the mode list: the three modes and Cancel; the yes/no steps: "No" first, a reset is never one press away */
static int ConfirmRows(void) {
    return sConfirm == CONFIRM_MODE && sConfirmStep == 0 ? 4 : 2;
}

/* the HD mode without anything of the HD mod imported: greyed, as the first start does not offer it */
static int ConfirmRowOff(int row) {
    return sConfirm == CONFIRM_MODE && sConfirmStep == 0 && row == 0 && !HdAnyImported();
}

static void ConfirmOpen(int what) {
    sConfirm = what;
    sConfirmStep = 0;
    sConfirmCursor = 0;
    if (what == CONFIRM_MODE) {
        sConfirmCursor = SDL_clamp(gPortConfig.graphicsMode, PORT_MODE_HD, PORT_MODE_GBA_CLASSIC) - PORT_MODE_HD;
        if (ConfirmRowOff(sConfirmCursor))
            sConfirmCursor = 1;
    }
}

/* 0 up, 1 down, 2 A, 3 B */
static void ConfirmKey(int key) {
    extern void PortMenuSoundClick(void);
    extern void PortMenuSoundDecide(int confirm);
    int rows = ConfirmRows(), last = sConfirm == CONFIRM_MODE ? 2 : sConfirm == CONFIRM_STATES ? 0 : 1;
    if (key == 0 || key == 1) {
        int c = sConfirmCursor;
        do
            c = (c + rows + (key ? 1 : -1)) % rows;
        while (ConfirmRowOff(c));
        if (c != sConfirmCursor) {
            sConfirmCursor = c;
            PortMenuSoundClick();
        }
        return;
    }
    PortMenuSoundDecide(key == 2);
    if (key == 3) { /* back a step, or out */
        if (sConfirmStep == 0)
            sConfirm = CONFIRM_NONE;
        else if (--sConfirmStep == 0 && sConfirm == CONFIRM_MODE)
            sConfirmCursor = sConfirmMode - PORT_MODE_HD;
        else
            sConfirmCursor = 0;
        return;
    }
    if (sConfirm == CONFIRM_MODE && sConfirmStep == 0) {
        if (sConfirmCursor == 3) {
            sConfirm = CONFIRM_NONE;
        } else {
            sConfirmMode = PORT_MODE_HD + sConfirmCursor;
            sConfirmStep = 1;
            sConfirmCursor = 0;
        }
        return;
    }
    if (sConfirmCursor == 0) { /* "No" */
        sConfirm = CONFIRM_NONE;
        return;
    }
    if (sConfirmStep < last) {
        sConfirmStep++;
        sConfirmCursor = 0;
        return;
    }
    if (sConfirm == CONFIRM_STATES) {
        sConfirm = CONFIRM_NONE;
        gPortConfig.cheatSaveStates = 1;
        PortConfigSave();
        return;
    }
    if (sConfirm == CONFIRM_TITLE) {
        sConfirm = CONFIRM_NONE;
        sTitleRequest = 1;
        Close();
        return;
    }
    sConfirm = CONFIRM_NONE;
    PortApplyGraphicsMode(sConfirmMode);
    PortConfigSave();
    PortVideoApplyConfig();
}

/* a white window in the middle of the darkened options, as the scene pause: the question, a red warning, the rows */
static void DrawConfirm(int l, int width, int height) {
    const char* labels[4];
    const char* subs[4] = { NULL, NULL, NULL, NULL };
    const char* warn;
    char ask[200];
    static char hd[600];
    int rows = ConfirmRows(), i, panelW = SDL_min(width - 24, 250), textW = panelW - 30, x0, y0, h, y, warnH, compact;

    if (sConfirm == CONFIRM_MODE && sConfirmStep == 0) {
        int imported = HdAnyImported();
        SDL_snprintf(hd, sizeof(hd), "%s%s%s", sModeDescHd[l], imported ? "" : " ", imported ? "" : sModeDescHdMissing[l]);
        labels[0] = sModeNames[PORT_MODE_HD][l]; subs[0] = hd;
        labels[1] = sModeNames[PORT_MODE_GBA_ENHANCED][l]; subs[1] = sModeDescEnhanced[l];
        labels[2] = sModeNames[PORT_MODE_GBA_CLASSIC][l]; subs[2] = sModeDescClassic[l];
        labels[3] = sConfirmCancel[l];
        SDL_strlcpy(ask, sConfirmModeAsk[l], sizeof(ask));
        warn = sConfirmModeWarn[0][l];
    } else if (sConfirm == CONFIRM_STATES) {
        labels[0] = sConfirmStatesRows[0][l];
        labels[1] = sConfirmStatesRows[1][l];
        SDL_strlcpy(ask, sConfirmStatesAsk[l], sizeof(ask));
        warn = sConfirmStatesWarn[l];
    } else {
        int last = sConfirmStep == (sConfirm == CONFIRM_MODE ? 2 : 1);
        for (i = 0; i < 2; i++)
            labels[i] = sConfirm == CONFIRM_TITLE ? sConfirmTitleRows[i][l] : sConfirmModeRows[i][l];
        if (last)
            SDL_strlcpy(ask, sConfirmSure[l], sizeof(ask));
        else if (sConfirm == CONFIRM_TITLE)
            SDL_strlcpy(ask, sConfirmTitleAsk[l], sizeof(ask));
        else
            SDL_snprintf(ask, sizeof(ask), sConfirmModeReset[l], sModeNames[sConfirmMode][l]);
        warn = sConfirm == CONFIRM_TITLE ? sConfirmTitleWarn[last][l] : sConfirmModeWarn[sConfirmStep][l];
    }
    /* measure; on the GBA's 160 lines a window too tall shows only the chosen row's description */
    warnH = DrawWrapped(warn, 0, 0, panelW - 32, 0, 0) * 11 + 4;
    for (compact = 0; compact < 2; compact++) {
        h = 22 + warnH;
        for (i = 0; i < rows; i++)
            h += 14 + (subs[i] && (!compact || i == sConfirmCursor) ? DrawWrapped(subs[i], 0, 0, textW, 0, 0) * 11 + 4 : 0);
        h += 16;
        if (h <= height - 4)
            break;
    }
    compact = compact > 0;
    x0 = (width - panelW) / 2;
    y0 = SDL_max(2, (height - h) / 2);
    Shade(0, 0, width, height, 0xFF000000u, 160); /* the options stay behind */
    Fill(x0 - 2, y0 - 2, panelW + 4, h + 4, MENU_GREY);
    Fill(x0, y0, panelW, h, 0xFFFFFFFFu);
    DrawText(ask, x0 + 8, y0 + 4, MENU_RED_DARK, 0xFFD8D8D8u);
    Fill(x0 + 8, y0 + 17, panelW - 16, 1, MENU_GREY);
    y = y0 + 22;
    DrawGlyph('!', x0 + 10, y, MENU_RED_DARK, 0xFFD8D8D8u);
    DrawWrapped(warn, x0 + 24, y, panelW - 32, MENU_RED_DARK, 1);
    y += warnH;
    for (i = 0; i < rows; i++) {
        int sel = i == sConfirmCursor, off = ConfirmRowOff(i);
        MenuBar(x0 + 8, y + 2, panelW - 16, sel);
        DrawText(labels[i], x0 + 24, y, off ? MENU_DIM : MENU_INK, 0xFFD8D8D8u);
        y += 14;
        if (subs[i] && (!compact || sel))
            y += DrawWrapped(subs[i], x0 + 24, y, textW, sel && !off ? MENU_INK : MENU_DIM, 1) * 11 + 4;
    }
    DrawText(sWizHelp[l], x0 + 8, y0 + h - 12, MENU_DIM, 0xFFFFFFFFu);
}

static void DrawMenu(int l, int width, int height) {
    int rows, i, x, y, labelW = 0, valueW = 0, barX = 10, barW, top = MENU_TOP, artX;
    int rowH = sPage == PAGE_CONTROLS && sControlPad ? 14 : 16;
    char value[64];
    int focus = sPage * 1024 + (sPage == PAGE_NEWGAME ? sWizStep * 16 + sWizCursor : sCursor);
    if (focus != sMenuLightFocus) {
        sMenuLightFocus = focus;
        sMenuLightStart = MenuLightNow();
    }

    if (sPage == PAGE_ACHIEVEMENTS) { DrawAchievementMenu(l, width, height); return; }
    if (sPage == PAGE_LOBBY) { DrawLobby(l, width, height); return; }
    if (sPage == PAGE_NEWGAME) {
        DrawWizard(l, width, height);
        return;
    }

    if (!sMenuArtReady)
        DecodeMenuArt();
    for (i = PageFirst(); i < PageEnd(); i++) {
        labelW = SDL_max(labelW, TextWidth(sLabels[i][l]));
        ShortValue(i, value, sizeof(value));
        valueW = SDL_max(valueW, TextWidth(value));
        /* Keep the menu width when battle zoom switches to a shorter numeric value. */
        if (i == ITEM_BATTLE_ZOOM)
            valueW = SDL_max(valueW, TextWidth(sMainZoom[l]));
    }
    barW = SDL_min(labelW + valueW + 40, width - barX - 6);
    rows = SDL_min(VisibleBetween(PageFirst(), PageEnd()), (height - top - 14) / rowH);
    /* Keep the pad beneath the list and above help. Compact resolutions scroll
     * the list instead of overlapping it or drawing over the character art. */
    if (sPage == PAGE_CONTROLS && sControlPad && height - top - 14 >= rowH + 80)
        rows = SDL_min(rows, (height - top - 14 - 80) / rowH);
    if (sScroll < PageFirst() || sScroll >= PageEnd())
        sScroll = PageFirst();
    if (sCursor < sScroll)
        sScroll = sCursor;
    /* rows are visible items: the hidden ones take no room */
    while (VisibleBetween(sScroll, sCursor + 1) > rows && sScroll < sCursor)
        sScroll++;

    /* white, the picture at the bottom right */
    Fill(0, 0, width, height, 0xFFFFFFFFu);
    /* the picture as tall as the screen (it was a fixed 160 lines, small on
       a large window), sampled pixel by pixel; artX/artY stay the left/top
       of an unscaled picture so the list's shading below still lines up */
    {
        int ah = height, aw = (GBA_SCREEN_W - MENU_ART_X) * height / GBA_SCREEN_H;
        int ax0 = width - aw;
        for (y = 0; y < ah; y++)
            for (x = 0; x < aw; x++)
                Pixel(ax0 + x, y, sMenuArt[(y * GBA_SCREEN_H / ah) * GBA_SCREEN_W + MENU_ART_X +
                                           x * GBA_SCREEN_H / ah]);
        artX = ax0 - MENU_ART_X;
    }
    /* keep the list readable where it runs over the picture */
    for (x = barX + barW - 24; x < barX + barW + 16; x++)
        if (x >= artX + MENU_ART_X)
            Shade(x, 0, 1, top + rows * rowH, 0xFFFFFFFFu,
                  SDL_clamp(256 - (x - (barX + barW)) * 16, 0, 230));
    if (barX + barW - 24 > artX + MENU_ART_X)
        Shade(artX + MENU_ART_X, 0, barX + barW - 24 - (artX + MENU_ART_X), top + rows * rowH, 0xFFFFFFFFu, 230);

    DrawText(sPage == PAGE_REMATCH ? sRematchTitle[l] : sPage == PAGE_CHEATS ? sCheatsTitle[l] : sPage == PAGE_PS2 ? sPs2Title[l] : sPage == PAGE_CONTROLS ? (l == LANG_ES ? (sControlPad ? "Controles - Mando" : "Controles - Teclado") : (sControlPad ? "Controls - Gamepad" : "Controls - Keyboard")) : sPage == PAGE_NEWGAME ? sNgTitle[l] : sTitle[l], barX, MENU_TITLE_Y, MENU_INK, 0xFFD8D8D8u);
    if (sScroll > PageFirst())
        DrawText("\xE2\x86\x91", barX + barW - 8, 2, MENU_INK, 0xFFD8D8D8u);
    for (i = sScroll, y = top; y < top + rows * rowH && i < PageEnd(); i++, y += rowH) {
        int on = ItemEnabled(i), sel = i == sCursor;
        if (!ItemVisible(i)) {
            y -= rowH;
            continue;
        }
        uint32_t face = on ? MENU_INK : MENU_DIM;

        if (i == ITEM_EXPERIMENTAL || i == ITEM_RECOM_EXTRAS || i == ITEM_CHEAT_DEBUG_HEADING ||
            i == ITEM_CONTROL_NINTENDO_HEADING) {
            /* a section heading: red text and a thin line, no square */
            int end = DrawText(sLabels[i][l], barX, y - 1, MENU_RED_DARK, 0xFFD8D8D8u);
            Fill(end + 6, y + 4, barX + barW - end - 6, 1, MENU_GREY);
            continue;
        }
        MenuBar(barX, y, barW, sel);
        DrawText(sLabels[i][l], barX + 16, y - 2, face, 0xFFD8D8D8u);
        if (i == ITEM_CLOSE || i == ITEM_PS2_BACK || i == ITEM_CHEATS_BACK || i == ITEM_REMATCH_BACK ||
            i == ITEM_CONTROLS_BACK || i == ITEM_NG_START)
            continue;
        ShortValue(i, value, sizeof(value));
        x = barX + barW - 4 - TextWidth(value);
        if (sel && on) {
            x -= 8;
            DrawText("\xE2\x86\x90", x - 10, y - 2, MENU_RED_DARK, 0xFFD8D8D8u);
            DrawText("\xE2\x86\x92", x + TextWidth(value) + 2, y - 2, MENU_RED_DARK, 0xFFD8D8D8u);
        }
        DrawText(value, x, y - 2, face, 0xFFD8D8D8u);
    }
    if (VisibleBetween(i, PageEnd()) > 0)
        DrawText("\xE2\x86\x93", barX + barW - 8, top + rows * rowH - 2, MENU_INK, 0xFFD8D8D8u);
    if (sPage == PAGE_CONTROLS && sControlPad)
        DrawControlDiagram(barX, top + rows * rowH + 2, barW, height - 15 - (top + rows * rowH + 2));
    if (sPage == PAGE_CHEATS && sCursor == ITEM_CHEAT_GBA_MAP)
        DrawGbaMapNote(l, width, height, barX + barW + 8);
    if (sPage == PAGE_CONTROLS && sCursor == ITEM_PAD_LAYOUT)
        DrawNintendoNote(l, width, height, barX + barW + 8);
    int zoom = sCursor == ITEM_BATTLE_ZOOM && gPortConfig.battleZoom ? gPortConfig.battleZoom : gPortConfig.zoom;
    if (sPage == PAGE_MAIN && (sCursor == ITEM_ZOOM || sCursor == ITEM_BATTLE_ZOOM) &&
        ItemEnabled(sCursor) && PortConfigZoomHeavy(zoom))
        /* a demanding zoom: say so where the help line is */
        MenuButton(barX, height - 15, sZoomHeavy[l], '!');
    else if (sPage == PAGE_REMATCH && sCursor >= ITEM_REMATCH_0 && sCursor <= ITEM_REMATCH_LAST && !ItemEnabled(sCursor))
        /* a rematch not available now: when it is, where the help line is */
        MenuButton(barX, height - 15, RematchWhy(sCursor - ITEM_REMATCH_0), '!');
    else
        DrawText(sPage == PAGE_REMATCH ? sRematchHelp[l] : sPage == PAGE_CHEATS ? sCheatsHelp[l] : sPage == PAGE_CONTROLS ? (l == LANG_ES ? "←/→: teclado/mando; confirmar: asignar; Supr: borrar" : "Left/right: keyboard or gamepad; confirm: bind; Delete: clear") : sPage == PAGE_NEWGAME ? sHelpNg[l] : sHelp[l], barX, height - 13, MENU_DIM, 0xFFFFFFFFu);
    if (sConfirm)
        DrawConfirm(l, width, height);
}

/* Test harness: KHCOM_OPTIONS_HOLD=n closes the menu n frames after it has
   faded in (the game is paused meanwhile, so frame-based keys cannot);
   KHCOM_FADE_SHOTS saves every frame of the fades as fade_<n>.bmp. */
static void HarnessFadeShot(int width, int height) {
    static int n;
    char name[32];
    SDL_Surface* s;

    if (!SDL_getenv("KHCOM_FADE_SHOTS"))
        return;
    s = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_XRGB8888, sFb, width * 4);
    SDL_snprintf(name, sizeof(name), "fade_%03d.bmp", n++);
    if (s) {
        SDL_SaveBMP(s, PortConfigPath(name));
        SDL_DestroySurface(s);
    }
}

/* Opt-in regression route, evaluated while the game is paused by this menu.
   KHCOM_OPTIONS_ACTIONS=tick:item:direction,... uses the same Change() as
   keyboard/controller input. Each action records its translated value and a
   rendered menu BMP; KHCOM_OPTIONS_HOLD supplies the ordinary close route.
   Tests must run in an isolated config/save directory (save bank is real). */
static int HarnessOptionsAction(void) {
    const char* route = SDL_getenv("KHCOM_OPTIONS_ACTIONS");
    int changed = 0;
    if (sPage == PAGE_ACHIEVEMENTS && SDL_getenv("KHCOM_ACHIEVEMENT_KEYS")) {
        const char* p = SDL_getenv("KHCOM_ACHIEVEMENT_KEYS");
        ++sHarnessTick;
        while (p && *p) {
            const char* comma = SDL_strchr(p, ',');
            const char* colon = SDL_strchr(p, ':');
            if (colon && (!comma || colon < comma) && SDL_atoi(p) == sHarnessTick) {
                SDL_Event event = {0}; int key = SDL_atoi(colon + 1);
                if (key < 0) { event.type = SDL_EVENT_GAMEPAD_BUTTON_DOWN; event.gbutton.button = -(key + 1); }
                else { event.type = SDL_EVENT_KEY_DOWN; event.key.key = (SDL_Keycode)key; }
                PortUiHandleEvent(&event); changed = 1;
                PortLog("achievement-menu-test: filter=%d choice=%d", sAchievementView, sAchievementChoice);
            }
            p = comma ? comma + 1 : NULL;
        }
        return changed;
    }
    if (sPage == PAGE_LOBBY && (SDL_getenv("KHCOM_LOBBY_KEYS") || SDL_getenv("KHCOM_LOBBY_SHOTS"))) {
        const char* w = SDL_getenv("KHCOM_LOBBY_KEYS");
        const char* t = SDL_getenv("KHCOM_LOBBY_SHOTS");
        sHarnessTick++;
        while (t && *t) { /* the window as lobby_<tick>.bmp */
            if (SDL_atoi(t) == sHarnessTick) {
                extern void PortHarnessShot(const char* name);
                char name[32];
                SDL_snprintf(name, sizeof(name), "lobby_%d.bmp", sHarnessTick);
                PortHarnessShot(name);
            }
            t = SDL_strchr(t, ',');
            if (t)
                t++;
        }
        while (w && *w) {
            const char* comma = SDL_strchr(w, ',');
            const char* colon = SDL_strchr(w, ':');
            if (colon && (!comma || colon < comma) && SDL_atoi(w) == sHarnessTick) {
                PortLog("lobby-test: tick=%d cursor=%d rooms=%d key=%d", sHarnessTick, sLobbyCursor,
                        PortLobbyRoomCount(), SDL_atoi(colon + 1));
                LobbyKey(SDL_atoi(colon + 1));
                changed = 1;
            }
            w = comma ? comma + 1 : NULL;
        }
        return 0;
    }
    if (sPage == PAGE_NEWGAME && SDL_getenv("KHCOM_WIZARD_KEYS")) {
        const char* w = SDL_getenv("KHCOM_WIZARD_KEYS");
        sHarnessTick++;
        while (w && *w) {
            const char* comma = SDL_strchr(w, ',');
            const char* colon = SDL_strchr(w, ':');
            if (colon && (!comma || colon < comma) && SDL_atoi(w) == sHarnessTick) {
                PortLog("wizard-test: tick=%d step=%d cursor=%d key=%d", sHarnessTick, sWizStep, sWizCursor,
                        SDL_atoi(colon + 1));
                WizKey(SDL_atoi(colon + 1));
                changed = 1;
            }
            w = comma ? comma + 1 : NULL;
        }
        if (SDL_getenv("KHCOM_WIZARD_SHOTS")) { /* tick list: the window's picture as wizard_<tick>.bmp */
            const char* t = SDL_getenv("KHCOM_WIZARD_SHOTS");
            while (t && *t) {
                if (SDL_atoi(t) == sHarnessTick) {
                    extern void PortHarnessShot(const char* name);
                    char name[32];
                    SDL_snprintf(name, sizeof(name), "wizard_%d.bmp", sHarnessTick);
                    PortHarnessShot(name);
                }
                t = SDL_strchr(t, ',');
                if (t) t++;
            }
        }
        return changed;
    }
    if (SDL_getenv("KHCOM_OPTIONS_MOVES")) { /* tick:dir moves the light, as the arrows do */
        const char* m = SDL_getenv("KHCOM_OPTIONS_MOVES");
        int tick = sHarnessTick + 1;
        while (m && *m) {
            const char* comma = SDL_strchr(m, ',');
            const char* colon = SDL_strchr(m, ':');
            if (colon && (!comma || colon < comma) && SDL_atoi(m) == tick) {
                Move(SDL_atoi(colon + 1) < 0 ? -1 : 1);
                changed = 1;
            }
            m = comma ? comma + 1 : NULL;
        }
        if (!route) {
            sHarnessTick = tick;
            return changed;
        }
    }
    if (!route)
        return 0;
    sHarnessTick++;
    if (sConfirm && SDL_getenv("KHCOM_CONFIRM_KEYS")) { /* tick:key inside the confirmation (0 up, 1 down, 2 A, 3 B) */
        const char* c = SDL_getenv("KHCOM_CONFIRM_KEYS");
        while (c && *c) {
            const char* comma = SDL_strchr(c, ',');
            const char* colon = SDL_strchr(c, ':');
            if (colon && (!comma || colon < comma) && SDL_atoi(c) == sHarnessTick) {
                ConfirmKey(SDL_atoi(colon + 1));
                PortLog("confirm-test: tick=%d what=%d step=%d cursor=%d mode=%d title=%d", sHarnessTick, sConfirm,
                        sConfirmStep, sConfirmCursor, gPortConfig.graphicsMode, sTitleRequest);
                changed = 1;
            }
            c = comma ? comma + 1 : NULL;
        }
    }
    while (route && *route) {
        const char* comma = SDL_strchr(route, ',');
        const char* item = SDL_strchr(route, ':');
        const char* dir = item ? SDL_strchr(item + 1, ':') : NULL;
        if (item && dir && (!comma || dir < comma) && SDL_atoi(route) == sHarnessTick) {
            int row = SDL_atoi(item + 1), direction = SDL_atoi(dir + 1);
            if (row >= 0 && row < ITEM_COUNT && row != ITEM_EXPERIMENTAL && row != ITEM_RECOM_EXTRAS && row != ITEM_CHEAT_DEBUG_HEADING && row != ITEM_CONTROL_NINTENDO_HEADING && row != ITEM_CLOSE) {
                char before[128], after[128];
                ValueText(row, before, sizeof(before));
                sCursor = row;
                Change(row, direction < 0 ? -1 : 1);
                ValueText(row, after, sizeof(after));
                PortLog("options-test: tick=%d item=%d enabled=%d before=[%s] after=[%s]",
                        sHarnessTick, row, ItemEnabled(row), before, after);
                if (row == ITEM_FULLSCREEN || row == ITEM_FS_RES || row == ITEM_VSYNC) {
                    int count = 0, vsync = -999, winW = 0, winH = 0;
                    SDL_Window** windows = SDL_GetWindows(&count);
                    if (windows && count > 0) {
                        SDL_SyncWindow(windows[0]);
                        const SDL_DisplayMode* mode = SDL_GetWindowFullscreenMode(windows[0]);
                        SDL_Renderer* renderer = SDL_GetRenderer(windows[0]);
                        SDL_GetWindowSizeInPixels(windows[0], &winW, &winH);
                        if (renderer)
                            SDL_GetRenderVSync(renderer, &vsync);
                        PortLog("options-video: tick=%d fullscreen=%d size=%dx%d mode=%dx%d vsync=%d driver=%s",
                                sHarnessTick, (SDL_GetWindowFlags(windows[0]) & SDL_WINDOW_FULLSCREEN) != 0,
                                winW, winH, mode ? mode->w : 0, mode ? mode->h : 0, vsync,
                                SDL_GetCurrentVideoDriver());
                    }
                    SDL_free(windows);
                }
                changed = 1;
            }
        }
        route = comma ? comma + 1 : NULL;
    }
    return changed;
}

static void HarnessOptionsShot(uint32_t* frame, int width, int height) {
    char name[64];
    SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_XRGB8888, frame, width * 4);
    if (!surface)
        return;
    SDL_snprintf(name, sizeof(name), "options_%04d_%02d.bmp", sHarnessTick, sCursor);
    SDL_SaveBMP(surface, PortConfigPath(name));
    SDL_DestroySurface(surface);
}

/* ---- scene pause (65) -------------------------------------------------------
   A short tap of START in a story scene (event_message.c) pauses it under a
   small window over the scene: back to the game, or skip the scene, which
   asks first and then ends it by the game's own skip (holding START). */
static const char* const sScenePauseTitle[LANG_COUNT] = { "Pause", "Pause", "Pause", "Pausa", "Pausa", "ポーズ" };
static const char* const sScenePauseRows[2][LANG_COUNT] = {
    { "Back to the game", "Retour au jeu", "Zurück zum Spiel", "Torna al gioco", "Volver al juego", "ゲームに戻る" },
    { "Skip scene", "Passer la scène", "Szene überspringen", "Salta scena", "Saltar escena", "シーンをスキップ" },
};
static const char* const sScenePauseAsk[LANG_COUNT] = {
    "Skip this scene?", "Passer cette scène ?", "Diese Szene überspringen?", "Saltare questa scena?",
    "¿Saltar esta escena?", "このシーンをスキップしますか？",
};
static const char* const sScenePauseYesNo[2][LANG_COUNT] = {
    { "Yes", "Oui", "Ja", "Sì", "Sí", "はい" },
    { "No", "Non", "Nein", "No", "No", "いいえ" },
};
/* The same pause over a cinematic (mode_movie.c, #24): START opens it. */
static const char* const sMoviePauseSkip[LANG_COUNT] = {
    "Skip cinematic", "Passer la cinématique", "Filmsequenz überspringen", "Salta filmato", "Saltar cinemática",
    "ムービーをスキップ",
};
static const char* const sMoviePauseAsk[LANG_COUNT] = {
    "Skip this cinematic?", "Passer cette cinématique ?", "Diese Filmsequenz überspringen?", "Saltare questo filmato?",
    "¿Saltar esta cinemática?", "このムービーをスキップしますか？",
};
static int sSceneSkippable, sSceneStep, sSceneCursor, sSceneMovie;

void PortUiOpenScenePause(int skippable) {
    if (sOpen || sScenePause)
        return;
    sScenePause = 1;
    sSceneSkippable = skippable;
    sSceneStep = 0;
    sSceneCursor = 0;
    sSceneMovie = 0;
}

void PortUiOpenMoviePause(void) {
    if (sOpen || sScenePause)
        return;
    PortUiOpenScenePause(1);
    sSceneMovie = 1;
}

/* The same pause over an imported HD movie or scene (port_hdmovie.c): skipping only stops the movie; the cartridge
   movie or the event then ends as after the movie itself. */
static int sHdMoviePause, sHdMovieSkip;

void PortUiOpenHdMoviePause(void) {
    PortUiOpenMoviePause();
    sHdMoviePause = sScenePause;
    sHdMovieSkip = 0;
}

int PortUiHdMovieSkipTaken(void) {
    int skip = sHdMovieSkip;
    sHdMovieSkip = 0;
    return skip;
}

static void ScenePauseClose(int skip) {
    extern void PortInputSuppressUntilRelease(void);
    extern void PortEventSkipScene(void);
    extern void PortMovieSkipRequest(void);
    sScenePause = 0;
    PortInputSuppressUntilRelease(); /* the closing press does not reach the scene */
    if (sHdMoviePause) {
        sHdMoviePause = 0;
        sHdMovieSkip = skip;
        return;
    }
    if (skip) {
        if (sSceneMovie)
            PortMovieSkipRequest();
        else
            PortEventSkipScene();
    }
}

/* 0 up, 1 down, 2 confirm, 3 back, 4 START (resume) */
static void ScenePauseKey(int key) {
    extern void PortMenuSoundClick(void);
    extern void PortMenuSoundDecide(int confirm);
    if (key == 0 || key == 1) {
        int next = key == 0 ? 0 : 1;
        if (sSceneStep == 0 && next == 1 && !sSceneSkippable)
            return;
        if (next != sSceneCursor) {
            sSceneCursor = next;
            PortMenuSoundClick();
        }
    } else if (key == 2) {
        PortMenuSoundDecide(1);
        if (sSceneStep == 0) {
            if (sSceneCursor == 0)
                ScenePauseClose(0);
            else {
                sSceneStep = 1;
                sSceneCursor = 1; /* "No" first: a skip is never one press away */
            }
        } else if (sSceneCursor == 0) {
            ScenePauseClose(1);
        } else {
            sSceneStep = 0;
            sSceneCursor = 1;
        }
    } else if (key == 3) {
        PortMenuSoundDecide(0);
        if (sSceneStep == 1) {
            sSceneStep = 0;
            sSceneCursor = 1;
        } else {
            ScenePauseClose(0);
        }
    } else if (key == 4 && sSceneStep == 0) {
        ScenePauseClose(0);
    }
}

static int ScenePauseEvent(const SDL_Event* ev) {
    int key = -1;
    if (ev->type == SDL_EVENT_KEY_DOWN && !ev->key.repeat) {
        int i;
        for (i = 0; i < 2; i++)
            if (gPortConfig.keys[PORT_ACT_START][i] != 0 && ev->key.key == (SDL_Keycode)gPortConfig.keys[PORT_ACT_START][i]
                && ev->key.key != SDLK_RETURN)
                key = 4;
        switch (ev->key.key) {
            case SDLK_UP: case SDLK_W: key = 0; break;
            case SDLK_DOWN: case SDLK_S: key = 1; break;
            case SDLK_RETURN: case SDLK_SPACE: case SDLK_X: case SDLK_K: key = 2; break;
            case SDLK_ESCAPE: case SDLK_BACKSPACE: case SDLK_Z: case SDLK_J: key = 3; break;
        }
    } else if (ev->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        switch (UiPadButton(ev)) {
            case SDL_GAMEPAD_BUTTON_DPAD_UP: key = 0; break;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN: key = 1; break;
            case SDL_GAMEPAD_BUTTON_SOUTH: key = 2; break;
            case SDL_GAMEPAD_BUTTON_EAST: key = 3; break;
            case SDL_GAMEPAD_BUTTON_START: key = 4; break;
        }
    }
    if (key >= 0)
        ScenePauseKey(key);
    return ev->type == SDL_EVENT_KEY_DOWN || ev->type == SDL_EVENT_KEY_UP ||
           ev->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || ev->type == SDL_EVENT_GAMEPAD_BUTTON_UP;
}

/* The panel's rectangle on the frame, kept for the presenter: it draws the panel again above the HD layers (an HD
   portrait, HD text or the black an HD scene ends on covered it, and the paused game looked hung: 23/10). */
static int sScenePauseRect[4];
int PortUiScenePauseRect(int r[4]) {
    if (!sScenePause)
        return 0;
    SDL_memcpy(r, sScenePauseRect, sizeof(sScenePauseRect));
    return r[2] > 0 && r[3] > 0;
}

static void DrawScenePause(int l, int width, int height) {
    const char* title = sSceneStep == 0 ? sScenePauseTitle[l] : (sSceneMovie ? sMoviePauseAsk[l] : sScenePauseAsk[l]);
    const char* rows[2];
    int i, panelW, h = 22 + 2 * 16 + 6, x0, y0, y;
    for (i = 0; i < 2; i++)
        rows[i] = sSceneStep == 0 ? sScenePauseRows[i][l] : sScenePauseYesNo[i][l];
    if (sSceneStep == 0 && sSceneMovie)
        rows[1] = sMoviePauseSkip[l];
    panelW = SDL_max(TextWidth(title) + 16, SDL_max(TextWidth(rows[0]), TextWidth(rows[1])) + 48);
    panelW = SDL_min(SDL_max(panelW, 120), width - 8);
    x0 = (width - panelW) / 2;
    y0 = (height - h) / 2;
    Shade(0, 0, width, height, 0xFF000000u, 120); /* the paused scene stays behind */
    sScenePauseRect[0] = x0 - 2;
    sScenePauseRect[1] = y0 - 2;
    sScenePauseRect[2] = panelW + 4;
    sScenePauseRect[3] = h + 4;
    Fill(x0 - 2, y0 - 2, panelW + 4, h + 4, MENU_GREY);
    Fill(x0, y0, panelW, h, 0xFFFFFFFFu);
    DrawText(title, x0 + 8, y0 + 4, MENU_RED_DARK, 0xFFD8D8D8u);
    Fill(x0 + 8, y0 + 17, panelW - 16, 1, MENU_GREY);
    for (i = 0, y = y0 + 22; i < 2; i++, y += 16) {
        int on = sSceneStep == 1 || i == 0 || sSceneSkippable;
        MenuBar(x0 + 8, y + 2, panelW - 16, i == sSceneCursor);
        DrawText(rows[i], x0 + 24, y, on ? MENU_INK : MENU_DIM, 0xFFD8D8D8u);
    }
}

/* 1 if PortUiDraw writes on the frame this time (port_main.c presents the PPU's frame without a copy otherwise):
   the options, the scene pause, the skip prompt or the title hint. Keep in step with PortUiDraw below. */
int PortUiDrawsOnFrame(void) {
    return sScenePause || sOpen || (sSkipPrompt && !sSkipOwnLayer) || sTitleHintFrames > 0;
}

void PortUiDraw(uint32_t* frame, int width, int height) {
    int l = Lang();
    int harnessShot = 0;
    ++sMenuLightDrawClock;

    sFb = frame;
    sFbW = width;
    sFbH = height;
    if (sScenePause) {
        static int shotStep = -1;
        DrawScenePause(l, width, height);
        if (SDL_getenv("KHCOM_SCENEPAUSE_SHOT") && shotStep != sSceneStep * 2 + sSceneCursor) {
            shotStep = sSceneStep * 2 + sSceneCursor; /* harness: one shot per state */
            sHarnessTick = 100 + shotStep;
            HarnessOptionsShot(frame, width, height);
            {
                /* and the window as presented, every layer drawn: the panel must be on top of them */
                extern void PortHarnessShot(const char* name);
                char name[48];
                SDL_snprintf(name, sizeof(name), "scenepause_out_%d.bmp", shotStep);
                PortHarnessShot(name);
            }
        }
        return;
    }

    if (sTouchEdit) { /* the touch layout editor: its hint at the top; the buttons are the touch layer's */
        static const char* const hint[LANG_COUNT] = {
            "Drag the buttons where you want them", "Faites glisser les boutons où vous voulez",
            "Ziehe die Tasten an ihren Platz", "Trascina i tasti dove vuoi", "Arrastra los botones a donde quieras",
            "ボタンを好きな位置へドラッグ" };
        MenuButton(gPortScreen.originX + (GBA_SCREEN_W - TextWidth(hint[l])) / 2 - 9, gPortScreen.originY + 22, hint[l], 0);
        return;
    }
    if (!sOpen && sReturn > 0) {
        /* The presenter fades the complete output, including HUD/text layers. */
        sReturn--;
        HarnessFadeShot(width, height);
    }
    if (!sOpen) {
        if (sSkipPrompt && !sSkipOwnLayer)
            DrawSkipPrompt(l);
        if (sTitleHintFrames > 0) {
            const char* hint = sTitleHint[l];
            int gx = gPortScreen.originX + GBA_SCREEN_W - TextWidth(hint) - 6;
            int gy = gPortScreen.originY + 4;
            sTitleHintFrames--;
            MenuButton(gx - 18, gy + 2, hint, 0);
        }
        return;
    }

    if (sClosing > 0) {
        DrawMenu(l, width, height);
        Shade(0, 0, width, height, MENU_FADE_COLOR, SDL_min(256, (MENU_FADE - sClosing + 1) * 256 / MENU_FADE));
        HarnessFadeShot(width, height);
        if (--sClosing == 0) {
            sOpen = 0;
            sReturn = MENU_FADE;
        }
        return;
    }
    if (sFade < MENU_FADE) { /* the game's picture to black */
        /* Keep the original scene canvas and presentation until fully black. */
        sFade++;
        HarnessFadeShot(width, height);
        return;
    }
    if (sFade >= MENU_FADE * 2)
        harnessShot = HarnessOptionsAction();
    DrawMenu(l, width, height);
    if (harnessShot)
        HarnessOptionsShot(frame, width, height);
    if (sFade < MENU_FADE * 2) { /* the menu in from black */
        Shade(0, 0, width, height, MENU_FADE_COLOR, (MENU_FADE * 2 - sFade) * 256 / MENU_FADE - 32);
        sFade++;
        HarnessFadeShot(width, height);
    } else if (SDL_getenv("KHCOM_OPTIONS_HOLD") && ++sFade >= MENU_FADE * 2 + SDL_atoi(SDL_getenv("KHCOM_OPTIONS_HOLD"))) {
        Close();
    }
}
