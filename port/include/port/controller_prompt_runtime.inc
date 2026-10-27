/* Included after the card renderer's private arrays. Only tile handles
   change; no dialogue work, reveal count, VM or audio calls. */
static unsigned char sCardPromptTokens[128],sCardPromptGlyphs[128];
static int sCardHasPromptSymbols;
static uint64_t sCardPromptRevision;
static void ResetControllerPromptGlyphs(void) {
    memset(sCardPromptTokens,0,sizeof(sCardPromptTokens));
    memset(sCardPromptGlyphs,0,sizeof(sCardPromptGlyphs));
    sCardHasPromptSymbols=0;sCardPromptRevision=0;
}
static void RememberControllerPromptGlyph(int slot,int token,int glyph) {
    if(slot<0 || slot>=128 || !token)return;
    sCardPromptTokens[slot]=(unsigned char)token;
    sCardPromptGlyphs[slot]=(unsigned char)glyph;
    sCardHasPromptSymbols=1;
}
static void RefreshControllerPromptGlyphs(void) {
    if(!sCardHasPromptSymbols || !sCardMsgGlyphSprites)return;
    uint64_t revision=PortControllerPromptRevision();
    if(revision==sCardPromptRevision)return;
    sCardPromptRevision=revision;
    for(int i=0;i<128;i++)if(sCardPromptTokens[i]) {
        int glyph=PortControllerSemanticGlyph(sCardPromptTokens[i]);
        if(glyph==sCardPromptGlyphs[i])continue;
        const unsigned char* source=PortControllerGlyphTiles(glyph);
        TextGlyphSprite* sprite=&sCardMsgGlyphSprites[i];
        /* Do not retain an old, now-wrong button if replacement cannot load. */
        if(sprite->tiles)ReleaseObjTiles(sprite->tiles);
        sprite->tiles=source?LoadObjTiles(source,128):NULL;
        sCardPromptGlyphs[i]=sprite->tiles?(unsigned char)glyph:0;
    }
}
