#ifndef PORT_HD_SUBTITLES_H
#define PORT_HD_SUBTITLES_H
#include "port_hd_media.h"

/* Subtitle text may come from another owned voice library. Its COMPLETE cue
 * schedule must match an existing timeline in the selected movie's generation.
 * Never infer timing compatibility from a movie name, label, or file duration.
 */
static inline int PortHdSubtitleCue(FILE* file, unsigned* first, unsigned* last) {
    char line[1024], a[16], b[16]; int used = 0; uint64_t x, y; size_t size;
    if (!fgets(line, sizeof(line), file)) return ferror(file) ? -1 : 0;
    size = strlen(line);
    if (!size) return -1;
    if (line[size - 1] != '\n' && !feof(file)) {
        if (fgetc(file) != EOF || ferror(file)) return -1; /* truncated line */
    }
    if (sscanf(line, "%15s %15s %n", a, b, &used) != 2 || !used ||
        !PortHdMediaNumber(a, 65535, &x) || !PortHdMediaNumber(b, 65535, &y) || x > y ||
        !line[used]) return -1;
    *first = (unsigned)x; *last = (unsigned)y; return 1;
}
static inline int PortHdSubtitleSameTiming(const char* candidate, const char* reference) {
    FILE *a = PortHdMediaOpenFile(candidate), *b = PortHdMediaOpenFile(reference);
    unsigned count = 0, previous = 0; int ok = a && b;
    while (ok) {
        unsigned af = 0, al = 0, bf = 0, bl = 0;
        int ar = PortHdSubtitleCue(a, &af, &al), br = PortHdSubtitleCue(b, &bf, &bl);
        if (ar != br || ar < 0) { ok = 0; break; }
        if (!ar) { ok = count > 0; break; }
        if (++count > 256 || af != bf || al != bl || (count > 1 && af < previous)) { ok = 0; break; }
        previous = af;
    }
    if (a) fclose(a);
    if (b) fclose(b);
    return ok;
}
static inline int PortHdSubtitlePath(const char* base, const char* movie, const char* language, char* out, size_t size) {
    int n = snprintf(out, size, "%s/subs/%s.%s.txt", base, movie, language);
    FILE* f;
    if (n < 0 || (size_t)n >= size) { if (size) out[0] = 0; return 0; }
    f = PortHdMediaOpenFile(out);
    if (!f) return 0;
    fclose(f); return 1;
}
static inline int PortHdSubtitleLanguage(const char* language) {
    static const char* const allowed[] = { "us", "uk", "fr", "de", "it", "es", "ja" };
    unsigned i;
    for (i = 0; i < sizeof(allowed) / sizeof(allowed[0]); ++i) if (!strcmp(language, allowed[i])) return 1;
    return 0;
}
static inline int PortHdSubtitleResolve(const char* data, int voice, const char* movie, const char* language,
                                      const char* moviePath, char* out, size_t size) {
    char selected[1600], reference[1800], candidate[1800], global[1300], *slash;
    const char* const refs[] = { voice == 2 ? "ja" : "us", "us", "uk", "ja" };
    unsigned i; int mode, component, haveReference = 0, written;
    if (!out || !size) return 0;
    out[0] = 0;
    if (!PortHdMediaLabel(voice) || !PortHdMediaName(movie) || !PortHdSubtitleLanguage(language) ||
        strlen(moviePath) >= sizeof(selected)) return 0;
    strcpy(selected, moviePath);
    slash = strrchr(selected, '/'); if (!slash) return 0; *slash = 0;
    slash = strrchr(selected, '/'); if (!slash) return 0; *slash = 0;
    /* This generation's own translation retains its source's native timings. */
    if (PortHdSubtitlePath(selected, movie, language, out, size)) return 1;
    for (i = 0; i < sizeof(refs) / sizeof(refs[0]); ++i) {
        if (PortHdSubtitlePath(selected, movie, refs[i], reference, sizeof(reference)) &&
            PortHdSubtitleSameTiming(reference, reference)) { haveReference = 1; break; }
    }
    if (!haveReference) { out[0] = 0; return 0; }
    /* Shared legacy text is also cross-source when an explicit voice is used. */
    written = snprintf(global, sizeof(global), "%s/hd", data);
    if (written >= 0 && written < (int)sizeof(global) &&
        PortHdSubtitlePath(global, movie, language, out, size) && PortHdSubtitleSameTiming(out, reference)) return 1;
    for (mode = 1; mode <= 2; ++mode) {
        char generations[2][64];
        if (!PortHdMediaActive(data, mode, generations)) continue;
        for (component = 0; component < 2; ++component) {
            char base[1300]; PortHdMediaInfo info;
            if (!PortHdMediaGeneration(data, mode, generations[component], base, sizeof(base)) ||
                !strcmp(base, selected)) continue;
            if (PortHdSubtitlePath(base, movie, language, candidate, sizeof(candidate)) && strlen(candidate) < size &&
                PortHdSubtitleSameTiming(candidate, reference) && PortHdMediaCheck(base, mode, component, &info, NULL, NULL)) {
                strcpy(out, candidate); return 1;
            }
        }
    }
    out[0] = 0; return 0;
}
#endif
