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
 * PokeySound is Copyright(c) 1997 by Ron Fries
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
 * Pokey.c
 * ----------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "Pokey.h"
#include "ProSystem.h"
#include "Maria.h"
#include "Cartridge.h"
#include "Mixer.h"
#include "Memory.h"

#define POKEY_NOTPOLY5 0x80
#define POKEY_POLY4 0x40
#define POKEY_PURE 0x20
#define POKEY_VOLUME_ONLY 0x10
#define POKEY_VOLUME_MASK 0x0f

#define POKEY_POLY9 0x80
#define POKEY_CH1_179 0x40
#define POKEY_CH3_179 0x20
#define POKEY_CH1_CH2 0x10
#define POKEY_CH3_CH4 0x08
#define POKEY_CH1_FILTER 0x04
#define POKEY_CH2_FILTER 0x02
#define POKEY_CLOCK_15 0x01

#define POKEY_CLK_64 28
#define POKEY_CLK_15 114

#define POKEY_POLY4_SIZE 0x0F
#define POKEY_POLY5_SIZE 0x1F
#define POKEY_POLY9_SIZE 0x1FF
#define POKEY_POLY17_SIZE 0x1FFFF

#define POKEY_CHANNEL1 0
#define POKEY_CHANNEL2 1
#define POKEY_CHANNEL3 2
#define POKEY_CHANNEL4 3

#define SK_RESET   0x03
#define SK_TWOTONE 0x08

static uint8_t pokey_audf[4];
static uint8_t pokey_audc[4];
static uint8_t pokey_audctl;

static uint8_t pokey_output[4];
static uint8_t pokey_filter[4];

static uint8_t pokey_poly04[POKEY_POLY4_SIZE] = {1,1,0,1,1,1,0,0,0,0,1,0,1,0,0};
static uint8_t pokey_poly05[POKEY_POLY5_SIZE] = {0,0,1,1,0,0,0,1,1,1,1,0,0,1,0,1,0,1,1,0,1,1,1,0,1,0,0,0,0,0,1};
static uint8_t pokey_poly09[POKEY_POLY9_SIZE];
static uint8_t pokey_poly17[POKEY_POLY17_SIZE];

static uint8_t pokey_poly04Cntr;
static uint8_t pokey_poly05Cntr;
static uint16_t pokey_poly09Cntr;
static uint32_t pokey_poly17Cntr;

static uint8_t pokey_divideCount[4];
static uint8_t pokey_borrowCount[4];

static uint8_t pokey_clocks[2];

static uint8_t rand9[POKEY_POLY9_SIZE];
static uint8_t rand17[POKEY_POLY17_SIZE];
static uint32_t r9;
static uint32_t r17;
static uint8_t pokey_skctl;

static uint8_t POT_input[8] = {228, 228, 228, 228, 228, 228, 228, 228};
static int pot_scanline;

static int random_scanline_counter;
static int prev_random_scanline_counter;

static int pokey_cycles;

int16_t pokey_buffer[MAX_SOUND_SAMPLES] = {0};
int pokey_outCount;

static int pokey_lpfCount[4] = {0};
static int pokey_lpfOld[4] = {0};
static int pokey_lpfNew[4] = {0};

/* #define POKEY_LOWPASS 1    /* 1.789 Mhz  = 315 / 88 / 2 */
#define POKEY_LOWPASS 80   /* 22362 */
/* #define POKEY_LOWPASS 95   /* 18839 */
/* #define POKEY_LOWPASS 111  /* 16124 */
/* #define POKEY_LOWPASS 112  /* 15980 */
/* #define POKEY_LOWPASS 120  /* 14914 */
/* #define POKEY_LOWPASS 128  /* 13984 */
/* #define POKEY_LOWPASS 224  /* 7990 */
/* #define POKEY_LOWPASS 1790  /* 1000 */
int pokey_lowpass = POKEY_LOWPASS;

