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
 * Riot.c
 * ----------------------------------------------------------------------------
 */

#include "ProSystem.h"
#include "Riot.h"
#include "Equates.h"
#include "Memory.h"
#include "LightGun.h"

static uint8_t riot_dra;
static uint8_t riot_drb;

static uint8_t riot_active;
static uint8_t riot_elapsed;

static int16_t riot_timer;
static uint8_t riot_intervals;

static int32_t riot_currentTime;
static uint16_t riot_clocks;

static uint32_t riot_halfcycle = 0;


void riot_Reset(void)
{
   memset(memory_ram + 0x280, 0, 0x20);

   riot_timer = TIM64T; 
   riot_intervals = 0;
   riot_clocks = 0;
   riot_currentTime = 0;

   riot_elapsed = 0;
   riot_active = 0;

   riot_halfcycle = 0;
}

/* ----------------------------------------------------------------------------
 * SetInput
 * +----------+--------------+-------------------------------------------------
 * | Offset   | Controller   | Control                                         
 * +----------+--------------+-------------------------------------------------
 * | 00       | Joystick 1   | Right
 * | 01       | Joystick 1   | Left
 * | 02       | Joystick 1   | Down
 * | 03       | Joystick 1   | Up
 * | 04       | Joystick 1   | Button 1
 * | 05       | Joystick 1   | Button 2
 * | 06       | Joystick 2   | Right
 * | 07       | Joystick 2   | Left
 * | 08       | Joystick 2   | Down
 * | 09       | Joystick 2   | Up
 * | 10       | Joystick 2   | Button 1
 * | 11       | Joystick 2   | Button 2
 * | 12       | Console      | Reset
 * | 13       | Console      | Select
 * | 14       | Console      | Pause
 * | 15       | Console      | Left Difficulty
 * | 16       | Console      | Right Difficulty
 * +----------+--------------+-------------------------------------------------
 */
void riot_SetInput(const uint8_t* input)
{
   /*gdement: 	Comments are messy, but wanted to document how this all works.
     Changed this routine to support 1 vs 2 button modes.
     Also added the interaction of CTLSWA and DRA on the SWCHA output, and same for SWCHB.
     SWCHA is directionals.  SWCHB is console switches and button mode.
     button signals are in high bits of INPT0-5.*/

   riot_dra = ((~memory_ram[CTLSWA]) | memory_ram[SWCHA]);	/* SWCHA as driven by RIOT */


   /*now console switches will force bits to ground:*/
   if (input[0x00]) riot_dra &= ~0x80;
   if (input[0x01]) riot_dra &= ~0x40;
   if (input[0x02]) riot_dra &= ~0x20;
   if (input[0x03]) riot_dra &= ~0x10;

   if (input[0x06]) riot_dra &= ~0x08;
   if (input[0x07]) riot_dra &= ~0x04;
   if (input[0x08]) riot_dra &= ~0x02;
   if (input[0x09]) riot_dra &= ~0x01;


   /*Switches can always push the appropriate bit of SWCHA to ground, as in above code block.
     In addition, RIOT can be configured to drive ground even when switch is open.
     By doing this it's possible for real hardware to behave as if switches are permanently held (tested this).*/


   /*As with swcha, the value seen at SWCHB is derived from CTLSWB and DRB (an internal RIOT register)
     (Any write to SWCHB actually gets stored in DRB)
     If a given bit in CTLSWB is 0 (input mode), then RIOT puts a 1 on SWCHB.
     If bit in CTLSWB is 1 (output mode), then RIOT puts stored DRB value on SWCHB.
     The SWCHB outputs from RIOT can be overdriven to 0 by console switches.
     The CTLSWB/DRB interaction is important at bits 2 and 4, which control button mode for each player.
     Bit 5 appears unused, and other bits are the console switches.

     CTLSWB		DRB		SWCHB		result on button mode (for bits 2 and 4)
     -------		---		-----		-----------------------------------------
     0			0		1			1 button mode  - this is default state after boot
     0			1		1			1 button mode
     1			0		0			2 button mode
     1			1		1			1 button mode
     This chart was confirmed on hardware
     From the default state after boot, simply changing CTLSWB to 1 will result in 2 button mode.
     Some games rely on this, and don't actually store anything to SWCHB.*/

   riot_drb = (~memory_ram[CTLSWB]) | memory_ram[SWCHB];	/*SWCHB as driven by RIOT*/


   /*now the console switches can force certain bits to ground:*/
   if (input[0x0c])	riot_drb &= ~0x01;
   if (input[0x0d])	riot_drb &= ~0x02;
   if (input[0x0e])	riot_drb &= ~0x08;
   if (input[0x0f])	riot_drb &= ~0x40;
   if (input[0x10])	riot_drb &= ~0x80;


   /*When in 1 button mode, only the legacy 2600 button signal is active.  The others stay off.
     When in 2 button mode, only the new signals are active.  2600 button stays off.	(tested)
see:  http://www.atariage.com/forums/index.php?showtopic=127162
also see 7800 schematic and RIOT datasheet  */

   if (riot_drb & 0x04)	 /* first player in 1 button mode */
   {
      memory_ram[INPT0] &= 0x7f;     /* new style buttons are always off in this mode */
      memory_ram[INPT1] &= 0x7f;

      memory_ram[INPT4] = (input[0x04] || input[0x05]) ? (memory_ram[INPT4] & 0x7f) : (memory_ram[INPT4] | 0x80);   /* in this mode, either button triggers only the legacy button signal */
   }

   else  /* first player in 2 button mode */
   {
      memory_ram[INPT4] |= 0x80;  /* 2600 button is always off in this mode */

      memory_ram[INPT1] = input[0x04] ? (memory_ram[INPT1] | 0x80) : (memory_ram[INPT1] & 0x7f);  /* left button (button 1) */
      memory_ram[INPT0] = input[0x05] ? (memory_ram[INPT0] | 0x80) : (memory_ram[INPT0] & 0x7f);  /* right button (button 2) */
   }


   /*now repeat for 2nd player*/
   if (riot_drb & 0x10)
   {
      memory_ram[INPT2] &= 0x7f;
      memory_ram[INPT3] &= 0x7f;

      memory_ram[INPT5] = (input[0x0a] || input[0x0b]) ? (memory_ram[INPT5] & 0x7f) : (memory_ram[INPT5] | 0x80);
   }
   else
   {
      memory_ram[INPT5] |= 0x80;

      memory_ram[INPT3] = input[0x0a] ? (memory_ram[INPT3] | 0x80) : (memory_ram[INPT3] & 0x7f);
      memory_ram[INPT2] = input[0x0b] ? (memory_ram[INPT2] | 0x80) : (memory_ram[INPT2] & 0x7f);
   }
}

