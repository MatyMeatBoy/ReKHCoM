/* Uses the launcher's white/black/red palette and the same data as the game. */
static KhRa* sRaUi;
static int sRaRomRegion = -1;
static char sRaUsername[128], sRaPassword[256], sAchievementSearch[128];
static bool sRaRemember = true;
static int sAchievementFilter;
static KhOfflineState sOfflineProgress; /* earned in the game (achievements_official_eu.dat) */
#include "../achievements/recom_set.inc"
static KhOfflineState sRecomProgress; /* Re:CoM games: achievements_recom_eu.dat, never online */
static unsigned long long sRecomExtraWhen[RECOM_EXTRA_COUNT]; /* achievements_recom_extra.dat */
static Uint64 sOfflineReadAt;
static bool sOfflineRead;
static bool sAchievementPreview;
static Uint64 sAchievementPreviewStart;
static bool sRaAccountShot;
#include "achievement_badges.inc"
#include "achievement_catalog.inc"

static void AchievementPreviewDraw() {
    if (!sAchievementPreview) return;
    unsigned elapsed = (unsigned)(SDL_GetTicks() - sAchievementPreviewStart);
    if (elapsed >= 4500) { sAchievementPreview = false; return; }
    ImVec2 screen = ImGui::GetIO().DisplaySize;
    KhToastLayout box = KhAchievementToastLayout(elapsed, (int)screen.x, (int)screen.y, 220);
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImVec2 a(box.x, box.y), b(box.x + box.width, box.y + box.height);
    auto fade = [box](const ImVec4& color) { return ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, color.w * box.alpha)); };
    dl->AddRectFilled(ImVec2(a.x + box.scale, a.y + 2 * box.scale), ImVec2(b.x + box.scale, b.y + 2 * box.scale),
                      ImGui::GetColorU32(ImVec4(0, 0, 0, 0.2f * box.alpha)));
    dl->AddRectFilled(a, b, fade(kPanel)); dl->AddRect(a, b, fade(kBorder));
    dl->AddRectFilled(a, ImVec2(a.x + 3 * box.scale, b.y), fade(kAccent));
    float textX = a.x + 38 * box.scale;
    dl->AddRectFilled(ImVec2(a.x + 10 * box.scale, a.y + 16 * box.scale),
                      ImVec2(a.x + 31 * box.scale, a.y + 37 * box.scale), fade(kText));
    dl->AddRectFilled(ImVec2(a.x + 15 * box.scale, a.y + 21 * box.scale),
                      ImVec2(a.x + 26 * box.scale, a.y + 32 * box.scale), fade(kAccent));
    dl->AddText(sFontSmall, 6.0f * box.scale, ImVec2(textX, a.y + 5 * box.scale), fade(kMuted),
                T("VISTA PREVIA · LOGRO", "PREVIEW · ACHIEVEMENT"));
    dl->AddText(sFontBody, 8.0f * box.scale, ImVec2(textX, a.y + 18 * box.scale), fade(kText), KhOfficialText(KhOfficialAt(0), sSpanish ? 4 : 0, 0));
    dl->AddText(sFontSmall, 6.0f * box.scale, ImVec2(textX, a.y + 33 * box.scale), fade(kMuted), KhOfficialText(KhOfficialAt(0), sSpanish ? 4 : 0, 1));
}

static bool AchievementMatches(const char* title, const char* description, bool unlocked) {
    if (sAchievementFilter == 1 && unlocked) return false;
    if (sAchievementFilter == 2 && !unlocked) return false;
    if (!sAchievementSearch[0]) return true;
    auto contains = [](const char* text) {
        return std::search(text, text + std::strlen(text), sAchievementSearch,
            sAchievementSearch + std::strlen(sAchievementSearch), [](unsigned char a, unsigned char b) {
                return std::tolower(a) == std::tolower(b);
            }) != text + std::strlen(text);
    };
    return contains(title) || contains(description);
}

