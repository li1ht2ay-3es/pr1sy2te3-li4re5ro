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

#define CARTRIDGE_TYPE_LINEAR           0
#define CARTRIDGE_TYPE_SUPERGAME        1
#define CARTRIDGE_TYPE_ACTIVISION       2
#define CARTRIDGE_TYPE_ABSOLUTE         3
#define CARTRIDGE_TYPE_SOUPER           4

#define CARTRIDGE_CONTROLLER_NONE 0
#define CARTRIDGE_CONTROLLER_JOYSTICK 1
#define CARTRIDGE_CONTROLLER_LIGHTGUN 2

#define POKEY_NONE                      0
#define POKEY_AT_440                    1
#define POKEY_AT_450                    2
#define POKEY_AT_440_450                3
#define POKEY_AT_800                    4
#define POKEY_AT_4000                   5
#define YM2151_AT_460                   6

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
extern bool cartridge_IsLoaded(void);
extern void cartridge_Release(bool persistent_data);

extern void cartridge_Reset(void);
extern void cartridge_Frame(void);
extern void cartridge_Map(void);
extern void cartridge_MapBios(void);

extern void cartridge_Run(int cycles);
extern void cartridge_ScanlineEnd(void);

extern uint8_t cartridge_Read(uint16_t address);
extern void cartridge_Write(uint16_t address, uint8_t data);

extern void cartridge_LoadState(void);
extern void cartridge_SaveState(void);

extern char cartridge_digest[33];
extern char cartridge_title[256];
extern uint8_t cartridge_type;
extern uint8_t cartridge_region;
extern uint8_t cartridge_controller[2];
extern uint8_t cartridge_bank;
extern uint8_t cartridge_flags;
extern uint8_t cartridge_souper_chr_bank[2];
extern uint8_t cartridge_souper_mode;
extern uint8_t cartridge_souper_ram_page_bank[2];

extern uint8_t cartridge_pokey;
extern uint8_t cartridge_ym2151;
extern uint8_t cartridge_bupchip;
extern uint8_t cartridge_xm;  /* expansion module */
extern uint8_t cartridge_atarivox;

extern uint8_t cartridge_bankset;  /* halt rom */
extern uint8_t cartridge_exrom;  /* extra 16 KB rom */
extern uint8_t cartridge_exfix;  /* last bank - 1 */
extern uint8_t cartridge_exram;  /* 16 KB ram */
extern uint8_t cartridge_exram_m2;  /* halt ram */
extern uint8_t cartridge_exram_x2;  /* paging ram */
extern uint8_t cartridge_exram_a8;  /* 2KB mirror */

extern uint8_t* cartridge_buffer;
extern uint32_t cartridge_size;

#ifdef __cplusplus
}
#endif

#endif
