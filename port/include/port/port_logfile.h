#ifndef GUARD_PORT_LOGFILE_H
#define GUARD_PORT_LOGFILE_H

#include <stdio.h>
#if __STDC_HOSTED__ && !defined(PORT_GBA_RELOC)
#include <time.h>
#endif

/* Current session plus five previous sessions. No SDL dependency: the guest
   libc forwards these file operations to the same native hosts as fopen. */
static FILE* PortRotateSessionLog(const char* path) {
    char from[2048], to[2048];
    FILE* old = fopen(path, "rb");
    int i, n;
    if (!old)
        return fopen(path, "a");
    fclose(old);
    n = snprintf(to, sizeof(to), "%s.5", path);
    if (n < 0 || (size_t)n >= sizeof(to))
        return fopen(path, "a");
    /* Probe first: remove must not discard anything other than an old log.
       A failed operation keeps the current log and appends this session. */
    old = fopen(to, "rb");
    if (old) {
        fclose(old);
        if (remove(to))
            return fopen(path, "a");
    }
    for (i = 4; i >= 0; --i) {
        if (i)
            snprintf(from, sizeof(from), "%s.%d", path, i);
        else
            snprintf(from, sizeof(from), "%s", path);
        snprintf(to, sizeof(to), "%s.%d", path, i + 1);
        old = fopen(from, "rb");
        if (!old)
            continue;
        fclose(old);
        if (rename(from, to))
            return fopen(path, "a");
    }
    return fopen(path, "a");
}

static FILE* PortOpenSessionLog(const char* path) {
    FILE* file = PortRotateSessionLog(path);
    if (file) {
#if __STDC_HOSTED__ && !defined(PORT_GBA_RELOC)
        time_t now = time(NULL);
        struct tm* stamp = gmtime(&now);
        char date[64];
        /* Numeric UTC avoids localized timezone names in a different encoding. */
        if (stamp && strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%SZ", stamp))
            fprintf(file, "# session: %s\n", date);
        else
#endif
            fputs("# session: log opened\n", file);
        fflush(file);
    }
    return file;
}

#endif
