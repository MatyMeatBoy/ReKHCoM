/* Flat image of the guest, loaded by the host at PS5G_BASE. */
ENTRY(ps5_boot)
SECTIONS {
    . = 0x10000000;
    .header : {
        LONG(0x35474B52)
        LONG(__image_end)
        LONG(ps5_boot)
        LONG(gHostTable)
        LONG(__heap_start)
        LONG(__heap_end)
        LONG(__stack_top)
        LONG(__region_end)
        LONG(gPortBiosArea)
        /* Optional metadata; older hosts still read the original nine words. */
        LONG(0x31564447)
        LONG(__text_start)
        LONG(__text_end)
    }
    .text : { __text_start = .; *(.text .text.*) __text_end = .; }
    .rodata : ALIGN(64) { *(.rodata .rodata.*) }
    .data : ALIGN(64) { *(.data .data.*) }
    khdata : ALIGN(64) {
        __start_khdata = .;
        *(khdata)
        __stop_khdata = .;
    }
    .bss : ALIGN(64) {
        __bss_start = .;
        *(.bss .bss.* COMMON)
    }
    khbss : ALIGN(64) {
        __start_khbss = .;
        *(khbss)
        __stop_khbss = .;
        . = ALIGN(4096);
        __image_end = .;
    }
    __heap_start = __image_end;
    __heap_end = __heap_start + 0x0C000000;
    __stack_top = __heap_end + 0x00400000;
    __region_end = __stack_top;
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) *(.llvm_addrsig) }
}