static void rand_init(uint8_t *rng, uint32_t size, uint32_t left, uint32_t right, uint32_t add)
{
   uint32_t mask = (1 << size) - 1;
   uint32_t i, x = 0;

   for (i = 0; i < mask; i++)
   {
      *rng++ = (size == 17) ? x >> 6 : x;  /* bits 6..13 -- bits 0..7 */

      x = ((x << left) + (x >> right) + add) & mask;  /* calculate next bit */
   }
}

static void init_poly09(void)
{
   int mask = (1 << 9) - 1;
   int lfsr = mask;
   int index;

   for (index = 0; index < mask; index++)
   {
      int bin = ((lfsr >> 0) & 1) ^ ((lfsr >> 5) & 1);

      lfsr >>= 1;
      lfsr = (bin << 8) | lfsr;
      pokey_poly09[index] = lfsr & 1;
   }
}

static void init_poly17(void)
{
   int mask = (1 << 17) - 1;
   int lfsr = mask;
   int index;

   for (index = 0; index < mask; index++)
   {
      int bin8 = ((lfsr >> 8) & 1) ^ ((lfsr >> 13) & 1);
      int bin = (lfsr & 1);

      lfsr >>= 1;
      lfsr = (lfsr & 0xFF7F) | (bin8 << 7);
      lfsr = (bin << 16) | lfsr;

      pokey_poly17[index] = lfsr & 1;
   }
}

void pokey_Frame(void)
{
   pokey_outCount = 0;
}

void pokey_SetLowpass(int rate)
{
   pokey_lowpass = rate;
}

void pokey_Reset(void)
{
   int index;

   init_poly09();
   init_poly17();

   pokey_poly04Cntr = 0;
   pokey_poly05Cntr = 0;
   pokey_poly09Cntr = 0;
   pokey_poly17Cntr = 0;

   for (index = POKEY_CHANNEL1; index <= POKEY_CHANNEL4; index++)
   {
      pokey_output[index] = 0;
      pokey_audc[index] = 0;
      pokey_audf[index] = 0;
      pokey_filter[index] = (index < 2) ? 1 : 0;
   }

   for (index = 0; index < 8; index++)
      POT_input[index] = 228;

   for (index = 0; index < 2; index++)
      pokey_clocks[index] = 0;

   pokey_audctl = 0;

   /* initialize the random arrays */
   rand_init(rand9,   9, 8, 1, 0x00180);
   rand_init(rand17, 17,16, 1, 0x1C000);

   pokey_skctl = SK_RESET;

   r9 = 0;
   r17 = 0;
   random_scanline_counter = 0;
   prev_random_scanline_counter = 0;  

   pokey_cycles = 0;

   memset(&pokey_buffer, 0, sizeof(pokey_buffer));
}

void pokey_Scanline(void)
{
   random_scanline_counter += CYCLES_PER_SCANLINE;

   if (pot_scanline < 228)
      pot_scanline++;
}

uint8_t pokey_Read(uint16_t address) 
{
   uint8_t data = memory_ReadOpenBus(address);

   address &= 0x0F;

   switch (address)
   {
      case POKEY_RANDOM:
      {
         int curr_scanline_counter =  (random_scanline_counter + prosystem_cycles);

         if (pokey_skctl & SK_RESET)
         {
            int adjust = ((curr_scanline_counter - prev_random_scanline_counter) >> 2);

            r9 = ((adjust + r9) % POKEY_POLY9_SIZE);
            r17 = ((adjust + r17) % POKEY_POLY17_SIZE);
         }

         else
         {
            r9 = 0;
            r17 = 0;
         }

         data = ((pokey_audctl & POKEY_POLY9) ? rand9[r9] : rand17[r17]) ^ 0xFF;

         prev_random_scanline_counter = curr_scanline_counter;
         break;
      }
   }

   return data;
}

