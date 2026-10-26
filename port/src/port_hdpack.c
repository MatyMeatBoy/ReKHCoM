/* The HD mod copied as one file (port/portable/hd_pack.c): unpacked into the data folder's hd on start.
 * Here for the builds whose game has its own C library (Windows, Linux, 32-bit Android); the console and 64-bit
 * Android images get it from their host, which owns the files. */
#include "port/port.h"

#if !defined(PORT_GBA_RELOC) && !defined(PORT_ANDROID64) && !defined(PORT_PS5_GUEST) && !defined(__EMSCRIPTEN__)
#include "../portable/hd_pack.c"
#include "../ps5/third_party/miniz/miniz.c"
#include "../ps5/third_party/miniz/miniz_tdef.c"
#include "../ps5/third_party/miniz/miniz_tinfl.c"
#include "../ps5/third_party/miniz/miniz_zip.c"

void PortHdPackCheck(void) {
    char root[1024];
    size_t n;
    snprintf(root, sizeof(root), "%s", PortConfigPath(""));
    n = strlen(root);
    while (n > 1 && (root[n - 1] == '/' || root[n - 1] == '\\'))
        root[--n] = 0; /* the folder itself, no trailing separator */
    HdPackInstall(root[0] ? root : ".", PortLog); /* PortAssetPresent looks again by itself */
}
#else
void PortHdPackCheck(void) {
}
#endif
