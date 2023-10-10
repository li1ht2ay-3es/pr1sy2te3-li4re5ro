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
 * Region.c
 * ----------------------------------------------------------------------------
 */
#include "Region.h"
#include "Cartridge.h"
#include "ProSystem.h"
#include "Maria.h"
#include "Palette.h"

uint8_t region_type = REGION_AUTO;

static const rect REGION_DISPLAY_AREA_NTSC = {0, 16, 319, 257};  /* 7800 test18 */
static const rect REGION_DISPLAY_AREA_PAL  = {0, 16, 319, 307};

static const rect REGION_VISIBLE_AREA_NTSC = {0, 27, 319, 250};  /* Screen Safe */
static const rect REGION_VISIBLE_AREA_PAL  = {0, 27, 319, 298};

static const uint8_t REGION_FREQUENCY_NTSC = 60;
static const uint8_t REGION_FREQUENCY_PAL  = 50;

static const uint16_t REGION_SCANLINES_NTSC = 263;  /* 7800 test11 */
static const uint16_t REGION_SCANLINES_PAL  = 313;

void region_Reset(void)
{
   if (region_type == REGION_PAL || (region_type == REGION_AUTO && cartridge_region == REGION_PAL))
   {
      maria_displayArea = REGION_DISPLAY_AREA_PAL;
      maria_visibleArea = REGION_VISIBLE_AREA_PAL;

      if (palette_default)
         palette_Preset(PALETTE_TYPE_PAL);

      prosystem_frequency = REGION_FREQUENCY_PAL;
      prosystem_scanlines = REGION_SCANLINES_PAL;
   }  

   else
   {
      maria_displayArea = REGION_DISPLAY_AREA_NTSC;
      maria_visibleArea = REGION_VISIBLE_AREA_NTSC;

	  if (palette_default)
         palette_Preset(PALETTE_TYPE_NTSC);

      prosystem_frequency = REGION_FREQUENCY_NTSC;
      prosystem_scanlines = REGION_SCANLINES_NTSC;
   }
}