void riot_SetTimer(uint16_t timer, uint8_t intervals)
{
   const uint16_t clocks[4] = { 1, 8, 64, 1024 };

   riot_clocks = clocks[timer & 3];
   riot_timer = timer;
   riot_intervals = intervals;

   riot_currentTime = riot_clocks * intervals;

   riot_elapsed = 0;
   riot_active = 1;
}

uint8_t riot_Read(uint16_t address)
{
   uint8_t oldval;

   address += 0x200;
   switch(address)
   {
   case SWCHA:
      return riot_dra;

   case SWCHB:
      return riot_drb;

   case CTLSWA:
   case CTLSWB:
	   return memory_ram[address];

   case INTIM:
   case INTIM | 0x2:
      memory_ram[INTFLG] &= 0x7f;
      return memory_ram[INTIM];

   case INTFLG:
   case INTFLG | 0x2:
      oldval = memory_ram[INTFLG];
      memory_ram[INTFLG] &= 0x7f;
      return oldval;
   }

   return memory_ReadOpenBus(address);
}

void riot_Write(uint16_t address, uint8_t data)
{
   address += 0x200;
   switch(address)
   {
   case SWCHA:
   case CTLSWA:
   case SWCHB:
   case CTLSWB:
      memory_ram[address] = data;
      break;

   case TIM1T:
   case TIM8T:
   case TIM64T:
   case T1024T:
   case TIM1T | 0x8:
   case TIM8T | 0x8:
   case TIM64T | 0x8:
   case T1024T | 0x8:
      riot_SetTimer(address, data);
      break;

   default:
      memory_WriteOpenBus(address, data);
      break;
   }
}

static void riot_Process()
{
   if (!riot_active)  /* timer stopped */
      return;


   riot_currentTime--;

   if (!riot_elapsed)
   {
      if (riot_currentTime > 0)
         memory_ram[INTIM] = riot_currentTime / riot_clocks;

      else
      {
         riot_currentTime = riot_clocks;

         memory_ram[INTIM] = 0;
         memory_ram[INTFLG] |= 0x80;

         riot_elapsed = 1;
      }
   }

   else  /* riot_elapsed */
   {
      if (riot_currentTime >= -255)
         memory_ram[INTIM] = riot_currentTime;

      else
      {
         memory_ram[INTIM] = 0;
         riot_active = 0;
      }
   }
}

void riot_Run(uint32_t cycles)
{
   cycles += riot_halfcycle;
   riot_halfcycle = cycles % 4;   /* leftover clocks */


   cycles /= 4;       /* Maria @ 1/4 */
   while (cycles > 0)
   {
      riot_Process();
      cycles--;
   }
}

void riot_LoadState(void)
{
   prosystem_ReadStatePtr(memory_ram + 0x280, 0x20);

   riot_active = prosystem_ReadState8();
   riot_elapsed = prosystem_ReadState8();
   riot_timer = prosystem_ReadState16();
   riot_intervals = prosystem_ReadState8();
   riot_currentTime = prosystem_ReadState32();
   riot_clocks = prosystem_ReadState16();
}

void riot_SaveState(void)
{
   prosystem_WriteStatePtr(memory_ram + 0x280, 0x20);

   prosystem_WriteState8(riot_active);
   prosystem_WriteState8(riot_elapsed);
   prosystem_WriteState16(riot_timer);
   prosystem_WriteState8(riot_intervals);
   prosystem_WriteState32(riot_currentTime);
   prosystem_WriteState16(riot_clocks);
}
