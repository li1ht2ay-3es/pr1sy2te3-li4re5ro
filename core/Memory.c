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
 * Memory.c
 * ----------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "Memory.h"
#include "Equates.h"
#include "Bios.h"
#include "Cartridge.h"
#include "Tia.h"
#include "Riot.h"
#include "Pokey.h"

uint8_t memory_ram[MEMORY_SIZE] = {0};
uint8_t memory_rom[MEMORY_SIZE] = {0};

uint8_t memory_souper_ram[MEMORY_SOUPER_EXRAM_SIZE] = {0};

void memory_Reset(void)
{
   uint32_t index;

   for(index = 0; index < MEMORY_SIZE; index++)
   {
      memory_ram[index] = 0;
      memory_rom[index] = 1;
   }

   for(index = 0; index < 0x4000; index++)
      memory_rom[index] = 0;
}

uint16_t memory_souper_GetRamAddress(uint16_t address)
{
   uint8_t page = (address - 0x4000) >> 12;

   if((cartridge_souper_mode & CARTRIDGE_SOUPER_MODE_EXS) != 0)
   {
      if(address >= 0x6000 && address < 0x7000)
         page = cartridge_souper_ram_page_bank[0];

      else if(address >= 0x7000 && address < 0x8000)
         page = cartridge_souper_ram_page_bank[1];
  }

  return (address & 0x0fff) | ((uint16_t)page << 12);
}

uint8_t memory_Read(uint16_t address)
{
   uint8_t tmp_data;

#if 0
   if (cartridge_xm)
   {
      if ((address >= 0x0470 && address < 0x0480) ||
          (xm_pokey_enabled && (address >= 0x0450 && address < 0x0470)) ||
          (xm_mem_enabled && (address >= 0x4000 && address < 0x8000)))
      {
         return xm_Read(address);
      } 
   }
#endif

   if (cartridge_pokey)
   {
      if (cartridge_pokey == POKEY_AT_4000)
      {
         if ((address & 0xFFF0) == 0x4000)
            return pokey_GetRegister(address);
      }
      else
      {
         // Not quite accurate as it will catch anything from 0x440 to 0x4C0 but that's 
         // good enough as nothing else should be mapped in this region except POKEY.
         if ((address & 0xFFC0) == 0x440)
            return pokey_GetRegister(address);

         if ((address & 0xFFF0) == 0x800)
            return pokey_GetRegister(address);
      }
   }


   if ((address >= 0x20 && address <= 0x3F) && (address != MSTAT)) return 0;  /* Maria = write-only */


   switch (address)
   {
      case INTIM:
      case INTIM | 0x2:
         memory_ram[INTFLG] &= 0x7f;
         return memory_ram[INTIM];

      case INTFLG:
      case INTFLG | 0x2:
         tmp_data = memory_ram[INTFLG];
         memory_ram[INTFLG] &= 0x7f;
         return tmp_data;

      default:
         if(cartridge_type == CARTRIDGE_TYPE_SOUPER && address >= 0x4000 && address < 0x8000)
            return memory_souper_ram[memory_souper_GetRamAddress(address)];

         break;
   }

   return memory_ram[address];
}

void memory_Write(uint16_t address, uint8_t data)
{
   if (cartridge_pokey)
   {
      if (cartridge_pokey == POKEY_AT_4000)
      {
         if ((address & 0xFFF0) == 0x4000)
         {
            pokey_SetRegister(address, data);
            return;
         }
      }

      else
      {
         // Not quite accurate as it will catch anything from 0x440 to 0x4C0 but that's 
         // good enough as nothing else should be mapped in this region except POKEY.
         if ((address & 0xFFC0) == 0x440)
         {
            pokey_SetRegister(address, data);
            return;
         }

         if ((address & 0xFFF0) == 0x800)  /* Pokey @ 800 */
         {
            pokey_SetRegister(address, data);
            return;
         }
      }
   }


   if (!memory_rom[address])
   {
      switch(address)
      {
         case INPTCTRL:
            if (data == 22 && cartridge_IsLoaded()) 
               cartridge_Store();

				else if (data == 2 && bios_enabled)
               bios_Store();
            break;

         /* read-only */
         case INPT0:
         case INPT1:
         case INPT2:
         case INPT3:
         case INPT4:
         case INPT5:
			case MSTAT:
            break;

         case AUDC0:  /* TIA audio */
         case AUDC1:
         case AUDF0:
         case AUDF1:
         case AUDV0:
         case AUDV1:
            tia_Write(address, data);
            break;

         case WSYNC:
            memory_ram[WSYNC] = 1;
            break;

/* gdement: Writing here actually writes to DRA inside the RIOT chip.
            This value only indirectly affects output of SWCHA.  Ditto for SWCHB. */
			case SWCHA:	riot_SetDRA(data); break;
         case SWCHB: riot_SetDRB(data); break;

         case TIM1T:
         case TIM8T:
         case TIM64T:
         case T1024T:
         case TIM1T | 0x8:
         case TIM8T | 0x8:
         case TIM64T | 0x8:
         case T1024T | 0x8:
            riot_SetTimer(address, data); break;

         default:
            if(cartridge_type == CARTRIDGE_TYPE_SOUPER && address >= 0x4000 && address < 0x8000)
            {
               memory_souper_ram[memory_souper_GetRamAddress(address)] = data;
               break;
            }

            memory_ram[address] = data;

            if (address >= 0x2040 && address <= 0x20FF)  /* Zero page shadow */
               memory_ram[address - 0x2000] = data;

            else if (address >= 0x2140 && address <= 0x21FE)  /* Stack page shadow */
               memory_ram[address - 0x2000] = data;

            else if (address >= 0x40 && address <= 0xFF)  /* RAM block 0 */
               memory_ram[address + 0x2000] = data;

            else if (address >= 0x140 && address <= 0x1FF)  /* RAM block 1 */
               memory_ram[address + 0x2000] = data;

            break;
            /*TODO: gdement:  test here for debug port.  Don't put it in the switch because that will change behavior.*/
      }
   }

   else
      cartridge_Write(address, data);
}

void memory_WriteROM(uint16_t address, uint16_t size, const uint8_t* data)
{
   uint32_t index;

   if((address + size) <= MEMORY_SIZE && data != NULL)
   {
      for(index = 0; index < size; index++)
      {
         memory_ram[address + index] = data[index];
         memory_rom[address + index] = 1;
      }
   }
}

void memory_ClearROM(uint16_t address, uint16_t size)
{
   uint32_t index;

   if((address + size) <= MEMORY_SIZE)
   {
      for(index = 0; index < size; index++)
      {
         memory_ram[address + index] = 0;
         memory_rom[address + index] = 0;
      }
   }
}
