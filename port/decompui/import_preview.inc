/* ---- import preview (included by decompui.cpp after the save editor page) --------- *
 * "Load this save?" before an imported save is written: what the incoming file holds
 * (Sora 1-2, Riku 1-2: level, place, time, deck) and the five banks it can go to, with
 * what each one holds now. Pictures (the faces, the deck's cards) are read from the
 * ROM the player installed, with LoadCardTextures of the save editor; nothing of the
 * game is shipped, and without a ROM the window is text only. Replacing a bank that is
 * in use asks twice and copies it to backups/ first, as before. */

struct PvFile {
    bool has = false;
    bool riku = false;
    int level = 0, world = 0, floor = 0, collection = 0, mapCards = 0;
    uint32_t seconds = 0;
    std::vector<std::pair<int, int>> deck; /* card id, copies, in the active deck's order */
    int deckCards = 0;
};

struct PvBank {
    bool exists = false;
    bool occupied = false;
    PvFile files[4];
};

static struct ImportPreview {
    bool pending = false, open = false, replaceAsked = false, scrollToBank = false;
    fs::path source;
    int version = 0, bank = 1, blocks = 0, from = 0;
    std::string how;
    std::vector<unsigned char> data;
    PvBank incoming, banks[6]; /* banks[1..5] */
} sPv;

/* GBA sprite frames of the HP gauge's faces: ROM offsets (frame, tiles, palette) per cartridge, Sora then Riku */
struct PvFaceRom {
    unsigned frame[2], tiles[2], palette[2];
};
static const PvFaceRom kPvFaceRom[3] = {
    { { 0xB4BC00, 0xB4C2D0 }, { 0xB4BC4E, 0xB4C318 }, { 0xF648FC, 0x154B004 } },  /* B8CP */
    { { 0xB20D20, 0xB213F0 }, { 0xB20D6E, 0xB21438 }, { 0xF683A4, 0x1618118 } },  /* B8CE */
    { { 0xB14308, 0xB149D8 }, { 0xB14356, 0xB14A20 }, { 0xF5B8AC, 0x15D0BF4 } },  /* B8CJ */
};
static GLuint sPvFaceTex[2];
static int sPvFaceW[2], sPvFaceH[2], sPvFaceVersion = -1;

/* The sprite frame as the game's OAM draws it (count, then attr0-2 per object; 1D tiles), cropped to its objects */
static void PvRenderFace(const std::vector<unsigned char>& rom, unsigned frame, unsigned tiles, unsigned palette,
                         std::vector<unsigned char>& rgba, int& w, int& h) {
    static const int sizes[3][4][2] = {
        { { 8, 8 }, { 16, 16 }, { 32, 32 }, { 64, 64 } },
        { { 16, 8 }, { 32, 8 }, { 32, 16 }, { 64, 32 } },
        { { 8, 16 }, { 8, 32 }, { 16, 32 }, { 32, 64 } },
    };
    auto u16 = [&](unsigned at) { return (unsigned)(rom[at] | rom[at + 1] << 8); };
    int count = (int)u16(frame);
    struct Obj { int x, y, w, h; unsigned tile; };
    std::vector<Obj> objs;
    int minX = 1 << 20, minY = 1 << 20, maxX = -(1 << 20), maxY = -(1 << 20);
    for (int i = 0; i < count && i < 8; i++) {
        unsigned a0 = u16(frame + 2 + i * 6), a1 = u16(frame + 4 + i * 6), a2 = u16(frame + 6 + i * 6);
        int shape = a0 >> 14, size = a1 >> 14;
        if (shape > 2) continue;
        Obj o;
        o.y = (int)(a0 & 255);
        if (o.y >= 128) o.y -= 256;
        o.x = (int)(a1 & 511);
        if (o.x >= 256) o.x -= 512;
        o.w = sizes[shape][size][0];
        o.h = sizes[shape][size][1];
        o.tile = a2 & 1023;
        objs.push_back(o);
        minX = std::min(minX, o.x); minY = std::min(minY, o.y);
        maxX = std::max(maxX, o.x + o.w); maxY = std::max(maxY, o.y + o.h);
    }
    w = h = 0;
    if (objs.empty()) return;
    w = maxX - minX;
    h = maxY - minY;
    rgba.assign((size_t)w * h * 4, 0);
    unsigned colors[16];
    for (int i = 0; i < 16; i++) colors[i] = u16(palette + i * 2);
    for (const Obj& o : objs)
        for (int ty = 0; ty < o.h / 8; ty++)
            for (int tx = 0; tx < o.w / 8; tx++) {
                size_t base = tiles + (size_t)(o.tile + ty * (o.w / 8) + tx) * 32;
                if (base + 32 > rom.size()) continue;
                for (int y = 0; y < 8; y++)
                    for (int x = 0; x < 8; x++) {
                        unsigned char b = rom[base + y * 4 + x / 2];
                        int c = (x & 1) ? b >> 4 : b & 15;
                        if (!c) continue;
                        unsigned col = colors[c];
                        unsigned char* p = &rgba[(((size_t)(o.y - minY + ty * 8 + y)) * w + (o.x - minX + tx * 8 + x)) * 4];
                        p[0] = (unsigned char)((col & 31) * 255 / 31);
                        p[1] = (unsigned char)(((col >> 5) & 31) * 255 / 31);
                        p[2] = (unsigned char)(((col >> 10) & 31) * 255 / 31);
                        p[3] = 255;
                    }
            }
}

