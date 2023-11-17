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
 * Activision.c
 * ----------------------------------------------------------------------------
 */
#include "ProSystem.h"
#include "Mapper.h"
#include "Memory.h"
#include "Activision.h"

static uint8_t max_bank;
static uint32_t prg_size;

static uint8_t *prg_start;
static uint8_t *chr_start;

static uint8_t mapper_bank;


void activision_MapBios(void)
{
   sally_SetRead(0xf000, 0x10000, prg_start + prg_size - 0x3000);
   maria_SetRead(0xf000, 0x10000, chr_start + prg_size - 0x3000);
}

void activision_Map(void)
{
   sally_SetRead(0x4000, 0x6000, prg_start + prg_size - 0x6000);
   maria_SetRead(0x4000, 0x6000, chr_start + prg_size - 0x6000);

   sally_SetRead(0x6000, 0x8000, prg_start + prg_size - 0x8000);
   maria_SetRead(0x6000, 0x8000, chr_start + prg_size - 0x8000);

   sally_SetRead(0x8000, 0xa000, prg_start + prg_size - 0x2000);
   maria_SetRead(0x8000, 0xa000, chr_start + prg_size - 0x2000);

   sally_SetRead(0xa000, 0xe000, prg_start + mapper_bank * 0x4000);
   maria_SetRead(0xa000, 0xe000, chr_start + mapper_bank * 0x4000);

   sally_SetRead(0xe000, 0x10000, prg_start + prg_size - 0x4000);
   maria_SetRead(0xe000, 0x10000, chr_start + prg_size - 0x4000);
}

void activision_Reset(void)
{
   prg_size = cartridge_size;

   prg_start = cartridge_buffer;
   chr_start = cartridge_buffer;

   max_bank = prg_size / 0x4000;

   mapper_bank = 0;

   activision_Map();
}

uint8_t activision_Read(uint16_t address)
{
   return memory_ReadOpenBus(address);
}

void activision_Write(uint16_t address, uint8_t data)
{
   if (address >= 0xff80)
   {
      data = address & 0x7f;

      if (mapper_bank == data)  /* skip remap */
         return;

      if (data < max_bank)
      {
         mapper_bank = data;

         sally_SetRead(0xa000, 0xe000, prg_start + data * 0x4000);
		 maria_SetRead(0xa000, 0xe000, chr_start + data * 0x4000);
	  }
   }
}

void activision_LoadState(void)
{
   mapper_bank = prosystem_ReadState8();
}

void activision_SaveState(void)
{
   prosystem_WriteState8(mapper_bank);
}
