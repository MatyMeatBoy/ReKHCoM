/* arm64_32 Mach-O images inside a Linux LP64 host. All MAP_FIXED replacements
 * stay within a reservation acquired without MAP_FIXED. Never overwrite a map. */
#include <unistd.h>
typedef struct { uint32_t magic, cpu, subtype, type, ncmds, cmds, flags; } ArmMachHeader;
typedef struct {
    uint32_t cmd, size; char name[16];
    uint32_t addr, length, offset, bytes, maxprot, prot, sections, flags;
} ArmMachSegment;
typedef struct { uint32_t cmd, size, offset, bytes; } ArmMachRebase;

static int ArmImageFail(const char *reason) { Log("ARM64 image: %s", reason); return 0; }
static int ArmMapped(const ArmMachSegment *s) {
    return s->length && strncmp(s->name, "__PAGEZERO", 16) && strncmp(s->name, "__LINKEDIT", 16);
}
static int ArmUleb(const uint8_t **p, const uint8_t *end, uint64_t *v) {
    *v = 0;
    for (unsigned shift = 0; shift < 64 && *p < end; shift += 7) {
        uint8_t b = *(*p)++;
        if (shift == 63 && (b & 0x7e)) return 0;
        *v |= (uint64_t)(b & 127) << shift;
        if (!(b & 128)) return 1;
    }
    return 0;
}
static int ArmRebase(const uint8_t *p, const uint8_t *end, const ArmMachSegment *const *segments,
                     unsigned n, uint32_t slide) {
    unsigned seg = n, type = 0; uint64_t at = 0, count, skip;
    while (p < end) {
        unsigned byte = *p++, imm = byte & 15;
        switch (byte & 240) {
        case 0: return 1;
        case 0x10: type = imm; continue;
        case 0x20: seg = imm; if (!ArmUleb(&p, end, &at) || at > UINT32_MAX) return 0; continue;
        case 0x30:
            if (!ArmUleb(&p, end, &skip) || skip > UINT32_MAX - at) return 0;
            at += skip; continue;
        case 0x40: if (at > UINT32_MAX - imm * 4u) return 0; at += imm * 4u; continue;
        case 0x50: count = imm; skip = 0; break;
        case 0x60: if (!ArmUleb(&p, end, &count)) return 0; skip = 0; break;
        case 0x70: count = 1; if (!ArmUleb(&p, end, &skip)) return 0; break;
        case 0x80: if (!ArmUleb(&p, end, &count) || !ArmUleb(&p, end, &skip)) return 0; break;
        default: return 0;
        }
        if (seg >= n || type != 1 || !ArmMapped(segments[seg]) || skip > UINT32_MAX || count > UINT32_MAX / 4) return 0;
        while (count--) {
            const ArmMachSegment *s = segments[seg];
            if (at > s->length || s->length - at < 4 || (at & 3)) return 0;
            uint32_t *word = (uint32_t *)(uintptr_t)(s->addr + slide + at);
            *word += slide;
            at += 4 + skip;
        }
    }
    return 0; /* A complete stream ends with DONE. */
}
static void *ArmReserve(uint32_t low, size_t length) {
    /* mmap hints are safe on old kernels too: accept only the exact free range. */
    if (!getenv("KHCOM_IMAGE_MOVE")) {
        void *p = mmap((void *)(uintptr_t)low, length, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p == (void *)(uintptr_t)low) return p;
        if (p != MAP_FAILED) munmap(p, length);
    }
    for (uint64_t at = (0xf0000000ull - length) & ~0xffffull; at >= 0x10000; ) {
        void *p = mmap((void *)(uintptr_t)at, length, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p == (void *)(uintptr_t)at) return p;
        if (p != MAP_FAILED) munmap(p, length);
        if (at < 0x100000) break;
        at -= 0x100000;
    }
    return MAP_FAILED;
}
static int ArmContains(const ArmMachSegment *const *segments, unsigned n, uint32_t slide,
                       uint32_t at, size_t length, unsigned prot) {
    for (unsigned i = 0; i < n; i++) {
        const ArmMachSegment *s = segments[i];
        uint64_t start = (uint32_t)(s->addr + slide);
        if (ArmMapped(s) && (s->prot & prot) == prot && at >= start &&
            (uint64_t)at + length <= start + s->length) return 1;
    }
    return 0;
}
static int LoadArm64Image(const char *path, Ps5GuestHeader *out) {
    size_t bytes = 0;
    uint8_t *file = SDL_LoadFile(path, &bytes);
    const ArmMachSegment *segments[16]; unsigned n = 0;
    const ArmMachRebase *rebase = NULL;
    uint64_t low = UINT32_MAX, high = 0;
    uint32_t headerAt = 0, slide = 0;
    void *reservation = MAP_FAILED;
    size_t length = 0;
    const char *error = "invalid or incomplete Mach-O image";
    long page = sysconf(_SC_PAGESIZE);
    if (page < 4096 || page > 65536 || (page & (page - 1))) goto fail;
    if (!file || bytes < sizeof(ArmMachHeader)) goto fail;
    const ArmMachHeader *h = (const ArmMachHeader *)file;
    if (h->magic != 0xfeedface || h->cpu != 0x0200000c || h->type != 2 || !(h->flags & 0x200000) ||
        h->cmds > bytes - sizeof(*h) || h->ncmds > h->cmds / 8) goto fail;
    const uint8_t *p = file + sizeof(*h), *cmdEnd = p + h->cmds;
    for (unsigned i = 0; i < h->ncmds; i++) {
        if ((size_t)(cmdEnd - p) < 8) goto fail;
        const ArmMachSegment *s = (const ArmMachSegment *)p;
        if (s->size < 8 || s->size > (size_t)(cmdEnd - p) || (s->size & 3)) goto fail;
        if (s->cmd == 1) {
            if (s->size < sizeof(*s) || n == 16) goto fail;
            segments[n++] = s;
            if (ArmMapped(s)) {
                uint64_t end = (uint64_t)s->addr + s->length;
                if (end >= 0x100000000ull || s->bytes > s->length ||
                    s->offset + (uint64_t)s->bytes > bytes || (s->addr & (page - 1))) goto fail;
                if (s->addr < low) low = s->addr;
                if (end > high) high = end;
            }
        } else if (s->cmd == 0x80000022) {
            if (s->size < sizeof(ArmMachRebase) || rebase) goto fail;
            rebase = (const ArmMachRebase *)p;
        }
        p += s->size;
    }
    high = (high + page - 1) & ~(uint64_t)(page - 1);
    if (low >= high || high >= 0x100000000ull || high - low > 0xe0000000ull || !rebase ||
        rebase->offset + (uint64_t)rebase->bytes > bytes) goto fail;
    /* Segments must not overlap after page rounding. */
    for (unsigned i = 0; i < n; i++) if (ArmMapped(segments[i]))
        for (unsigned j = i + 1; j < n; j++) if (ArmMapped(segments[j])) {
            const ArmMachSegment *a = segments[i], *b = segments[j];
            uint64_t ae = ((uint64_t)a->addr + a->length + page - 1) & ~(uint64_t)(page - 1);
            uint64_t be = ((uint64_t)b->addr + b->length + page - 1) & ~(uint64_t)(page - 1);
            if (a->addr < be && b->addr < ae) goto fail;
        }
    length = (size_t)(high - low);
    reservation = ArmReserve((uint32_t)low, length);
    if (reservation == MAP_FAILED) { error = "no free address range below 4 GiB"; goto fail; }
    slide = (uint32_t)(uintptr_t)reservation - (uint32_t)low;
    for (unsigned i = 0; i < n; i++) {
        const ArmMachSegment *s = segments[i];
        if (!ArmMapped(s)) continue;
        size_t len = ((size_t)s->length + page - 1) & ~(size_t)(page - 1);
        void *at = (void *)(uintptr_t)(s->addr + slide);
        if (mprotect(at, len, PROT_READ | PROT_WRITE)) { error = "cannot populate reserved image"; goto fail; }
        memcpy(at, file + s->offset, s->bytes);
        if (!strncmp(s->name, "__RKHDR", 16)) {
            if (s->length < sizeof(*out) || headerAt) goto fail;
            headerAt = s->addr + slide;
        }
    }
    if (!ArmRebase(file + rebase->offset, file + rebase->offset + rebase->bytes, segments, n, slide)) {
        error = "invalid pointer relocation stream"; goto fail;
    }
    if (!headerAt) { error = "missing bridge header"; goto fail; }
    memcpy(out, (const void *)(uintptr_t)headerAt, sizeof(*out));
    uint64_t base = (uintptr_t)reservation, end = base + length;
    if (out->magic != PS5G_MAGIC || (out->boot & 3) || (out->hostTable & 7) || (out->stackTop & 15) ||
        !ArmContains(segments, n, slide, out->boot, 4, 4) ||
        !ArmContains(segments, n, slide, out->hostTable, HF_COUNT * 8, 2) ||
        out->stackTop < 16 || !ArmContains(segments, n, slide, out->stackTop - 16, 16, 2) ||
        out->boot < base || out->boot >= end ||
        out->hostTable < base || out->hostTable + (uint64_t)HF_COUNT * 8 > end ||
        out->stackTop <= base || out->stackTop > end) { error = "invalid bridge header"; goto fail; }
    for (unsigned i = 0; i < n; i++) {
        const ArmMachSegment *s = segments[i];
        if (!ArmMapped(s)) continue;
        char *at = (char *)(uintptr_t)(s->addr + slide);
        size_t len = ((size_t)s->length + page - 1) & ~(size_t)(page - 1);
        int prot = (s->prot & 1 ? PROT_READ : 0) | (s->prot & 2 ? PROT_WRITE : 0) | (s->prot & 4 ? PROT_EXEC : 0);
        if ((prot & PROT_WRITE) && (prot & PROT_EXEC)) { error = "writable executable segment refused"; goto fail; }
        if (prot & PROT_EXEC) __builtin___clear_cache(at, at + len);
        if (mprotect(at, len, prot)) { error = "cannot protect image segments"; goto fail; }
    }
    out->imageEnd = out->regionEnd = (uint32_t)end;
    Log("ARM64 image %s at %08x, slide %08x", path, (unsigned)base, slide);
    SDL_free(file);
    return 1;
fail:
    if (reservation != MAP_FAILED) munmap(reservation, length);
    SDL_free(file);
    return ArmImageFail(error);
}
