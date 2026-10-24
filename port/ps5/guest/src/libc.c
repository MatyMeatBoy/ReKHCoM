/* The small C library of the guest: strings, formatting, a heap in the guest
 * arena, and stdio/math over the host bridge. */
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "bridge_calls.h"

/* ---- strings ------------------------------------------------------------ */

int memcmp(const void* a, const void* b, size_t n) {
    const uint8_t *p = a, *q = b;
    for (; n; n--, p++, q++)
        if (*p != *q)
            return *p - *q;
    return 0;
}

void* memchr(const void* s, int c, size_t n) {
    const uint8_t* p = s;
    for (; n; n--, p++)
        if (*p == (uint8_t)c)
            return (void*)p;
    return NULL;
}

size_t strlen(const char* s) {
    const char* e = s;
    while (*e) e++;
    return (size_t)(e - s);
}

int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) a++, b++;
    return (uint8_t)*a - (uint8_t)*b;
}

int strncmp(const char* a, const char* b, size_t n) {
    for (; n; n--, a++, b++) {
        if (*a != *b) return (uint8_t)*a - (uint8_t)*b;
        if (!*a) return 0;
    }
    return 0;
}

static int Lower(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }

int strcasecmp(const char* a, const char* b) {
    while (*a && Lower((uint8_t)*a) == Lower((uint8_t)*b)) a++, b++;
    return Lower((uint8_t)*a) - Lower((uint8_t)*b);
}

int strncasecmp(const char* a, const char* b, size_t n) {
    for (; n; n--, a++, b++) {
        int d = Lower((uint8_t)*a) - Lower((uint8_t)*b);
        if (d || !*a) return d;
    }
    return 0;
}

char* strcpy(char* d, const char* s) {
    char* r = d;
    while ((*d++ = *s++)) {}
    return r;
}

char* strncpy(char* d, const char* s, size_t n) {
    size_t i = 0;
    for (; i < n && s[i]; i++) d[i] = s[i];
    for (; i < n; i++) d[i] = 0;
    return d;
}

char* strcat(char* d, const char* s) {
    strcpy(d + strlen(d), s);
    return d;
}

char* strncat(char* d, const char* s, size_t n) {
    char* e = d + strlen(d);
    while (n-- && *s) *e++ = *s++;
    *e = 0;
    return d;
}

char* strchr(const char* s, int c) {
    for (;; s++) {
        if (*s == (char)c) return (char*)s;
        if (!*s) return NULL;
    }
}

char* strrchr(const char* s, int c) {
    const char* r = NULL;
    for (;; s++) {
        if (*s == (char)c) r = s;
        if (!*s) return (char*)r;
    }
}

char* strstr(const char* h, const char* n) {
    size_t l = strlen(n);
    if (!l) return (char*)h;
    for (; *h; h++)
        if (*h == *n && strncmp(h, n, l) == 0) return (char*)h;
    return NULL;
}

char* strdup(const char* s) {
    size_t n = strlen(s) + 1;
    char* d = malloc(n);
    if (d) memcpy(d, s, n);
    return d;
}

/* ---- heap ----------------------------------------------------------------
 * Boundary-tag allocator over [heapStart, heapEnd): 16-byte blocks, a header
 * with the size and the previous block's size, first fit with coalescing. */

#ifdef __MACH__
/* the Switch image (Mach-O): the heap is a zero-filled segment of port/switch/guest/crt0_a64.S */
extern char rk_heap[];
#define __heap_start rk_heap
#define __heap_end (rk_heap + 0x06000000)
#else
extern char __heap_start[], __heap_end[];
#endif

typedef struct Blk {
    uint32_t size;  /* bytes of the whole block, bit 0: in use */
    uint32_t prev;  /* size of the previous block (0 for the first) */
    uint32_t pad[2];
} Blk;

#define HDR sizeof(Blk)
static Blk* sFirst;
static Blk* sEnd;
static Blk* sRover;

