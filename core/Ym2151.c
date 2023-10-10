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
 * ym2151.c
 * ----------------------------------------------------------------------------
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "libretro.h"
#include "BupChip.h"
#include "ProSystem.h"
#include "Cartridge.h"
#include "Maria.h"
#include "ProSystem.h"
#include "Memory.h"


int16_t ym2151_buffer[MAX_SOUND_SAMPLES * 2] = {0};
int ym2151_outCount;

static int ym2151_lpfCount[2] = {0};
static int ym2151_lpfOld[2] = {0};
static int ym2151_lpfNew[2] = {0};


#define NUKED_OPM  /* slow but kinda works */


#ifdef NUKED_OPM

#include "../nuked/opm.h"
static opm_t opm;  /* ym2151 */

static int32_t ym2151_out[2];

void ym2151_Reset(void)
{
   OPM_Reset(&opm);
}

void ym2151_Frame(void)
{
   ym2151_outCount = 0;
}

uint8_t ym2151_Read(uint16_t address)
{
   if (address >= 0x460 && address < 0x462)
      return OPM_Read(&opm, address);

   return memory_ReadOpenBus(address);
}

void ym2151_Write(uint16_t address, uint8_t data)
{
   if (address >= 0x460 && address < 0x462)
     OPM_Write(&opm, address, data);
}

void ym2151_Run(int cycles)
{
   int index;

   for (index = 0; index < cycles / 2; index++)  /* Maria 1/2 ~ 3.5 MHz */
   {
      OPM_Clock(&opm, ym2151_out, 0, 0, 0);

      ym2151_lpfOld[0] = ym2151_out[0];
      ym2151_lpfOld[1] = ym2151_out[1];
   }
}

void ym2151_Output(void)
{
   static int max = 0;
   int currentValue = 0;


   if (!cartridge_ym2151)
      return;

   max = (max < ym2151_lpfOld[0]) ? ym2151_lpfOld[0] : max;  /* debug */
   max = (max < ym2151_lpfOld[1]) ? ym2151_lpfOld[1] : max;  /* debug */

   ym2151_buffer[ym2151_outCount++] = (int16_t) ym2151_lpfOld[0];
   ym2151_buffer[ym2151_outCount++] = (int16_t) ym2151_lpfOld[1];
}

#endif
