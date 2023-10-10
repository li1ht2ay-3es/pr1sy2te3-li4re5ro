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
 * Souper.c
 * ----------------------------------------------------------------------------
 */
#include "ProSystem.h"
#include "Mapper.h"
#include "Memory.h"
#include "Souper.h"
#include "BupChip.h"


#define SOUPER_BANK_SEL 0x8000
#define SOUPER_CHR_A_SEL 0x8001
#define SOUPER_CHR_B_SEL 0x8002
#define SOUPER_MODE_SEL 0x8003
#define SOUPER_EXRAM_V_SEL 0x8004
#define SOUPER_EXRAM_D_SEL 0x8005
#define SOUPER_AUDIO_CMD 0x8007

#define SOUPER_MODE_MFT 0x1  /* Maria fetch trap */
#define SOUPER_MODE_CHR 0x2  /* Character remap */
#define SOUPER_MODE_EXS 0x4  /* EXRAM paging */


static uint8_t max_bank;
static uint32_t prg_size;

static uint8_t *prg_start;
static uint8_t *chr_start;


void souper_MapBios(void)
{
   sally_SetRead(0xf000, 0x10000, prg_start + prg_size - 0x1000);
   maria_SetRead(0xf000, 0x10000, chr_start + prg_size - 0x1000);
}

void souper_Map(void)
{
   uint32_t index;


   sally_SetRead(0x4000, 0x6000, memory_exram);
   maria_SetRead(0x4000, 0x6000, memory_exram);

   sally_SetWrite(0x4000, 0x6000, memory_exram);


   if (memory_ram[SOUPER_MODE_SEL] & SOUPER_MODE_EXS)  /* Paging exram */
   {
	  sally_SetRead(0x6000, 0x7000, memory_exram + memory_ram[SOUPER_EXRAM_V_SEL] * 0x1000);
      maria_SetRead(0x6000, 0x7000, memory_exram + memory_ram[SOUPER_EXRAM_V_SEL] * 0x1000);

	  sally_SetRead(0x7000, 0x8000, memory_exram + memory_ram[SOUPER_EXRAM_D_SEL] * 0x1000);
      maria_SetRead(0x7000, 0x8000, memory_exram + memory_ram[SOUPER_EXRAM_D_SEL] * 0x1000);

	  sally_SetWrite(0x6000, 0x7000, memory_exram + memory_ram[SOUPER_EXRAM_V_SEL] * 0x1000);
	  sally_SetWrite(0x7000, 0x8000, memory_exram + memory_ram[SOUPER_EXRAM_D_SEL] * 0x1000);
   }

   else
   {
      sally_SetRead(0x6000, 0x8000, memory_exram + 0x2000);
      maria_SetRead(0x6000, 0x8000, memory_exram + 0x2000);

      sally_SetWrite(0x6000, 0x8000, memory_exram + 0x2000);
   }


   /* ############## */


   sally_SetRead(0x8000, 0xb000, prg_start + memory_ram[SOUPER_BANK_SEL] * 0x4000);

   if (memory_ram[SOUPER_MODE_SEL] & SOUPER_MODE_CHR)  /* Chr paging */
   {
      maria_SetRead(0x8000, 0xa000, chr_start + prg_size - 0x4000);

      for (index = 0xa000; index < 0xb000; index += 0x100)
	  {
         maria_SetRead(index + 0x00, index + 0x080, chr_start + (((memory_ram[SOUPER_CHR_A_SEL] & 0xfe) << 11) | ((memory_ram[SOUPER_CHR_A_SEL] & 1) << 7)) + (index & 0xf00));
         maria_SetRead(index + 0x80, index + 0x100, chr_start + (((memory_ram[SOUPER_CHR_B_SEL] & 0xfe) << 11) | ((memory_ram[SOUPER_CHR_B_SEL] & 1) << 7)) + (index & 0xf00));
	  }
   }

   else
      maria_SetRead(0x8000, 0xb000, chr_start + memory_ram[SOUPER_BANK_SEL] * 0x4000);


   sally_SetRead(0xb000, 0xc000, prg_start + memory_ram[SOUPER_BANK_SEL] * 0x4000 + 0x3000);
   maria_SetRead(0xb000, 0xc000, chr_start + memory_ram[SOUPER_BANK_SEL] * 0x4000 + 0x3000);


   /* ############## */


   sally_SetRead(0xc000, 0x10000, prg_start + prg_size - 0x4000);

   if (memory_ram[SOUPER_MODE_SEL] & SOUPER_MODE_MFT)  /* Maria trap */
      memcpy(maria_readmap + 0xc000 / 0x40, maria_readmap + 0x4000 / 0x40, sizeof(uint8_t*) * 0x4000 / 0x40);

   else
      maria_SetRead(0xc000, 0x10000, chr_start + prg_size - 0x4000);
}

void souper_Reset(void)
{
   prg_size = cartridge_size;

   prg_start = cartridge_buffer;
   chr_start = cartridge_buffer;

   max_bank = prg_size / 0x4000;

   memset(memory_ram + 0x8000, 0, 8);
   memory_exram_size = 0x8000;

   souper_Map();
}

uint8_t souper_Read(uint16_t address)
{
   return memory_ReadOpenBus(address);
}

void souper_Write(uint16_t address, uint8_t data)
{
   if (address >= 0x8000 && address < 0x8008)
   {
      switch (address)
	  {
      case SOUPER_AUDIO_CMD:
         bupchip_ProcessAudioCommand(data);
         break;

	  default:
         if (memory_ram[address] == data)  /* skip remap */
            return;

         memory_ram[address] = data;
         souper_Map();
         break;
	  }
   }
}

void souper_LoadState(void)
{
   prosystem_ReadStatePtr(memory_ram + 0x8000, 8);
}

void souper_SaveState(void)
{
   prosystem_WriteStatePtr(memory_ram + 0x8000, 8);
}