static void HeapInit(void) {
    uintptr_t a = ((uintptr_t)__heap_start + 15) & ~(uintptr_t)15;
    uintptr_t e = (uintptr_t)__heap_end & ~(uintptr_t)15;
    sFirst = (Blk*)a;
    sEnd = (Blk*)e;
    sFirst->size = (uint32_t)(e - a);
    sFirst->prev = 0;
    sRover = sFirst;
}

static Blk* Next(Blk* b) {
    Blk* n = (Blk*)((char*)b + (b->size & ~1u));
    return n >= sEnd ? NULL : n;
}

void* malloc(size_t n) {
    uint32_t need;
    Blk *b, *start;
    int wrapped = 0;

    if (!sFirst) HeapInit();
    if (n == 0) n = 1;
    if (n > 0x7FFFFF00u) return NULL;
    need = (uint32_t)((n + HDR + 15) & ~(size_t)15);
    b = start = sRover;
    for (;;) {
        if (!(b->size & 1) && b->size >= need) {
            if (b->size - need >= 64) {
                Blk* rest = (Blk*)((char*)b + need);
                rest->size = b->size - need;
                rest->prev = need;
                b->size = need;
                if (Next(rest)) Next(rest)->prev = rest->size;
            }
            b->size |= 1;
            sRover = Next(b) ? Next(b) : sFirst;
            return (char*)b + HDR;
        }
        b = Next(b);
        if (!b) {
            b = sFirst;
            wrapped = 1;
        }
        if (b == start && wrapped) return NULL;
        if (b == start) return NULL;
    }
}

void free(void* p) {
    Blk *b, *n, *pr;
    if (!p) return;
    b = (Blk*)((char*)p - HDR);
    b->size &= ~1u;
    n = Next(b);
    if (n && !(n->size & 1)) {
        if (sRover == n) sRover = b;
        b->size += n->size;
        n = Next(b);
        if (n) n->prev = b->size;
    }
    if (b->prev) {
        pr = (Blk*)((char*)b - b->prev);
        if (!(pr->size & 1)) {
            if (sRover == b) sRover = pr;
            pr->size += b->size;
            n = Next(pr);
            if (n) n->prev = pr->size;
        }
    }
}

void* calloc(size_t a, size_t b) {
    uint64_t total = (uint64_t)a * b;
    void* p;
    if (total > 0x7FFFFF00u) return NULL;
    p = malloc((size_t)total);
    if (p) memset(p, 0, (size_t)total);
    return p;
}

void* realloc(void* p, size_t n) {
    Blk* b;
    size_t old;
    void* q;
    if (!p) return malloc(n);
    if (!n) { free(p); return NULL; }
    b = (Blk*)((char*)p - HDR);
    old = (b->size & ~1u) - HDR;
    if (n <= old) return p;
    q = malloc(n);
    if (!q) return NULL;
    memcpy(q, p, old);
    free(p);
    return q;
}

/* ---- numbers ------------------------------------------------------------ */

int abs(int v) { return v < 0 ? -v : v; }
long labs(long v) { return v < 0 ? -v : v; }

unsigned long strtoul(const char* s, char** end, int base) {
    unsigned long v = 0;
    int any = 0;
    while (*s == ' ' || (*s >= 9 && *s <= 13)) s++;
    if (*s == '+') s++;
    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] | 32) == 'x') { s += 2; base = 16; }
    else if (base == 0) base = s[0] == '0' ? 8 : 10;
    for (;; s++) {
        int d;
        if (*s >= '0' && *s <= '9') d = *s - '0';
        else if ((*s | 32) >= 'a' && (*s | 32) <= 'z') d = (*s | 32) - 'a' + 10;
        else break;
        if (d >= base) break;
        v = v * (unsigned)base + (unsigned)d;
        any = 1;
    }
    if (end) *end = (char*)(any ? s : (const char*)0);
    return v;
}

