/* ---- save editor page (included by decompui.cpp) ------------------------------ *
 * Off until the player confirms a warning. Edits one file of the selected
 * version's save bank; every change is listed before it is written, the save
 * is copied to backups/ first, and both copies plus the header are sealed
 * with the game's checksum (save_editor.h). Card pictures come from the
 * installed cartridge, drawn at x1 without filtering. */

namespace Ed = RekhcomSaveEdit;
namespace Cards = RekhcomCards;

static bool sEditorOn;
static int sEdVersion = -1, sEdFile = 0, sEdLoadedVersion = -1, sEdLoadedFile = -1, sEdLoadedBank = -1;
static bool sEdHasFile;
static Ed::File sEdOriginal, sEdWork;
static int sEdGroup, sEdAddKind, sEdAddValue, sEdAddCopies = 1;
static GLuint sCardTex[sizeof(Cards::kSprites) / sizeof(Cards::kSprites[0])];
static int sCardTexVersion = -1;

static const char* const kFileNamesEs[4] = { "Sora - archivo 1", "Sora - archivo 2", "Riku - archivo 1", "Riku - archivo 2" };
static const char* const kFileNamesEn[4] = { "Sora - file 1", "Sora - file 2", "Riku - file 1", "Riku - file 2" };

/* the 16x32 collection pictures of the cartridge, as GL textures */
static void LoadCardTextures(int v) {
    if (sCardTexVersion == v)
        return;
    std::vector<unsigned char> rom;
    if (!sState[v].installed || !ReadFile(RomPath(v), rom) || rom.size() != kRomSize)
        return;
    const int count = (int)(sizeof(sCardTex) / sizeof(sCardTex[0]));
    if (sCardTexVersion >= 0)
        glDeleteTextures(count, sCardTex);
    std::memset(sCardTex, 0, sizeof(sCardTex));
    for (int i = 0; i < count; i++) {
        unsigned tiles = Cards::kSprites[i].tiles[v], pal = Cards::kSprites[i].palette[v];
        if (tiles == 0 || pal == 0 || tiles + 256 > rom.size() || pal + 32 > rom.size())
            continue;
        unsigned char rgba[16 * 32 * 4] = {};
        for (int t = 0; t < 8; t++)
            for (int y = 0; y < 8; y++)
                for (int x = 0; x < 8; x++) {
                    unsigned char b = rom[tiles + t * 32 + y * 4 + x / 2];
                    int c = (x & 1) ? b >> 4 : b & 15;
                    if (!c) continue;
                    unsigned col = rom[pal + c * 2] | rom[pal + c * 2 + 1] << 8;
                    unsigned char* p = rgba + (((t / 2) * 8 + y) * 16 + (t % 2) * 8 + x) * 4;
                    p[0] = (unsigned char)((col & 31) * 255 / 31);
                    p[1] = (unsigned char)(((col >> 5) & 31) * 255 / 31);
                    p[2] = (unsigned char)(((col >> 10) & 31) * 255 / 31);
                    p[3] = 255;
                }
        glGenTextures(1, &sCardTex[i]);
        glBindTexture(GL_TEXTURE_2D, sCardTex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    }
    sCardTexVersion = v;
}

/* the card's own picture at x1 (one texel per screen pixel) */
static void CardPicture(int id) {
    int sprite = id >= 0 && id < Cards::kCardCount ? Cards::kCards[id].sprite[sEdVersion] : -1;
    if (sprite >= 0 && sCardTex[sprite] != 0)
        ImGui::Image((ImTextureID)(intptr_t)sCardTex[sprite], ImVec2(16, 32));
    else
        ImGui::Dummy(ImVec2(16, 32));
}

static std::string CardName(int id) {
    const Cards::Card& c = Cards::kCards[id];
    std::string name = c.kind >= 0 && c.kind < Cards::kKindCount ? Cards::kKindNames[c.kind] : "?";
    return name + " " + std::to_string(c.value);
}

static void EditorLoad() {
    std::vector<unsigned char> sram;
    sEdLoadedVersion = sEdVersion;
    sEdLoadedFile = sEdFile;
    sEdLoadedBank = Ini("save_bank");
    sEdHasFile = ReadFile(SavePath(sEdVersion), sram) && Ed::Read(sram, sEdFile, sEdOriginal);
    sEdWork = sEdOriginal;
}

/* every difference between the file as read and as edited, one per line */
static std::vector<std::string> EditorChanges() {
    std::vector<std::string> out;
    const Ed::File& a = sEdOriginal;
    const Ed::File& b = sEdWork;
    char line[200];
    auto num = [&](const char* name, long long x, long long y) {
        if (x != y) {
            std::snprintf(line, sizeof(line), "%s: %lld -> %lld", name, x, y);
            out.push_back(line);
        }
    };
    num(T("Nivel", "Level"), a.Level(), b.Level());
    num(T("Exp", "Exp"), a.Exp(), b.Exp());
    num(T("HP máximo", "Max HP"), a.MaxHp(), b.MaxHp());
    if (b.riku()) {
        num("AP", a.Ap(), b.Ap());
        num("DP", a.Dp(), b.Dp());
    } else {
        num("CP", a.Cp(), b.Cp());
    }
    num(T("Puntos moguri", "Moogle points"), a.Moogle(), b.Moogle());
    for (int g = 0; g < Cards::kMapGroupCount; g++)
        for (int v = 0; v < 10; v++) {
            int x = a.MapCards()[g * 10 + v], y = b.MapCards()[g * 10 + v];
            if (x != y) {
                std::snprintf(line, sizeof(line), "%s %s %d: %d -> %d", T("Carta de mapa", "Map card"),
                              Cards::kMapGroupNames[g], v, x, y);
                out.push_back(line);
            }
        }
    if (!b.riku())
        for (int id = 0; id < Cards::kCardCount; id++) {
            int x = a.CountCard(id), y = b.CountCard(id);
            if (x != y) {
                std::snprintf(line, sizeof(line), "%s %s: %d -> %d", T("Carta", "Card"), CardName(id).c_str(), x, y);
                out.push_back(line);
            }
        }
    return out;
}

static void EditorApply() {
    std::vector<unsigned char> sram;
    Ed::File fresh;
    /* the file on disk now, so nothing written since it was read is lost */
    if (!ReadFile(SavePath(sEdVersion), sram) || !Ed::Read(sram, sEdFile, fresh) || fresh.block != sEdOriginal.block) {
        Notice(T("La partida cambió en el disco; se volvió a leer sin aplicar nada.",
                 "The save changed on disk; it was read again and nothing was applied."), true);
        EditorLoad();
        return;
    }
    if (!BackupSave(sEdVersion))
        return;
    Ed::Write(sram, sEdWork);
    if (!RekhcomSave::Replace(SavePath(sEdVersion), sram)) {
        Notice(T("No se pudo escribir la partida (la copia de seguridad se conserva).",
                 "Could not write the save (the backup is kept)."), true);
        return;
    }
    {
        /* an edited save plays in Rebel mode (user 25/10): the game reads this mark beside the save */
        std::ofstream mark(fs::path(SavePath(sEdVersion)).concat(".rebel"), std::ios::trunc);
        mark << "edited in the launcher\n";
    }
    Notice(T("Cambios guardados; la copia anterior está en backups. Esta partida jugará en modo rebelde (logros en su propia lista).",
             "Changes saved; the previous copy is in backups. This save plays in Rebel mode (achievements in their own list)."),
           false);
    EditorLoad();
}

static bool NumberRow(const char* label, int value, int lo, int hi, int* out) {
    float w = ImGui::GetContentRegionAvail().x, ctrl = std::min(Px(220), w * 0.45f);
    ImGui::PushID(label);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(w - ctrl);
    ImGui::SetNextItemWidth(ctrl);
    int v = value;
    bool changed = ImGui::InputInt("##n", &v) && v != value;
    *out = std::clamp(v, lo, hi);
    ImGui::PopID();
    return changed;
}

static void PageEditor(SDL_Window* window) {
    if (sEdVersion < 0)
        sEdVersion = sSelected;
    SaveCard(window); /* the save manager: the save itself first, then the editor */
    ImGui::Spacing();
    BeginCard("editor");
    Eyebrow(T("EDITOR DE PARTIDAS", "SAVE EDITOR"));
    if (!sEditorOn) {
        ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
        ImGui::TextWrapped("%s", T("Cambia nivel, estadísticas y cartas de una partida guardada. Está desactivado hasta "
                                   "que lo actives y confirmes el aviso.",
                                   "Changes the level, stats and cards of a saved game. It stays off until you turn "
                                   "it on and accept the warning."));
        ImGui::PopStyleColor();
        ImGui::Spacing();
        if (ImGui::Button(T("Activar el editor...", "Turn the editor on..."), ImVec2(-FLT_MIN, 0)))
            ImGui::OpenPopup("##editor-warning");
        if (ImGui::BeginPopupModal("##editor-warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::PushTextWrapPos(Px(520));
            ImGui::TextWrapped("%s", T("Aviso: editar una partida puede dejarla en un estado que el juego no espera "
                                       "(por ejemplo, saltarse el equilibrio de las batallas). Antes de cada cambio se "
                                       "guarda una copia en backups/, y conviene cerrar el juego mientras editas.",
                                       "Warning: editing a save can leave it in a state the game does not expect (for "
                                       "example, skipping the balance of its battles). A copy is kept in backups/ "
                                       "before every change, and the game should be closed while you edit."));
            ImGui::PopTextWrapPos();
            ImGui::Spacing();
            if (ImGui::Button(T("Cancelar", "Cancel"), ImVec2(Px(200), 0)))
                ImGui::CloseCurrentPopup();
            ImGui::SameLine();
            if (ImGui::Button(T("Activar", "Turn on"), ImVec2(Px(200), 0))) {
                sEditorOn = true;
                sEdLoadedVersion = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::EndChild();
        return;
    }

    /* which save: version, bank (the game's own setting) and file */
    const char* versions[kVersionCount];
    for (int v = 0; v < kVersionCount; v++)
        versions[v] = T(kVersions[v].regionEs, kVersions[v].regionEn);
    CycleRow(T("Versión", "Version"), nullptr, sEdVersion, kVersionCount, versions);
    const char* const* files = sSpanish ? kFileNamesEs : kFileNamesEn;
    CycleRow(T("Archivo", "File"), nullptr, sEdFile, 4, files);
    if (sEdLoadedVersion != sEdVersion || sEdLoadedFile != sEdFile || sEdLoadedBank != Ini("save_bank"))
        EditorLoad();
    LoadCardTextures(sEdVersion);
    ImGui::PushFont(sFontSmall);
    ImGui::TextColored(kMuted, "%s %s (%s %d)", T("Partida:", "Save:"), SavePath(sEdVersion).filename().string().c_str(),
                       T("banco", "bank"), std::clamp(Ini("save_bank"), 1, 5));
    ImGui::PopFont();
    if (!sEdHasFile) {
        ImGui::TextColored(kMuted, "%s", T("Este archivo no tiene una partida guardada.", "This file holds no saved game."));
        ImGui::EndChild();
        return;
    }
    ImGui::EndChild();

    ImGui::Spacing();
    BeginCard("editor-stats");
    Eyebrow(T("PERSONAJE", "CHARACTER"));
    int v;
    if (NumberRow(T("Nivel (1-99)", "Level (1-99)"), sEdWork.Level(), 1, 99, &v)) sEdWork.SetLevel(v);
    if (NumberRow(T("HP máximo (hasta 560)", "Max HP (up to 560)"), sEdWork.MaxHp(), 1, 560, &v)) sEdWork.SetMaxHp(v);
    if (sEdWork.riku()) {
        if (NumberRow("AP", sEdWork.Ap(), 0, 999, &v)) sEdWork.SetAp(v);
        if (NumberRow("DP", sEdWork.Dp(), 0, 999, &v)) sEdWork.SetDp(v);
    } else {
        if (NumberRow("CP", sEdWork.Cp(), 0, 9999, &v)) sEdWork.SetCp(v);
    }
    if (NumberRow(T("Puntos moguri", "Moogle points"), (int)sEdWork.Moogle(), 0, 9999999, &v)) sEdWork.SetMoogle(v);
    ImGui::EndChild();

    ImGui::Spacing();
    BeginCard("editor-map");
    Eyebrow(T("CARTAS DE MAPA (HASTA 9 POR CARTA)", "MAP CARDS (AT MOST 9 EACH)"));
    CycleRow(T("Tipo", "Kind"), nullptr, sEdGroup, Cards::kMapGroupCount, Cards::kMapGroupNames);
    for (int value = 0; value < 10; value++) {
        char label[32];
        std::snprintf(label, sizeof(label), "%s %d", T("Valor", "Value"), value);
        if (NumberRow(label, sEdWork.MapCards()[sEdGroup * 10 + value], 0, 9, &v))
            sEdWork.SetMapCard(sEdGroup * 10 + value, v);
    }
    if (ImGui::Button(T("Todas las cartas de mapa a 9", "Every map card at 9"), ImVec2(-FLT_MIN, 0)))
        for (int id = 0; id < Cards::kMapGroupCount * 10; id++)
            sEdWork.SetMapCard(id, 9);
    ImGui::EndChild();

    if (!sEdWork.riku()) {
        ImGui::Spacing();
        BeginCard("editor-cards");
        Eyebrow(T("CARTAS DE COMBATE (99 POR CARTA, 999 EN TOTAL)", "BATTLE CARDS (99 EACH, 999 IN ALL)"));
        /* add: kind, value, copies, with the card's picture */
        {
            std::vector<int> kinds;
            for (int id = 0; id < Cards::kCardCount; id++) {
                const Cards::Card& c = Cards::kCards[id];
                /* Card kinds 0..97 are player kinds, except boss-only Marluxia 2 (95). */
                if (c.kind <= 97 && c.kind != 95 && c.category <= 3 && !(c.flags & (0x8 | 0x10)) &&
                    std::find(kinds.begin(), kinds.end(), c.kind) == kinds.end())
                    kinds.push_back(c.kind);
            }
            sEdAddKind = std::clamp(sEdAddKind, 0, (int)kinds.size() - 1);
            std::vector<const char*> names;
            for (int k : kinds)
                names.push_back(Cards::kKindNames[k]);
            CycleRow(T("Carta", "Card"), nullptr, sEdAddKind, (int)names.size(), names.data());
            static const char* const values[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
            CycleRow(T("Valor", "Value"), nullptr, sEdAddValue, 10, values);
            int id = -1;
            for (int i = 0; i < Cards::kCardCount && id < 0; i++)
                if (Cards::kCards[i].kind == kinds[sEdAddKind] && Cards::kCards[i].value == sEdAddValue)
                    id = i;
            if (NumberRow(T("Copias", "Copies"), sEdAddCopies, 1, 99, &v)) sEdAddCopies = v;
            if (id >= 0) {
                CardPicture(id);
                ImGui::SameLine();
                ImGui::BeginDisabled(sEdWork.CountCard(id) >= 99);
                if (ImGui::Button(T("Añadir", "Add"), ImVec2(-FLT_MIN, 0)) &&
                    sEdWork.AddCard(id, sEdAddCopies, false) < sEdAddCopies)
                    Notice(T("No caben más: 99 por carta y 999 en la colección.",
                             "No room left: 99 per card, 999 in the collection."), true);
                ImGui::EndDisabled();
            } else {
                ImGui::TextColored(kMuted, "%s", T("Esta carta no existe con ese valor.", "This card has no such value."));
            }
        }
        ImGui::Separator();
        /* the collection, one row per card */
        std::vector<int> owned;
        for (int id = 0; id < Cards::kCardCount; id++)
            if (sEdWork.CountCard(id) > 0)
                owned.push_back(id);
        int total = 0;
        for (int i = 0; i < Ed::kCollection; i++)
            total += sEdWork.Card(i) != Ed::kEmpty;
        ImGui::TextColored(kMuted, "%s %d / 999", T("En la colección:", "In the collection:"), total);
        ImGuiListClipper clip;
        clip.Begin((int)owned.size(), 34.0f + ImGui::GetStyle().ItemSpacing.y);
        while (clip.Step())
            for (int row = clip.DisplayStart; row < clip.DisplayEnd; row++) {
                int id = owned[row], n = sEdWork.CountCard(id);
                ImGui::PushID(id);
                CardPicture(id);
                ImGui::SameLine();
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s  x%d", CardName(id).c_str(), n);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - Px(90));
                if (ImGui::Button("-", ImVec2(Px(40), 0)) && sEdWork.RemoveCard(id, 1) == 0)
                    Notice(T("Las copias que quedan están en un mazo.", "The copies left are in a deck."), true);
                ImGui::SameLine();
                if (ImGui::Button("+", ImVec2(Px(40), 0)))
                    sEdWork.AddCard(id, 1, false);
                ImGui::PopID();
            }
        ImGui::EndChild();
    }

    /* apply: every change listed first */
    ImGui::Spacing();
    std::vector<std::string> changes = EditorChanges();
    ImGui::BeginDisabled(changes.empty());
    if (ImGui::Button(T("Revisar y guardar cambios...", "Review and save changes..."), ImVec2(-FLT_MIN, Px(40))))
        ImGui::OpenPopup("##editor-apply");
    ImGui::EndDisabled();
    if (!changes.empty() && ImGui::Button(T("Descartar cambios", "Discard changes"), ImVec2(-FLT_MIN, 0)))
        sEdWork = sEdOriginal;
    if (ImGui::BeginPopupModal("##editor-apply", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s %s - %s", T("Se cambiará", "Will change"), SavePath(sEdVersion).filename().string().c_str(),
                    files[sEdFile]);
        ImGui::BeginChild("##list", ImVec2(Px(560), Px(260)), ImGuiChildFlags_Borders);
        for (const std::string& line : changes)
            ImGui::TextUnformatted(line.c_str());
        ImGui::EndChild();
        if (ImGui::Button(T("Cancelar", "Cancel"), ImVec2(Px(200), 0)))
            ImGui::CloseCurrentPopup();
        ImGui::SameLine();
        if (ImGui::Button(T("Copiar y guardar", "Back up and save"), ImVec2(Px(200), 0))) {
            EditorApply();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