void pokey_Write(uint16_t address, uint8_t value)
{
   address &= 0x0F;
   //address += 0x450;  /* Pokey 1 */

   switch(address)
   {
      case POKEY_SKCTLS:
         if (pokey_skctl == value) break;  /* no change */
         pokey_skctl = value;

         if (!(value & SK_RESET))  /* reset */
         {
            pokey_clocks[0] = 0;
            pokey_clocks[1] = 0;

            pokey_poly04Cntr = 0;
            pokey_poly05Cntr = 0;
            pokey_poly09Cntr = 0;
            pokey_poly17Cntr = 0;
         }
         break;

      case POKEY_AUDF1:
      case POKEY_AUDF2:
      case POKEY_AUDF3:
      case POKEY_AUDF4:
         pokey_audf[address / 2] = value;
         break;

      case POKEY_AUDC1:
      case POKEY_AUDC2:
      case POKEY_AUDC3:
      case POKEY_AUDC4:
         pokey_audc[address / 2] = value;
         break;

      case POKEY_AUDCTL:
         pokey_audctl = value;
         break;
   }
}

static void inc_channel(int index, int cycles)
{
   if ((--pokey_divideCount[index]) == 0 && pokey_borrowCount[index] == 0)
      pokey_borrowCount[index] = cycles;
}

static bool check_borrow(int index)
{
   if (pokey_borrowCount[index])
      return (--pokey_borrowCount[index]) == 0;

   return false;
}

static void reset_channel(int index)
{
   pokey_divideCount[index] = pokey_audf[index] + 1;
   pokey_borrowCount[index] = 0;
}

static void process_channel(int index, int mode)
{
   if ((pokey_audc[index] & POKEY_NOTPOLY5) | pokey_poly05[pokey_poly05Cntr])
   {
      switch(mode)
	  {
	  case 0:
         pokey_output[index] ^= 1;
         break;

	  case 1:
         pokey_output[index] = pokey_poly04[pokey_poly04Cntr];
         break;

	  case 2:
         pokey_output[index] = pokey_poly09[pokey_poly09Cntr];
         break;

	  case 3:
         pokey_output[index] = pokey_poly17[pokey_poly17Cntr];
         break;
	  }
   }
}

