/*
       ______/  _____/  _____/     /   _/    /             /
     _/           /     /     /   /  _/     /   ______/   /  _/             ____/     /   ______/   ____/
      ___/       /     /     /   ___/      /   /         __/                    _/   /   /         /     /
         _/    _/    _/    _/   /  _/     /  _/         /  _/             _____/    /  _/        _/    _/
  ______/   _____/  ______/   _/    _/  _/    _____/  _/    _/          _/        _/    _____/    ____/

  reSIDWrapper.cc

  SIDKick pico - SID-replacement with dual-SID/SID+fm emulation using a RPi pico, reSID 0.16 and fmopl
  Copyright (c) 2023/2024 Carsten Dachsbacher <frenetic@dachsbacher.de>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include <pico/multicore.h>
#include "hardware/clocks.h"
#include "reSID16/sid.h"

#include "reSID_LUT_bin.h"

// TODO: Move this to header

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

#define CFG_REGISTER_READ 2
#define CFG_TRIGGER 17
#define CFG_CLOCKSPEED 19
#define CFG_DIGIDETECT 21
#define CFG_CONFIG_WRITE 29
#define CFG_CRC1 30
#define CFG_CRC2 31
#define CFG_SIZE 32  // size of config-array
#define CFG_REG_MAX CFG_SIZE-3  // highest possible register. Last two bytes are crc

static int32_t cfgVolSID1_Left, cfgVolSID1_Right;
static int32_t cfgVolSID2_Left, cfgVolSID2_Right;
static int32_t actVolSID1_Left, actVolSID1_Right;
static int32_t actVolSID2_Left, actVolSID2_Right;

uint32_t C64_CLOCK = 985248;
uint8_t SID_DIGI_DETECT = 0;
uint32_t SID2_FLAG = 0;
uint8_t SID2_IOx_global = 0;
uint8_t FM_ENABLE = 0;
uint32_t SID2_ADDR_PREV = 255;
uint8_t config[CFG_SIZE];


SID16 *sid16a;
SID16 *sid16b;

extern "C"
{
    uint16_t crc16(const uint8_t *p, uint8_t l)
    {
        uint8_t x;
        uint16_t crc = 0xFFFF;

        while (l--)
        {
            x = crc >> 8 ^ *p++;
            x ^= x >> 4;
            crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
        }
        return crc;
    }

    void setDefaultConfiguration()
    {
        for (uint8_t i = 0; i < CFG_REG_MAX; i++)
        {
            config[i] = 0;
        }

        config[CFG_SID1_TYPE] = MOS8580;
        config[CFG_SID2_TYPE] = MOS8580;
        config[CFG_REGISTER_READ] = 1;
        config[CFG_SID2_ADDRESS] = 1 + 4 * 0;
        config[CFG_SID1_DIGIBOOST] = 12;
        config[CFG_SID2_DIGIBOOST] = 12;
        config[CFG_SID1_VOLUME] = 12;
        config[CFG_SID2_VOLUME] = 12;
        config[CFG_SID_PANNING] = 5;
        config[CFG_SID_BALANCE] = 7;
        config[CFG_CLOCKSPEED] = 0;
        config[CFG_DIGIDETECT] = 1;
        config[CFG_TRIGGER] = 0;

        uint16_t c = crc16(config, CFG_CRC1);
        config[CFG_CRC1] = (c & 255);
        config[CFG_CRC2] = (c >> 8);
    }

    void updateConfiguration()
    {
        if (config[CFG_SID1_TYPE] == 0)
            sid16a->set_chip_model(MOS6581);
        else
            sid16a->set_chip_model(MOS8580);

        const uint32_t c64clock[3] = {985248, 1022727, 1023440};

        if (config[CFG_SID1_TYPE] == 2)
            sid16a->input(-(1 << config[CFG_SID1_DIGIBOOST]));
        else
            sid16a->input(0);

        if (config[CFG_SID2_TYPE] == 0)
            sid16b->set_chip_model(MOS6581);
        else
            sid16b->set_chip_model(MOS8580);

        if (config[CFG_SID2_TYPE] == 2)
            sid16b->input(-(1 << config[CFG_SID2_DIGIBOOST]));
        else
            sid16b->input(0);

        C64_CLOCK = c64clock[config[CFG_CLOCKSPEED] % 3];
        sid16a->set_sampling_parameters(C64_CLOCK, SAMPLE_INTERPOLATE, 44100);
        sid16b->set_sampling_parameters(C64_CLOCK, SAMPLE_INTERPOLATE, 44100);

        extern const uint32_t sidFlags[6];
        SID2_FLAG = sidFlags[config[CFG_SID2_ADDRESS] % 6];
        SID2_IOx_global = config[CFG_SID2_ADDRESS] >= 4 ? 1 : 0;

        if (SID2_FLAG == 0 && config[CFG_SID2_TYPE] != 3) // $d400 && SID #2 != none?
        {
            SID2_FLAG = (1 << 31);
            SID2_IOx_global = 0;
        }

        if (config[CFG_SID2_TYPE] >= 4) // FM
            FM_ENABLE = 6 - config[CFG_SID2_TYPE];
        else
            FM_ENABLE = 0;

        if (config[CFG_SID2_ADDRESS] != SID2_ADDR_PREV)
            sid16b->reset();

        SID2_ADDR_PREV = config[CFG_SID2_ADDRESS];

        uint8_t panning = config[CFG_SID_PANNING];

        // only one SID? => center audio
        if (config[CFG_SID2_TYPE] == 3)
            panning = 7;

        cfgVolSID1_Left = (int)(config[CFG_SID1_VOLUME]) * (int)(14 - panning);
        cfgVolSID1_Right = (int)(config[CFG_SID1_VOLUME]) * (int)(panning);

        if (config[CFG_SID2_TYPE] == 3)
        {
            cfgVolSID2_Left = cfgVolSID2_Right = 0;
        }
        else
        {
            cfgVolSID2_Left = (int)(config[CFG_SID2_VOLUME]) * (int)(panning);
            cfgVolSID2_Right = (int)(config[CFG_SID2_VOLUME]) * (int)(14 - panning);
        }

        actVolSID1_Left = cfgVolSID1_Left;
        actVolSID1_Right = cfgVolSID1_Right;
        actVolSID2_Left = cfgVolSID2_Left;
        actVolSID2_Right = cfgVolSID2_Right;

        {
            const int32_t maxVolFactor = 14 * 15;
            const int32_t globalVolume = 256;
            int32_t balanceLeft, balanceRight;
            balanceLeft = balanceRight = 256;
            if (config[CFG_SID_BALANCE] < 7)
                balanceRight -= (int)(7 - config[CFG_SID_BALANCE]) * 32;
            if (config[CFG_SID_BALANCE] > 7)
                balanceLeft -= (int)(config[CFG_SID_BALANCE] - 7) * 32;
            actVolSID1_Left = actVolSID1_Left * balanceLeft * globalVolume / maxVolFactor;
            actVolSID1_Right = actVolSID1_Right * balanceRight * globalVolume / maxVolFactor;
            actVolSID2_Left = actVolSID2_Left * balanceLeft * globalVolume / maxVolFactor;
            actVolSID2_Right = actVolSID2_Right * balanceRight * globalVolume / maxVolFactor;
        }

        SID_DIGI_DETECT = config[CFG_DIGIDETECT] ? 1 : 0;

        extern void resetEverything();
        resetEverything();
    }

    volatile int doe =1;
    void initReSID()
    {

        sid16a = new SID16();
        sid16a->set_chip_model(MOS8580);
        sid16a->reset();
        sid16a->set_sampling_parameters(C64_CLOCK, SAMPLE_INTERPOLATE, 44100);

        sid16b = new SID16();
        sid16b->set_chip_model(MOS8580);
        sid16b->reset();
        sid16b->set_sampling_parameters(C64_CLOCK, SAMPLE_INTERPOLATE, 44100);

        updateConfiguration();

    }

    void emulateCyclesReSID(int cyclesToEmulate)
    {
        sid16a->clock(cyclesToEmulate);
        sid16b->clock(cyclesToEmulate);
    }

    void emulateCyclesReSIDSingle(int cyclesToEmulate)
    {
        sid16a->clock(cyclesToEmulate);
    }

    void writeReSID1(uint8_t A, uint8_t D)
    {
        sid16a->write(A, D);
    }

    void writeReSID2(uint8_t A, uint8_t D)
    {
        sid16b->write(A, D);
    }

    void outputDigi(uint8_t voice, int32_t value)
    {
        sid16a->forceDigiOutput(voice, value);
    }

    void outputReSID(int16_t *left, int16_t *right)
    {
        int32_t sid1 = sid16a->output(),
                sid2 = sid16b->output();

        int32_t L = sid1 * actVolSID1_Left + sid2 * actVolSID2_Left;
        int32_t R = sid1 * actVolSID1_Right + sid2 * actVolSID2_Right;

        *left = L >> 16;
        *right = R >> 16;

    }

    void outputReSIDFM(int16_t *left, int16_t *right, int32_t fm, uint8_t fmHackEnable, uint8_t *fmDigis)
    {
        int32_t sid1 = sid16a->output();

        int32_t L = sid1 * actVolSID1_Left + (fm * actVolSID2_Left * 2);
        int32_t R = sid1 * actVolSID1_Right + (fm * actVolSID2_Right * 2);

        L >>= 16;
        R >>= 16;
        if (L > 32767)
            L = 32767;
        if (R > 32767)
            R = 32767;
        if (L < -32767)
            L = -32767;
        if (R < -32767)
            R = -32767;
        *left = L;
        *right = R;

    }

    void resetReSID()
    {
        sid16a->reset();
        sid16b->reset();
    }

    void readRegs(uint8_t *p1, uint8_t *p2)
    {
        sid16a->readRegisters(p1);
        sid16b->readRegisters(p2);
    }

    uint8_t readSID(uint8_t offset)
    {
        return sid16a->read(offset);
    }

    uint8_t readSID2(uint8_t offset)
    {
        return sid16b->read(offset);
    }
}
