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
#if defined(_WIN32) && !defined(PORT_PS5_GUEST)
#include <windows.h>
typedef LSTATUS (WINAPI *PortDeviceRegGetValue)(HKEY, LPCWSTR, LPCWSTR, DWORD, LPDWORD, PVOID, LPDWORD);
/* Provider parameter keeps registry failure and malformed UTF-16 testable. Only
   exact ASCII names enter the classifier; non-ASCII input cannot become a match. */
static void PortDeviceReadBiosValue(PortDeviceRegGetValue readValue, const WCHAR* name, char* out, size_t size) {
    WCHAR value[128];
    DWORD bytes = sizeof(value), type = 0;
    /* RRF_SUBKEY_WOW6464KEY: older SDK target defaults hide its name even
       though the dynamically loaded API accepts the documented flag value. */
    const DWORD registry64View = 0x00010000u;
    size_t i, count;
    out[0] = 0;
    if (!readValue || readValue(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", name,
                              RRF_RT_REG_SZ | RRF_ZEROONFAILURE | registry64View,
                              &type, value, &bytes) != ERROR_SUCCESS || type != REG_SZ ||
        bytes < sizeof(WCHAR) || bytes > sizeof(value) || bytes % sizeof(WCHAR)) return;
    count = bytes / sizeof(WCHAR);
    if (value[count - 1] != 0 || count > size) return;
    for (i = 0; i + 1 < count; i++) {
        if (!value[i] || value[i] > 127) return;
    }
    for (i = 0; i < count; i++) out[i] = (char)value[i];
}
static void PortDeviceReadWindowsBios(char* vendor, size_t vendorSize, char* product, size_t productSize) {
    HMODULE library;
    PortDeviceRegGetValue readValue = NULL;
    vendor[0] = product[0] = 0;
    library = LoadLibraryW(L"advapi32.dll");
    if (!library) return;
    /* memcpy avoids casting incompatible function signatures under -Wcast-function-type. */
    {
        FARPROC address = GetProcAddress(library, "RegGetValueW");
        if (sizeof(address) == sizeof(readValue)) memcpy(&readValue, &address, sizeof(readValue));
    }
    PortDeviceReadBiosValue(readValue, L"SystemManufacturer", vendor, vendorSize);
    PortDeviceReadBiosValue(readValue, L"SystemProductName", product, productSize);
    FreeLibrary(library);
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
    char vendor[128], product[128];
    PortDeviceReadWindowsBios(vendor, sizeof(vendor), product, sizeof(product));
    result = PortDeviceClassify(PORT_DEVICE_WINDOWS, getenv("SteamDeck"), vendor, product);
#else
    result = PortDeviceClassify(PORT_DEVICE_OTHER, NULL, NULL, NULL);
#endif
    if (result.profile == PORT_DEVICE_STEAM_DECK)
        return strcmp(result.reason, "steam-env") == 0 ? "steam-deck:steam-env" :
               strcmp(result.reason, "valve-bios") == 0 ? "steam-deck:valve-bios" : "steam-deck:valve-dmi";
    return strcmp(result.reason, "unsupported-platform") == 0 ?
        "generic:unsupported-platform" : "generic:unrecognized-hardware";
}
#endif
