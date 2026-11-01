/* Typed text for the guest (the online lobby's room name), over the optional getenv channel like the keys: old guests
 * never ask, old hosts answer -1 and the guest has no typing.
 *   KHCOM_HOST_TEXT_START / KHCOM_HOST_TEXT_STOP  the window starts or stops taking text (phones and the Deck show
 *                                                 their keyboard); answers 0
 *   KHCOM_HOST_TEXT_EVENT                         the text typed since the last ask, whole UTF-8 letters ("" none)
 * The host includes this after its window (sWindow) and calls HostTextQueue from its SDL_EVENT_TEXT_INPUT. */
#ifndef HOST_TEXT_H
#define HOST_TEXT_H

static char sHostText[256];

static void HostTextQueue(const char* text) {
    size_t have = strlen(sHostText), n = strlen(text);
    if (have + n < sizeof(sHostText))
        memcpy(sHostText + have, text, n + 1);
}

/* -2: not a text query */
static int HostTextQuery(const char* name, char* out, int cap) {
    if (!strcmp(name, "KHCOM_HOST_TEXT_START") || !strcmp(name, "KHCOM_HOST_TEXT_STOP")) {
        sHostText[0] = 0;
        if (sWindow) {
            if (!strcmp(name, "KHCOM_HOST_TEXT_START"))
                SDL_StartTextInput(sWindow);
            else
                SDL_StopTextInput(sWindow);
        }
        if (cap > 0) out[0] = 0;
        return 0;
    }
    if (!strcmp(name, "KHCOM_HOST_TEXT_EVENT")) {
        int n = (int)strlen(sHostText);
        if (cap <= 1) return -1;
        if (n > cap - 1) { /* what fits, cut before a letter */
            n = cap - 1;
            while (n > 0 && ((unsigned char)sHostText[n] & 0xC0) == 0x80) n--;
        }
        memcpy(out, sHostText, (size_t)n);
        out[n] = 0;
        memmove(sHostText, sHostText + n, strlen(sHostText + n) + 1);
        return n;
    }
    return -2;
}

#endif
