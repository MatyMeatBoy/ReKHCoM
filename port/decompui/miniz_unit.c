/* miniz (MIT, port/ps5/third_party/miniz) for the launcher: the HD importer writes the HD sprites as PNG. */
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#define MINIZ_NO_ARCHIVE_APIS
#include "../ps5/third_party/miniz/miniz.c"
#include "../ps5/third_party/miniz/miniz_tdef.c"
#include "../ps5/third_party/miniz/miniz_tinfl.c"