static void AchievementRow(int id, const char* title, const char* description, bool unlocked,
                           const char* detail = nullptr, GLuint badge = 0,
                           ImVec2 uv0 = ImVec2(0, 0), ImVec2 uv1 = ImVec2(1, 1), int framed = 0) { /* 1 gold (Re:CoM), 2 black (Rebel) */
    ImGui::PushID(id);
    ImVec2 origin = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(Px(42), Px(42)));
    ImGui::SameLine(0, Px(12));
    ImGui::BeginGroup();
    ImGui::TextColored(unlocked ? kAccentDim : kText, "%s", title);
    ImGui::PushFont(sFontSmall);
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + width - Px(70));
    ImGui::TextColored(kMuted, "%s", description);
    if (detail && *detail) ImGui::TextColored(kMuted, "%s", detail);
    else ImGui::TextColored(unlocked ? kGood : kMuted, "%s", unlocked ? T("Conseguido", "Unlocked") : T("No conseguido", "Locked"));
    ImGui::PopTextWrapPos();
    ImGui::PopFont();
    ImGui::EndGroup();
    ImGui::EndGroup();
    if (badge) ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)badge, origin,
        ImVec2(origin.x + Px(42), origin.y + Px(42)), uv0, uv1);
    if (badge && framed) { /* the Re:CoM list's gold filigree, 1.25x the badge around it */
        ImVec2 f0, f1;
        GLuint frame = OfficialAchievementBadge(kRecomFrameCell, true, &f0, &f1);
        if (frame) ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)frame,
            ImVec2(origin.x - Px(5.25f), origin.y - Px(5.25f)), ImVec2(origin.x + Px(47.25f), origin.y + Px(47.25f)), f0, f1,
            framed == 2 ? IM_COL32(48, 46, 52, 255) : IM_COL32_WHITE); /* Rebel mode: the filigree in black */
    }
    if (!badge) GameSquare(ImGui::GetWindowDrawList(), ImVec2(origin.x + Px(20), origin.y + Px(21)), Px(28), unlocked);
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::PopID();
}

static void AchievementFilters() {
    ImGui::SetNextItemWidth(Px(240));
    ImGui::InputTextWithHint("##achsearch", T("Buscar logro...", "Find achievement..."), sAchievementSearch, sizeof(sAchievementSearch));
    ImGui::SameLine();
    const char* filters[] = {T("Todos", "All"), T("No conseguidos", "Locked"), T("Conseguidos", "Unlocked")};
    ImGui::SetNextItemWidth(Px(180));
    ImGui::Combo("##achfilter", &sAchievementFilter, filters, 3);
    ImGui::Spacing();
}

static void PageAchievementAccount() {
    BeginCard("ra-account");
    Eyebrow("RETROACHIEVEMENTS");
    ImGui::TextWrapped("%s", T("Opcional: los logros se consiguen igual sin cuenta. Con la cuenta conectada también ves su progreso.",
        "Optional: achievements are earned without an account too. With the account connected you also see its progress."));
    bool enabled = KhRaEnabled(sRaUi);
    ImGui::BeginDisabled(KhRaBusy(sRaUi));
    if (ImGui::Checkbox(T("Conectar con RetroAchievements", "Connect to RetroAchievements"), &enabled)) KhRaSetEnabled(sRaUi, enabled);
    ImGui::EndDisabled();
    ImGui::PushFont(sFontSmall);
    ImGui::TextColored(KhRaError(sRaUi) ? kBad : kMuted, "%s", KhRaStatus(sRaUi));
    ImGui::PopFont();
    rc_client_t* client = KhRaClient(sRaUi);
    const rc_client_user_t* user = client ? rc_client_get_user_info(client) : nullptr;
    if (user) {
        ImGui::Spacing();
        ImGui::TextColored(kGood, "%s", user->display_name);
        ImGui::Text(T("Puntos: %u · Softcore: %u", "Points: %u · Softcore: %u"), user->score, user->score_softcore);
        ImGui::BeginDisabled(KhRaBusy(sRaUi));
        if (ImGui::Button(T("Cerrar sesión y olvidar la cuenta", "Sign out and forget the account"))) KhRaLogout(sRaUi);
        ImGui::EndDisabled();
    } else if (enabled) {
        ImGui::BeginDisabled(KhRaBusy(sRaUi));
        ImGui::SetNextItemWidth(Px(320));
        ImGui::InputText(T("Usuario", "Username"), sRaUsername, sizeof(sRaUsername));
        ImGui::SetNextItemWidth(Px(320));
        ImGui::InputText(T("Contraseña", "Password"), sRaPassword, sizeof(sRaPassword), ImGuiInputTextFlags_Password);
        ImGui::Checkbox(T("Recordar esta cuenta en Windows", "Remember this account on Windows"), &sRaRemember);
        if (ImGui::Button(T("Iniciar sesión", "Sign in"))) {
            KhRaLogin(sRaUi, sRaUsername, sRaPassword, sRaRemember);
            SecureZeroMemory(sRaPassword, sizeof(sRaPassword));
        }
        ImGui::EndDisabled();
        ImGui::PushFont(sFontSmall);
        ImGui::TextWrapped("%s", T("La contraseña se envía por HTTPS a RetroAchievements y no se guarda."
            " Si recuerdas la cuenta, Windows cifra el token de sesión para tu usuario.",
            "The password is sent to RetroAchievements over HTTPS and is never stored."
            " Remembering your account stores a session token encrypted for your Windows user."));
        ImGui::PopFont();
    }
    ImGui::EndChild();
}

