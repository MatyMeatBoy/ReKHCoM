# Map-card menu crash386

Reproduced with the reporter's copied EU SRAM: 12 map-card kinds. On the exact
published Windows64 pre1 host/guest, loading slot0 and entering mode_mapinspect
ends with0xC00000FD. Original SRAM/log hashes remain unchanged.

A debugger breakpoint at CrashFilter, before it attempts any logging, captures
the original exception0xC0000094 at guest0x101314b9. The matching pre1 guest ELF
maps this to MapInspectDraw's signed division. Its range is ceil(kinds/3)-4;
10/11/12 kinds all give zero, and scroll0 passes the old scroll<=range test.
This is a valid one-page inventory, independent of HD being present.

The reported stack overflow is secondary: the Windows64 crash reporter calls
CRT/file/module functions on the foreign guest stack. The second fault lands
inside ucrtbase with a return to CrashWhere, hiding the original divide error.
That shared reporting mechanism is fixed separately by PR387; increasing the
PE stack reserve does not correct this divisor.

The PC-port scrollbar now requires a positive range and a valid scroll offset.
Empty/single-page or out-of-range presentation states keep the marker at its
top. Valid multi-page positions keep the original formula. All ports use the
same PLATFORM_PC core; original GBA compilation retains its original code.

test_mapinspect_scroll_range.py compiles the production calculation and checks
812 count/scroll pairs (0..27 kinds, scroll-1..27). RED was0xC0000094 on the
original calculation; the extra negative-offset control failed97 before its
guard. Both now pass. An interim guarded guest using read-only pre1 cached
objects opened this same saved menu and reached frame1900, exit0, under actual
hidden D3D11. The final combined build is verified separately before delivery.

Private evidence is in pruebas/issue380-door/crash386-{loaded64,first64,stack64,fixed64}*
and pruebas/issue386-inputs. The save and debugger dumps are not distributed.
The related MsCharge scrollbar is handled by PR388. This does not claim every
64-bit report shares this cause or that all device reports are confirmed fixed.
