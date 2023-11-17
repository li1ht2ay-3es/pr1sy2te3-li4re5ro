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
 * Linear.c
 * ----------------------------------------------------------------------------
 */
#include "ProSystem.h"
#include "Mapper.h"
#include "Memory.h"

/*
Normal
4K ROM
8K ROM
16K ROM
32K ROM
32K ROM + MirrorRAM@4000
32K ROM + POKEY@4000
48K ROM


Banksets
2x32K ROM
2x32K ROM + RAM@4000 + POKEY@800
2x32K ROM + POKEY@4000
2x48K ROM
2x48K ROM + POKEY@4000(write-only)
2x52K ROM
2x52K ROM + POKEY@4000(write-only)
*/

static uint32_t prg_size;

static uint8_t *prg_start;
static uint8_t *chr_start;

void linear_MapBios(void)
{
   sally_SetRead(0xf000, 0x10000, prg_start + prg_size - 0x1000);
   maria_SetRead(0xf000, 0x10000, chr_start + prg_size - 0x1000);
}

void linear_Map(void)
{
   sally_SetRead(0x10000 - prg_size, 0x10000, prg_start);
   maria_SetRead(0x10000 - prg_size, 0x10000, chr_start);
}

void linear_Reset(void)
{
   prg_size = cartridge_size / (cartridge_bankset ? 2 : 1);

   prg_start = cartridge_buffer;
   chr_start = cartridge_buffer + (cartridge_bankset ? prg_size : 0);

   linear_Map();
}

uint8_t linear_Read(uint16_t address)
{
   return memory_ReadOpenBus(address);
}

void linear_Write(uint16_t address, uint8_t data)
{
}

void linear_LoadState(void)
{
}

void linear_SaveState(void)
{
}
