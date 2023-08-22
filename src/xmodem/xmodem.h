#ifndef _XMODEM_H_
#define _XMODEM_H_

#include <string.h>
#include <pico/stdlib.h>
int xmodemReceive(unsigned char *dest, int destsz);
int xmodemTransmit(unsigned char *src, int srcsz);

//extern int _inbyte(unsigned short timeout); // msec timeout
//extern void _outbyte(int c);

#define _inbyte(t) getchar_timeout_us(((uint32_t)t)*1000)
#define _outbyte(c) putchar_raw(c)
#endif