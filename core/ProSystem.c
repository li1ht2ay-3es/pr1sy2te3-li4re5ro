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
 * This library is free software; you can redistribute it and/or modify it   
 * under the terms of version 2 of the GNU Library General Public License    
 * as published by the Free Software Foundation.                             
 *                                                                           
 * This library is distributed in the hope that it will be useful, but       
 * WITHOUT ANY WARRANTY; without even the implied warranty of                
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library 
 * General Public License for more details.                                  
 * To obtain a copy of the GNU Library General Public License, write to the  
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.   
 *                                                                           
 * Any permitted reproduction of these routines, in whole or in part, must   
 * bear this legend.                                                         
 * ----------------------------------------------------------------------------
 * ProSystem.c
 * ----------------------------------------------------------------------------
 */
#include "ProSystem.h"
#include "Mixer.h"
#include "Bios.h"
#include "Cartridge.h"
#include "Maria.h"
#include "Memory.h"
#include "Region.h"
#include "Riot.h"
#include "Sally.h"
#include "Tia.h"
#include "Pokey.h"
#include "BupChip.h"

#define PRO_SYSTEM_STATE_HEADER "PRO-SYSTEM STATE"

int prosystem_frequency = 60;
int prosystem_scanlines = 263;
int prosystem_cycles = 0;

uint8_t *prosystem_statePtr;

static void prosystem_Map()
{
   cartridge_Map();

   if (bios_enabled)
      bios_Map();
}

void prosystem_Reset()
{
   if (!cartridge_IsLoaded())
      return;

   region_Reset();

   sally_Reset();
   maria_Reset();
   memory_Reset();
   riot_Reset();

   mixer_Reset();
   tia_Reset();
   pokey_Reset();
   bupchip_Reset();
   /* cartridge_LoadHighScoreCart(); */

   cartridge_Reset();
   prosystem_Map();

   prosystem_cycles = 0;
}

void prosystem_SetRate(int rate)
{
   mixer_rate = rate;

   mixer_SetRate();
   tia_SetRate();
   bupchip_SetRate();
}

static void prosystem_FireLightGun()
{
/*
   if(((maria_scanline >= lightgun_scanline) && 
       (maria_scanline <= (lightgun_scanline + 3))) && 
       (prosystem_cycles >= ((int)lightgun_cycle ) - 1))
   {
      memory_ram[INPT4] &= 0x7f;                        
   } 
   else 
   {
      memory_ram[INPT4] |= 0x80;                        
   }
*/
}

void prosystem_Run(int cycles)
{
   prosystem_cycles += cycles;

   riot_Run(cycles);
   cartridge_Run(cycles);
   mixer_Run(cycles);

   tia_Run();
}

void prosystem_ExecuteFrame(const uint8_t* input)
{
   int cycles_total = 0;

   mixer_Frame();
   tia_Frame();
   pokey_Frame();
   bupchip_Frame();


   maria_scanline = maria_displayArea.bottom+1;  /* vblank start */
   memory_ram[MSTAT] = 0x80;
   riot_SetInput(input);


   while (1)
   {
      int cycles;
      cycles_total -= prosystem_cycles;  /* debug */


      /* 0 = hsync */
      {
         maria_Scanline();
         pokey_Scanline();
      }


      /* 0-27 = pre-dma */
      while (prosystem_cycles < 28)
      {
         cycles = sally_Run();
         cycles += (!cycles) ? 28 - prosystem_cycles : 0;  /* wsync */
         cycles += sally_SlowCycles();  /* TIA + RIOT slow access */

		 prosystem_Run(cycles);
      }


      /* 28-x = maria render */
      {
         cycles = maria_Run();
         //if (cycles > CYCLES_PER_SCANLINE) cycles = CYCLES_PER_SCANLINE;

         prosystem_Run(cycles);
      }


      /* x-453 = sally scanline */
      while (prosystem_cycles < CYCLES_PER_SCANLINE)
      {
         cycles = sally_Run();
         cycles += (!cycles) ? CYCLES_PER_SCANLINE - prosystem_cycles : 0;  /* wsync */
         cycles += sally_SlowCycles();  /* TIA + RIOT slow access */

		 prosystem_Run(cycles);
      }


      tia_ScanlineEnd();
      cartridge_ScanlineEnd();


	  cycles_total += prosystem_cycles;  /* debug */
      prosystem_cycles -= CYCLES_PER_SCANLINE;  /* overflow */

      maria_scanline = (maria_scanline + 1) % prosystem_scanlines;
      if (maria_scanline == maria_displayArea.bottom+1)
         break;
   }


   mixer_FrameEnd();
}

void prosystem_Close(bool persistent_data)
{
   bupchip_Release();
   cartridge_Release(persistent_data);
   maria_Reset();
   memory_Reset();
   tia_Reset();
}

bool prosystem_LoadState(const uint8_t *buffer, bool fast_saves)
{
   prosystem_statePtr = buffer;

   if (memcmp(prosystem_statePtr, PRO_SYSTEM_STATE_HEADER, sizeof(PRO_SYSTEM_STATE_HEADER)-1) != 0)
      return false;
   prosystem_statePtr += sizeof(PRO_SYSTEM_STATE_HEADER)-1;

   cartridge_LoadState();
   maria_LoadState();
   memory_LoadState();
   riot_LoadState();
   sally_LoadState();
   tia_LoadState();

   return true;
}

int prosystem_SaveState(uint8_t *buffer, bool fast_saves)
{
   prosystem_statePtr = buffer;

   memcpy(prosystem_statePtr, PRO_SYSTEM_STATE_HEADER, sizeof(PRO_SYSTEM_STATE_HEADER)-1);
   prosystem_statePtr += sizeof(PRO_SYSTEM_STATE_HEADER)-1;

   cartridge_SaveState();
   maria_SaveState();
   memory_SaveState();
   riot_SaveState();
   sally_SaveState();
   tia_SaveState();

   return prosystem_statePtr - buffer;
}
