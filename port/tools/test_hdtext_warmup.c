/* Regression probe: compile with the production FreeType rasterizer and SDL.
 * Run from an isolated package: probe <package-data-directory>.
 * Actual fonts/textures, controlled width table and absent optional CJK face;
 * this does not run the game or certify a physical display/Gaming Mode. */
#include <stdarg.h>
#include <string.h>
#include "../portable/hdtext_freetype.c"

PortConfig gPortConfig;
static char root[1024];
const char* PortConfigPath(const char* name) {
    static char path[1200];
    SDL_snprintf(path, sizeof(path), "%s/%s", root, name);
    return path;
}
void PortLog(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt); vfprintf(stdout, fmt, ap); va_end(ap); putchar('\n');
}
const uint16_t* PortLatinGlyphWidths(void) {
    static uint16_t widths[256]; int i;
    for (i = 0; i < 256; i++) widths[i] = 6;
    return widths;
}
unsigned PortSjisToCodepoint(unsigned code) {
    switch (code) {
        case 0x81A9: return 0x2190; case 0x81AA: return 0x2191;
        case 0x81A8: return 0x2192; case 0x81AB: return 0x2193;
        default: return 0;
    }
}
int PortCodepointToSjis(unsigned cp) {
    switch (cp) {
        case 0x2190: return 0x81A9; case 0x2191: return 0x81AA;
        case 0x2192: return 0x81A8; case 0x2193: return 0x81AB;
        default: return 0;
    }
}
static int Reset(void) {
    PortHdTextShutdown();
    return PortPortableHdReady();
}
int main(int argc, char** argv) {
    SDL_Window* window; SDL_Renderer* renderer; HdFont* font;
    int failures = 0, ok;
    if (argc != 2) return 2;
    SDL_strlcpy(root, argv[1], sizeof(root));
    if (!SDL_Init(SDL_INIT_VIDEO)) return 3;
    window = SDL_CreateWindow("HD warmup regression", 640, 480, SDL_WINDOW_HIDDEN);
    renderer = window ? SDL_CreateRenderer(window, "software") : NULL;
    if (!renderer) return 4;
    gPortConfig.hdText = gPortConfig.hdTitle = 1;
    if (!Reset()) return 5;
    PortHdTextWarmGame(renderer, 2, 4);
    font = FontForScale(2, 0);
    ok = ready && font->font && Glyph(renderer, font, 'A')->tex;
    printf("CASE game-warmup ready=%d ascii_texture=%d pass=%d\n", ready,
           font->font && font->glyphs['A'].tex != NULL, ok); failures += !ok;

    if (!Reset()) return 5;
    if (japanese) FT_Done_Face(japanese);
    japanese = NULL; /* Optional CJK fonts need not exist in a Latin package. */
    PortHdTextWarmUi(renderer, 2);
    font = FontForScale(2, 3);
    ok = ready && font->font && Glyph(renderer, font, 'A')->tex;
    printf("CASE ui-without-cjk ready=%d ascii_texture=%d pass=%d\n", ready,
           font->font && font->glyphs['A'].tex != NULL, ok); failures += !ok;

    if (!Reset()) return 5;
    font = FontForScale(2, 0);
    /* An unsupported glyph actually requested by the game still disables HD
     * so the PPU resumes the original text. Only speculative warmup skips it. */
    Glyph(renderer, font, 0x81);
    ok = !ready;
    printf("CASE required-missing-glyph original_fallback=%d pass=%d\n", !ready, ok);
    failures += !ok;
    PortHdTextShutdown(); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit();
    printf("FAILURES %d\n", failures);
    return failures ? 1 : 0;
}