static void PageOfficialAchievements() {
    rc_client_t* client = KhRaClient(sRaUi);
    (void)client;
    if (!sOfflineRead || SDL_GetTicks() - sOfflineReadAt > 1000) { /* the game may be writing it */
        KhOfflineLoad(&sOfflineProgress, (sRoot / "achievements_official_eu.dat").u8string().c_str());
        sOfflineRead = true; sOfflineReadAt = SDL_GetTicks();
    }
    BeginCard("ra-catalog");
    RememberAchievementProgress();
    Eyebrow(T("LOGROS OFICIALES · VERSIÓN EUROPEA", "OFFICIAL ACHIEVEMENTS · EUROPEAN VERSION"));
    ImGui::PushFont(sFontSmall);
    ImGui::TextWrapped("%s", T("El set de RetroAchievements (sus autores en retroachievements.org). Se consiguen al jugar la versión europea,"
        " con o sin cuenta, y quedan guardados en este equipo. Los trucos, las revanchas y cargar un savestate pausan los logros hasta reiniciar el juego.",
        "The RetroAchievements set (its authors at retroachievements.org). They are earned playing the European version,"
        " with or without an account, and kept on this computer. Cheats, rematches and loading a save state pause achievements until the game is restarted."));
    ImGui::PopFont();
    bool account = KhRaAccountProgressAvailable(sRaUi) != 0;
    unsigned earned = 0, points = 0, total = 0;
    for (int i = 0; i < KhOfficialCount(); ++i) {
        const KhOfficialAchievement* a = KhOfficialAt(i);
        total += a->points;
        if (sOfflineProgress.when[i] || (account && KhRaAccountUnlocked(sRaUi, a->id))) { ++earned; points += a->points; }
    }
    char progress[120];
    std::snprintf(progress, sizeof(progress), T("%u / %d · %u / %u puntos", "%u / %d · %u / %u points"),
                  earned, KhOfficialCount(), points, total);
    ImGui::Spacing();
    ImGui::ProgressBar((float)earned / KhOfficialCount(), ImVec2(-1, Px(24)), progress);
    ImGui::Spacing();
    AchievementFilters();
    bool table = ImGui::BeginTable("official-achievement-grid", ImGui::GetContentRegionAvail().x >= Px(850) ? 2 : 1,
        ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_PadOuterX);
    int shown = 0;
    for (int i = 0; i < KhOfficialCount(); ++i) {
        const KhOfficialAchievement* a = KhOfficialAt(i);
        bool game = sOfflineProgress.when[i] != 0, onAccount = account && KhRaAccountUnlocked(sRaUi, a->id);
        bool unlocked = game || onAccount;
        const char* title = KhOfficialText(a, sSpanish ? 4 : 0, 0);
        const char* description = KhOfficialText(a, sSpanish ? 4 : 0, 1);
        if (!AchievementMatches(title, description, unlocked)) continue;
        char detail[120];
        std::snprintf(detail, sizeof(detail), a->points == 1 ? T("%u punto · %s", "%u point · %s") : T("%u puntos · %s", "%u points · %s"), a->points,
            onAccount ? T("Conseguido en tu cuenta", "Earned on your account") :
            game ? T("Conseguido", "Unlocked") : T("No conseguido", "Locked"));
        ImVec2 uv0, uv1;
        GLuint badge = OfficialAchievementBadge(i, unlocked, &uv0, &uv1);
        if (table) ImGui::TableNextColumn();
        AchievementRow((int)a->id, title, description, unlocked, detail, badge, uv0, uv1);
        ++shown;
    }
    if (table) ImGui::EndTable();
    if (!shown) ImGui::TextDisabled("%s", T("No hay logros con ese filtro.", "No achievements match this filter."));
    ImGui::EndChild();
}

