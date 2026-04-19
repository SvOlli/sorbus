# CC65 configuration for sorbus JAM variant (kernal rom)
# based upon C64-asm.cfg

MEMORY {
    ROM:      start = $E000,  size = $1F00, define = yes, file = %O, fill = yes, fillval = $FF;
    BIOS:     start = $FF00,  size = $00CA, define = yes, file = %O, fill = yes, fillval = $FF;
    FIXEND:   start = $FFCA,  size = $001A, define = yes, file = %O, fill = yes;
    VECTORS:  start = $FFE4,  size = $001C, define = yes, file = %O, fill = yes;
    ZP:       start = $0004,  size = $00FC, define = yes;
    STACK:    start = $0100,  size = $0100, define = yes;
}
SEGMENTS {
    ZEROPAGE: load = ZP,       type = zp,  optional = yes;
    CODE:     load = ROM,      type = ro;
    DATA:     load = ROM,      type = ro,  optional = yes;
    BIOS:     load = BIOS,     type = ro;
    FIXEND:   load = FIXEND,   type = ro;
    VECTORS:  load = VECTORS,  type = ro;
}
