#ifndef PORT_HD_MEDIA_H
#define PORT_HD_MEDIA_H
/* Shared plain-stdio runtime contract. A committed generation is immutable;
 * only the tiny active index changes atomically. Labels are user declarations. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#endif
typedef struct PortHdMediaInfo {
    char label[4], source[64];
    int quality, movies, scenes, count;
} PortHdMediaInfo;
static inline FILE* PortHdMediaOpenFile(const char* path) {
#ifdef _WIN32
    wchar_t wide[2048];
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, 2048)) return NULL;
    return _wfopen(wide, L"rb");
#else
    return fopen(path, "rb");
#endif
}
static inline const char* PortHdMediaLabel(int mode) { return mode == 1 ? "en" : mode == 2 ? "ja" : NULL; }
static inline int PortHdMediaClipAllowed(int mode, int offsetSeconds) {
    return mode != 2 || offsetSeconds == 0; /* JP clip offsets need owned-source timing QA */
}
static inline int PortHdMediaToken(const char* text, const char* extra) {
    size_t i;
    if (!text || !*text) return 0;
    for (i = 0; text[i]; ++i)
        if (!(text[i] >= 'a' && text[i] <= 'z') && !(text[i] >= '0' && text[i] <= '9') && !strchr(extra, text[i])) return 0;
    return 1;
}
static inline int PortHdMediaName(const char* name) {
    size_t i;
    if (!name || !*name || strlen(name) > 80) return 0;
    for (i = 0; name[i]; ++i)
        if (!(name[i] >= 'a' && name[i] <= 'z') && !(name[i] >= 'A' && name[i] <= 'Z') &&
            !(name[i] >= '0' && name[i] <= '9') && name[i] != '_') return 0;
    return 1;
}
static inline int PortHdMediaMember(const char* member) {
    char name[96];
    const char* base;
    size_t n;
    if (strncmp(member, "movies/", 7) == 0) base = member + 7;
    else if (strncmp(member, "scenes/", 7) == 0) base = member + 7;
    else return 0;
    n = strlen(base);
    if (n < 5 || n >= sizeof(name) || strcmp(base + n - 4, ".mp4")) return 0;
    memcpy(name, base, n - 4); name[n - 4] = 0;
    return PortHdMediaName(name);
}
static inline int PortHdMediaLine(FILE* f, char* line, size_t size) {
    size_t n;
    if (!fgets(line, (int)size, f)) return 0;
    n = strlen(line);
    if (n == size - 1 && line[n - 1] != '\n') return 0;
    for (n = 0; line[n] && line[n] != '\r' && line[n] != '\n'; ++n) {}
    line[n] = 0;
    return 1;
}
static inline int PortHdMediaReadActive(FILE* f, char generations[2][64]) {
    char line[256], key[16], gen[64]; int i, used;
    if (!PortHdMediaLine(f, line, sizeof(line)) || strcmp(line, "REKHCOM_MEDIA_ACTIVE_V1")) return 0;
    for (i = 0; i < 2; ++i) {
        used = 0;
        if (!PortHdMediaLine(f, line, sizeof(line)) || sscanf(line, "%15s %63s%n", key, gen, &used) != 2 || line[used] ||
            strcmp(key, i ? "scenes" : "movies") || (strcmp(gen, "-") && (strncmp(gen, "g-", 2) || !PortHdMediaToken(gen, "-")))) {
            return 0;
        }
        strcpy(generations[i], gen);
    }
    i = fgetc(f);
    return i == EOF;
}
static inline int PortHdMediaActive(const char* data, int mode, char generations[2][64]) {
    char path[1200]; FILE* f; int ok;
    const char* label = PortHdMediaLabel(mode);
    if (!label || snprintf(path, sizeof(path), "%s/hd/media/%s/active.txt", data, label) >= (int)sizeof(path)) return 0;
    f = PortHdMediaOpenFile(path);
    if (!f) return 0;
    ok = PortHdMediaReadActive(f, generations); fclose(f); return ok;
}
static inline int PortHdMediaNumber(const char* text, uint64_t limit, uint64_t* out) {
    uint64_t value = 0;
    if (!*text) return 0;
    for (; *text; ++text) {
        unsigned digit = (unsigned)(*text - '0');
        if (digit > 9 || digit > limit || value > (limit - digit) / 10) return 0;
        value = value * 10 + digit;
    }
    *out = value; return 1;
}
static inline FILE* PortHdMediaManifest(const char* base, int mode, PortHdMediaInfo* info) {
    char path[1400], line[256], evidence[16], q[4], movies[4], scenes[4], count[4]; int used = 0;
    uint64_t qualityValue, moviesValue, scenesValue, countValue;
    FILE* f;
    snprintf(path, sizeof(path), "%s/media.meta", base);
    f = PortHdMediaOpenFile(path);
    if (!f) return NULL;
    if (!PortHdMediaLine(f, line, sizeof(line)) ||
        sscanf(line, "REKHCOM_MEDIA_V1 %3s %15s %63s %3s %3s %3s %3s%n", info->label, evidence, info->source,
               q, movies, scenes, count, &used) != 7 || !used || line[used] ||
        !PortHdMediaLabel(mode) || strcmp(info->label, PortHdMediaLabel(mode)) || strcmp(evidence, "user") ||
        !PortHdMediaToken(info->source, "") || !PortHdMediaNumber(q, 2, &qualityValue) ||
        !PortHdMediaNumber(movies, 1, &moviesValue) || !PortHdMediaNumber(scenes, 1, &scenesValue) ||
        !PortHdMediaNumber(count, 256, &countValue) || !countValue) {
        fclose(f); return NULL;
    }
    info->quality = (int)qualityValue; info->movies = (int)moviesValue;
    info->scenes = (int)scenesValue; info->count = (int)countValue;
    return f;
}
static inline int PortHdMediaEntry(FILE* f, char member[128], uint64_t* bytes) {
    char line[256], number[24]; uint64_t size = 0; int used = 0;
    if (!PortHdMediaLine(f, line, sizeof(line)) || sscanf(line, "%127s %23s%n", member, number, &used) != 2 ||
        !used || line[used] || !PortHdMediaMember(member) || !PortHdMediaNumber(number, 1024u * 1024u * 1024u, &size) || size < 16) return 0;
    *bytes = size;
    return 1;
}
static inline uint32_t PortHdMediaBe32(const unsigned char* p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
static inline int PortHdMediaMp4(const char* path, uint64_t expected) {
    FILE* f = PortHdMediaOpenFile(path);
    uint64_t at = 0; long actual; unsigned boxes = 0; int moov = 0, mdat = 0;
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) || (actual = ftell(f)) < 16 || (uint64_t)actual != expected || expected > INT_MAX) { fclose(f); return 0; }
    while (at + 8 <= expected && ++boxes <= 4096) {
        unsigned char h[16]; uint64_t size;
        if (fseek(f, (long)at, SEEK_SET) || fread(h, 1, 8, f) != 8) break;
        size = PortHdMediaBe32(h);
        if (size == 1) {
            if (fread(h + 8, 1, 8, f) != 8) break;
            size = (uint64_t)PortHdMediaBe32(h + 8) << 32 | PortHdMediaBe32(h + 12);
            if (size < 16) break;
        } else if (!size) size = expected - at;
        if (size < 8 || size > expected - at) break;
        moov |= !memcmp(h + 4, "moov", 4); mdat |= !memcmp(h + 4, "mdat", 4);
        at += size;
    }
    fclose(f);
    return at == expected && moov && mdat;
}
static inline int PortHdMediaGeneration(const char* data, int mode, const char* generation, char* base, size_t size) {
    int n;
    if (!PortHdMediaLabel(mode) || strncmp(generation, "g-", 2) || !PortHdMediaToken(generation, "-")) return 0;
    n = snprintf(base, size, "%s/hd/media/%s/%s", data, PortHdMediaLabel(mode), generation);
    return n >= 0 && (size_t)n < size;
}
static inline int PortHdMediaCheck(const char* base, int mode, int component, PortHdMediaInfo* info, const char* wanted, uint64_t* wantedSize) {
    FILE* f = PortHdMediaManifest(base, mode, info);
    int i, ok = f != NULL, found = wanted == NULL;
    char previous[128] = "", member[128], path[1500]; uint64_t bytes;
    if (!f) return 0;
    for (i = 0; i < info->count && ok; ++i) {
        ok = PortHdMediaEntry(f, member, &bytes) && strcmp(previous, member) < 0;
        if (!ok) break;
        strcpy(previous, member);
        if (!wanted && (component < 0 || (component == 1 && !strncmp(member, "scenes/", 7)) ||
            (component == 0 && (!strncmp(member, "movies/", 7) || !strcmp(member, "scenes/zz_event_151.mp4") ||
                               !strcmp(member, "scenes/zz_event_621.mp4") || !strcmp(member, "scenes/zz_event_631.mp4"))))) {
            snprintf(path, sizeof(path), "%s/%s", base, member);
            ok = PortHdMediaMp4(path, bytes);
        } else if (!strcmp(wanted, member)) { found = 1; *wantedSize = bytes; }
    }
    ok = ok && fgetc(f) == EOF && found && (component < 0 || (component ? info->scenes : info->movies));
    fclose(f); return ok;
}
static inline int PortHdMediaAvailable(const char* data, int mode, int component) {
    char generations[2][64], base[1300]; PortHdMediaInfo info;
    return component >= 0 && component < 2 && PortHdMediaActive(data, mode, generations) &&
           PortHdMediaGeneration(data, mode, generations[component], base, sizeof(base)) &&
           PortHdMediaCheck(base, mode, component, &info, NULL, NULL);
}
typedef struct PortHdMediaAvailabilityCache {
    char root[1024], generation[64];
    int known, result;
} PortHdMediaAvailabilityCache;
static PortHdMediaAvailabilityCache sPortHdMediaAvailability[4];
static inline void PortHdMediaAvailabilityReset(void) { memset(sPortHdMediaAvailability, 0, sizeof(sPortHdMediaAvailability)); }
static inline int PortHdMediaCachedAvailable(const char* data, int mode, int component) {
    char generations[2][64]; PortHdMediaAvailabilityCache* cache;
    if (mode < 1 || mode > 2 || component < 0 || component > 1) return 0;
    if (strlen(data) >= sizeof(sPortHdMediaAvailability[0].root)) return PortHdMediaAvailable(data, mode, component);
    if (!PortHdMediaActive(data, mode, generations)) return 0;
    cache = &sPortHdMediaAvailability[(mode - 1) * 2 + component];
    if (!cache->known || strcmp(cache->root, data) || strcmp(cache->generation, generations[component])) {
        strcpy(cache->root, data); strcpy(cache->generation, generations[component]);
        cache->result = PortHdMediaAvailable(data, mode, component); cache->known = 1;
    }
    return cache->result; // immutable generation: only tiny index reads until explicit rescan
}
static inline int PortHdMediaResolve(const char* data, int mode, const char* name, char* out, size_t size) {
    char generations[2][64], base[1300], member[128]; PortHdMediaInfo info;
    uint64_t bytes = 0; int i, n, scene;
    if (out && size) out[0] = 0;
    if (!out || !size || !PortHdMediaName(name)) return 0;
    scene = !strncmp(name, "zz_event_", 9);
    if (!mode) {
        FILE* f;
        n = snprintf(out, size, "%s/hd/%s/%s.mp4", data, scene ? "scenes" : "movies", name);
        if (n < 0 || (size_t)n >= size) return 0;
        f = PortHdMediaOpenFile(out);
        if (f) { fclose(f); return 1; }
        if (!scene) { out[0] = 0; return 0; }
        n = snprintf(out, size, "%s/hd/movies/%s.mp4", data, name);
        f = n >= 0 && (size_t)n < size ? PortHdMediaOpenFile(out) : NULL;
        if (f) { fclose(f); return 1; }
        out[0] = 0; return 0;
    }
    if (!PortHdMediaActive(data, mode, generations)) return 0;
    snprintf(member, sizeof(member), "%s/%s.mp4", scene ? "scenes" : "movies", name);
    for (i = scene ? 1 : 0; i >= 0; --i) {
        if (!PortHdMediaGeneration(data, mode, generations[i], base, sizeof(base)) ||
            !PortHdMediaCheck(base, mode, i, &info, member, &bytes)) continue;
        n = snprintf(out, size, "%s/%s", base, member);
        if (n >= 0 && (size_t)n < size && PortHdMediaMp4(out, bytes)) return 1;
    }
    out[0] = 0; return 0; // explicit selection never falls into legacy/other voice
}
/* Transfer only immutable generation content; defer active.txt until all files
   arrived and every referenced component validates. No stage/private paths. */
