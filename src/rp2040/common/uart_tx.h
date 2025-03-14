#ifndef _UART_TX_H_
#define _UART_TX_H_ _UART_TX_H_

#include "hardware/pio.h"

void uart_tx_init( PIO pio, uint sm, uint pin_tx, uint baud );
void uart_tx_printf( const char *fmt, ... );

#endif // _UART_TX_H_
