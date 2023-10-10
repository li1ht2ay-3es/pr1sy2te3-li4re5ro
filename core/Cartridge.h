/* ----------------------------------------------------------------------------
 *   ___  ___  ___  ___       ___  ____  ___  _  _
 *  /__/ /__/ /  / /__  /__/ /__    /   /_   / |/ /
 * /    / \  /__/ ___/ ___/ ___/   /   /__  /    /  emulator
 *
 * ----------------------------------------------------------------------------
 * Copyright 2005 Greg Stanton
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ----------------------------------------------------------------------------
 * Cartridge.h
 * ----------------------------------------------------------------------------
 */
#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#define CARTRIDGE_TYPE_NORMAL           0
#define CARTRIDGE_TYPE_SUPERCART        1
#define CARTRIDGE_TYPE_SUPERCART_LARGE  2
#define CARTRIDGE_TYPE_SUPERCART_RAM    3
#define CARTRIDGE_TYPE_SUPERCART_ROM    4
#define CARTRIDGE_TYPE_SUPERCART_RAMX2  5
#define CARTRIDGE_TYPE_ABSOLUTE         6
#define CARTRIDGE_TYPE_ACTIVISION       7
#define CARTRIDGE_TYPE_SOUPER           8
#define CARTRIDGE_TYPE_FRACTALUS        9
#define CARTRIDGE_TYPE_FLAT_WITH_RAM    10
#define CARTRIDGE_TYPE_BANKSETS         11
#define CARTRIDGE_TYPE_BANKSETS_RAM     12
#define CARTRIDGE_TYPE_BANKSETS_HALTRAM 13

#define CARTRIDGE_CONTROLLER_NONE 0
#define CARTRIDGE_CONTROLLER_JOYSTICK 1
#define CARTRIDGE_CONTROLLER_LIGHTGUN 2

#define CARTRIDGE_WSYNC_MASK 2
#define CARTRIDGE_CYCLE_STEALING_MASK 1

#define CARTRIDGE_SOUPER_BANK_SEL 0x8000
#define CARTRIDGE_SOUPER_CHR_A_SEL 0x8001
#define CARTRIDGE_SOUPER_CHR_B_SEL 0x8002
#define CARTRIDGE_SOUPER_MODE_SEL 0x8003
#define CARTRIDGE_SOUPER_EXRAM_V_SEL 0x8004
#define CARTRIDGE_SOUPER_EXRAM_D_SEL 0x8005
#define CARTRIDGE_SOUPER_AUDIO_CMD 0x8007
#define CARTRIDGE_SOUPER_MODE_MFT 0x1
#define CARTRIDGE_SOUPER_MODE_CHR 0x2
#define CARTRIDGE_SOUPER_MODE_EXS 0x4

#define POKEY_NONE                      0
#define POKEY_AT_4000                   1
#define POKEY_AT_4000_W                 2
#define POKEY_AT_450                    3
#define POKEY_AT_800                    4

#include <stdint.h>
#include <boolean.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern char* cartridge_GetNextNonemptyLine(const char **stream, size_t* size);
extern bool cartridge_ReadFile(uint8_t** outData, size_t* outSize, const char* subpath, const char* relativeTo);
extern bool cartridge_LoadFromCDF(const char* data, size_t size, const char *workingDir);
extern uint8_t cartridge_LoadROM(uint32_t address);
extern bool cartridge_Load(bool persistent_data, const uint8_t* data, uint32_t size);
extern void cartridge_Store(void);
extern void cartridge_StoreBank(uint8_t bank);
extern void cartridge_Write(uint16_t address, uint8_t data);
extern bool cartridge_IsLoaded(void);
extern void cartridge_Release(bool persistent_data);

extern char cartridge_digest[33];
extern int cartridge_type;
extern int cartridge_region;
extern int cartridge_pokey;
extern int cartridge_controller[2];
extern int cartridge_bank;
extern int cartridge_flags;
extern int cartridge_bupchip;
extern int cartridge_souper_chr_bank[2];
extern int cartridge_souper_mode;
extern int cartridge_souper_ram_page_bank[2];

#ifdef __cplusplus
}
#endif

#endif
