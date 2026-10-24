#ifndef GUARD_TYPES_H
#define GUARD_TYPES_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
#ifdef PLATFORM_PC
/* ARM (APCS) aligns 64-bit integers to 4 bytes inside structs; x86 MinGW uses 8.
   Keep the GBA layout so struct sizes, save data and ROM tables stay compatible. */
typedef unsigned long long __attribute__((aligned(4))) u64;
typedef signed long long __attribute__((aligned(4))) s64;

#define PORT_STRINGIFY_(x) #x
#define PORT_STRINGIFY(x) PORT_STRINGIFY_(x)
/* Assembler name of a C symbol (i686 COFF prefixes C names with an underscore). */
#define PORT_ASM_SYMBOL(name) PORT_STRINGIFY(__USER_LABEL_PREFIX__) name
#else
typedef unsigned long long u64;
typedef signed long long s64;
#endif

typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;

/*
 * BIOS_PTR(p): the original game dereferences NULL in a few places (marked
 * "@bug"). On hardware those accesses hit the BIOS area: reads return the
 * open-bus value and writes are ignored. The PC build redirects them to a
 * buffer that reproduces that behaviour.
 */
#ifdef PLATFORM_PC
extern unsigned char gPortBiosArea[];
#define BIOS_PTR(p) ((p) != 0 ? (p) : (__typeof__(p))(void*)gPortBiosArea)
/* BIOS_LOW_PTR(p): the same for any pointer into the first 64 KiB (a small
   number read as a pointer, e.g. a counter of another task's work). */
#define BIOS_LOW_PTR(p) ((unsigned long)(p) < 0x10000ul ?     (__typeof__(p))(void*)(gPortBiosArea + (unsigned long)(p)) : (p))
/* BUS_PTR(type, p): a value used as a pointer although it is none (an open-bus word, a word past a ROM table),
   decoded as the GBA bus does (port/src/port_bus.c); a real pointer into the GBA space is left as it is. */
void* PortBusPtr(__UINTPTR_TYPE__ p);
#define BUS_PTR(type, p) ((type)PortBusPtr((__UINTPTR_TYPE__)(p)))
#else
#define BIOS_PTR(p) (p)
#define BIOS_LOW_PTR(p) (p)
#define BUS_PTR(type, p) ((type)(p))
#endif

/*
 * DIV0/MOD0: the cartridge's division routines return 0 for a zero divisor (quotient and
 * remainder). Hardware x86 raises SIGFPE and WebAssembly traps instead, so the places the
 * original reaches with a zero divisor (marked "@bug ... division by zero") use these.
 */
#ifdef PLATFORM_PC
/* both operands are evaluated once and in the original's order (GetRandom() still advances) */
#define DIV0(a, b) ({ __typeof__((a) / 1) _a = (a); __typeof__(b) _b = (b); _b == 0 ? 0 : _a / _b; })
#define MOD0(a, b) ({ __typeof__((a) % 1) _a = (a); __typeof__(b) _b = (b); _b == 0 ? 0 : _a % _b; })
#else
#define DIV0(a, b) ((a) / (b))
#define MOD0(a, b) ((a) % (b))
#endif

#define TRUE 1
#define FALSE 0

#endif /* GUARD_TYPES_H */
