#ifndef PORT_DEVICE_PROFILE_HOST_H
#define PORT_DEVICE_PROFILE_HOST_H
#include "port_device_profile.h"
#include <stdlib.h>
#include <stdio.h>

#if defined(__linux__) && !defined(__ANDROID__) && !defined(PORT_PS5_GUEST)
static void PortDeviceReadDmi(const char* path, char* out, size_t size) {
    FILE* file = fopen(path, "r");
    out[0] = 0;
    if (!file) return;
    if (fgets(out, (int)size, file)) out[strcspn(out, "\r\n")] = 0;
    fclose(file);
}
#endif
/* A bridge host answers this reserved query itself, never from an environment override.
   The guest uses exactly the same classifier result as a direct native build. */
static const char* PortDeviceHostQuery(const char* name) {
    PortDeviceResult result;
    if (strcmp(name, "KHCOM_HOST_DEVICE_PROFILE") != 0) return NULL;
#if defined(__ANDROID__) || defined(PORT_PS5_GUEST)
    result = PortDeviceClassify(PORT_DEVICE_OTHER, NULL, NULL, NULL);
#elif defined(__linux__)
    char vendor[128], product[128];
    PortDeviceReadDmi("/sys/class/dmi/id/sys_vendor", vendor, sizeof(vendor));
    PortDeviceReadDmi("/sys/class/dmi/id/product_name", product, sizeof(product));
    result = PortDeviceClassify(PORT_DEVICE_LINUX, getenv("SteamDeck"), vendor, product);
#elif defined(_WIN32)
    result = PortDeviceClassify(PORT_DEVICE_WINDOWS, getenv("SteamDeck"), NULL, NULL);
#else
    result = PortDeviceClassify(PORT_DEVICE_OTHER, NULL, NULL, NULL);
#endif
    if (result.profile == PORT_DEVICE_STEAM_DECK)
        return strcmp(result.reason, "steam-env") == 0 ? "steam-deck:steam-env" : "steam-deck:valve-dmi";
    return strcmp(result.reason, "unsupported-platform") == 0 ?
        "generic:unsupported-platform" : "generic:unrecognized-hardware";
}
#endif
