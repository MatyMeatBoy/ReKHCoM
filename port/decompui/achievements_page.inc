/* Uses the launcher's white/black/red palette and the same data as the game. */
static KhRa* sRaUi;
static int sRaRomRegion = -1;
static char sRaUsername[128], sRaPassword[256], sAchievementSearch[128];
static bool sRaRemember = true;
static int sAchievementFilter;
static KhOfflineState sOfflineProgress; /* earned in the game (achievements_official_eu.dat) */
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
                           ImVec2 uv0 = ImVec2(0, 0), ImVec2 uv1 = ImVec2(1, 1)) {
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
    else GameSquare(ImGui::GetWindowDrawList(), ImVec2(origin.x + Px(20), origin.y + Px(21)), Px(28), unlocked);
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
    if (sRaAccountShot) ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader(T("Cuenta RetroAchievements (opcional)", "RetroAchievements account (optional)")))
        PageAchievementAccount();
    PageOfficialAchievements();
}
