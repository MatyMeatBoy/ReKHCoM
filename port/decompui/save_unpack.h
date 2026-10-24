#ifndef REKHCOM_SAVE_UNPACK_H
#define REKHCOM_SAVE_UNPACK_H
/*
 * Containers that can hold a GBA save: gzip (VBA / VBA-M .sgm and .gz), zlib, ZIP,
 * PNG with mGBA's state chunks (.ss1 ... .ssN), RetroArch's RZIP states, and any
 * deflate stream found inside a file that none of those describes. Everything is
 * bounded (output size, nesting, number of buffers). ROM-free, no dependencies:
 * a small RFC 1951 decoder lives here so the parser also builds for consoles / web.
 */
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace RekhcomSave {

using Bytes = std::vector<unsigned char>;

namespace inflate_detail {
struct Tree {
    uint16_t counts[16];
    uint16_t symbols[288];
};
inline void Build(Tree& t, const uint8_t* lengths, unsigned n) {
    uint16_t offsets[16];
    std::memset(t.counts, 0, sizeof(t.counts));
    for (unsigned i = 0; i < n; i++) t.counts[lengths[i]]++;
    t.counts[0] = 0;
    unsigned sum = 0;
    for (int i = 0; i < 16; i++) { offsets[i] = (uint16_t)sum; sum += t.counts[i]; }
    for (unsigned i = 0; i < n; i++)
        if (lengths[i]) t.symbols[offsets[lengths[i]]++] = (uint16_t)i;
}
struct Stream {
    const uint8_t* p;
    const uint8_t* end;
    uint32_t tag = 0;
    int bits = 0;
    bool error = false;
    int Bit() {
        if (!bits) {
            if (p >= end) { error = true; return 0; }
            tag = *p++;
            bits = 8;
        }
        int b = tag & 1;
        tag >>= 1;
        bits--;
        return b;
    }
    unsigned Read(int n, unsigned base = 0) {
        unsigned v = 0;
        for (int i = 0; i < n; i++) v |= (unsigned)Bit() << i;
        return v + base;
    }
    int Symbol(const Tree& t) {
        int sum = 0, cur = 0, len = 0;
        do {
            cur = 2 * cur + Bit();
            if (++len > 15) { error = true; return 0; }
            sum += t.counts[len];
            cur -= t.counts[len];
        } while (cur >= 0 && !error);
        return t.symbols[sum + cur];
    }
};
}  // namespace inflate_detail

/* Raw deflate (RFC 1951) of [data, data+size) appended to out, at most limit bytes in all.
 * Returns false on malformed data or when the limit is exceeded; out keeps what was decoded. */
inline bool Inflate(const uint8_t* data, size_t size, Bytes& out, size_t limit, size_t* used = nullptr) {
    using namespace inflate_detail;
    static const uint16_t lengthBase[] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
                                           35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
    static const uint8_t lengthBits[] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
    static const uint16_t distBase[] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513,
                                         769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
    static const uint8_t distBits[] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };
    static const uint8_t order[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
    Stream s;
    s.p = data;
    s.end = data + size;
    size_t start = out.size();
    int final_block;
    do {
        final_block = s.Bit();
        unsigned type = s.Read(2);
        if (s.error) return false;
        if (type == 0) {
            s.bits = 0; /* stored: byte aligned */
            if (s.end - s.p < 4) return false;
            unsigned len = s.p[0] | s.p[1] << 8, nlen = s.p[2] | s.p[3] << 8;
            s.p += 4;
            if ((len ^ 0xFFFF) != nlen || (size_t)(s.end - s.p) < len) return false;
            if (out.size() - start + len > limit) return false;
            out.insert(out.end(), s.p, s.p + len);
            s.p += len;
        } else if (type == 1 || type == 2) {
            Tree lit, dist;
            if (type == 1) {
                uint8_t l[288];
                for (int i = 0; i < 144; i++) l[i] = 8;
                for (int i = 144; i < 256; i++) l[i] = 9;
                for (int i = 256; i < 280; i++) l[i] = 7;
                for (int i = 280; i < 288; i++) l[i] = 8;
                Build(lit, l, 288);
                uint8_t d[30];
                for (int i = 0; i < 30; i++) d[i] = 5;
                Build(dist, d, 30);
            } else {
                unsigned hlit = s.Read(5, 257), hdist = s.Read(5, 1), hclen = s.Read(4, 4);
                if (s.error || hlit > 286 || hdist > 30) return false;
                uint8_t lengths[320] = {};
                for (unsigned i = 0; i < hclen; i++) lengths[order[i]] = (uint8_t)s.Read(3);
                Tree code;
                Build(code, lengths, 19);
                uint8_t all[320] = {};
                unsigned n = 0;
                while (n < hlit + hdist) {
                    int sym = s.Symbol(code);
                    if (s.error) return false;
                    if (sym < 16) { all[n++] = (uint8_t)sym; continue; }
                    unsigned rep, value = 0;
                    if (sym == 16) {
                        if (!n) return false;
                        value = all[n - 1];
                        rep = s.Read(2, 3);
                    } else if (sym == 17) {
                        rep = s.Read(3, 3);
                    } else {
                        rep = s.Read(7, 11);
                    }
                    if (n + rep > hlit + hdist) return false;
                    while (rep--) all[n++] = (uint8_t)value;
                }
                Build(lit, all, hlit);
                Build(dist, all + hlit, hdist);
            }
            for (;;) {
                int sym = s.Symbol(lit);
                if (s.error) return false;
                if (sym < 256) {
                    if (out.size() - start + 1 > limit) return false;
                    out.push_back((unsigned char)sym);
                } else if (sym == 256) {
                    break;
                } else {
                    sym -= 257;
                    if (sym >= 29) return false;
                    unsigned len = s.Read(lengthBits[sym], lengthBase[sym]);
                    int ds = s.Symbol(dist);
                    if (s.error || ds >= 30) return false;
                    unsigned d = s.Read(distBits[ds], distBase[ds]);
                    if (s.error || d > out.size() - start || out.size() - start + len > limit) return false;
                    size_t from = out.size() - d;
                    for (unsigned i = 0; i < len; i++) out.push_back(out[from + i]);
                }
            }
        } else {
            return false;
        }
    } while (!final_block);
    if (used) *used = (size_t)(s.p - data);
    return true;
}

namespace unpack_detail {
inline uint32_t LE32(const unsigned char* p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
inline uint32_t BE32(const unsigned char* p) { return p[3] | p[2] << 8 | p[1] << 16 | (uint32_t)p[0] << 24; }
inline uint16_t LE16(const unsigned char* p) { return (uint16_t)(p[0] | p[1] << 8); }

inline bool LooksZlib(const unsigned char* p, size_t size) {
    return size >= 6 && (p[0] & 15) == 8 && (p[0] >> 4) <= 7 && ((p[0] << 8) | p[1]) % 31 == 0 && !(p[1] & 32);
}
inline bool Zlib(const unsigned char* p, size_t size, Bytes& out, size_t limit) {
    return LooksZlib(p, size) && Inflate(p + 2, size - 2, out, limit) && !out.empty();
}
inline bool Gzip(const unsigned char* p, size_t size, Bytes& out, size_t limit) {
    if (size < 18 || p[0] != 0x1f || p[1] != 0x8b || p[2] != 8) return false;
    size_t at = 10;
    unsigned flags = p[3];
    if (flags & 4) { if (at + 2 > size) return false; at += 2 + LE16(p + at); }
    if (flags & 8) { while (at < size && p[at]) at++; at++; }
    if (flags & 16) { while (at < size && p[at]) at++; at++; }
    if (flags & 2) at += 2;
    if (at >= size) return false;
    return Inflate(p + at, size - at, out, limit) && !out.empty();
}
}  // namespace unpack_detail

/* Every buffer that may hold a save: the input itself and what its containers unpack to
 * (nested up to four levels, never more than maxBuffers or maxBytes of output). */
class Unpacker {
public:
    std::vector<Bytes> buffers;
    explicit Unpacker(const Bytes& input, size_t maxBytes = 48u << 20, size_t maxBuffers = 96)
        : maxBytes_(maxBytes), maxBuffers_(maxBuffers) {
        buffers.push_back(input);
        depths_.push_back(0);
        for (size_t i = 0; i < buffers.size(); i++) {
            const Bytes cur = buffers[i]; /* a copy: Add() grows buffers */
            Expand(cur, depths_[i]);
        }
    }

private:
    size_t maxBytes_, maxBuffers_, total_ = 0;
    std::vector<int> depths_;

    void Add(Bytes&& b, int depth) {
        if (b.empty() || buffers.size() >= maxBuffers_ || total_ + b.size() > maxBytes_) return;
        total_ += b.size();
        buffers.push_back(std::move(b));
        depths_.push_back(depth + 1);
    }
    size_t Room() const { return total_ < maxBytes_ ? maxBytes_ - total_ : 0; }

    void Expand(const Bytes& in, int depth) {
        using namespace unpack_detail;
        if (depth >= 4 || in.size() < 8) return;
        const unsigned char* p = in.data();
        size_t n = in.size();
        bool known = false;
        Bytes out;
        if (p[0] == 0x1f && p[1] == 0x8b) {
            known = true;
            if (Gzip(p, n, out, Room())) Add(std::move(out), depth);
        } else if (p[0] == 0x89 && !std::memcmp(p + 1, "PNG\r\n\x1a\n", 7)) {
            known = true;
            Png(in, depth);
        } else if (p[0] == 'P' && p[1] == 'K') {
            known = true;
            Zip(in, depth);
        } else if (!std::memcmp(p, "#RZIPv", 6) && n > 20) {
            known = true;
            Rzip(in, depth);
        } else if (LooksZlib(p, n)) {
            if (Zlib(p, n, out, Room())) { known = true; Add(std::move(out), depth); }
        }
        if (!known) Sweep(in, depth);
    }

    /* mGBA: chunk "gbAs" is the zlib state; each "gbAx" is tag(4) size(4) + zlib data (the save is one of them) */
    void Png(const Bytes& in, int depth) {
        using namespace unpack_detail;
        size_t at = 8;
        while (at + 12 <= in.size()) {
            uint32_t len = BE32(in.data() + at);
            const unsigned char* type = in.data() + at + 4;
            if (len > in.size() || at + 12 + (size_t)len > in.size()) break;
            const unsigned char* data = in.data() + at + 8;
            if (!std::memcmp(type, "gbAs", 4) || !std::memcmp(type, "gbAx", 4)) {
                bool ext = type[3] == 'x';
                size_t skip = ext ? 8 : 0;
                Bytes out;
                if (len > skip && Zlib(data + skip, len - skip, out, Room())) Add(std::move(out), depth);
                else Add(Bytes(data + skip, data + len), depth); /* stored without zlib */
            } else if (!std::memcmp(type, "IEND", 4)) {
                break;
            }
            at += 12 + len;
        }
    }

    void Zip(const Bytes& in, int depth) {
        using namespace unpack_detail;
        const unsigned char* p = in.data();
        size_t n = in.size();
        size_t eocd = std::string::npos;
        for (size_t i = n >= 22 ? n - 22 : 0; i + 4 <= n; i--) {
            if (!std::memcmp(p + i, "PK\5\6", 4)) { eocd = i; break; }
            if (!i || n - i > 22 + 65535) break;
        }
        auto entry = [&](size_t local, uint32_t csize, uint32_t usize, unsigned method) {
            if (local + 30 > n || std::memcmp(p + local, "PK\3\4", 4)) return;
            size_t data = local + 30 + LE16(p + local + 26) + LE16(p + local + 28);
            if (data > n || csize > n - data || usize > Room()) return;
            Bytes out;
            if (method == 0) out.assign(p + data, p + data + csize);
            else if (method == 8) { if (!Inflate(p + data, csize, out, Room())) return; }
            else return;
            Add(std::move(out), depth);
        };
        if (eocd != std::string::npos && eocd + 22 <= n) {
            uint32_t count = LE16(p + eocd + 10), cd = LE32(p + eocd + 16);
            size_t at = cd;
            for (uint32_t i = 0; i < count && at + 46 <= n && !std::memcmp(p + at, "PK\1\2", 4); i++) {
                entry(LE32(p + at + 42), LE32(p + at + 20), LE32(p + at + 24), LE16(p + at + 10));
                at += 46 + LE16(p + at + 28) + LE16(p + at + 30) + LE16(p + at + 32);
            }
            return;
        }
        /* no directory: walk the local headers (sizes in the header, no data descriptors) */
        size_t at = 0;
        while (at + 30 <= n && !std::memcmp(p + at, "PK\3\4", 4)) {
            uint32_t csize = LE32(p + at + 18);
            entry(at, csize, LE32(p + at + 22), LE16(p + at + 8));
            at += 30 + LE16(p + at + 26) + LE16(p + at + 28) + csize;
        }
    }

    /* libretro-common rzip: "#RZIPv" ver '#', chunk size u32, total size u64, then u32 size + zlib per chunk */
    void Rzip(const Bytes& in, int depth) {
        using namespace unpack_detail;
        size_t at = 20;
        Bytes out;
        while (at + 4 <= in.size()) {
            uint32_t len = LE32(in.data() + at);
            at += 4;
            if (len > in.size() - at) break;
            if (!Zlib(in.data() + at, len, out, Room())) break;
            at += len;
        }
        Add(std::move(out), depth);
    }

    /* Unknown wrapper: try every gzip / zlib header inside it (an emulator's own container with
     * compressed parts). Each attempt costs at most one inflate; the number of attempts is capped. */
    void Sweep(const Bytes& in, int depth) {
        using namespace unpack_detail;
        const unsigned char* p = in.data();
        int attempts = 0;
        for (size_t i = 1; i + 8 < in.size() && attempts < 64; i++) {
            Bytes out;
            bool ok = false;
            if (p[i] == 0x1f && p[i + 1] == 0x8b && p[i + 2] == 8) {
                attempts++;
                ok = Gzip(p + i, in.size() - i, out, Room());
            } else if (p[i] == 0x78 && (p[i + 1] == 0x01 || p[i + 1] == 0x5e || p[i + 1] == 0x9c || p[i + 1] == 0xda) &&
                       LooksZlib(p + i, in.size() - i)) {
                attempts++;
                ok = Zlib(p + i, in.size() - i, out, Room());
            }
            if (ok && out.size() >= 1024) Add(std::move(out), depth);
        }
    }
};

}  // namespace RekhcomSave
#endif