unsigned long long strtoull(const char* s, char** end, int base) {
    unsigned long long v = 0;
    int any = 0;
    while (*s == ' ' || (*s >= 9 && *s <= 13)) s++;
    if (*s == '+') s++;
    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] | 32) == 'x') { s += 2; base = 16; }
    else if (base == 0) base = s[0] == '0' ? 8 : 10;
    for (;; s++) {
        int d;
        if (*s >= '0' && *s <= '9') d = *s - '0';
        else if ((*s | 32) >= 'a' && (*s | 32) <= 'z') d = (*s | 32) - 'a' + 10;
        else break;
        if (d >= base) break;
        v = v * (unsigned)base + (unsigned)d;
        any = 1;
    }
    if (end) *end = (char*)(any ? s : (const char*)0);
    return v;
}

long strtol(const char* s, char** end, int base) {
    const char* t = s;
    int neg = 0;
    unsigned long v;
    while (*t == ' ' || (*t >= 9 && *t <= 13)) t++;
    if (*t == '-') { neg = 1; t++; }
    v = strtoul(t, end, base);
    if (end && !*end) *end = (char*)s;
    return neg ? -(long)v : (long)v;
}

int atoi(const char* s) { return (int)strtol(s, NULL, 10); }

double atof(const char* s) {
    double v = 0, f = 0.1;
    int neg = 0;
    while (*s == ' ') s++;
    if (*s == '-') neg = 1, s++;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    if (*s == '.')
        for (s++; *s >= '0' && *s <= '9'; s++, f *= 0.1) v += (*s - '0') * f;
    return neg ? -v : v;
}

static uint32_t sRand = 1;
int rand(void) {
    sRand = sRand * 1103515245u + 12345u;
    return (int)((sRand >> 1) & 0x7fffffff);
}
void srand(unsigned s) { sRand = s; }

static void Swap(char* a, char* b, size_t n) {
    while (n--) { char t = *a; *a++ = *b; *b++ = t; }
}

void qsort(void* base, size_t n, size_t size, int (*cmp)(const void*, const void*)) {
    char* a = base;
    while (n > 1) {
        size_t i = 0, j = n - 1, m = n / 2;
        Swap(a + m * size, a + (n - 1) * size, size);
        for (i = 0, j = 0; j < n - 1; j++)
            if (cmp(a + j * size, a + (n - 1) * size) < 0) {
                if (i != j) Swap(a + i * size, a + j * size, size);
                i++;
            }
        Swap(a + i * size, a + (n - 1) * size, size);
        if (i < n - i - 1) {
            qsort(a, i, size, cmp);
            a += (i + 1) * size;
            n -= i + 1;
        } else {
            qsort(a + (i + 1) * size, n - i - 1, size, cmp);
            n = i;
        }
    }
}

/* ---- formatting ---------------------------------------------------------- */

typedef struct Out {
    char* buf;
    size_t cap, len;
} Out;

static void Put(Out* o, char c) {
    if (o->len + 1 < o->cap) o->buf[o->len] = c;
    o->len++;
}

static void PutPadded(Out* o, const char* s, size_t n, int width, int left, char pad) {
    size_t i;
    if (!left) for (i = n; (int)i < width; i++) Put(o, pad);
    for (i = 0; i < n; i++) Put(o, s[i]);
    if (left) for (i = n; (int)i < width; i++) Put(o, ' ');
}

static size_t Utoa(char* out, uint64_t v, unsigned base, int upper) {
    char tmp[24];
    size_t n = 0, i;
    do {
        unsigned d = (unsigned)(v % base);
        tmp[n++] = (char)(d < 10 ? '0' + d : (upper ? 'A' : 'a') + d - 10);
        v /= base;
    } while (v);
    for (i = 0; i < n; i++) out[i] = tmp[n - 1 - i];
    return n;
}

