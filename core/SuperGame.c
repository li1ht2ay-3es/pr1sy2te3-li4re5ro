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
 * SuperGame.c
 * ----------------------------------------------------------------------------
 */
#include "ProSystem.h"
#include "Mapper.h"
#include "Memory.h"
#include "SuperGame.h"

/*
Normal
32K ROM
48K ROM
128K ROM
128K ROM + 8K RAM@4000
128K ROM + 16K RAM@4000
128K ROM + POKEY@4000
144K ROM


Banksets
2x32K ROM
2x32K ROM + RAM@4000 + POKEY@800
2x32K ROM + POKEY@4000
2x48K ROM
2x48K ROM + POKEY@4000(write-only)
2x52K ROM
2x52K ROM + POKEY@4000(write-only)
2x128K ROM + BANK6@4000
2x128K ROM + RAM@4000
2x128K ROM + RAM@4000 + POKEY@800
2x128K ROM + POKEY@4000
*/

static uint8_t max_bank;
static uint32_t prg_size;

static uint8_t *prg_start, *prg0_start;
static uint8_t *chr_start, *chr0_start;

static uint8_t mapper_bank;


void supergame_MapBios(void)
{
   sally_SetRead(0xf000, 0x10000, prg_start + prg_size - 0x1000);
   maria_SetRead(0xf000, 0x10000, chr_start + prg_size - 0x1000);
}

void supergame_Map(void)
{
   if (cartridge_exrom)  /* extra bank 0 */
   {
      sally_SetRead(0x4000, 0x8000, prg0_start);
      maria_SetRead(0x4000, 0x8000, chr0_start);
   }

   else if (cartridge_exfix)  /* 2nd last bank */
   {
      sally_SetRead(0x4000, 0x8000, prg_start + prg_size - 0x8000);
      maria_SetRead(0x4000, 0x8000, chr_start + prg_size - 0x8000);
   }


   sally_SetRead(0x8000, 0xc000, prg_start + mapper_bank * 0x4000);
   maria_SetRead(0x8000, 0xc000, chr_start + mapper_bank * 0x4000);

   sally_SetRead(0xc000, 0x10000, prg_start + prg_size - 0x4000);
   maria_SetRead(0xc000, 0x10000, chr_start + prg_size - 0x4000);
}

void supergame_Reset(void)
{
   prg_size = cartridge_size / (cartridge_bankset ? 2 : 1);

   prg_start = cartridge_buffer;
   chr_start = cartridge_buffer + (cartridge_bankset ? prg_size : 0);

   if (cartridge_exrom)
   {
      prg0_start = prg_start;  /* first bank */
      chr0_start = chr_start;

      prg_start += 0x4000;  /* banks 1-x = paging */
      chr_start += 0x4000;
      prg_size -= 0x4000;
   }

   max_bank = prg_size / 0x4000;

   mapper_bank = 0;

   supergame_Map();
}

uint8_t supergame_Read(uint16_t address)
{
   return memory_ReadOpenBus(address);
}

void supergame_Write(uint16_t address, uint8_t data)
{
   if (address >= 0x8000 && address < 0xc000)
   {
      if (max_bank < 8 && data >= max_bank)  /* high mirror */
         data -= max_bank;

      if (mapper_bank == data)  /* skip remap */
         return;

      if (data < max_bank)
      {
         mapper_bank = data;

         sally_SetRead(0x8000, 0xc000, prg_start + data * 0x4000);
         maria_SetRead(0x8000, 0xc000, chr_start + data * 0x4000);
	  }
   }
}

void supergame_LoadState(void)
{
   mapper_bank = prosystem_ReadState8();
}

void supergame_SaveState(void)
{
   prosystem_WriteState8(mapper_bank);
}
