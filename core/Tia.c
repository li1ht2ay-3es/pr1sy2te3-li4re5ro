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
 * TiaSound is Copyright(c) 1997 by Ron Fries                                
 *                                                                           
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
 * Tia.c
 * ----------------------------------------------------------------------------
 */
#include "ProSystem.h"
#include "Tia.h"
#include "Maria.h"
#include "Mixer.h"
#include "Bios.h"
#include "Cartridge.h"
#include "Memory.h"
#include "LightGun.h"

#define TIA_POLY4_SIZE 15
#define TIA_POLY5_SIZE 31
#define TIA_POLY9_SIZE 511

static const uint8_t TIA_POLY4[ ] = {1,1,0,1,1,1,0,0,0,0,1,0,1,0,0};
static const uint8_t TIA_POLY5[ ] = {0,0,1,0,1,1,0,0,1,1,1,1,1,0,0,0,1,1,0,1,1,1,0,1,0,1,0,0,0,0,1};
static const uint8_t TIA_POLY9[ ] = {0,0,1,0,1,0,0,0,1,0,0,0,0,0,0,0,1,0,1,1,1,0,0,1,0,1,0,0,1,1,1,1,1,0,0,1,1,0,1,1,0,1,0,1,1,1,0,1,1,0,0,1,0,0,1,1,1,1,0,1,0,0,0,0,1,1,0,1,1,0,0,0,1,0,0,0,1,1,1,1,0,1,0,1,1,0,1,0,1,0,0,0,0,1,1,0,1,0,1,0,0,0,1,0,1,0,0,0,1,1,1,0,0,1,1,0,1,1,0,0,1,1,1,1,1,0,0,1,1,0,0,0,1,1,0,1,0,0,0,1,1,0,0,1,1,1,1,0,0,1,0,0,0,1,1,1,0,0,1,1,0,1,0,1,1,0,1,1,0,1,0,0,1,0,0,1,1,1,1,1,1,0,1,1,1,1,0,1,1,0,0,0,0,1,1,1,1,1,0,0,0,1,0,0,0,0,1,0,0,0,1,0,1,0,1,1,0,0,0,0,1,0,1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,1,0,1,1,1,0,1,0,0,0,0,0,0,0,0,1,0,1,0,0,1,0,0,0,0,1,1,1,0,0,0,1,1,1,0,0,1,1,0,0,1,0,0,1,0,1,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,1,1,1,1,0,0,0,1,1,1,0,0,0,1,0,0,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,1,1,0,1,0,1,1,1,1,0,0,1,0,1,0,1,1,1,0,0,0,0,0,1,1,0,1,1,0,0,0,1,0,1,0,1,0,0,0,0,1,0,1,1,1,0,0,0,0,1,0,0,1,0,1,0,0,0,1,0,1,1,1,0,0,1,1,1,1,1,1,1,0,0,0,0,0,1,0,0,1,1,0,1,0,0,1,0,0,0,1,0,0,1,0,1,0,0,0,1,1,0,1,0,0,0,0,0,1,1,1,1,0,0,1,0,0,1,0,1,1,1,1,1,1,1,0,1,0,0,1,0,0,0,1,1,0,1,1,1,0,0,0,1,0,1,0,0,1,0,1,0,1,0,1,1,1,0,0,1,0,1,1,0,0,1,1,1,1,1,0,0,0,1,1,0};
static const uint8_t TIA_DIV31[ ] = {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0};

#define tia_audc (memory_ram + AUDC0)
#define tia_audf (memory_ram + AUDF0)
#define tia_audv (memory_ram + AUDV0)

uint8_t tia_volume[2];
uint8_t tia_counter[2];
uint8_t tia_counterMax[2];

uint8_t tia_poly4Cntr[2];
uint8_t tia_poly5Cntr[2];
uint16_t tia_poly9Cntr[2];

static const int TIA_CYCLES = CYCLES_PER_SCANLINE / 2;  /* Maria x 1/227 ~ 1/2 scanline */

static int out_tick;
static int out_tick2;
static int out_rate;
static int out_clock;
static int out_clock2;

int16_t tia_buffer[MAX_SOUND_SAMPLES];
int tia_outCount;

static int tia_lpfCount[2];
static int tia_lpfOld[2];
static int tia_lpfNew[2];
int tia_lowpass = 2;


void tia_SetLowpass(int limit)
{
/*
1 = 31 KHz, off
2 = 15 KHz
3 = 10 KHz
4 = 7 KHz
*/

   tia_lowpass = limit;
}

static void tia_ProcessChannel(uint8_t channel)
{
   tia_poly5Cntr[channel] = (tia_poly5Cntr[channel] + 1) % TIA_POLY5_SIZE;

   if( ((tia_audc[channel] & 2) == 0) ||
       ( ((tia_audc[channel] & 1) == 0) && TIA_DIV31[tia_poly5Cntr[channel]] ) ||
       ( ((tia_audc[channel] & 1) == 1) && TIA_POLY5[tia_poly5Cntr[channel]] )
     )
   {
      if (tia_audc[channel] & 4)
         tia_volume[channel] = (!tia_volume[channel]) ? tia_audv[channel]: 0;

      else if (tia_audc[channel] & 8)
      {
         if (tia_audc[channel] == 8)
         {
            tia_poly9Cntr[channel] = (tia_poly9Cntr[channel]+1) % TIA_POLY9_SIZE;
            tia_volume[channel] = (TIA_POLY9[tia_poly9Cntr[channel]]) ? tia_audv[channel]: 0;
         }

         else
            tia_volume[channel] = (TIA_POLY5[tia_poly5Cntr[channel]]) ? tia_audv[channel]: 0;
      }

      else
      {
         tia_poly4Cntr[channel] = (tia_poly4Cntr[channel] + 1) % TIA_POLY4_SIZE;
         tia_volume[channel] = (TIA_POLY4[tia_poly4Cntr[channel]]) ? tia_audv[channel]: 0;
      }
   }
}

