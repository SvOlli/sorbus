# CC65 configuration for sorbus apple1 and mcp variants
# based upon C64-asm.cfg

FEATURES {
    STARTADDRESS: default = $0400;
}
MEMORY {
    ZP:       file = "", start = $0000,  size = $0100,      define = yes;
    MAIN:     file = %O, start = %S,     size = $D000 - %S;
}
SEGMENTS {
    ZEROPAGE: load = ZP,       type = zp,  optional = yes;
    EXEHDR:   load = MAIN,     type = ro,  optional = yes;
    CODE:     load = MAIN,     type = rw;
    RODATA:   load = MAIN,     type = ro,  optional = yes;
    DATA:     load = MAIN,     type = rw,  optional = yes;
    BSS:      load = MAIN,     type = bss, optional = yes, define = yes;
}