static size_t Ftoa(char* out, double v, int prec) {
    char* p = out;
    uint64_t ip, fp, scale = 1;
    int i;
    if (v != v) { strcpy(out, "nan"); return 3; }
    if (v < 0) { *p++ = '-'; v = -v; }
    if (v > 1e18) { strcpy(p, "inf"); return (size_t)(p - out) + 3; }
    if (prec > 9) prec = 9;
    for (i = 0; i < prec; i++) scale *= 10;
    ip = (uint64_t)v;
    fp = (uint64_t)((v - (double)ip) * (double)scale + 0.5);
    if (fp >= scale) { ip++; fp -= scale; }
    p += Utoa(p, ip, 10, 0);
    if (prec) {
        char f[16];
        size_t n = Utoa(f, fp, 10, 0);
        *p++ = '.';
        for (i = (int)n; i < prec; i++) *p++ = '0';
        memcpy(p, f, n);
        p += n;
    }
    return (size_t)(p - out);
}

int vsnprintf(char* buf, size_t cap, const char* f, va_list ap) {
    Out o = { buf, cap, 0 };
    for (; *f; f++) {
        int left = 0, zero = 0, plus = 0, width = 0, prec = -1, lng = 0;
        char tmp[48];
        size_t n;
        if (*f != '%') { Put(&o, *f); continue; }
        f++;
        for (;; f++) {
            if (*f == '-') left = 1;
            else if (*f == '0') zero = 1;
            else if (*f == '+') plus = 1;
            else if (*f == ' ' || *f == '#') {}
            else break;
        }
        if (*f == '*') { width = va_arg(ap, int); if (width < 0) { left = 1; width = -width; } f++; }
        else while (*f >= '0' && *f <= '9') width = width * 10 + (*f++ - '0');
        if (*f == '.') {
            f++;
            prec = 0;
            if (*f == '*') { prec = va_arg(ap, int); f++; }
            else while (*f >= '0' && *f <= '9') prec = prec * 10 + (*f++ - '0');
        }
        for (;; f++) {
            if (*f == 'l') lng++;
            else if (*f == 'z' || *f == 't' || *f == 'j') lng = 1;
            else if (*f == 'h') {}
            else break;
        }
        switch (*f) {
        case 'd': case 'i': {
            int64_t v = lng >= 2 ? va_arg(ap, int64_t) : (int64_t)va_arg(ap, int);
            size_t k = 0;
            if (v < 0) { tmp[k++] = '-'; n = Utoa(tmp + k, (uint64_t)-v, 10, 0); }
            else { if (plus) tmp[k++] = '+'; n = Utoa(tmp + k, (uint64_t)v, 10, 0); }
            n += k;
            if (zero && !left && width > (int)n) {
                int sign = k;
                PutPadded(&o, tmp, (size_t)sign, 0, 1, ' ');
                PutPadded(&o, tmp + sign, n - sign, width - sign, 0, '0');
            } else PutPadded(&o, tmp, n, width, left, ' ');
            break;
        }
        case 'u': case 'x': case 'X': case 'o': {
            uint64_t v = lng >= 2 ? va_arg(ap, uint64_t) : (uint64_t)va_arg(ap, unsigned);
            n = Utoa(tmp, v, *f == 'u' ? 10 : *f == 'o' ? 8 : 16, *f == 'X');
            PutPadded(&o, tmp, n, width, left, zero && !left ? '0' : ' ');
            break;
        }
        case 'p': {
            uint64_t v = (uint32_t)(uintptr_t)va_arg(ap, void*);
            tmp[0] = '0'; tmp[1] = 'x';
            n = Utoa(tmp + 2, v, 16, 0) + 2;
            PutPadded(&o, tmp, n, width, left, ' ');
            break;
        }
        case 'c':
            tmp[0] = (char)va_arg(ap, int);
            PutPadded(&o, tmp, 1, width, left, ' ');
            break;
        case 's': {
            const char* s = va_arg(ap, const char*);
            if (!s) s = "(null)";
            n = strlen(s);
            if (prec >= 0 && (size_t)prec < n) n = (size_t)prec;
            PutPadded(&o, s, n, width, left, ' ');
            break;
        }
        case 'f': case 'F': case 'g': case 'G': case 'e': {
            double v = va_arg(ap, double);
            n = Ftoa(tmp, v, prec < 0 ? 6 : prec);
            if (*f == 'g' || *f == 'G') {
                if (prec < 0 && memchr(tmp, '.', n)) {
                    while (n && tmp[n - 1] == '0') n--;
                    if (n && tmp[n - 1] == '.') n--;
                }
            }
            PutPadded(&o, tmp, n, width, left, zero && !left ? '0' : ' ');
            break;
        }
        case '%': Put(&o, '%'); break;
        case 0: f--; break;
        default: Put(&o, '%'); Put(&o, *f); break;
        }
    }
    if (cap) o.buf[o.len < cap ? o.len : cap - 1] = 0;
    return (int)o.len;
}

