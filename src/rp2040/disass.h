
#ifndef DISASS_H

#include <stdint.h>

/*
 * return value: internal buffer will be overwritten with next call
 * info: 0x12345678
 * 1: bytes
 * 2: cycles
 * 3: penalties
 * 4: 0
 * 56: opcode
 * 7: bit0=numeric suffix 0-7
 * 8: addr_mode
 */

const char *dis65c02( uint16_t addr, uint8_t opcode,
                      uint8_t operand, uint8_t operand2, uint32_t *info );

#endif
