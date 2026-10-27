#ifndef PORT_DEVICE_PROFILE_H
#define PORT_DEVICE_PROFILE_H
#include <string.h>

typedef enum PortDevicePlatform {
    PORT_DEVICE_OTHER, PORT_DEVICE_LINUX, PORT_DEVICE_WINDOWS
} PortDevicePlatform;
typedef enum PortDeviceProfile { PORT_DEVICE_GENERIC, PORT_DEVICE_STEAM_DECK } PortDeviceProfile;
typedef struct PortDeviceResult {
    PortDeviceProfile profile;
    const char* reason;
} PortDeviceResult;

/* Pure classifier: no architecture guesses, file access or environment mutation. */
static PortDeviceResult PortDeviceClassify(PortDevicePlatform platform, const char* steamDeck,
                                          const char* vendor, const char* product) {
    PortDeviceResult result = { PORT_DEVICE_GENERIC, "unrecognized-hardware" };
    if (platform != PORT_DEVICE_LINUX && platform != PORT_DEVICE_WINDOWS) {
        result.reason = "unsupported-platform";
        return result;
    }
    if (steamDeck && strcmp(steamDeck, "1") == 0) {
        result.profile = PORT_DEVICE_STEAM_DECK;
        result.reason = "steam-env";
    } else if (vendor && product && strcmp(vendor, "Valve") == 0 &&
               (strcmp(product, "Jupiter") == 0 || strcmp(product, "Galileo") == 0)) {
        result.profile = PORT_DEVICE_STEAM_DECK;
        result.reason = platform == PORT_DEVICE_WINDOWS ? "valve-bios" : "valve-dmi";
    }
    return result;
}
#endif
