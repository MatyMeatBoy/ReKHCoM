/* Native options menu: user-selected Ultimate 2C sprite + interactive regions.
 * All edges, labels and highlights share the framebuffer's integer pixel grid.
 * No texture filtering, device-specific remapping, or input side effects. */
static void ControlBindingName(int binding, int l, char* out, size_t size) {
    if (PortControllerBindingLabel(binding,out,size)) return;
    static const char* const names[] = {
        "A", "B", "X", "Y", "Back", "Guide", "Start", "LS", "RS", "LB", "RB"
    };
    static const char* const arrows[] = { "↑", "↓", "←", "→" };
    if (binding >= PORT_PAD_AXIS_BASE) {
        int axis = (binding - PORT_PAD_AXIS_BASE) / 2;
        int neg = (binding - PORT_PAD_AXIS_BASE) & 1;
        if (axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER || axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)
            SDL_strlcpy(out, axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER ? "LT" : "RT", size);
        else if (axis >= 0 && axis < 4)
            SDL_snprintf(out, size, "%s %s", axis < 2 ? "LS" : "RS", arrows[(axis & 1) ? !neg : (neg ? 2 : 3)]);
        else SDL_strlcpy(out, "?", size);
    } else if (binding >= 0 && binding < 11) SDL_strlcpy(out, names[binding], size);
    else if (binding >= SDL_GAMEPAD_BUTTON_DPAD_UP && binding <= SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
        SDL_snprintf(out, size, "%s %s", l == LANG_ES ? "Cruceta" : "D-pad", arrows[binding - SDL_GAMEPAD_BUTTON_DPAD_UP]);
    else {
        const char* name = binding < 0 ? "-" : SDL_GetGamepadStringForButton((SDL_GamepadButton)binding);
        SDL_strlcpy(out, name ? name : "?", size);
    }
}

enum { CD_LT = SDL_GAMEPAD_BUTTON_COUNT, CD_RT, CD_COUNT };

static int ControlDiagramRegion(int binding) {
    if (binding >= PORT_PAD_AXIS_BASE) {
        int axis = (binding - PORT_PAD_AXIS_BASE) / 2;
        if (axis < 4) return axis < 2 ? SDL_GAMEPAD_BUTTON_LEFT_STICK : SDL_GAMEPAD_BUTTON_RIGHT_STICK;
        if (axis < 6) return CD_LT + axis - 4;
        return -1;
    }
    return binding >= 0 && binding <= SDL_GAMEPAD_BUTTON_DPAD_RIGHT ? binding : -1;
}

static void ControlDiagramRect(int ox, int oy, int scale, int x, int y, int w, int h, uint32_t color) {
    Fill(ox + x * scale, oy + y * scale, w * scale, h * scale, color);
}

/* Deliberately tiny 3x5 lettering; remains on the same grid as the body. */
static void ControlDiagramLabel(int ox, int oy, int scale, int x, int y, const char* text, uint32_t color) {
    for (; *text; ++text, x += 4) {
        unsigned bits = 0;
        switch (*text) {
            case 'A': bits = 0x2BED; break; case 'B': bits = 0x6BAE; break;
            case 'X': bits = 0x5AAD; break; case 'Y': bits = 0x5A92; break;
            case 'L': bits = 0x4927; break; case 'R': bits = 0x6BAD; break;
            case 'T': bits = 0x7492; break; case 'S': bits = 0x788F; break;
            case '-': bits = 0x01C0; break; case '+': bits = 0x05D0; break;
        }
        for (int yy = 0; yy < 5; ++yy)
            for (int xx = 0; xx < 3; ++xx)
                if (bits & (1u << (14 - yy * 3 - xx)))
                    ControlDiagramRect(ox, oy, scale, x + xx, y + yy, 1, 1, color);
    }
}

#include "port_controller_art.inc"

/* Direction bits: up/down/left/right. A stick click lights the complete cap. */
static unsigned ControlDiagramDirections(int binding) {
    if (binding < PORT_PAD_AXIS_BASE) return 15;
    int a = (binding - PORT_PAD_AXIS_BASE) / 2;
    int neg = (binding - PORT_PAD_AXIS_BASE) & 1;
    return a >= 4 ? 15 : (a & 1) ? (neg ? 1 : 2) : (neg ? 4 : 8);
}

static int ControlDiagramPixelLit(int r, int x, int y, const unsigned* lit) {
    if (r < 0 || r >= CD_COUNT || !lit[r]) return 0;
    if (r != SDL_GAMEPAD_BUTTON_LEFT_STICK && r != SDL_GAMEPAD_BUTTON_RIGHT_STICK) return 1;
    int dx = x - (r == SDL_GAMEPAD_BUTTON_LEFT_STICK ? 20 : 68);
    int dy = y - (r == SDL_GAMEPAD_BUTTON_LEFT_STICK ? 27 : 44);
    return ((lit[r]&1) && dy <= 0) || ((lit[r]&2) && dy >= 0) ||
           ((lit[r]&4) && dx <= 0) || ((lit[r]&8) && dx >= 0);
}

static void DrawControlDiagram(int x, int y, int w, int h) {
    enum { TOP = 7, HEIGHT = CONTROL_ART_H + TOP };
    static signed char region[HEIGHT][CONTROL_ART_W];
    static int ready;
    static const struct { int x, y, radius, id; } discs[] = {
        {20,27,7,SDL_GAMEPAD_BUTTON_LEFT_STICK}, {68,44,7,SDL_GAMEPAD_BUTTON_RIGHT_STICK},
        {83,20,4,SDL_GAMEPAD_BUTTON_NORTH}, {75,28,4,SDL_GAMEPAD_BUTTON_WEST},
        {90,28,4,SDL_GAMEPAD_BUTTON_EAST}, {83,36,4,SDL_GAMEPAD_BUTTON_SOUTH},
        {37,18,3,SDL_GAMEPAD_BUTTON_BACK}, {67,18,3,SDL_GAMEPAD_BUTTON_START},
        {52,18,4,SDL_GAMEPAD_BUTTON_GUIDE}
    };
    static const struct { int x, id; const char* name; } tabs[] = {
        {4,CD_LT,"LT"}, {22,SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,"LB"},
        {65,SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,"RB"}, {83,CD_RT,"RT"}
    };
    unsigned lit[CD_COUNT] = {0};
    int scale = SDL_min(w / CONTROL_ART_W, h / HEIGHT);
    if (scale < 1) return;
    if (!ready) {
        SDL_memset(region, -1, sizeof(region));
        for (unsigned i = 0; i < sizeof(discs) / sizeof(discs[0]); ++i)
            for (int dy = -discs[i].radius; dy <= discs[i].radius; ++dy)
                for (int dx = -discs[i].radius; dx <= discs[i].radius; ++dx)
                    if (dx*dx + dy*dy <= discs[i].radius*discs[i].radius + 1)
                        region[discs[i].y+dy][discs[i].x+dx] = (signed char)discs[i].id;
        /* Four arms of the actual cross; the middle is deliberately neutral. */
        for (int yy = 35; yy <= 54; ++yy)
            for (int xx = 28; xx <= 46; ++xx) {
                if (xx >= 34 && xx <= 39)
                    region[yy][xx] = yy < 42 ? SDL_GAMEPAD_BUTTON_DPAD_UP :
                                     yy > 47 ? SDL_GAMEPAD_BUTTON_DPAD_DOWN : -1;
                else if (yy >= 42 && yy <= 47)
                    region[yy][xx] = xx < 34 ? SDL_GAMEPAD_BUTTON_DPAD_LEFT : SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
            }
        /* Front shoulder caps also respond, in addition to the labelled tabs.
         * Triggers sit behind the shell in this front view: only their tabs light. */
        for (int yy = 7; yy <= 10; ++yy)
            for (int xx = 16; xx <= 30; ++xx) {
                region[yy][xx] = SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
                region[yy][CONTROL_ART_W-1-xx] = SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
            }
        ready = 1;
    }
    if (sCursor >= ITEM_CONTROL_A && sCursor <= ITEM_CONTROL_UNSTOCK) {
        int binding = gPortConfig.pad[sCursor - ITEM_CONTROL_A];
        int r = ControlDiagramRegion(binding);
        if (r >= 0 && r < CD_COUNT) lit[r] |= ControlDiagramDirections(binding);
    }
    if (sControlCapture) {
        int count = 0;
        SDL_JoystickID* ids = SDL_GetGamepads(&count);
        SDL_Gamepad* pad = count > 0 ? SDL_GetGamepadFromID(ids[0]) : NULL;
        if (pad) {
            for (int b = 0; b <= SDL_GAMEPAD_BUTTON_DPAD_RIGHT; ++b)
                if (SDL_GetGamepadButton(pad, (SDL_GamepadButton)b)) lit[b] = 15;
            for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; ++a) {
                int value = SDL_GetGamepadAxis(pad, (SDL_GamepadAxis)a);
                if (SDL_abs(value) > 16000) {
                    int binding = PORT_PAD_AXIS_BASE + a*2 + (value < 0);
                    int r = ControlDiagramRegion(binding);
                    if (r >= 0 && r < CD_COUNT) lit[r] |= ControlDiagramDirections(binding);
                }
            }
        }
        SDL_free(ids);
    }
    x += (w - CONTROL_ART_W * scale) / 2;
    y += (h - HEIGHT * scale) / 2;
    for (int yy = TOP; yy < HEIGHT; ++yy)
        for (int xx = 0; xx < CONTROL_ART_W; ++xx) {
            unsigned index = sControlArt[yy - TOP][xx];
            if (!index) continue; /* alpha: never draw a white/background plate */
            uint32_t color = sControlArtPalette[index];
            int r = region[yy][xx], active = ControlDiagramPixelLit(r,xx,yy,lit);
            if (active) {
                int edge = !xx || yy == TOP || xx == CONTROL_ART_W-1 || yy == HEIGHT-1 ||
                           region[yy-1][xx] != r || region[yy+1][xx] != r ||
                           region[yy][xx-1] != r || region[yy][xx+1] != r;
                unsigned red = (color >> 16)&255, green = (color >> 8)&255, blue = color&255;
                /* Keep the dark pixel lettering legible inside the warm tint. */
                if (edge) color = MENU_RED_DARK;
                else if (red + green + blue > 180)
                    color = 0xFF000000u | (((red + 255)/2) << 16) |
                            (((green + 158)/2) << 8) | ((blue + 52)/2);
            } else if ((xx > 0 && ControlDiagramPixelLit(region[yy][xx-1],xx-1,yy,lit)) ||
                       (xx+1 < CONTROL_ART_W && ControlDiagramPixelLit(region[yy][xx+1],xx+1,yy,lit)) ||
                       (yy > TOP && ControlDiagramPixelLit(region[yy-1][xx],xx,yy-1,lit)) ||
                       (yy+1 < HEIGHT && ControlDiagramPixelLit(region[yy+1][xx],xx,yy+1,lit)))
                color = 0xFFFFE68Eu; /* one-pixel light rim, clipped to opaque art */
            ControlDiagramRect(x,y,scale,xx,yy,1,1,color);
        }
    for (unsigned i = 0; i < sizeof(tabs)/sizeof(tabs[0]); ++i) {
        int on = lit[tabs[i].id] != 0;
        ControlDiagramRect(x,y,scale,tabs[i].x,0,17,7,on ? MENU_RED_DARK : 0xFF43343Cu);
        ControlDiagramRect(x,y,scale,tabs[i].x+1,1,15,5,on ? 0xFFCB563Du : 0xFF78626Bu);
        ControlDiagramLabel(x,y,scale,tabs[i].x+5,1,tabs[i].name,0xFFFFF2CDu);
    }
}