static void PvLoadFaces(int v) {
    if (sPvFaceVersion == v) return;
    std::vector<unsigned char> rom;
    if (!sState[v].installed || !ReadFile(RomPath(v), rom) || rom.size() != kRomSize) return;
    if (sPvFaceVersion >= 0) glDeleteTextures(2, sPvFaceTex);
    std::memset(sPvFaceTex, 0, sizeof(sPvFaceTex));
    for (int who = 0; who < 2; who++) {
        std::vector<unsigned char> rgba;
        int w = 0, h = 0;
        PvRenderFace(rom, kPvFaceRom[v].frame[who], kPvFaceRom[v].tiles[who], kPvFaceRom[v].palette[who], rgba, w, h);
        sPvFaceW[who] = w;
        sPvFaceH[who] = h;
        if (!w) continue;
        glGenTextures(1, &sPvFaceTex[who]);
        glBindTexture(GL_TEXTURE_2D, sPvFaceTex[who]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    }
    sPvFaceVersion = v;
}

/* the ROM the pictures come from: this version's, else any installed one; -1 text only */
static int PvArtVersion(int v) {
    if (sState[v].installed) return v;
    for (int i = 0; i < kVersionCount; i++)
        if (sState[i].installed) return i;
    return -1;
}

static fs::path PvBankPath(int v, int bank) {
    static const char* const names[] = { "kh_com", "kh_com_us", "kh_com_jp" };
    return sRoot / (std::string(names[v]) + (bank > 1 ? "_bank" + std::to_string(bank) : "") + ".sav");
}

static const char* PvWorld(int id) {
    static const char* const es[] = { "Inicio", "Agrabah", "Atl\xC3\xA1ntica", "Coliseo del Olimpo", "Pa\xC3\xADs de las Maravillas", "Monstro",
                                      "Ciudad de Halloween", "Nunca Jam\xC3\xA1s", "Basti\xC3\xB3n Hueco", "Islas del Destino",
                                      "Ciudad de Paso", "Villa Crep\xC3\xBAsculo", "Castillo del Olvido", "Bosque de los 100 Acres" };
    static const char* const en[] = { "Start", "Agrabah", "Atlantica", "Olympus Coliseum", "Wonderland", "Monstro", "Halloween Town",
                                      "Never Land", "Hollow Bastion", "Destiny Islands", "Traverse Town", "Twilight Town",
                                      "Castle Oblivion", "100 Acre Wood" };
    return id >= 0 && id <= 13 ? (sSpanish ? es : en)[id] : "?";
}

static PvFile PvReadFile(const std::vector<unsigned char>& sram, int file) {
    PvFile out;
    Ed::File f;
    if (!Ed::Read(sram, file, f)) return out;
    out.has = true;
    out.riku = f.riku();
    out.level = f.Level();
    out.world = f.World();
    out.floor = f.Floor();
    out.seconds = f.PlayTime();
    const uint8_t* maps = f.MapCards();
    for (int i = 0; i < (int)Ed::kMapCardCount; i++) out.mapCards += maps[i];
    if (!f.riku()) {
        for (int i = 0; i < Ed::kCollection; i++)
            out.collection += f.Card(i) != Ed::kEmpty;
        int deck = std::min<int>(f.data()[Ed::kSlice], 2);
        const uint8_t* d = f.data() + Ed::kDecks + deck * 0xE0;
        int count = std::min<int>(Ed::Get16(d + 0xDC), 99);
        for (int i = 0; i < count; i++) {
            unsigned slot = Ed::Get16(d + 2 * i);
            if (slot >= (unsigned)Ed::kCollection || f.Card((int)slot) == Ed::kEmpty) continue;
            int id = f.Card((int)slot) & Ed::kIdMask;
            if (id >= Cards::kCardCount) continue;
            out.deckCards++;
            auto at = std::find_if(out.deck.begin(), out.deck.end(), [&](const std::pair<int, int>& e) { return e.first == id; });
            if (at == out.deck.end()) out.deck.push_back({ id, 1 });
            else at->second++;
        }
    }
    return out;
}

static PvBank PvReadBank(const std::vector<unsigned char>& sram) {
    PvBank bank;
    bank.exists = true;
    for (int i = 0; i < 4; i++) {
        bank.files[i] = PvReadFile(sram, i);
        bank.occupied = bank.occupied || bank.files[i].has;
    }
    return bank;
}

static void PvLoadBanks() {
    for (int b = 1; b <= 5; b++) {
        std::vector<unsigned char> sram;
        sPv.banks[b] = PvBank();
        if (ReadFile(PvBankPath(sPv.version, b), sram, 4u << 20))
            sPv.banks[b] = PvReadBank(sram);
        else
            sPv.banks[b].exists = fs::exists(PvBankPath(sPv.version, b));
    }
}

/* backups/<name>-<date>-<time>.sav, same place and naming as the launcher's own backup */
static bool PvBackup(const fs::path& from) {
    std::error_code ec;
    static unsigned sequence = 0;
    char stamp[80];
    std::time_t now = std::time(nullptr);
    std::tm t = *std::localtime(&now);
    std::snprintf(stamp, sizeof(stamp), "-%04d%02d%02d-%02d%02d%02d-%03u-%u", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                  t.tm_hour, t.tm_min, t.tm_sec, (unsigned)(SDL_GetTicks() % 1000), ++sequence);
    fs::path dir = sRoot / "backups";
    fs::create_directories(dir, ec);
    if (fs::copy_file(from, dir / (from.stem().string() + stamp + ".sav"), fs::copy_options::none, ec))
        return true;
    Notice(T("No se pudo copiar la partida.", "Could not copy the save."), true);
    return false;
}

/* the write itself: the bank's current file is copied to backups/, then replaced in one step */
static bool ImportCommit(int v, int bank, const std::vector<unsigned char>& out, int blocks, int from) {
    std::error_code ec;
    fs::path target = PvBankPath(v, bank);
    bool exists = fs::exists(target, ec);
    if (ec) {
        Notice(T("No se pudo verificar el destino.", "Could not verify the destination."), true);
        return false;
    }
    if (exists && !PvBackup(target)) return false;
    if (!RekhcomSave::Replace(target, out)) {
        Notice(T("No se pudo escribir la partida. Original conservado.", "Could not write the save. Original preserved."), true);
        return false;
    }
    if (Ini("save_bank") != bank) { /* the imported bank is the one the game opens next */
        Ini("save_bank") = bank;
        SaveIni();
    }
    char msg[200];
    bool cross = (from == 0) != (v == 0);
    std::snprintf(msg, sizeof(msg), "%s %s (%d %s)%s", T("Partida importada en", "Save imported into"),
                  target.filename().string().c_str(), blocks, T("bloques", "blocks"),
                  cross ? T(", convertida de otra regi\xC3\xB3n", ", converted from another region") : "");
    Notice(msg, false);
    return true;
}

/* The converted save is ready: ask first. Harness: DECOMPUI_IMPORT_PREVIEW=1 (or "replace" for the second
 * question) shows the window in the shots; DECOMPUI_IMPORT_BANK=n picks the bank; without PREVIEW a shot run
 * writes at once, as it always did. */
static bool ImportPreviewBegin(const fs::path& file, int v, const std::vector<unsigned char>& out, int blocks, int from,
                               const char* how) {
    int bank = std::clamp(Ini("save_bank"), 1, 5);
    const char* shot = SDL_getenv("DECOMPUI_SHOT");
    if (shot && SDL_getenv("DECOMPUI_IMPORT_BANK")) bank = std::clamp(SDL_atoi(SDL_getenv("DECOMPUI_IMPORT_BANK")), 1, 5);
    if (shot && !SDL_getenv("DECOMPUI_IMPORT_PREVIEW"))
        return ImportCommit(v, bank, out, blocks, from);
    sPv.source = file;
    sPv.version = v;
    sPv.bank = bank;
    sPv.blocks = blocks;
    sPv.from = from;
    sPv.how = how ? how : "";
    sPv.data = out;
    sPv.incoming = PvReadBank(out);
    PvLoadBanks();
    sPv.pending = true;
    sPv.scrollToBank = true;
    sPv.replaceAsked = shot && !std::strcmp(SDL_getenv("DECOMPUI_IMPORT_PREVIEW"), "replace");
    return true;
}

/* one file: face, name and level, place / time / cards, then the deck's pictures */
static void PvFileRow(const PvFile& f, int file, int art, int maxCards, float textW) {
    int z = std::max(1, (int)std::lround(Px(1)));
    float rowH = std::max(36.0f * z, Px(44));
    int who = file >= 2 ? 1 : 0;
    char name[32];
    std::snprintf(name, sizeof(name), "%s %d", who ? "Riku" : "Sora", file % 2 + 1);
    ImGui::PushID(file);
    ImVec2 p = ImGui::GetCursorScreenPos();
    float faceW = 44.0f * z;
    bool face = art >= 0 && sPvFaceVersion == art && sPvFaceTex[who] != 0;
    if (face) {
        float w = sPvFaceW[who] * z, h = sPvFaceH[who] * z;
        ImGui::SetCursorScreenPos(ImVec2(p.x + (faceW - w) * 0.5f, p.y + (rowH - h) * 0.5f));
        ImGui::Image((ImTextureID)(intptr_t)sPvFaceTex[who], ImVec2(w, h));
    }
    float tx = p.x + (art >= 0 ? faceW + Px(8) : 0);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImFont* body = ImGui::GetFont();
    float bs = ImGui::GetFontSize(), ss = sFontSmall->FontSize;
    if (!f.has) {
        dl->AddText(body, bs, ImVec2(tx, p.y + (rowH - bs) * 0.5f), ImGui::GetColorU32(kMuted),
                    (std::string(name) + "   " + T("Vac\xC3\xADo", "Empty")).c_str());
    } else {
        char l1[96], l2[160];
        std::snprintf(l1, sizeof(l1), "%s   %s %d", name, T("Nv.", "Lv."), f.level);
        int n = std::snprintf(l2, sizeof(l2), "%s \xC2\xB7 %s %d \xC2\xB7 %u:%02u:%02u", PvWorld(f.world), T("Planta", "Floor"),
                              f.floor + 1, f.seconds / 3600, f.seconds / 60 % 60, f.seconds % 60);
        if (f.riku) std::snprintf(l2 + n, sizeof(l2) - n, " \xC2\xB7 %d %s", f.mapCards, T("cartas de mapa", "map cards"));
        else std::snprintf(l2 + n, sizeof(l2) - n, " \xC2\xB7 %s %d \xC2\xB7 %s %d", T("mazo", "deck"), f.deckCards,
                           T("colecci\xC3\xB3n", "collection"), f.collection);
        dl->AddText(body, bs, ImVec2(tx, p.y + rowH * 0.5f - bs - Px(1)), ImGui::GetColorU32(kText), l1);
        dl->AddText(sFontSmall, ss, ImVec2(tx, p.y + rowH * 0.5f + Px(1)), ImGui::GetColorU32(kMuted), l2);
        if (art >= 0 && !f.riku) {
            float x = p.x + faceW + Px(8) + textW;
            LoadCardTextures(art);
            int shown = 0;
            for (const auto& e : f.deck) {
                if (shown == maxCards) {
                    char more[16];
                    std::snprintf(more, sizeof(more), "+%d", (int)f.deck.size() - maxCards);
                    dl->AddText(body, bs, ImVec2(x, p.y + (rowH - bs) * 0.5f), ImGui::GetColorU32(kMuted), more);
                    break;
                }
                int sprite = Cards::kCards[e.first].sprite[art];
                if (sprite >= 0 && sCardTex[sprite] != 0 && sCardTexVersion == art) {
                    ImVec2 a(x, p.y + (rowH - 32.0f * z) * 0.5f);
                    dl->AddImage((ImTextureID)(intptr_t)sCardTex[sprite], a, ImVec2(a.x + 16.0f * z, a.y + 32.0f * z));
                    if (e.second > 1) {
                        char c[8];
                        std::snprintf(c, sizeof(c), "%d", e.second);
                        ImVec2 sz = sFontSmall->CalcTextSizeA(ss, FLT_MAX, 0, c);
                        ImVec2 q(a.x + 16.0f * z - sz.x, a.y + 32.0f * z - sz.y);
                        dl->AddRectFilled(ImVec2(q.x - 1, q.y), ImVec2(q.x + sz.x + 1, q.y + sz.y), IM_COL32(0, 0, 0, 170));
                        dl->AddText(sFontSmall, ss, q, IM_COL32(255, 255, 255, 255), c);
                    }
                    x += 18.0f * z;
                    shown++;
                }
            }
        }
    }
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy(ImVec2(1, rowH));
    ImGui::PopID();
}

static const char* PvHow(const std::string& how) {
    if (how == "unpacked") return T("contenedor comprimido (estado, ZIP o GZIP)", "compressed container (state, ZIP or GZIP)");
    if (how.rfind("swap", 0) == 0 || how == "reversed8") return T("con el orden de bytes cambiado", "with its byte order swapped");
    return T("partida guardada", "saved game");
}

static void ImportPreviewDraw() {
    if (sPv.pending) {
        sPv.pending = false;
        sPv.open = true;
        ImGui::OpenPopup("##import-preview");
    }
    if (!sPv.open) return;
    ImGuiIO& io = ImGui::GetIO();
    int art = PvArtVersion(sPv.version);
    if (art >= 0) {
        LoadCardTextures(art);
        PvLoadFaces(art);
    }
    float width = std::min(Px(880), io.DisplaySize.x - Px(24));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(width, 0));
    bool visible = ImGui::BeginPopupModal("##import-preview", nullptr,
                                          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                              ImGuiWindowFlags_AlwaysAutoResize);
    if (!visible) {
        sPv.open = false;
        return;
    }
    bool cancel = ImGui::IsKeyPressed(ImGuiKey_Escape) && !sPv.replaceAsked;
    bool occupied = sPv.banks[sPv.bank].occupied;
    ImGui::PushFont(sFontMenu);
    ImGui::TextUnformatted(T("\xC2\xBF" "Cargar esta partida guardada?", "Load this saved game?"));
    ImGui::PopFont();
    ImGui::PushFont(sFontSmall);
    {
        std::string line = sPv.source.filename().string() + "  \xC2\xB7  " + PvHow(sPv.how);
        if ((sPv.from == 0) != (sPv.version == 0))
            line += std::string("  \xC2\xB7  ") + T("se convierte a ", "converted to ") + T(kVersions[sPv.version].regionEs, kVersions[sPv.version].regionEn);
        ImGui::TextColored(kMuted, "%s", line.c_str());
    }
    ImGui::PopFont();
    ImGui::Spacing();
    float textW = Px(360);
    for (int i = 0; i < 4; i++)
        PvFileRow(sPv.incoming.files[i], i, art, 12, textW);
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::PushFont(sFontMenu);
    ImGui::TextUnformatted(T("\xC2\xBF" "En qu\xC3\xA9 banco?", "Into which bank?"));
    ImGui::PopFont();
    ImGui::BeginChild("##pv-banks", ImVec2(0, std::min(Px(250), io.DisplaySize.y * 0.30f)), ImGuiChildFlags_Borders);
    int z = std::max(1, (int)std::lround(Px(1)));
    for (int b = 1; b <= 5; b++) {
        const PvBank& bank = sPv.banks[b];
        int shown = 0;
        for (int i = 0; i < 4; i++) shown += bank.files[i].has;
        float headH = Px(30), rowH = std::max(36.0f * z, Px(44));
        float h = headH + shown * rowH + Px(6);
        ImGui::PushID(b);
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        bool selected = sPv.bank == b;
        if (ImGui::InvisibleButton("##bank", ImVec2(w, h), ImGuiButtonFlags_EnableNav)) sPv.bank = b;
        if (selected && sPv.scrollToBank) { /* the list opens on the bank that is picked */
            ImGui::SetScrollHereY(0.0f);
            sPv.scrollToBank = false;
        }
        bool hover = ImGui::IsItemHovered();
        dl->AddRectFilled(p0, ImVec2(p0.x + w, p0.y + h), ImGui::GetColorU32(selected ? kPanelHover : (hover ? kControl : kPanel)));
        dl->AddRectFilled(ImVec2(p0.x, p0.y + h - Px(2)), ImVec2(p0.x + w, p0.y + h), ImGui::GetColorU32(selected ? kText : kBorder));
        if (ImGui::IsItemFocused())
            dl->AddRect(ImVec2(p0.x - 1, p0.y - 1), ImVec2(p0.x + w + 1, p0.y + h + 1), ImGui::GetColorU32(kFocus), 0, 0, Px(2));
        GameSquare(dl, ImVec2(p0.x + Px(18), p0.y + headH * 0.5f), Px(14), selected);
        char title[64];
        std::snprintf(title, sizeof(title), "%s %d", T("Banco", "Bank"), b);
        dl->AddText(sFontMenu, sFontMenu->FontSize, ImVec2(p0.x + Px(36), p0.y + (headH - sFontMenu->FontSize) * 0.5f),
                    ImGui::GetColorU32(kText), title);
        const char* state = bank.occupied ? T("En uso", "In use")
                            : bank.exists ? T("Vac\xC3\xADo (sin partidas)", "Empty (no saves)")
                                          : T("Vac\xC3\xADo", "Empty");
        float sw = sFontSmall->CalcTextSizeA(sFontSmall->FontSize, FLT_MAX, 0, state).x;
        dl->AddText(sFontSmall, sFontSmall->FontSize, ImVec2(p0.x + w - sw - Px(14), p0.y + (headH - sFontSmall->FontSize) * 0.5f),
                    ImGui::GetColorU32(bank.occupied ? kWarn : kMuted), state);
        int row = 0;
        for (int i = 0; i < 4; i++) {
            if (!bank.files[i].has) continue;
            ImGui::SetCursorScreenPos(ImVec2(p0.x + Px(36), p0.y + headH + row * rowH));
            PvFileRow(bank.files[i], i, art, 8, Px(330));
            row++;
        }
        ImGui::SetCursorScreenPos(ImVec2(p0.x, p0.y + h + Px(4)));
        ImGui::Dummy(ImVec2(1, 1));
        ImGui::PopID();
    }
    ImGui::EndChild();
    ImGui::Spacing();
    if (ImGui::Button(T("Cancelar", "Cancel"), ImVec2(Px(200), 0)) || cancel) {
        ImGui::CloseCurrentPopup();
        sPv.open = false;
    }
    ImGui::SameLine();
    char label[96];
    std::snprintf(label, sizeof(label), occupied ? T("Reemplazar el banco %d...", "Replace bank %d...")
                                                 : T("Importar en el banco %d", "Import into bank %d"),
                  sPv.bank);
    bool go = ImGui::Button(label, ImVec2(-FLT_MIN, 0));
    if (go && !occupied) {
        ImportCommit(sPv.version, sPv.bank, sPv.data, sPv.blocks, sPv.from);
        ImGui::CloseCurrentPopup();
        sPv.open = false;
    } else if (go) {
        sPv.replaceAsked = true;
    }
    if (sPv.replaceAsked && !ImGui::IsPopupOpen("##import-replace")) ImGui::OpenPopup("##import-replace");
    if (ImGui::BeginPopupModal("##import-replace", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
        ImGui::PushTextWrapPos(Px(520));
        ImGui::TextWrapped(sSpanish ? "El banco %d ya tiene partidas. Se guardar\xC3\xA1 una copia en backups/ y despu\xC3\xA9s "
                                      "se reemplazar\xC3\xA1 por esta."
                                    : "Bank %d already holds saves. A copy goes to backups/ first, then it is replaced by this one.",
                           sPv.bank);
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        if (ImGui::Button(T("Cancelar", "Cancel"), ImVec2(Px(200), 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            sPv.replaceAsked = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(T("Copiar y reemplazar", "Back up and replace"), ImVec2(Px(200), 0))) {
            ImportCommit(sPv.version, sPv.bank, sPv.data, sPv.blocks, sPv.from);
            sPv.replaceAsked = false;
            ImGui::CloseCurrentPopup();
            sPv.open = false;
        }
        ImGui::EndPopup();
    }
    if (!sPv.open) ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}
