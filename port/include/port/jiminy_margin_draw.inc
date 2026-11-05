/* Extend the native paper's rules and colours, never a texture column.
   Its original foreground remains where the game authored it. */
static int PaintJiminyRect(float x, float y, float w, float h, uint32_t color) {
    SDL_FRect area = {x,y,w,h};
    if (w <= 0 || h <= 0) return 1;
    return SDL_SetRenderDrawColor(sRenderer, (Uint8)(color >> 16), (Uint8)(color >> 8), (Uint8)color, 255) &&
           SDL_RenderFillRect(sRenderer, &area);
}

/* The authored HD right sheet has outer radius4 and inner radius3.
 * Evaluate its contour at output-pixel centres, rather than four diagonal
 * native rows: it keeps the same curve at fractional presentation scales. */
static float JiminyPageRight(float y, float right, float scale, int inner) {
    float radius = inner ? 3.0f : 4.0f, dy;
    if (inner && (y < 17.0f || y >= 159.0f)) return -1.0f;
    dy = y < 20.0f ? 20.0f-y : y > 156.0f ? y-156.0f : 0.0f;
    if (dy > radius) dy = radius;
    return right - 4.0f*scale + SDL_sqrtf(radius*radius-dy*dy)*scale;
}

/* Invert PixelEdge's half-pixel rule. At an exact boundary the previous
 * native row still owns the output pixel; floor(gy) starts a rule early. */
static int JiminyPageRowAt(float outputY, const SDL_FRect* dst) {
    float gy = (outputY+0.5f-dst->y)/(dst->w/240);
    return SDL_clamp((int)SDL_ceilf(gy)-1,16,159);
}

static int PaintJiminySheet(const SDL_FRect* dst, const PortJiminyPageRow* rows,
                           float oldRight, float right, int inner) {
    float scale = dst->w/240;
    int bottom = (int)PixelEdge(dst->y,160,scale);
    int y = (int)PixelEdge(dst->y,16,scale);
    while (y < bottom) {
        float gy = SDL_clamp((y+0.5f-dst->y)/scale,16.0f,159.999f);
        float edge = JiminyPageRight(gy,right,scale,inner);
        int row = JiminyPageRowAt((float)y,dst);
        uint32_t color = inner ? rows[row].fill : rows[row].border;
        int end = y+1;
        if (edge < 0) { y++; continue; }
        edge = SDL_floorf(edge+0.5f);
        while (end < bottom) {
            float nextY = SDL_clamp((end+0.5f-dst->y)/scale,16.0f,159.999f);
            float nextEdge = JiminyPageRight(nextY,right,scale,inner);
            int nextRow = JiminyPageRowAt((float)end,dst);
            uint32_t nextColor = inner ? rows[nextRow].fill : rows[nextRow].border;
            if (nextEdge < 0 || SDL_floorf(nextEdge+0.5f) != edge || nextColor != color) break;
            end++;
        }
        if (!PaintJiminyRect(oldRight,(float)y,edge-oldRight,(float)(end-y),color)) return 0;
        y = end;
    }
    return 1;
}

/* 1: native fallback/not applicable; 2: complete repair; 0: draw rejected. */
static int DrawJiminyPageMargin(const SDL_FRect* src, const SDL_FRect* dst, int outW) {
    const PortJiminyPageRow* rows = PortPpuJiminyPageRows();
    uint16_t masks[160] = {0};
    uint8_t leftMasks[160] = {0};
    float scale = dst->w / 240, oldRight, right, guide;
    int y, x, ox = (int)src->x, oy = (int)src->y;
    if (!rows || scale <= 0 || ox < 0 || oy < 0 ||
        ox > gPortScreen.width - 240 || oy > gPortScreen.height - 160) return 1;
    oldRight = PixelEdge(dst->x, 240, scale);
    if (outW - oldRight <= 12 * scale) return 1;
    for (y = 16; y < 160; y++)
        if (!rows[y].fill || !rows[y].guide || !rows[y].border || !rows[y].ground) return 1;
    for (y = 16; y < 160; y++) {
        for (x = 0; x < 3; x++) {
            size_t offset = (size_t)(oy+y)*gPortScreen.width+ox+x;
            if ((rows[y].leftOwned & (1u << x)) && sComposite[offset] == gPortFrame[offset])
                leftMasks[y] |= (uint8_t)(1u << x);
        }
        for (x = 224; x < 240; x++) {
            size_t offset = (size_t)(oy + y) * gPortScreen.width + ox + x;
            if ((rows[y].owned & (1u << (x - 224))) && sComposite[offset] == gPortFrame[offset])
                masks[y] |= (uint16_t)(1u << (x - 224));
        }
    }
    right = outW - 3 * scale;
    guide = right - 8 * scale;
    for (y = 16; y < 160;) {
        const PortJiminyPageRow* r = &rows[y];
        float top = PixelEdge(dst->y, y, scale);
        int end = y + 1;
        float height;
        while (end < 160 && masks[end] == masks[y] && leftMasks[end] == leftMasks[y] &&
               rows[end].fill == r->fill && rows[end].guide == r->guide &&
               rows[end].border == r->border && rows[end].ground == r->ground) end++;
        height = PixelEdge(dst->y, end, scale) - top;
        /* The three native columns outside the left page still contain old
           frame bars. Clear only their opaque BG1 winner, never a name plate,
           object, blend or later UI edit. The paper starts at native x3. */
        for (x = 0; x < 3;) {
            int first = x;
            if (!(leftMasks[y] & (1u << x))) { x++; continue; }
            do { x++; } while (x < 3 && (leftMasks[y] & (1u << x)));
            if (!PaintJiminyRect(PixelEdge(dst->x,first,scale),top,
                    PixelEdge(dst->x,x,scale)-PixelEdge(dst->x,first,scale),height,r->ground)) return 0;
        }
        for (x = 224; x < 240;) {
            int first = x;
            if (!(masks[y] & (1u << (x - 224)))) { x++; continue; }
            do { x++; } while (x < 240 && (masks[y] & (1u << (x - 224))));
            if (!PaintJiminyRect(PixelEdge(dst->x, first, scale), top,
                    PixelEdge(dst->x, x, scale) - PixelEdge(dst->x, first, scale), height, r->fill)) return 0;
        }
        if (!PaintJiminyRect(oldRight,top,outW-oldRight,height,r->ground)) return 0;
        y = end;
    }
    if (!PaintJiminySheet(dst,rows,oldRight,right,0) || !PaintJiminySheet(dst,rows,oldRight,right,1)) return 0;
    for (y = 16; y < 160;) {
        const PortJiminyPageRow* r = &rows[y];
        int end = y+1;
        while (end < 160 && rows[end].guide == r->guide && rows[end].fill == r->fill) end++;
        if (r->guide != r->fill && !PaintJiminyRect(guide,PixelEdge(dst->y,y,scale),scale,
                PixelEdge(dst->y,end,scale)-PixelEdge(dst->y,y,scale),r->guide)) return 0;
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
