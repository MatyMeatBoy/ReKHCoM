#ifndef REKHCOM_LINUX64_ARGS_H
#define REKHCOM_LINUX64_ARGS_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Host options precede --; game options follow it. Only --language currently
 * has a guest contract here (KHCOM_LANGUAGE -> guest_main -> main argv).
 * Keep data, ROM/region selection and the boot marker owned by the wrapper. */
static inline const char* Linux64ApplyArguments(int argc, char** argv, char* data, size_t dataSize) {
    const char* bootStatus = NULL;
    int gameArguments = 0;
    for (int i = 2; i < argc; i++) {
        if (!gameArguments && !strcmp(argv[i], "--"))
            gameArguments = 1;
        else if (!gameArguments && !strcmp(argv[i], "--data") && i + 1 < argc)
            snprintf(data, dataSize, "%s", argv[++i]);
        else if (!gameArguments && !strcmp(argv[i], "--boot-status") && i + 1 < argc)
            bootStatus = argv[++i];
        else if (gameArguments && !strcmp(argv[i], "--language") && i + 1 < argc)
            setenv("KHCOM_LANGUAGE", argv[++i], 1);
    }
    return bootStatus;
}

#endif
