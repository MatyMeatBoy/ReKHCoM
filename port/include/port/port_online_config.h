#ifndef PORT_ONLINE_CONFIG_H
#define PORT_ONLINE_CONFIG_H

/* Shared by native launchers and the game; deliberately outside PortConfig's ABI. */
static inline int PortOnlineParseEngine(const char* text) {
    static const char modern[] = "modern";
    unsigned i;
    if (!text) return 0;
    for (i = 0; i < sizeof(modern)-1; i++)
        if (text[i] != modern[i]) return 0;
    if (text[i] == '\r') i++;
    if (text[i] == '\n') i++;
    return text[i] == 0;
}

static inline const char* PortOnlineEngineName(int modern) {
    return modern ? "modern" : "classic";
}

#endif
