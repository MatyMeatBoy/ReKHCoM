/* Command line around the launcher's save parser: save_convert_cli <in> <out> <region 0-2>.
 * Prints "<blocks> <found-as> <from-region>" and writes the converted 64 KB, exit 1 when no save is found. */
#include "save_import.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
int main(int argc, char** argv) {
    if (argc != 4) return 2;
    std::ifstream in(argv[1], std::ios::binary);
    std::vector<unsigned char> data((std::istreambuf_iterator<char>(in)), {}), out;
    int from = -1;
    const char* how = "";
    int blocks = RekhcomSave::Convert(data, std::atoi(argv[3]), out, from, &how);
    if (blocks <= 0) { std::printf("0 none -1\n"); return 1; }
    std::ofstream(argv[2], std::ios::binary).write((const char*)out.data(), (std::streamsize)out.size());
    std::printf("%d %s %d\n", blocks, how, from);
    return 0;
}