void pokey_Run(int cycles)
{
   int base_clock = (pokey_audctl & POKEY_CLOCK_15) ? POKEY_CLK_15 : POKEY_CLK_64;
   int joined12 = pokey_audctl & POKEY_CH1_CH2;
   int joined34 = pokey_audctl & POKEY_CH3_CH4;
   int hiclk1 = pokey_audctl & POKEY_CH1_179;
   int hiclk3 = pokey_audctl & POKEY_CH3_179;
   int newvol;
   int index;
   int volonly[4];
   int volmask[4];
   int poly_ticks = 0;
   int volmode[4];


   pokey_cycles += cycles;
   if (pokey_cycles < CYCLES_PER_SCANLINE / 64)  /* 4 = fuzzy, 8 = good, 32 = high, 113 = max */
      return;


   for (index = 0; index < 4; index++)
   {
      volonly[index] = pokey_audc[index] & POKEY_VOLUME_ONLY;
      volmask[index] = pokey_audc[index] & POKEY_VOLUME_MASK;


      if (pokey_lpfCount[index] >= pokey_lowpass)  /* latch new value */
         pokey_lpfOld[index] = pokey_lpfNew[index];

      newvol = ((pokey_output[index] ^ pokey_filter[index]) | volonly[index]) ? volmask[index] : 0;
      pokey_lpfCount[index] = (pokey_lpfNew[index] == newvol) ? pokey_lpfCount[index] : 1;


      if (pokey_audc[index] & POKEY_PURE)
         volmode[index] = 0;
	  else if (pokey_audc[index] & POKEY_POLY4)
         volmode[index] = 1;
	  else if (pokey_audctl & POKEY_POLY9)
         volmode[index] = 2;
	  else
         volmode[index] = 3;
   }

	  
   while (pokey_cycles >= 4)  /* Maria 1/4 tick*/
   {
      int clock_triggered = 0;

      pokey_cycles -= 4;

      if (pokey_skctl & SK_RESET)  /* timers running */
      {
         if ((++pokey_clocks[0]) >= POKEY_CLK_64)
         {
            pokey_clocks[0] = 0;

            if (base_clock == POKEY_CLK_64)
               clock_triggered = 1;
         }

         if ((++pokey_clocks[1]) >= POKEY_CLK_15)
         {
            pokey_clocks[1] = 0;

            if (base_clock == POKEY_CLK_15)
               clock_triggered = 1;
         }


         poly_ticks++;


         if (hiclk1 && (joined12 || pokey_audf[0] > 0))  /* ultrasonic speed hack */
	        inc_channel(0, joined12 ? 7 : 4);

         if (hiclk3 && (joined34 || pokey_audf[2] > 0))
	        inc_channel(2, joined34 ? 7 : 4);


         if (clock_triggered)
		 {
            if (!hiclk1)
	           inc_channel(0, 1);

            if (!joined12)
               inc_channel(1, 1);

		    if (!hiclk3)
	           inc_channel(2, 1);

            if (!joined34)
               inc_channel(3, 1);
		 }
      }


      if (*((uint32_t *) pokey_borrowCount) == 0)  /* no volume changes */
	  {
         pokey_lpfCount[0]++;
         pokey_lpfCount[1]++;
         pokey_lpfCount[2]++;
         pokey_lpfCount[3]++;

		 continue;
	  }


      pokey_poly04Cntr = (pokey_poly04Cntr + poly_ticks) % POKEY_POLY4_SIZE;
      pokey_poly05Cntr = (pokey_poly05Cntr + poly_ticks) % POKEY_POLY5_SIZE;
      pokey_poly09Cntr = (pokey_poly09Cntr + poly_ticks) % POKEY_POLY9_SIZE;
      pokey_poly17Cntr = (pokey_poly17Cntr + poly_ticks) % POKEY_POLY17_SIZE;
      poly_ticks = 0;


      if (check_borrow(2))  /* ch3 */
	  {
         if (joined34)
            inc_channel(3, 1);
         else
            reset_channel(2);

         process_channel(2, volmode[2]);

         pokey_filter[0] = (pokey_audctl & POKEY_CH1_FILTER) ? pokey_output[0] : 1;
	  }


      if (check_borrow(3))  /* ch4 */
	  {
         if (joined34)
            reset_channel(2);

         reset_channel(3);
         process_channel(3, volmode[3]);

         pokey_filter[1] = (pokey_audctl & POKEY_CH2_FILTER) ? pokey_output[1] : 1;

         /* irq4 */
	  }


      if ((pokey_skctl & SK_TWOTONE) && (pokey_borrowCount[1] == 1))  /* ch1 */
         reset_channel(0);
	
      if (check_borrow(0))
	  {
         if (joined12)
            inc_channel(1, 1);

         else
            reset_channel(0);

         process_channel(0, volmode[0]);

         /* irq1 */
	  }


      if (check_borrow(1))
	  {
         if (joined12)
            reset_channel(0);  /* low counter */

         reset_channel(1);
         process_channel(1, volmode[1]);

         /* irq2 */
	  }


      for (index = 0; index < 4; index++)
	  {
         newvol = 0;
         if (volmask[index])
            newvol = ((pokey_output[index] ^ pokey_filter[index]) | volonly[index]) ? volmask[index] : 0;


         if (pokey_lpfNew[index] == newvol)  /* no frequency change */
            pokey_lpfCount[index]++;

         else
		 {
            if (pokey_lpfCount[index] >= pokey_lowpass)  /* latch new value */
               pokey_lpfOld[index] = pokey_lpfNew[index];

            pokey_lpfCount[index] = 1;
            pokey_lpfNew[index] = newvol;
         }
      }
   }


   pokey_poly04Cntr = (pokey_poly04Cntr + poly_ticks) % POKEY_POLY4_SIZE;
   pokey_poly05Cntr = (pokey_poly05Cntr + poly_ticks) % POKEY_POLY5_SIZE;
   pokey_poly09Cntr = (pokey_poly09Cntr + poly_ticks) % POKEY_POLY9_SIZE;
   pokey_poly17Cntr = (pokey_poly17Cntr + poly_ticks) % POKEY_POLY17_SIZE;


   for (index = 0; index < 4; index++)
   {
      if (pokey_lpfCount[index] >= pokey_lowpass)  /* latch new value */
         pokey_lpfOld[index] = pokey_lpfNew[index];

      pokey_lpfCount[index] &= 0xfffff;
   }
}