/* The Re:CoM list: games of the Re:CoM version earn it instead of the official one; kept on this computer only. */
static void PageRecomAchievements() {
    static Uint64 readAt;
    static bool read;
    if (!read || SDL_GetTicks() - readAt > 1000) { /* the game may be writing them */
        KhOfflineLoad(&sRecomProgress, (sRoot / "achievements_recom_eu.dat").u8string().c_str());
#ifdef _WIN32
        FILE* f = _wfopen((sRoot / "achievements_recom_extra.dat").wstring().c_str(), L"r");
#else
        FILE* f = std::fopen((sRoot / "achievements_recom_extra.dat").u8string().c_str(), "r");
#endif
        KhRecomExtraParse(f, sRecomExtraWhen);
        if (f) fclose(f);
        read = true; readAt = SDL_GetTicks();
    }
    int lang = sSpanish ? 4 : 0;
    BeginCard("recom-catalog");
    Eyebrow(T("LOGROS RE:COM · SOLO EN ESTE EQUIPO", "RE:COM ACHIEVEMENTS · ON THIS COMPUTER ONLY"));
    ImGui::PushFont(sFontSmall);
    ImGui::TextWrapped("%s", T("Las partidas de la versión Re:CoM tienen su propia lista: los mismos logros oficiales y los del modo."
        " Los de mundos, jefes y la historia piden la partida entera en Experto. Nunca se suben a RetroAchievements.",
        "Games of the Re:CoM version have their own list: the same official achievements plus the mode's own."
        " Worlds, bosses and the story need the whole game on Proud. They are never sent to RetroAchievements."));
    ImGui::PopFont();
    int total = KhOfficialCount() + RECOM_EXTRA_COUNT, earned = 0;
    for (int i = 0; i < KhOfficialCount(); ++i) earned += sRecomProgress.when[i] != 0;
    for (int i = 0; i < RECOM_EXTRA_COUNT; ++i) earned += sRecomExtraWhen[i] != 0;
    char progress[80];
    std::snprintf(progress, sizeof(progress), "%d / %d", earned, total);
    ImGui::Spacing();
    ImGui::ProgressBar((float)earned / total, ImVec2(-1, Px(24)), progress);
    ImGui::Spacing();
    AchievementFilters();
    bool table = ImGui::BeginTable("recom-achievement-grid", ImGui::GetContentRegionAvail().x >= Px(850) ? 2 : 1,
        ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_PadOuterX);
    int shown = 0;
    for (int i = 0; i < RECOM_EXTRA_COUNT; ++i) { /* the mode's own first */
        bool unlocked = sRecomExtraWhen[i] != 0;
        const char* title = kRecomExtraText[i][lang][0];
        const char* description = kRecomExtraText[i][lang][1];
        if (!AchievementMatches(title, description, unlocked)) continue;
        if (table) ImGui::TableNextColumn();
        ImVec2 uv0, uv1;
        GLuint badge = OfficialAchievementBadge(kRecomBadgeCell + i, unlocked, &uv0, &uv1);
        AchievementRow(0x52450000 + i, title, description, unlocked, nullptr, badge, uv0, uv1, 1);
        ++shown;
    }
    for (int i = 0; i < KhOfficialCount(); ++i) {
        const KhOfficialAchievement* a = KhOfficialAt(i);
        bool unlocked = sRecomProgress.when[i] != 0;
        const char* title = KhOfficialText(a, lang, 0);
        char description[800];
        std::snprintf(description, sizeof(description), "%s%s", KhOfficialText(a, lang, 1),
                      KhRecomNeedsProud(a) ? kRecomProudNote[lang] : "");
        if (!AchievementMatches(title, description, unlocked)) continue;
        ImVec2 uv0, uv1;
        GLuint badge = OfficialAchievementBadge(i, unlocked, &uv0, &uv1);
        if (table) ImGui::TableNextColumn();
        AchievementRow(0x52460000 + (int)a->id, title, description, unlocked, nullptr, badge, uv0, uv1, 1);
        ++shown;
    }
    if (table) ImGui::EndTable();
    if (!shown) ImGui::TextDisabled("%s", T("No hay logros con ese filtro.", "No achievements match this filter."));
    ImGui::EndChild();
}

