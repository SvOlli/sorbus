#ifndef _XMODEM_H_
#define _XMODEM_H_

#ifdef RECEIVE_FULL_FILE
int xmodemReceive(unsigned char *dest, int destsz);
#else
int xmodemReceive(void);
int received_chunk(unsigned char * buf, int size);
#endif
int _inbyte(int msec);
void _outbyte(unsigned char c);

#endif