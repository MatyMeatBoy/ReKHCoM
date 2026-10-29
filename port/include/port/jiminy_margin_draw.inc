/* Extend the native paper's rules and colours, never a texture column.
   Its original foreground remains where the game authored it. */
static int PaintJiminyRect(float x, float y, float w, float h, uint32_t color) {
    SDL_FRect area = {x,y,w,h};
    if (w <= 0 || h <= 0) return 1;
    return SDL_SetRenderDrawColor(sRenderer, (Uint8)(color >> 16), (Uint8)(color >> 8), (Uint8)color, 255) &&
           SDL_RenderFillRect(sRenderer, &area);
}

/* 1: native fallback/not applicable; 2: complete repair; 0: draw rejected. */
static int DrawJiminyPageMargin(const SDL_FRect* src, const SDL_FRect* dst, int outW) {
    const PortJiminyPageRow* rows = PortPpuJiminyPageRows();
    uint16_t masks[160] = {0};
    float scale = dst->w / 240, oldRight, right, guide;
    int y, x, ox = (int)src->x, oy = (int)src->y;
    if (!rows || scale <= 0 || ox < 0 || oy < 0 ||
        ox > gPortScreen.width - 240 || oy > gPortScreen.height - 160) return 1;
    oldRight = PixelEdge(dst->x, 240, scale);
    if (outW - oldRight <= 12 * scale) return 1;
    for (y = 16; y < 160; y++)
        if (!rows[y].fill || !rows[y].guide || !rows[y].border || !rows[y].ground) return 1;
    for (y = 16; y < 160; y++)
        for (x = 224; x < 240; x++) {
            size_t offset = (size_t)(oy + y) * gPortScreen.width + ox + x;
            if ((rows[y].owned & (1u << (x - 224))) && sComposite[offset] == gPortFrame[offset])
                masks[y] |= (uint16_t)(1u << (x - 224));
        }
    right = outW - 3 * scale;
    guide = right - 8 * scale;
    for (y = 16; y < 160;) {
        const PortJiminyPageRow* r = &rows[y];
        float top = PixelEdge(dst->y, y, scale);
        int corner = y < 20 ? 19 - y : y >= 156 ? y - 156 : 0;
        int end = y + 1;
        float height;
        float edge = right - corner * scale;
        /* One rectangle per colour/ownership band, rather than hundreds
           of identical row commands through the native/guest renderer. */
        while (end < 160 && masks[end] == masks[y] &&
               rows[end].fill == r->fill && rows[end].guide == r->guide &&
               rows[end].border == r->border && rows[end].ground == r->ground &&
               (end < 20 ? 19-end : end >= 156 ? end-156 : 0) == corner) end++;
        height = PixelEdge(dst->y, end, scale) - top;
        /* Only the BG1 tail can change. Objects, windows, alpha and UI
           edits keep their pixels, even when they overlap the old edge. */
        for (x = 224; x < 240;) {
            int first = x;
            if (!(masks[y] & (1u << (x - 224)))) { x++; continue; }
            do { x++; } while (x < 240 && (masks[y] & (1u << (x - 224))));
            if (!PaintJiminyRect(PixelEdge(dst->x, first, scale), top,
                    PixelEdge(dst->x, x, scale) - PixelEdge(dst->x, first, scale), height, r->fill)) return 0;
        }
        if (!PaintJiminyRect(oldRight, top, edge - oldRight, height, r->fill) ||
            !PaintJiminyRect(guide, top, scale, height, r->guide) ||
            !PaintJiminyRect(edge - scale, top, scale, height, r->border) ||
            !PaintJiminyRect(edge, top, outW - edge, height, r->ground)) return 0;
        y = end;
    }
    return 2;
}

static int RepairJiminyPageMargin(const SDL_FRect* src, const SDL_FRect* dst, int outW) {
    int result = DrawJiminyPageMargin(src, dst, outW);
    if (!result) {
        const PortJiminyPageRow* rows = PortPpuJiminyPageRows();
        float scale = dst->w / 240, right = PixelEdge(dst->x,240,scale);
        int y;
        DrawJournalPicture(src,dst); /* complete native picture survives a partial repair */
        if (rows)
            for (y = 16; y < 160; y++) {
                float top = PixelEdge(dst->y,y,scale);
                PaintJiminyRect(right,top,outW-right,PixelEdge(dst->y,y+1,scale)-top,rows[y].ground);
            }
    }
    return result == 2;
}