int snprintf(char* b, size_t n, const char* f, ...) {
    va_list ap;
    int r;
    va_start(ap, f);
    r = vsnprintf(b, n, f, ap);
    va_end(ap);
    return r;
}

int sprintf(char* b, const char* f, ...) {
    va_list ap;
    int r;
    va_start(ap, f);
    r = vsnprintf(b, 0x7fffffff, f, ap);
    va_end(ap);
    return r;
}

/* sscanf: %d %u %x %i %s %c %[...] %f %n, with widths, %*, and literals. */
int sscanf(const char* s, const char* f, ...) {
    va_list ap;
    int count = 0;
    const char* start = s;
    va_start(ap, f);
    for (; *f; f++) {
        if (*f == ' ' || (*f >= 9 && *f <= 13)) {
            while (*s == ' ' || (*s >= 9 && *s <= 13)) s++;
            continue;
        }
        if (*f != '%') {
            if (*s != *f) goto done;
            s++;
            continue;
        }
        {
            int skip = 0, width = 0, lng = 0;
            f++;
            if (*f == '*') { skip = 1; f++; }
            while (*f >= '0' && *f <= '9') width = width * 10 + (*f++ - '0');
            while (*f == 'l' || *f == 'h' || *f == 'z') { if (*f == 'l') lng++; f++; }
            if (*f == 'n') {
                if (!skip) *va_arg(ap, int*) = (int)(s - start);
                continue;
            }
            if (*f != 'c' && *f != '[')
                while (*s == ' ' || (*s >= 9 && *s <= 13)) s++;
            if (!*s) goto done;
            switch (*f) {
            case 'd': case 'i': case 'u': case 'x': case 'X': {
                char* e;
                long v;
                int base = *f == 'x' || *f == 'X' ? 16 : *f == 'i' ? 0 : 10;
                if (*f == 'u' || base == 16) v = (long)strtoul(s, &e, base);
                else v = strtol(s, &e, base);
                if (e == s) goto done;
                s = e;
                if (!skip) {
                    if (lng) *va_arg(ap, long*) = v;
                    else *va_arg(ap, int*) = (int)v;
                    count++;
                }
                break;
            }
            case 'f': case 'g': case 'e': {
                const char* e = s;
                double v;
                if (*e == '-' || *e == '+') e++;
                while ((*e >= '0' && *e <= '9') || *e == '.') e++;
                if (e == s) goto done;
                v = atof(s);
                s = e;
                if (!skip) {
                    if (lng) *va_arg(ap, double*) = v;
                    else *va_arg(ap, float*) = (float)v;
                    count++;
                }
                break;
            }
            case 's': {
                char* out = skip ? NULL : va_arg(ap, char*);
                int n = 0;
                while (*s && *s != ' ' && !(*s >= 9 && *s <= 13) && (!width || n < width)) {
                    if (out) out[n] = *s;
                    n++, s++;
                }
                if (out) { out[n] = 0; count++; }
                break;
            }
            case 'c': {
                char* out = skip ? NULL : va_arg(ap, char*);
                if (out) { *out = *s; count++; }
                s++;
                break;
            }
            case '[': {
                char* out = skip ? NULL : va_arg(ap, char*);
                const char* set;
                int neg = 0, n = 0;
                f++;
                if (*f == '^') neg = 1, f++;
                set = f;
                if (*f == ']') f++;
                while (*f && *f != ']') f++;
                while (*s && (!width || n < width)) {
                    const char* q;
                    int in = 0;
                    for (q = set; q < f; q++) {
                        if (q + 2 < f && q[1] == '-') {
                            if ((uint8_t)*s >= (uint8_t)q[0] && (uint8_t)*s <= (uint8_t)q[2]) in = 1;
                            q += 2;
                        } else if (*q == *s) in = 1;
                    }
                    if (in == neg) break;
                    if (out) out[n] = *s;
                    n++, s++;
                }
                if (!n) goto done;
                if (out) { out[n] = 0; count++; }
                break;
            }
            default: goto done;
            }
        }
    }
done:
    va_end(ap);
    return count;
}

