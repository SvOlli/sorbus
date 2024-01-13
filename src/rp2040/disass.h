
#ifndef DISASS_H

#include <stdint.h>

// return value: internal buffer will be overwritten with next call
const char *dis65c02( uint16_t addr, uint8_t opcode,
                      uint8_t operand, uint8_t operand2 );

#endif
