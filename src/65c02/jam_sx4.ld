# CC65 configuration for Sorbus JAM "SX4" executable
# SX4 = Sorbus eXecutable at address $0400

MEMORY {
    ZP:       file = "",      start = $0010,  size = $00F0, define = yes;
    MAIN:     file = "%O",    start = $0400,  size = $CC00;
}
SEGMENTS {
    ZEROPAGE: load = ZP,      type = zp,  optional = yes;
    CODE:     load = MAIN,    type = rw;
    RODATA:   load = MAIN,    type = ro,  optional = yes;
    DATA:     load = MAIN,    type = rw,  optional = yes;
    BSS:      load = MAIN,    type = bss, optional = yes, define = yes;
}