/* ---- stdio ---------------------------------------------------------------- */

struct GFile {
    int h;
    int eof, err;
    int ungot;
};

static struct GFile sStd[3] = { { -1, 0, 0, -1 }, { -2, 0, 0, -1 }, { -3, 0, 0, -1 } };
FILE* stdin = &sStd[0];
FILE* stdout = &sStd[1];
FILE* stderr = &sStd[2];

FILE* fopen(const char* path, const char* mode) {
    int m = mode[0] == 'w' ? 1 : mode[0] == 'a' ? 2 : 0;
    FILE* f;
    int h;
    if (strchr(mode, '+')) m = m == 0 ? 3 : m;
    h = h_fopen(path, m);
    if (h < 0) return NULL;
    f = malloc(sizeof(*f));
    if (!f) { h_fclose(h); return NULL; }
    f->h = h;
    f->eof = f->err = 0;
    f->ungot = -1;
    return f;
}

int fclose(FILE* f) {
    int r;
    if (!f) return EOF;
    r = f->h >= 0 ? h_fclose(f->h) : 0;
    if (f >= &sStd[0] && f <= &sStd[2]) return r;
    free(f);
    return r;
}

size_t fread(void* buf, size_t size, size_t count, FILE* f) {
    uint64_t total = (uint64_t)size * count;
    int n;
    if (!f || f->h < 0 || !total) return 0;
    n = h_fread(f->h, buf, (uint32_t)total);
    if (n < 0) { f->err = 1; return 0; }
    if ((uint64_t)n < total) f->eof = 1;
    return size ? (size_t)n / size : 0;
}

size_t fwrite(const void* buf, size_t size, size_t count, FILE* f) {
    uint64_t total = (uint64_t)size * count;
    int n;
    if (!f || !total) return 0;
    if (f->h < 0) {
        char line[512];
        size_t k = total < sizeof(line) - 1 ? (size_t)total : sizeof(line) - 1;
        memcpy(line, buf, k);
        line[k] = 0;
        h_log(line);
        return count;
    }
    n = h_fwrite(f->h, buf, (uint32_t)total);
    if (n < 0) { f->err = 1; return 0; }
    return size ? (size_t)n / size : 0;
}

int fseek(FILE* f, long off, int whence) {
    f->eof = 0;
    return f && f->h >= 0 ? h_fseek(f->h, (int32_t)off, whence) : -1;
}

long ftell(FILE* f) { return f && f->h >= 0 ? h_ftell(f->h) : -1; }
void rewind(FILE* f) { fseek(f, 0, SEEK_SET); }
int fflush(FILE* f) { return f && f->h >= 0 ? h_fflush(f->h) : 0; }
int feof(FILE* f) { return f->eof; }
int ferror(FILE* f) { return f->err; }

int fgetc(FILE* f) {
    uint8_t c;
    if (fread(&c, 1, 1, f) != 1) return EOF;
    return c;
}

