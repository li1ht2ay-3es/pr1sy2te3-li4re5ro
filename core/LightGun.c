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
 * LightGun.c
 * ----------------------------------------------------------------------------
 */
#include "ProSystem.h"
#include "Memory.h"
#include "Maria.h"
#include "LightGun.h"

int lightgun_enabled;

static int lightgun_x;
static int lightgun_y;
static int lightgun_fire;

static int lightgun_scanline_start;
static int lightgun_scanline_end;
static int lightgun_scanline_cycle;

void lightgun_Reset(void)
{
   lightgun_scanline_start = 0x7FFF;
   lightgun_scanline_end = 0x7FFF;
}

void lightgun_Trigger(int16_t x, int16_t y)
{
   lightgun_scanline_start = y - LIGHTGUN_PAD;
   lightgun_scanline_end = y + LIGHTGUN_PAD;

   x -= LIGHTGUN_PAD;
   if (x < 0)
      x = 0;

   lightgun_scanline_cycle = 28 + (CYCLES_PER_SCANLINE - 28) * x / 320;
}

void lightgun_Run(void)
{
   if (!lightgun_enabled)
      return;


   memory_ram[INPT4] |= 0x80;  /* no light */

   if (maria_scanline < lightgun_scanline_start)
      return;

   if (maria_scanline > lightgun_scanline_end)
      return;

   if (prosystem_cycles < lightgun_scanline_cycle)
      return;

   if ((memory_ram[CTRL] & 0x60) == 0x40)  /* avoid dma mode */
      return;


   memory_ram[INPT4] &= 0x7F;
}
