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
 * Mapper.c
 * ----------------------------------------------------------------------------
 */
#include "ProSystem.h"
#include "Cartridge.h"
#include "Mapper.h"
#include "Memory.h"
#include "Pokey.h"
#include "HighScore.h"
#include "Ym2151.h"

#include "Linear.h"
#include "SuperGame.h"
#include "Activision.h"
#include "Absolute.h"
#include "Souper.h"

static void exram_Map(void)
{
   uint32_t addr;

   if (cartridge_exram_m2)  /* halt ram */
   {
      sally_SetRead(0x4000, 0x8000, memory_exram);
      maria_SetRead(0x4000, 0x8000, memory_exram + 0x4000);

      sally_SetWrite(0x4000, 0x8000, memory_exram);
      sally_SetWrite(0xc000, 0x10000, memory_exram + 0x4000);

      memory_exram_size = 0x8000;
   }

   else if (cartridge_exram)  /* normal ram */
   {
      sally_SetRead(0x4000, 0x8000, memory_exram);
      maria_SetRead(0x4000, 0x8000, memory_exram);

      sally_SetWrite(0x4000, 0x8000, memory_exram);

      memory_exram_size = 0x4000;
   }

   else if (cartridge_exram_a8)  /* mirror ram */
   {
      uint8_t *ptr = memory_exram;

      for (addr = 0x4000; addr < 0x8000; addr += 0x100)
      {
         sally_SetRead(addr, addr+0x100, ptr);
         maria_SetRead(addr, addr+0x100, ptr);

         sally_SetWrite(addr, addr+0x100, ptr);

         if (addr % 0x200)
            ptr += 0x100;
      }

      memory_exram_size = 0x2000;
   }
}

void mapper_Reset(void)
{
   switch (cartridge_type)
   {
   case CARTRIDGE_TYPE_LINEAR:
      linear_Reset();
      break;

   case CARTRIDGE_TYPE_SUPERGAME:
      supergame_Reset();
      break;

   case CARTRIDGE_TYPE_ACTIVISION:
      activision_Reset();
      break;

   case CARTRIDGE_TYPE_ABSOLUTE:
      absolute_Reset();
      break;

   case CARTRIDGE_TYPE_SOUPER:
      souper_Reset();
      break;
   }
}

void mapper_MapBios(void)
{
   switch (cartridge_type)
   {
   case CARTRIDGE_TYPE_LINEAR:
      linear_MapBios();
      break;

   case CARTRIDGE_TYPE_SUPERGAME:
      supergame_MapBios();
      break;

   case CARTRIDGE_TYPE_ACTIVISION:
      activision_MapBios();
      break;

   case CARTRIDGE_TYPE_ABSOLUTE:
      absolute_MapBios();
      break;

   case CARTRIDGE_TYPE_SOUPER:
      souper_MapBios();
      break;
   }
}

void mapper_Map(void)
{
   highscore_Map();  /* low priority */
   exram_Map();


   switch (cartridge_type)  /* rom gets high priority */
   {
   case CARTRIDGE_TYPE_LINEAR:
      linear_Map();
      break;

   case CARTRIDGE_TYPE_SUPERGAME:
      supergame_Map();
      break;

   case CARTRIDGE_TYPE_ACTIVISION:
      activision_Map();
      break;

   case CARTRIDGE_TYPE_ABSOLUTE:
      absolute_Map();
      break;

   case CARTRIDGE_TYPE_SOUPER:
      souper_Map();
      break;
   }
}

uint8_t mapper_Read(uint16_t address)
{
   if (address >= 0x440 && address < 0x450)
   {
      if (cartridge_pokey == POKEY_AT_440)  /* pokey 2 */
         return pokey_Read(address);
   }

   if (address >= 0x450 && address < 0x460)
   {
      if (cartridge_pokey == POKEY_AT_450)  /* pokey 1 */
         return pokey_Read(address);
   }

   if (address >= 0x460 && address < 0x470)
   {
      return ym2151_Read(address);
   }

   if (address >= 0x470 && address < 0x480)
   {
      return 0xff;  /* xm */
   }

   if (address >= 0x800 && address < 0x1000)
   {
      if (cartridge_pokey == POKEY_AT_800)  /* pokey 1 */
         return pokey_Read(address);
   }

   if (address >= 0x4000 && address < 0x8000)
   {
      if (cartridge_pokey == POKEY_AT_4000)  /* pokey 1 */
         return pokey_Read(address);
   }

   return memory_ReadOpenBus(address);
}

void mapper_Write(uint16_t address, uint8_t data)
{
   if (address >= 0x8000)
   {
      switch (cartridge_type)
      {
      case CARTRIDGE_TYPE_LINEAR:
         linear_Write(address, data);
         break;

      case CARTRIDGE_TYPE_SUPERGAME:
         supergame_Write(address, data);
         break;

      case CARTRIDGE_TYPE_ACTIVISION:
         activision_Write(address, data);
         break;

      case CARTRIDGE_TYPE_ABSOLUTE:
         absolute_Write(address, data);
         break;

      case CARTRIDGE_TYPE_SOUPER:
         souper_Write(address, data);
         break;
      }
   }

   else if (address >= 0x440 && address < 0x450)
   {
      if (cartridge_pokey == POKEY_AT_440)  /* pokey 2 */
         pokey_Write(address, data);
   }

   else if (address >= 0x450 && address < 0x460)
   {
      if (cartridge_pokey == POKEY_AT_450)  /* pokey 1 */
         pokey_Write(address, data);
   }

   else if (address >= 0x460 && address < 0x470)
   {
      //cartridge_ym2151 = 1;
      ym2151_Write(address, data);
   }

   else if (address >= 0x470 && address < 0x480)
   {
      if (data & 0x08)
         highscore_Map();

      return;
   }

   else if (address >= 0x800 && address < 0x1000)
   {
      if (cartridge_pokey == POKEY_AT_800)  /* pokey 1 */
         pokey_Write(address, data);
   }

   else if (address >= 0x4000 && address < 0x8000)
   {
      if (cartridge_pokey == POKEY_AT_4000)  /* pokey 1 */
         pokey_Write(address, data);
   }
}

void mapper_LoadState(void)
{
   switch (cartridge_type)
   {
   case CARTRIDGE_TYPE_LINEAR:
      linear_LoadState();
      break;

   case CARTRIDGE_TYPE_SUPERGAME:
      supergame_LoadState();
      break;

   case CARTRIDGE_TYPE_ACTIVISION:
      activision_LoadState();
      break;

   case CARTRIDGE_TYPE_ABSOLUTE:
      absolute_LoadState();
      break;

   case CARTRIDGE_TYPE_SOUPER:
      souper_LoadState();
      break;
   }

   mapper_Map();
}

void mapper_SaveState(void)
{
   switch (cartridge_type)
   {
   case CARTRIDGE_TYPE_LINEAR:
      linear_SaveState();
      break;

   case CARTRIDGE_TYPE_SUPERGAME:
      supergame_SaveState();
      break;

   case CARTRIDGE_TYPE_ACTIVISION:
      activision_SaveState();
      break;

   case CARTRIDGE_TYPE_ABSOLUTE:
      absolute_SaveState();
      break;

   case CARTRIDGE_TYPE_SOUPER:
      souper_SaveState();
      break;
   }
}