/* Rebel mode (the game's port_achievements.c): the official set as earned with cheats on, kept on this computer
   only; its tab shows once the game has written its list (user 25/10). */
static KhOfflineState sRebelProgress;
static bool RebelListExists() {
    std::error_code ec;
    return fs::exists(sRoot / "achievements_rebel_eu.dat", ec);
}
static void PageRebelAchievements() {
    static Uint64 readAt;
    static bool read;
    if (!read || SDL_GetTicks() - readAt > 1000) { /* the game may be writing it */
        KhOfflineLoad(&sRebelProgress, (sRoot / "achievements_rebel_eu.dat").u8string().c_str());
        read = true; readAt = SDL_GetTicks();
    }
    BeginCard("rebel-catalog");
    Eyebrow(T("LOGROS · MODO REBELDE", "ACHIEVEMENTS · REBEL MODE"));
    ImGui::PushFont(sFontSmall);
    ImGui::TextWrapped("%s", T("Los logros oficiales conseguidos con trucos. Quedan en este equipo, en su propia lista, y nunca se envían a"
        " RetroAchievements.", "The official achievements earned with cheats on. They stay on this computer, in their own list, and are"
        " never sent to RetroAchievements."));
    ImGui::PopFont();
    unsigned earned = 0;
    for (int i = 0; i < KhOfficialCount(); ++i) earned += sRebelProgress.when[i] != 0;
    char progress[80];
    std::snprintf(progress, sizeof(progress), "%u / %d", earned, KhOfficialCount());
    ImGui::Spacing();
    ImGui::ProgressBar((float)earned / KhOfficialCount(), ImVec2(-1, Px(24)), progress);
    ImGui::Spacing();
    AchievementFilters();
    bool table = ImGui::BeginTable("rebel-achievement-grid", ImGui::GetContentRegionAvail().x >= Px(850) ? 2 : 1,
        ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_PadOuterX);
    int shown = 0;
    for (int i = 0; i < KhOfficialCount(); ++i) {
        const KhOfficialAchievement* a = KhOfficialAt(i);
        bool unlocked = sRebelProgress.when[i] != 0;
        const char* title = KhOfficialText(a, sSpanish ? 4 : 0, 0);
        char description[600];
        std::snprintf(description, sizeof(description), "%s%s", T("En modo rebelde: ", "In Rebel mode: "),
                      KhOfficialText(a, sSpanish ? 4 : 0, 1));
        if (!AchievementMatches(title, description, unlocked)) continue;
        ImVec2 uv0, uv1;
        GLuint badge = OfficialAchievementBadge(i, unlocked, &uv0, &uv1);
        if (table) ImGui::TableNextColumn();
        AchievementRow((int)a->id, title, description, unlocked, unlocked ? T("Conseguido", "Unlocked") : T("No conseguido", "Locked"),
                       badge, uv0, uv1, 2);
        ++shown;
    }
    if (table) ImGui::EndTable();
    if (!shown) ImGui::TextDisabled("%s", T("No hay logros con ese filtro.", "No achievements match this filter."));
    ImGui::EndChild();
}

