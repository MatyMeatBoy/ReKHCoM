#ifndef PORT_TUNING_H
#define PORT_TUNING_H

/* Renderer and audio fast paths whose output is identical to the reference
 * paths (host equivalence tests, GA36 captures and on-device measurements).
 * They are on by default on every platform. KHCOM_<NAME>=0 turns one off and
 * KHCOM_FAST_PATHS=0 turns all of them off, for comparisons. */
static const char* const kPortFastPaths[] = {
    "KHCOM_PPU_BG_PLAIN_ROWS", "KHCOM_PPU_FUSE_ALL_ROWS", "KHCOM_PPU_FUSE_PICTURE_BOTTOM",
    "KHCOM_PPU_HEADER_GONE_PARALLEL", "KHCOM_PPU_JOURNAL_CENTRAL_VECTOR", "KHCOM_PPU_JOURNAL_PAGE_VECTOR",
    "KHCOM_PPU_LIBRARY_PAIR", "KHCOM_PPU_MAPINSPECT_OPAQUE_VECTOR", "KHCOM_PPU_MENU_BG_COPY",
    "KHCOM_PPU_MENU_BG_VECTOR", "KHCOM_PPU_MENU_HEADER_PARALLEL", "KHCOM_PPU_MOSAIC_MARGINS",
    "KHCOM_PPU_NARROW_PLAIN_VECTOR", "KHCOM_PPU_NARROW_SECOND_VECTOR", "KHCOM_PPU_NARROW_SIDE_FILL",
    "KHCOM_PPU_NEON_EFFECT_COMPOSE", "KHCOM_PPU_NEON_NARROW_COMPOSE", "KHCOM_PPU_NEON_OPAQUE_COMPOSE",
    "KHCOM_PPU_NEON_SCREEN_COMPOSE", "KHCOM_PPU_NEON_TEXT_COVERAGE", "KHCOM_PPU_OBJ_ROW_RUNS",
    "KHCOM_PPU_SPOTLIGHT_BG0_EDGES", "KHCOM_PPU_SPOTLIGHT_BG0_RUNS", "KHCOM_PPU_SPOTLIGHT_CLEAR_ROWS",
    "KHCOM_PPU_SPOTLIGHT_OVERLAY_FAST", "KHCOM_PPU_SPOTLIGHT_SKIP_MARGINS", "KHCOM_PPU_SPOTLIGHT_TILE_RUNS",
    "KHCOM_PPU_SPOTLIGHT_TRIM_MARGINS", "KHCOM_PPU_STATIC_MOSAIC", "KHCOM_PPU_TEXT_BUCKETS",
    "KHCOM_PPU_TEXT_NEON", "KHCOM_PPU_TRANSPARENT_BG_MARGINS", "KHCOM_PPU_WORLDINSPECT_BACKGROUND_VECTOR",
    "KHCOM_PSG_REUSE_INTEGRAL", "KHCOM_PSG_SKIP_UNROUTED_WAVE",
};

static inline const char* PortTuningValue(const char* name, const char* value, const char* allPaths) {
    for (unsigned i = 0; i < sizeof(kPortFastPaths) / sizeof(kPortFastPaths[0]); i++) {
        const char* a = kPortFastPaths[i];
        const char* b = name;
        while (*a && *a == *b) a++, b++;
        if (*a || *b) continue;
        if (value) return value[0] == '0' && !value[1] ? (const char*)0 : value;
        return allPaths && allPaths[0] == '0' && !allPaths[1] ? (const char*)0 : "1";
    }
    return value;
}

#endif
