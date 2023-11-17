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
 * Absolute.c
 * ----------------------------------------------------------------------------
 */
#include "ProSystem.h"
#include "Mapper.h"
#include "Memory.h"
#include "Absolute.h"

static uint8_t max_bank;
static uint32_t prg_size;

static uint8_t *prg_start;
static uint8_t *chr_start;

static uint8_t mapper_bank;


void absolute_MapBios(void)
{
   sally_SetRead(0xf000, 0x10000, prg_start + prg_size - 0x1000);
   maria_SetRead(0xf000, 0x10000, chr_start + prg_size - 0x1000);
}

void absolute_Map(void)
{
   sally_SetRead(0x4000, 0x8000, prg_start + (mapper_bank-1) * 0x4000);
   maria_SetRead(0x4000, 0x8000, chr_start + (mapper_bank-1) * 0x4000);

   sally_SetRead(0x8000, 0x10000, prg_start + prg_size - 0x8000);
   maria_SetRead(0x8000, 0x10000, chr_start + prg_size - 0x8000);
}

void absolute_Reset(void)
{
   prg_size = cartridge_size;

   prg_start = cartridge_buffer;
   chr_start = cartridge_buffer;

   max_bank = prg_size / 0x4000;

   mapper_bank = 1;

   absolute_Map();
}

uint8_t absolute_Read(uint16_t address)
{
   return memory_ReadOpenBus(address);
}

void absolute_Write(uint16_t address, uint8_t data)
{
   if (address == 0x8000)
   {
      if (mapper_bank == data)  /* skip remap */
         return;

      if (data > 0 && data <= max_bank-2)
      {
         mapper_bank = data;

         sally_SetRead(0x4000, 0x8000, prg_start + (data-1) * 0x4000);
         maria_SetRead(0x4000, 0x8000, chr_start + (data-1) * 0x4000);
	  }
   }
}

void absolute_LoadState(void)
{
   mapper_bank = prosystem_ReadState8();
}

void absolute_SaveState(void)
{
   prosystem_WriteState8(mapper_bank);
}