void tia_Frame(void)
{
   tia_outCount = 0;
}

void tia_Reset(void)
{
   uint32_t index;

   memset(memory_ram, 0, 0x20);

   for (index = 0; index < 2; index++)
   {
      tia_volume[index] = 0;
      tia_counter[index] = 0;
      tia_counterMax[index] = 0;

      tia_poly4Cntr[index] = 0;
      tia_poly5Cntr[index] = 0;
      tia_poly9Cntr[index] = 0;
   }

   out_clock = 0;
   out_clock2 = out_rate;
}

static void update_channel(int channel)
{
   uint8_t frequency;

   if (tia_audc[channel] == 0)  /* can update faster than 31400 counter (several times per frame) */
   {
      frequency = 0;

      tia_volume[channel] = tia_audv[channel];
   }

   else
   {
      frequency = tia_audf[channel] + 1;

      if(tia_audc[channel] > 11)
         frequency *= 3;
   }


   if (frequency != tia_counterMax[channel])
   {
      tia_counterMax[channel] = frequency;

      if (tia_counter[channel] == 0 || frequency == 0)
         tia_counter[channel] = frequency;
   }
}

uint8_t tia_Read(uint16_t address)
{
   switch(address)
   {
   case INPT0:
   case INPT1:
   case INPT2:
   case INPT3:
   case INPT5:
      return memory_ram[address];

   case INPT4:
      if (lightgun_enabled)
         return lightgun_Strobe();

      return memory_ram[INPT4];
   }

   return memory_ReadOpenBus(address);
}

void tia_Write(uint16_t address, uint8_t data)
{
   uint8_t channel = (address + 1) & 1;

   switch (address)
   {
   case INPTCTRL:
      if (bios_enabled)
	  {
         if (data == 22 && bios_IsMapped())
            cartridge_MapBios();

         else if (data == 2 && !bios_IsMapped())
            bios_Map();
	  }
      break;

   case AUDC0:
   case AUDC1:
      tia_audc[channel] = data & 0x0F;
      update_channel(channel);
      break;

   case AUDF0:
   case AUDF1:
      tia_audf[channel] = data & 0x1F;
      update_channel(channel);
      break;

   case AUDV0:
   case AUDV1:
      tia_audv[channel] = data & 0x0F;
      update_channel(channel);
      break;

   default:
      memory_WriteOpenBus(address, data);
      break;
   }
}

static void tia_Tick()
{
   int index;
   int outvol = 0;


   for (index = 0; index < 2; index++)
   {
      if (tia_counter[index] > 1)
         tia_counter[index]--;

      else if (tia_counter[index] == 1)
      {
         tia_counter[index] = tia_counterMax[index];
         tia_ProcessChannel(index);
      }


      tia_lpfCount[index] = (tia_lpfNew[index] == tia_volume[index]) ? tia_lpfCount[index] + 1 : 1;  /* frequency change */
      tia_lpfNew[index] = tia_volume[index];
      tia_lpfOld[index] = (tia_lpfCount[index] >= tia_lowpass) ? tia_lpfNew[index] : tia_lpfOld[index];  /* latch new value */

      outvol += tia_lpfOld[index];  /* 4-bit unsigned */
   }


   /* outvol *= 0x400;  /* 10-bit expansion */
   outvol *= 0x300;  /* 9-bit expansion */


   index = out_tick;

   out_clock2 -= out_tick2;
   if (out_clock2 <= 0)
   {
      out_clock2 += out_rate;
      index++;
   }

   while (index > 0)
   {
      tia_buffer[tia_outCount] = outvol;
      tia_outCount++;

      index--;
   }
}

void tia_Run()
{
   if (out_clock)  /* past cycle 227 */
      return;

   if (prosystem_cycles < TIA_CYCLES)  /* wait mid-scanline */
      return;

   tia_Tick();

   out_clock++;
}

void tia_ScanlineEnd()
{
   tia_Tick();  /* cycle 454 */

   out_clock--;
}

void tia_SetRate()
{
   int clock = 2 * prosystem_scanlines * prosystem_frequency;  /* 2x per scanline */

   out_rate = clock;

   out_tick = mixer_rate / out_rate;
   out_tick2 = mixer_rate % out_rate;

   out_clock = 0;
   out_clock2 = out_rate;  /* always zero at end-of-frame */
}

void tia_LoadState(void)
{
   uint32_t index;

   prosystem_ReadStatePtr(memory_ram, 0x20);

   for (index = 0; index < 2; index++)
   {
      tia_volume[index] = prosystem_ReadState8();
      tia_counter[index] = prosystem_ReadState8();
      tia_counterMax[index] = prosystem_ReadState8();

      tia_poly4Cntr[index] = prosystem_ReadState8();
      tia_poly5Cntr[index] = prosystem_ReadState8();
      tia_poly9Cntr[index] = prosystem_ReadState16();
   }
}

void tia_SaveState(void)
{
   uint32_t index;

   prosystem_WriteStatePtr(memory_ram, 0x20);

   for (index = 0; index < 2; index++)
   {
      prosystem_WriteState8(tia_volume[index]);
      prosystem_WriteState8(tia_counter[index]);
      prosystem_WriteState8(tia_counterMax[index]);

      prosystem_WriteState8(tia_poly4Cntr[index]);
      prosystem_WriteState8(tia_poly5Cntr[index]);
      prosystem_WriteState16(tia_poly9Cntr[index]);
   }
}