static inline int PortHdMediaTransferMember(const char* member) {
    const char *rest, *slash; char generation[64]; size_t n;
    int mode;
    if (!strncmp(member, "media/en/", 9)) mode = 1;
    else if (!strncmp(member, "media/ja/", 9)) mode = 2;
    else return 0;
    rest = member + 9;
    if (!strcmp(rest, "active.txt")) return mode + 1;
    slash = strchr(rest, '/');
    if (!slash || (n = (size_t)(slash - rest)) < 3 || n >= sizeof(generation)) return 0;
    memcpy(generation, rest, n); generation[n] = 0;
    if (strncmp(generation, "g-", 2) || !PortHdMediaToken(generation, "-")) return 0;
    rest = slash + 1;
    if (!strcmp(rest, "media.meta")) return 1;
    if (strncmp(rest, "movies/", 7) && strncmp(rest, "scenes/", 7) && strncmp(rest, "subs/", 5)) return 0;
    rest = strchr(rest, '/') + 1;
    if (!*rest || strstr(rest, "..") || strchr(rest, '/') || strchr(rest, '\\')) return 0;
    for (; *rest; ++rest)
        if (!((*rest >= 'a' && *rest <= 'z') || (*rest >= 'A' && *rest <= 'Z') ||
              (*rest >= '0' && *rest <= '9') || strchr("_.-", *rest))) return 0;
    return 1;
}
static inline int PortHdMediaPendingValid(const char* data, int mode, const char* pending) {
    char generations[2][64], base[1300]; PortHdMediaInfo info;
    FILE* f = PortHdMediaOpenFile(pending); int i, ok, any = 0;
    if (!f) return 0;
    ok = PortHdMediaReadActive(f, generations); fclose(f);
    for (i = 0; i < 2 && ok; ++i) {
        if (!strcmp(generations[i], "-")) continue;
        any = 1;
        ok = PortHdMediaGeneration(data, mode, generations[i], base, sizeof(base)) &&
             PortHdMediaCheck(base, mode, i, &info, NULL, NULL);
    }
    return ok && any;
}
static inline int PortHdMediaReplaceFile(const char* from, const char* to) {
#ifdef _WIN32
    wchar_t a[2048], b[2048];
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, from, -1, a, 2048) &&
           MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, to, -1, b, 2048) &&
           MoveFileExW(a, b, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(from, to) == 0;
#endif
}
#endif
