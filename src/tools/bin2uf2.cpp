/*
 * This is a heavily stripped down and modified version of elf2uf2,
 * which is
 *
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstdio>
#include <map>
#include <set>
#include <vector>
#include <cstring>
#include <cstdarg>
#include <algorithm>
#include <string>
#include "boot/uf2.h"

typedef unsigned int uint;

#define ERROR_ARGS -1
#define ERROR_FORMAT -2
#define ERROR_INCOMPATIBLE -3
#define ERROR_READ_FAILED -4
#define ERROR_WRITE_FAILED -5
#define ERROR_OUT_OF_FLASH_RANGE -6

#define FLASH_SECTOR_ERASE_SIZE 4096u

static char error_msg[512];
static bool verbose;

static int fail(int code, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(error_msg, sizeof(error_msg), format, args);
    va_end(args);
    return code;
}

static int fail_read_error() {
    return fail(ERROR_READ_FAILED, "Failed to read input file");
}

static int fail_write_error() {
    return fail(ERROR_WRITE_FAILED, "Failed to write output file");
}

// we require 256 (as this is the page size supported by the device)
#define LOG2_PAGE_SIZE 8u
#define PAGE_SIZE (1u << LOG2_PAGE_SIZE)

#define MAIN_RAM_START        0x20000000u // same as SRAM_BASE in addressmap.h
#define MAIN_RAM_END          0x20042000u // same as SRAM_END in addressmap.h
#define FLASH_START           0x10000000u // same as XIP_MAIN_BASE in addressmap.h
#define FLASH_END             0x15000000u
#define XIP_SRAM_START        0x15000000u // same as XIP_SRAM_BASE in addressmap.h
#define XIP_SRAM_END          0x15004000u // same as XIP_SRAM_END in addressmap.h
#define MAIN_RAM_BANKED_START 0x21000000u // same as SRAM0_BASE in addressmap.h
#define MAIN_RAM_BANKED_END   0x21040000u
#define ROM_START             0x00000000u // same as ROM_BASE in addressmap.h
#define ROM_END               0x00004000u

struct page_fragment {
    page_fragment(uint32_t file_offset, uint32_t page_offset, uint32_t bytes) : file_offset(file_offset), page_offset(page_offset), bytes(bytes) {}
    page_fragment& operator=(const page_fragment& that)
    {
        this->file_offset = that.file_offset;
        this->page_offset = that.page_offset;
        this->bytes       = that.bytes;
        return *this;
    }
    uint32_t file_offset;
    uint32_t page_offset;
    uint32_t bytes;
};

static int usage() {
    fprintf(stderr, "Usage: bin2uf2 (-v) <input binary file> <start address> <output UF2 file>\n");
    return ERROR_ARGS;
}

int realize_page(FILE *in, const std::vector<page_fragment> &fragments, uint8_t *buf, uint buf_len) {
    assert(buf_len >= PAGE_SIZE);
    for(auto& frag : fragments) {
        assert(frag.page_offset >= 0 && frag.page_offset < PAGE_SIZE && frag.page_offset + frag.bytes <= PAGE_SIZE);
        if (fseek(in, frag.file_offset, SEEK_SET)) {
            return fail_read_error();
        }
        if (1 != fread(buf + frag.page_offset, frag.bytes, 1, in)) {
            return fail_read_error();
        }
    }
    return 0;
}

int bin2uf2(FILE *in, long address, FILE *out) {
    std::map<uint32_t, std::vector<page_fragment>> pages;
    fseek(in, 0, SEEK_END);
    long in_size = ftell(in);
    fseek(in, 0, SEEK_SET);
    int rc = 0;

    if( (address < FLASH_START) ||
        (address + in_size) > (FLASH_START + 0x1000000u) ) // 16MB, maximum available range
    {
        return fail(ERROR_OUT_OF_FLASH_RANGE, "Data no in flash range");
    }

    for( long i = 0; i < in_size; i += PAGE_SIZE )
    {
        uint32_t bytes = PAGE_SIZE;
        if( (in_size - i) < PAGE_SIZE )
        {
            bytes = in_size - i;
        }
        pages[address+i].push_back( page_fragment(i, 0, bytes) );
    }

    std::set<uint32_t> touched_sectors;
    for (auto& page_entry : pages) {
        uint32_t sector = page_entry.first / FLASH_SECTOR_ERASE_SIZE;
        touched_sectors.insert(sector);
    }

    uint32_t last_page = pages.rbegin()->first;
    for (uint32_t sector : touched_sectors) {
        for (uint32_t page = sector * FLASH_SECTOR_ERASE_SIZE; page < (sector + 1) * FLASH_SECTOR_ERASE_SIZE; page += PAGE_SIZE) {
            if (page < last_page) {
                // Create a dummy page, if it does not exist yet. note that all present pages are first
                // zeroed before they are filled with any contents, so a dummy page will be all zeros.
            }
        }
    }

    uint page_num = 0;
    uf2_block block;
    block.magic_start0 = UF2_MAGIC_START0;
    block.magic_start1 = UF2_MAGIC_START1;
    block.flags = UF2_FLAG_FAMILY_ID_PRESENT;
    block.payload_size = PAGE_SIZE;
    block.num_blocks = (uint32_t)pages.size();
    block.file_size = RP2040_FAMILY_ID;
    block.magic_end = UF2_MAGIC_END;
    for(auto& page_entry : pages) {
        block.target_addr = page_entry.first;
        block.block_no = page_num++;
        if (verbose) {
            printf("Page %d / %d %08x%s\n", block.block_no, block.num_blocks, block.target_addr,
                   page_entry.second.empty() ? " (padding)": "");
        }
        memset(block.data, 0, sizeof(block.data));
        rc = realize_page(in, page_entry.second, block.data, sizeof(block.data));
        if (rc) return rc;
        if (1 != fwrite(&block, sizeof(uf2_block), 1, out)) {
            return fail_write_error();
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    int arg = 1;
    if (arg < argc && !strcmp(argv[arg], "-v")) {
        verbose = true;
        arg++;
    }
    if (argc < arg + 3) {
        return usage();
    }
    const char *in_filename = argv[arg++];
    FILE *in = fopen(in_filename, "rb");
    if (!in) {
        fprintf(stderr, "Can't open input file '%s'\n", in_filename);
        return ERROR_ARGS;
    }
    long address = std::stol(argv[arg++], 0, 16);
    const char *out_filename = argv[arg++];
    FILE *out = fopen(out_filename, "wb");
    if (!out) {
        fprintf(stderr, "Can't open output file '%s'\n", out_filename);
        return ERROR_ARGS;
    }

    int rc = bin2uf2(in, address, out);
    fclose(in);
    fclose(out);
    if (rc) {
        remove(out_filename);
        if (error_msg[0]) {
            fprintf(stderr, "ERROR: %s\n", error_msg);
        }
    }
    return rc;
}
