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

/*
xpos [160] = meltdown + barnyard / crossbow + sentinel
300 = 100
310 = 114
400 = 206
426 = 234

427 = 236
428 = 236
429 = 236
430 = 236

431 = 238
432 = 238
433 = 238
434 = 238

435 = 240
436 = 240
437 = 240
438 = 240

439 = x
440 = x
450 = x
452 = x

0 = 242
1 = 242
2 = 242

3 = 262
4 = 262 / 272


ypos = meltdown + barnyard / crossbow + brigade / sentinel
 0 = x [18]
22 = x
24 = x
26 = x
28 = x
30 = x
31 = x [49]
32 = 50
33 = 51
34 = 52
35 = 53
36 = 54
37 = 55
38 = 56
39 = 57
40 = 58
41 = 59
42 = 60
43 = 61
44 = 62
45 = 63 / 59 / 46
52 = 70 / 66 / 53
62 = 80
72 = 90
*/

void lightgun_Reset(void)
{
   lightgun_y = 0x7FFF;
}

#include <stdio.h>
void lightgun_Trigger(int16_t x, int16_t y)
{
   lightgun_y = y - 18 + maria_visibleArea.top;
   lightgun_x = x + maria_visibleArea.left;

   printf("%d %d -- %d %d\n", x, y, lightgun_x, lightgun_y);
}

uint8_t lightgun_Strobe(void)
{
   uint8_t data = memory_ram[INPT4] | 0x80;  /* not lit */


   if (!lightgun_enabled)
      return memory_ram[INPT4];


   if (maria_draw)
      return data;


   //lightgun_y = 54 + maria_visibleArea.top;
   //lightgun_x = 0 + maria_visibleArea.left;


   if (maria_scanline < lightgun_y)
      return data;

   if (prosystem_cycles < lightgun_x)
      return data;

   return data & 0x7f;
}
