
#ifndef __RESIDWRAPPER_H__
#define __RESIDWRAPPER_H__ __RESIDWRAPPER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


// 6581, 8580, 8580+digiboost, none
#define CFG_SID1_TYPE 0
// 0 .. 15
#define CFG_SID1_DIGIBOOST 1
// 0 .. 14
#define CFG_SID1_VOLUME 3

#define CFG_SID2_TYPE 8
#define CFG_SID2_DIGIBOOST 9
#define CFG_SID2_ADDRESS 10
#define CFG_SID2_VOLUME 11

// 0 .. 14
#define CFG_SID_PANNING 12
#define CFG_SID_BALANCE 18

//#define CFG_REGISTER_READ 2
#define CFG_TRIGGER 17
#define CFG_CLOCKSPEED 19
#define CFG_DIGIDETECT 21
#define CFG_CONFIG_WRITE 29
#define CFG_CRC1 30
#define CFG_CRC2 31
#define CFG_SIZE 32  // size of config-array
#define CFG_REG_MAX CFG_SIZE-3  // highest possible register. Last two bytes are crc


uint16_t crc16(const uint8_t *p, uint8_t l);
void setDefaultConfiguration();
void updateConfiguration();
void initReSID();
void emulateCyclesReSID(int cyclesToEmulate);
void emulateCyclesReSIDSingle(int cyclesToEmulate);
void writeReSID1(uint8_t A, uint8_t D);
void writeReSID2(uint8_t A, uint8_t D);
void outputDigi(uint8_t voice, int32_t value);
void outputReSID(int16_t *left, int16_t *right);
void outputReSID32(int32_t *left, int32_t *right);
void outputReSIDFM(int16_t *left, int16_t *right, int32_t fm, uint8_t fmHackEnable, uint8_t *fmDigis);
void resetReSID();
void readRegs(uint8_t *p1, uint8_t *p2);
uint8_t readSID1(uint8_t offset);
uint8_t readSID2(uint8_t offset);

#ifdef __cplusplus
}
#endif

#endif