void pokey_Output(void)
{
   static int max = 0;
   int index;
   int currentValue = 0;
   int active = 2;
   int adjust[3] = { 0x400, 0x300, 0x200 };  /* 10-bit, 9.5-bit, 9-bit expansion */


   if (!cartridge_pokey)
      return;


   for (index = POKEY_CHANNEL1; index <= POKEY_CHANNEL4; index++)  /* 4x 4-bit unsigned */
      currentValue += pokey_lpfOld[index];


   active -= (pokey_audctl & POKEY_CH1_CH2) ? 1 : 0;  /* 16-bit joined timer */
   active -= (pokey_audctl & POKEY_CH3_CH4) ? 1 : 0;

   currentValue *= adjust[active];  /* 15-bit unsigned */

   /* max = (max < currentValue) ? currentValue : max;  /* debug */

   pokey_buffer[pokey_outCount++] = (int16_t) currentValue;
}

void pokey_LoadState(void)
{
   uint8_t index;

   for (index = 0; index < 4; index++)
   {
      pokey_audf[index] = prosystem_ReadState8();
      pokey_audc[index] = prosystem_ReadState8();
      pokey_output[index] = prosystem_ReadState8();
      pokey_filter[index] = prosystem_ReadState8();

      pokey_divideCount[index] = prosystem_ReadState8();
      pokey_borrowCount[index] = prosystem_ReadState8();

      pokey_lpfOld[index] = ((pokey_output[index] ^ pokey_filter[index]) || (pokey_audc[index] & POKEY_VOLUME_ONLY)) ? pokey_audc[index] & POKEY_VOLUME_MASK : 0;
	  pokey_lpfNew[index] = pokey_lpfOld[index];
      pokey_lpfCount[index] = 0;
   }

   pokey_audctl = prosystem_ReadState8();

   pokey_poly04Cntr = prosystem_ReadState8();
   pokey_poly05Cntr = prosystem_ReadState8();
   pokey_poly09Cntr = prosystem_ReadState16();
   pokey_poly17Cntr = prosystem_ReadState32();

   pokey_clocks[0] = prosystem_ReadState8();
   pokey_clocks[1] = prosystem_ReadState8();
}

void pokey_SaveState(void)
{
   uint8_t index;

   for (index = 0; index < 4; index++)
   {
      prosystem_WriteState8(pokey_audf[index]);
      prosystem_WriteState8(pokey_audc[index]);
      prosystem_WriteState8(pokey_output[index]);
      prosystem_WriteState8(pokey_filter[index]);

      prosystem_WriteState8(pokey_divideCount[index]);
      prosystem_WriteState8(pokey_borrowCount[index]);
   }

   prosystem_WriteState8(pokey_audctl);

   prosystem_WriteState8(pokey_poly04Cntr);
   prosystem_WriteState8(pokey_poly05Cntr);
   prosystem_WriteState16(pokey_poly09Cntr);
   prosystem_WriteState32(pokey_poly17Cntr);

   prosystem_WriteState8(pokey_clocks[0]);
   prosystem_WriteState8(pokey_clocks[1]);
}