/* Official / Re:CoM (/ Rebel mode): the lists as tabs, drawn like Settings' (the launcher's buttons with the game's square) */
static int sAchievementList = -1; /* 0 official, 1 Re:CoM, 2 Rebel mode; -1 not chosen yet */
static void AchievementListTabs() {
    const char* names[3] = { T("Oficiales", "Official"), T("Re:CoM", "Re:CoM"), T("Modo rebelde", "Rebel mode") };
    ImDrawList* dl = ImGui::GetWindowDrawList();
    int tabs = RebelListExists() ? 3 : 2;
    if (sAchievementList < 0)
        sAchievementList = SDL_getenv("DECOMPUI_ACHIEVEMENT_LIST") ? SDL_atoi(SDL_getenv("DECOMPUI_ACHIEVEMENT_LIST")) : 0;
    if (sAchievementList >= tabs) sAchievementList = 0;
    for (int i = 0; i < tabs; i++) {
        bool active = sAchievementList == i;
        if (i) ImGui::SameLine(0, Px(6));
        ImGui::PushStyleColor(ImGuiCol_Button, active ? kPanelHover : kControl);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kControlHover);
        char label[64];
        snprintf(label, sizeof(label), "%s%s##achlist%d", active ? "    " : "", names[i], i);
        if (ImGui::Button(label, ImVec2(0, Px(34)))) sAchievementList = i;
        ImGui::PopStyleColor(2);
        if (active) {
            ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
            GameSquare(dl, ImVec2(mn.x + Px(14), (mn.y + mx.y) * 0.5f), Px(10), true);
            dl->AddRectFilled(ImVec2(mn.x, mx.y - Px(3)), mx, ImGui::GetColorU32(kAccent));
        }
    }
    ImGui::Spacing();
}

/* the emblem beside the account (port/tools/gen_ra_emblem.py: our own trophy, not RetroAchievements' logo) */
static const unsigned char sRaEmblemPng[] = {
#include "port/ra_emblem_png.inc"
};
static GLuint RaEmblem() {
    static GLuint texture;
    static bool tried;
    if (!tried) {
        tried = true;
        int w = 0, h = 0, channels;
        unsigned char* rgba = stbi_load_from_memory(sRaEmblemPng, (int)sizeof(sRaEmblemPng), &w, &h, &channels, 4);
        if (rgba) {
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
            stbi_image_free(rgba);
        }
    }
    return texture;
}

static void PageAchievements() {
    Eyebrow(T("LOGROS · KINGDOM HEARTS: CHAIN OF MEMORIES", "ACHIEVEMENTS · KINGDOM HEARTS: CHAIN OF MEMORIES"));
    /* the account reads the set with the European cartridge, when installed */
    int catalogRegion = sState[0].installed ? 0 : -1;
    if (sRaRomRegion != catalogRegion && !KhRaBusy(sRaUi)) {
        std::vector<unsigned char> rom;
        if (catalogRegion >= 0 && ReadFile(RomPath(catalogRegion), rom)) KhRaSetRom(sRaUi, rom.data(), rom.size());
        else KhRaSetRom(sRaUi, nullptr, 0);
        sRaRomRegion = catalogRegion;
    }
    /* the account always in sight (25/10): our emblem and the title, then the account's rows */
    {
        GLuint emblem = RaEmblem();
        float h = Px(40);
        ImVec2 at = ImGui::GetCursorScreenPos();
        if (emblem) {
            ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)emblem, at, ImVec2(at.x + h, at.y + h));
            ImGui::Dummy(ImVec2(h, h));
            ImGui::SameLine(0, Px(12));
        }
        ImGui::BeginGroup();
        ImGui::TextColored(kAccentDim, "%s", "RetroAchievements");
        ImGui::PushFont(sFontSmall);
        ImGui::TextColored(kMuted, "%s", T("Cuenta opcional: los logros oficiales también en retroachievements.org.",
                                           "Optional account: the official achievements on retroachievements.org too."));
        ImGui::PopFont();
        ImGui::EndGroup();
        ImGui::Spacing();
        PageAchievementAccount();
        ImGui::Separator();
        ImGui::Spacing();
    }
    AchievementListTabs();
    if (sAchievementList == 2) PageRebelAchievements();
    else if (sAchievementList == 1) PageRecomAchievements();
    else PageOfficialAchievements();
}
