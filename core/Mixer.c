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
#include "Ym2151.h"

int mixer_rate = 48000;

int16_t mixer_buffer[MAX_SOUND_SAMPLES] = {0};
int mixer_outCount = 0;

static int mixer_cycles = 0;
static int mixer_cycles2 = 0;
static int mixer_tick = 0;
static int mixer_tick2 = 0;

static int mixer_volume = 100;
static int tia_volume = 100;
static int pokey_volume = 100;
static int bupchip_volume = 100;
static int yamaha_volume = 100;
static int covox_volume = 100;

static int mixer_filter = 0;
static int tia_filter = 0;

void mixer_Reset(void)
{
   mixer_cycles = 0;
   mixer_cycles2 = 0;
}

void mixer_SetRate()
{
   int clock = CYCLES_PER_SCANLINE * prosystem_scanlines * prosystem_frequency;

   mixer_tick  = clock / mixer_rate;
   mixer_tick2 = clock % mixer_rate;
}

void mixer_Frame(void)
{
   mixer_outCount = 0;
}

void mixer_SetFilter(int limit)
{
   mixer_filter = limit;
}

void mixer_SetTiaFilter(int limit)
{
   tia_filter = limit;
}

void mixer_Run(int cycles)
{
   mixer_cycles += cycles;
   while (mixer_cycles >= mixer_tick)
   {
      if (mixer_cycles == mixer_tick)  /* fractional */
      {
         //if (mixer_cycles2 < mixer_tick2)
            //break;
      }


      //tia_Output();
      pokey_Output();
      //bupchip_Output();
      ym2151_Output();

      mixer_outCount++;


      mixer_cycles -= mixer_tick;
      mixer_cycles2 -= mixer_tick2;
      if (mixer_cycles2 < 0)  /* borrow */
      {
         mixer_cycles--;
         mixer_cycles2 += mixer_rate;
      }
	}
}

void mixer_SetMixerVolume(uint16_t volume)
{
   mixer_volume = volume;
}

void mixer_SetTiaVolume(uint16_t volume)
{
   tia_volume = volume;
}

void mixer_SetPokeyVolume(uint16_t volume)
{
   pokey_volume = volume;
}

void mixer_SetBupchipVolume(uint16_t volume)
{
   bupchip_volume = volume;
}

void mixer_SetYamahaVolume(uint16_t volume)
{
   yamaha_volume = volume;
}

void mixer_SetCovoxVolume(uint16_t volume)
{
   covox_volume = volume;
}

void mixer_FrameEnd(void)
{
   int left, right;
   int index;


   /* Single-pole low-pass filter (6 dB/octave) */
   static int tia_lowpass_mono = 0;
   static int mixer_lowpass_left = 0;
   static int mixer_lowpass_right = 0;

   int mixer_filter_a = (mixer_filter * 0x10000) / 100;
   int mixer_filter_b = 0x10000 - mixer_filter_a;

   int tia_filter_a = (tia_filter * 0x10000) / 100;
   int tia_filter_b = 0x10000 - tia_filter_a;


   for (index = 0; index < tia_outCount; index += 2)
   {
      left = (tia_buffer[index] * tia_volume) / 100;

      if (tia_filter)
	  {
         left = ((tia_lowpass_mono * tia_filter_a) + (left * tia_filter_b)) >> 16;
	     tia_lowpass_mono = left;
	  }


      if (cartridge_pokey)
         left += (pokey_buffer[index] * pokey_volume) / 100;

      right = left;  /* mono expand */


      if (cartridge_bupchip)
	  {
         left += (bupchip_buffer[index*2 + 0] * 2 * bupchip_volume) / 100;  /* 7f = max unscaled volume */
         right += (bupchip_buffer[index*2 + 1] * 2 * bupchip_volume) / 100;
	  }


      if (cartridge_ym2151)
	  {
         left += (ym2151_buffer[index*2 + 0] * 2 * bupchip_volume) / 100;  /* 7f = max unscaled volume */
         right += (ym2151_buffer[index*2 + 1] * 2 * bupchip_volume) / 100;
	  }


	  left = (left > 0x7FFF) ? 0x7FFF : left;
      right = (right > 0x7FFF) ? 0x7FFF : right;


      if (mixer_filter)
	  {
         left = ((mixer_lowpass_left * mixer_filter_a) + (left * mixer_filter_b)) >> 16;
	     mixer_lowpass_left = left;

         right = ((mixer_lowpass_right * mixer_filter_a) + (right * mixer_filter_b)) >> 16;
	     mixer_lowpass_right = right;
	  }


      mixer_buffer[index*2 + 0] = (int16_t) left;
      mixer_buffer[index*2 + 1] = (int16_t) right;
   }
}
