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
#include "Tia.h"
#include "Maria.h"
#include "ProSystem.h"
#include "Mixer.h"

#define TIA_POLY4_SIZE 15
#define TIA_POLY5_SIZE 31
#define TIA_POLY9_SIZE 511

static const uint8_t TIA_POLY4[ ] = {1,1,0,1,1,1,0,0,0,0,1,0,1,0,0};
static const uint8_t TIA_POLY5[ ] = {0,0,1,0,1,1,0,0,1,1,1,1,1,0,0,0,1,1,0,1,1,1,0,1,0,1,0,0,0,0,1};
static const uint8_t TIA_POLY9[ ] = {0,0,1,0,1,0,0,0,1,0,0,0,0,0,0,0,1,0,1,1,1,0,0,1,0,1,0,0,1,1,1,1,1,0,0,1,1,0,1,1,0,1,0,1,1,1,0,1,1,0,0,1,0,0,1,1,1,1,0,1,0,0,0,0,1,1,0,1,1,0,0,0,1,0,0,0,1,1,1,1,0,1,0,1,1,0,1,0,1,0,0,0,0,1,1,0,1,0,1,0,0,0,1,0,1,0,0,0,1,1,1,0,0,1,1,0,1,1,0,0,1,1,1,1,1,0,0,1,1,0,0,0,1,1,0,1,0,0,0,1,1,0,0,1,1,1,1,0,0,1,0,0,0,1,1,1,0,0,1,1,0,1,0,1,1,0,1,1,0,1,0,0,1,0,0,1,1,1,1,1,1,0,1,1,1,1,0,1,1,0,0,0,0,1,1,1,1,1,0,0,0,1,0,0,0,0,1,0,0,0,1,0,1,0,1,1,0,0,0,0,1,0,1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,1,0,1,1,1,0,1,0,0,0,0,0,0,0,0,1,0,1,0,0,1,0,0,0,0,1,1,1,0,0,0,1,1,1,0,0,1,1,0,0,1,0,0,1,0,1,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,1,1,1,1,0,0,0,1,1,1,0,0,0,1,0,0,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,1,1,0,1,0,1,1,1,1,0,0,1,0,1,0,1,1,1,0,0,0,0,0,1,1,0,1,1,0,0,0,1,0,1,0,1,0,0,0,0,1,0,1,1,1,0,0,0,0,1,0,0,1,0,1,0,0,0,1,0,1,1,1,0,0,1,1,1,1,1,1,1,0,0,0,0,0,1,0,0,1,1,0,1,0,0,1,0,0,0,1,0,0,1,0,1,0,0,0,1,1,0,1,0,0,0,0,0,1,1,1,1,0,0,1,0,0,1,0,1,1,1,1,1,1,1,0,1,0,0,1,0,0,0,1,1,0,1,1,1,0,0,0,1,0,1,0,0,1,0,1,0,1,0,1,1,1,0,0,1,0,1,1,0,0,1,1,1,1,1,0,0,0,1,1,0};
static const uint8_t TIA_DIV31[ ] = {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0};

uint8_t tia_volume[2] = {0};
uint8_t tia_counterMax[2] = {0};
uint8_t tia_counter[2] = {0};
uint8_t tia_audc[2] = {0};
uint8_t tia_audf[2] = {0};
uint8_t tia_audv[2] = {0};
int tia_poly4Cntr[2] = {0};
int tia_poly5Cntr[2] = {0};
int tia_poly9Cntr[2] = {0};

static int tia_count;
static int tia_cycles;
static int16_t tia_buffer[MAX_SOUND_SAMPLES];


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
   tia_count = 0;
}

void tia_Reset(void)
{
   int index;

   for (index = 0; index < 2; index++)
   {
      tia_volume[index] = 0;
      tia_counterMax[index] = 0;
      tia_counter[index] = 0;
      tia_audc[index] = 0;
      tia_audf[index] = 0;
      tia_audv[index] = 0;
      tia_poly4Cntr[index] = 0;
      tia_poly5Cntr[index] = 0;
      tia_poly9Cntr[index] = 0;
   }

   tia_cycles = 0;
}

void tia_Write(uint16_t address, uint8_t data)
{
   uint8_t frequency;
   uint8_t channel = (address + 1) & 1;

   switch (address)
   {
      case AUDC0:  /* Audio control */
      case AUDC1:
         tia_audc[channel] = data & 0x0F;
         break;

      case AUDF0:  /* Audio frequency */
      case AUDF1:
         tia_audf[channel] = data & 0x1F;
         break;

      case AUDV0:  /* Audio volume */
      case AUDV1:
         tia_audv[channel] = data & 0x0F;
         break;

      default:  /* Unmapped */
         return;
   }


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

static void tia_Process()
{
   if (tia_counter[0] > 1)
      tia_counter[0]--;

   else if (tia_counter[0] == 1)
   {
      tia_counter[0] = tia_counterMax[0];
      tia_ProcessChannel(0);
   }


   if (tia_counter[1] > 1)
      tia_counter[1]--;

   else if (tia_counter[1] == 1)
   {
      tia_counter[1] = tia_counterMax[1];
      tia_ProcessChannel(1);
   }
}

void tia_Run(int cycles)
{
   tia_cycles += cycles;
   while(tia_cycles >= (CYCLES_PER_SCANLINE / 2))  /* Maria / 228 ~ 1/2 scanline tick */
	{
      tia_Process();
      tia_cycles -= (CYCLES_PER_SCANLINE / 2);
	}
}

int tia_Output(void)
{
   int currentValue = tia_volume[0] + tia_volume[1];  /* 2x 4-bit unsigned */
   currentValue *= (0x400 - 0x80);  /* 15-bit expand + overflow adjust */

	tia_buffer[tia_count] = currentValue;
   tia_count++;

   return currentValue;
}

int16_t *tia_GetBuffer(void)
{
   return tia_buffer;
}
