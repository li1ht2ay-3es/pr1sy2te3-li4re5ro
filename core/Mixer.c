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
 * Mixer.c
 * ----------------------------------------------------------------------------
 */
#include "Mixer.h"
#include "ProSystem.h"
#include "Cartridge.h"
#include "Tia.h"
#include "Pokey.h"
#include "BupChip.h"
#include "Maria.h"

static int16_t mixer_buffer[MAX_SOUND_SAMPLES] = {0};
static int mixer_rate = 48000;

int mixer_count = 0;

static int mixer_cycles = 0;
static int mixer_cycles2 = 0;
static int mixer_tick = 0;
static int mixer_tick2 = 0;

void mixer_Reset(void)
{
   mixer_SetRate(mixer_rate);
}

void mixer_SetRate(int rate)
{
   int clock = CYCLES_PER_SCANLINE * prosystem_scanlines * prosystem_frequency;

   mixer_rate = rate;

   mixer_tick  = clock / mixer_rate;
   mixer_tick2 = clock % mixer_rate;

   mixer_cycles = 0;
   mixer_cycles2 = 0;

   ct_setrate(rate);  /* bupchip */
}

void mixer_Frame(void)
{
   mixer_count = 0;

   tia_Frame();
   pokey_Frame();
   bupchip_Frame();
}

void mixer_Run(int cycles)
{
   mixer_cycles += cycles;
   while(mixer_cycles >= mixer_tick)
	{
      if (mixer_cycles == mixer_tick)  /* fractional */
	   {
         if (mixer_cycles2 < mixer_tick2)
            break;
	   }


      tia_Output();
      pokey_Output();
      bupchip_Output();
      mixer_count++;


      mixer_cycles -= mixer_tick;
      mixer_cycles2 -= mixer_tick2;
      if (mixer_cycles2 < 0)  /* borrow */
		{
         mixer_cycles--;
         mixer_cycles2 += mixer_rate;
		}
	}
}

int mixer_GetCount(void)
{
   return mixer_count;
}

int16_t *mixer_GetBuffer(void)
{
   return mixer_buffer;
}

void mixer_FrameEnd(void)
{
   int left, right;
   int index;
   int16_t *tia_buffer = tia_GetBuffer();
   int16_t *pokey_buffer = pokey_GetBuffer();
   int16_t *bupchip_buffer = bupchip_GetBuffer();

   /* Single-pole low-pass filter (6 dB/octave) */
   static int lowpass_output = 0;
   int factor_a = (0 * 0x10000) / 100;
   int factor_b = 0x10000 - factor_a;


   for( index = 0; index < mixer_count; index += 2 )
	{
      left = tia_buffer[index];
		left += pokey_buffer[index];
      right = left;

		left += bupchip_buffer[index*2 + 0];
		right += bupchip_buffer[index*2 + 1];

		left = (left > 0x7FFF) ? 0x7FFF : left;
		right = (right > 0x7FFF) ? 0x7FFF : right;


		//output = (lowpass_output * factor_a) + (output * factor_b);
		//output >>= 16;
		lowpass_output = left;


      mixer_buffer[index*2 + 0] = left;
      mixer_buffer[index*2 + 1] = right;
	}
}
