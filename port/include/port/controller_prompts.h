#ifndef PORT_CONTROLLER_PROMPTS_H
#define PORT_CONTROLLER_PROMPTS_H
#include <stddef.h>
#include <string.h>
enum { KH_PROMPT_ORIGINAL, KH_PROMPT_AUTO, KH_PROMPT_NINTENDO, KH_PROMPT_XBOX,
       KH_PROMPT_PLAYSTATION, KH_PROMPT_GENERIC };
static inline int KhPromptPresetBindings(int family,int nintendoMapping,int* confirm,int* cancel) {
    if(family!=KH_PROMPT_NINTENDO && family!=KH_PROMPT_XBOX && family!=KH_PROMPT_PLAYSTATION)return 0;
    int physical=family==KH_PROMPT_XBOX?0:1;
    *confirm=nintendoMapping?physical^1:physical;
    *cancel=*confirm^1;
    return 1;
}
static inline const char* KhPromptButtonLabel(int binding, int family, int nintendoMapping) {
    /* Stored face bindings are normalized by pad_layout.h. Undo that step
       before assigning a visual family's labels to physical SDL positions. */
    static const char* const face[4][4] = {
        {"B","A","Y","X"}, {"A","B","X","Y"},
        {"Cross","Circle","Square","Triangle"}, {"South","East","West","North"}
    };
    int row = family == KH_PROMPT_NINTENDO ? 0 : family == KH_PROMPT_XBOX ? 1 :
              family == KH_PROMPT_PLAYSTATION ? 2 : 3;
    if (binding < 0) return "-";
    if (binding < 4) return face[row][nintendoMapping ? binding ^ 1 : binding];
    if (binding == 4) return family == KH_PROMPT_NINTENDO ? "-" : family == KH_PROMPT_PLAYSTATION ? "Share" : "View";
    if (binding == 6) return family == KH_PROMPT_NINTENDO ? "+" : family == KH_PROMPT_PLAYSTATION ? "Options" : "Menu";
    if (binding == 7) return "LS";
    if (binding == 8) return "RS";
    if (binding == 9) return family == KH_PROMPT_PLAYSTATION ? "L1" : family == KH_PROMPT_NINTENDO ? "L" : "LB";
    if (binding == 10) return family == KH_PROMPT_PLAYSTATION ? "R1" : family == KH_PROMPT_NINTENDO ? "R" : "RB";
    if (binding >= 11 && binding <= 14) {
        static const char* const arrows[] = {"Up","Down","Left","Right"};
        return arrows[binding-11];
    }
    if (binding >= PORT_PAD_AXIS_BASE && binding < PORT_PAD_AXIS_BASE + 12) {
        int axis=(binding-PORT_PAD_AXIS_BASE)/2, negative=(binding-PORT_PAD_AXIS_BASE)&1;
        static const char* const sticks[] = {"LS Right","LS Left","LS Down","LS Up","RS Right","RS Left","RS Down","RS Up"};
        if (axis < 4) return sticks[axis*2+negative];
        if (negative) return "?"; /* A negative trigger is not a normal trigger press. */
        return axis == 4 ? (family == KH_PROMPT_PLAYSTATION ? "L2" : family == KH_PROMPT_NINTENDO ? "ZL" : "LT") :
                           (family == KH_PROMPT_PLAYSTATION ? "R2" : family == KH_PROMPT_NINTENDO ? "ZR" : "RT");
    }
    return "?";
}
static inline int KhPromptTokenAction(const char* text, size_t length) {
    static const char* const keys[]={"A","B","L","R","START","SELECT"};
    static const char* const before[]={"Bouton ","bouton ","Bot\363n ","Pulsante ","pulsante ","boutons ","pulsanti "};
    static const char* const after[]={" Button","-Knopf","-Taste"};
    while(length && text[length-1]==' ')length--;
    for (int i=0;i<6;i++) if (strlen(keys[i])==length && !memcmp(text,keys[i],length)) return i;
    for (unsigned i=0;i<sizeof(before)/sizeof(before[0]);i++) {
        size_t n=strlen(before[i]);
        if(length==n+1 && !memcmp(text,before[i],n))
            for(int k=0;k<4;k++)if(text[n]==keys[k][0])return k;
    }
    for (unsigned i=0;i<sizeof(after)/sizeof(after[0]);i++) {
        size_t n=strlen(after[i]);
        if(length==n+1 && !memcmp(text+1,after[i],n))
            for(int k=0;k<4;k++)if(text[0]==keys[k][0])return k;
    }
    return -1;
}
static inline int KhPromptTokenActions(const char* text,size_t length,int actions[2]) {
    int action=KhPromptTokenAction(text,length);
    if(action>=0) { actions[0]=action;return 1; }
    static const char* const pairs[]={"L and R Buttons","L- und R-Tasten","L + R","L+R"};
    for(unsigned i=0;i<sizeof(pairs)/sizeof(pairs[0]);i++)
        if(strlen(pairs[i])==length && !memcmp(text,pairs[i],length)) { actions[0]=2;actions[1]=3;return 2; }
    return 0;
}
/* Shift-JIS full-width key names immediately followed by the authored
   Japanese word for button. Return only the key-byte extent, preserving
   the localized suffix. Never reinterpret unrelated Latin/kana prose. */
static inline int KhPromptSjisActions(const unsigned char* text,size_t length,int actions[2],size_t* bytes) {
    static const unsigned char button[]={0x83,0x7b,0x83,0x5e,0x83,0x93};
    static const unsigned char select[]={0x82,0x72,0x82,0x64,0x82,0x6b,0x82,0x64,0x82,0x62,0x82,0x73};
    if(length>=18 && !memcmp(text,select,12) && !memcmp(text+12,button,6)) { actions[0]=5;*bytes=12;return 1; }
    if(length>=12 && text[0]==0x82 && (text[1]==0x6b || text[1]==0x71) && text[2]==0x81 && text[3]==0x45 &&
       text[4]==0x82 && (text[5]==0x6b || text[5]==0x71) && text[1]!=text[5] && !memcmp(text+6,button,6)) {
        actions[0]=text[1]==0x6b?2:3;actions[1]=text[5]==0x6b?2:3;*bytes=6;return 2;
    }
    if(length>=8 && text[0]==0x82 && !memcmp(text+2,button,6)) {
        int low=text[1];
        int action=low==0x60?0:low==0x61?1:low==0x6b?2:low==0x71?3:-1;
        if(action>=0) { actions[0]=action;*bytes=2;return 1; }
    }
    return 0;
}
#endif
