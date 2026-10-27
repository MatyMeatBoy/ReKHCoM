/* Only existing card slots change tiles/codes. No VM, layout or reveal calls. */
static unsigned char sCardPromptTokens[128],sCardPromptGlyphs[128];
static unsigned char sCardPromptOriginal[128],sCardPromptFollower[128],sCardPromptOverlay[128];
static void* sCardPromptBackupTiles[128];
static u16 sCardPromptBackupCodes[128];
static int sCardHasPromptSymbols,sCardHasOriginalPrompts,sCardHasOriginalOverlays;
static uint64_t sCardPromptRevision;
static void RestoreControllerOriginalGlyph(int slot) {
    if(!sCardPromptOverlay[slot] || !sCardMsgGlyphSprites)return;
    TextGlyphSprite* sprite=&sCardMsgGlyphSprites[slot];
    if(sprite->tiles)ReleaseObjTiles(sprite->tiles);
    sprite->tiles=sCardPromptBackupTiles[slot];
    sCardMsgGlyphCodes[slot]=sCardPromptBackupCodes[slot];
    sCardPromptBackupTiles[slot]=NULL;sCardPromptOverlay[slot]=0;
}
static void ResetControllerPromptGlyphs(void) {
    for(int i=0;i<128;i++)RestoreControllerOriginalGlyph(i);
    memset(sCardPromptTokens,0,sizeof(sCardPromptTokens));
    memset(sCardPromptGlyphs,0,sizeof(sCardPromptGlyphs));
    memset(sCardPromptOriginal,0,sizeof(sCardPromptOriginal));
    memset(sCardPromptFollower,0,sizeof(sCardPromptFollower));
    sCardHasPromptSymbols=0;sCardHasOriginalPrompts=0;sCardHasOriginalOverlays=0;sCardPromptRevision=0;
}
static void RememberControllerPromptGlyph(int slot,int token,int glyph) {
    if(slot<0 || slot>=128 || !token)return;
    sCardPromptTokens[slot]=(unsigned char)token;
    sCardPromptGlyphs[slot]=(unsigned char)glyph;
    sCardHasPromptSymbols=1;
}
static void RememberControllerOriginalGlyph(int slot,const void* source,int sjis) {
    int follower=0,token=PortControllerOriginalPromptAt(source,sjis,&follower);
    if(slot<0 || slot>=128 || (!token&&!follower))return;
    /* A continued key name starts a new cell when its anchor is on the prior page. */
    if(follower && (slot==0 || sCardPromptOriginal[slot-1]!=token))follower=0;
    sCardPromptOriginal[slot]=(unsigned char)token;
    sCardPromptFollower[slot]=(unsigned char)follower;
    sCardHasOriginalPrompts=1;
}
static void RefreshControllerPromptGlyphs(void) {
    if((!sCardHasPromptSymbols&&!sCardHasOriginalPrompts) || !sCardMsgGlyphSprites)return;
    /* Original's default draw does no context/SDL queries or tile work. */
    if(!sCardHasPromptSymbols && !gPortConfig.controllerPrompts) {
        if(sCardHasOriginalOverlays)for(int i=0;i<128;i++)RestoreControllerOriginalGlyph(i);
        sCardHasOriginalOverlays=0;sCardPromptRevision=0;return;
    }
    uint64_t revision=PortControllerPromptRevision();
    if(revision==sCardPromptRevision)return;
    sCardPromptRevision=revision;
    for(int i=0;i<128;i++) {
        TextGlyphSprite* sprite=&sCardMsgGlyphSprites[i];
        int original=sCardPromptOriginal[i]||sCardPromptFollower[i];
        if(original) {
            if(!gPortConfig.controllerPrompts) { RestoreControllerOriginalGlyph(i);continue; }
            if(!sCardPromptOverlay[i]) {
                /* Move the original reference into our backup; no duplicate retain. */
                sCardPromptBackupTiles[i]=sprite->tiles;
                sCardPromptBackupCodes[i]=sCardMsgGlyphCodes[i];
                sprite->tiles=NULL;sCardPromptOverlay[i]=1;sCardHasOriginalOverlays=1;
            }
            if(sCardPromptFollower[i]) { sCardMsgGlyphCodes[i]=PORT_PROMPT_TOKEN;continue; }
        }
        int token=original?sCardPromptOriginal[i]:sCardPromptTokens[i];
        if(!token)continue;
        int glyph=PortControllerSemanticGlyph(token);
        if(glyph==sCardPromptGlyphs[i] && sprite->tiles)continue;
        const unsigned char* source=PortControllerGlyphTiles(glyph+(original?256:0));
        /* Allocation failure hides the incorrect old binding until another revision. */
        if(sprite->tiles)ReleaseObjTiles(sprite->tiles);
        sprite->tiles=source?LoadObjTiles(source,128):NULL;
        sCardPromptGlyphs[i]=sprite->tiles?(unsigned char)glyph:0;
        sCardMsgGlyphCodes[i]=PORT_PROMPT_TOKEN;
    }
}