int ungetc(int c, FILE* f) {
    if (c == EOF || !f || f->h < 0) return EOF;
    h_fseek(f->h, -1, SEEK_CUR);
    f->eof = 0;
    return c;
}

int fputc(int c, FILE* f) {
    uint8_t b = (uint8_t)c;
    return fwrite(&b, 1, 1, f) == 1 ? c : EOF;
}

char* fgets(char* s, int n, FILE* f) {
    int i = 0;
    while (i < n - 1) {
        int c = fgetc(f);
        if (c == EOF) break;
        s[i++] = (char)c;
        if (c == '\n') break;
    }
    if (!i) return NULL;
    s[i] = 0;
    return s;
}

int fputs(const char* s, FILE* f) { return (int)fwrite(s, 1, strlen(s), f); }

int vfprintf(FILE* f, const char* fmt, va_list ap) {
    char line[1024];
    int n = vsnprintf(line, sizeof(line), fmt, ap);
    if (n > (int)sizeof(line) - 1) n = (int)sizeof(line) - 1;
    fwrite(line, 1, (size_t)n, f);
    return n;
}

int fprintf(FILE* f, const char* fmt, ...) {
    va_list ap;
    int r;
    va_start(ap, fmt);
    r = vfprintf(f, fmt, ap);
    va_end(ap);
    return r;
}

int printf(const char* fmt, ...) {
    va_list ap;
    int r;
    va_start(ap, fmt);
    r = vfprintf(stdout, fmt, ap);
    va_end(ap);
    return r;
}

int puts(const char* s) { h_log(s); return 1; }
int remove(const char* p) { return h_remove(p); }
int rename(const char* a, const char* b) { return h_rename(a, b); }

/* ---- process ------------------------------------------------------------- */

/* Values longer than a slot (the harness' KHCOM_MODE / KHCOM_KEYS lists run to kilobytes) are read again into a
   bigger one: cut at 159 bytes, the Android story scenarios lost their key presses and stalled in the event. */
char* getenv(const char* name) {
    enum { SLOTS = 128 };
    static char* slots[SLOTS];
    static const char* keys[SLOTS];
    static int used;
    int i, n, cap = 160;
    for (i = 0; i < used; i++)
        if (strcmp(keys[i], name) == 0)
            return slots[i] && slots[i][0] == 1 ? NULL : slots[i];
    if (used == SLOTS) /* the table is full: start over, nothing is returned from it for long */
        used = 0;
    i = used++;
    free((void*)keys[i]);
    keys[i] = strdup(name);
    for (;;) {
        free(slots[i]);
        slots[i] = malloc((size_t)cap);
        if (!slots[i])
            return NULL;
        n = h_getenv(name, slots[i], cap - 1);
        if (n < cap - 2 || cap >= (1 << 20))
            break;
        cap *= 4; /* it filled the slot: maybe cut */
    }
    if (n < 0) { slots[i][0] = 1; slots[i][1] = 0; return NULL; }
    slots[i][n] = 0;
    return slots[i];
}

void exit(int code) { h_exit(code); for (;;) {} }
void abort(void) { h_log("abort()"); h_exit(134); for (;;) {} }

double sin(double x) { return h_sin(x); }
double cos(double x) { return h_cos(x); }
double tan(double x) { return h_tan(x); }
double atan2(double y, double x) { return h_atan2(y, x); }
double pow(double x, double y) { return h_pow(x, y); }
double exp(double x) { return h_exp(x); }
double log(double x) { return h_log_e(x); }
double fmod(double x, double y) { return h_fmod(x, y); }
float sinf(float x) { return h_sinf(x); }
float cosf(float x) { return h_cosf(x); }
float powf(float x, float y) { return h_powf(x, y); }

long lround(double x) { return (long)(x < 0 ? x - 0.5 : x + 0.5); }
long lroundf(float x) { return (long)(x < 0 ? x - 0.5f : x + 0.5f); }
