// ----------------------------------------------------------------------------
//   ___  ___  ___  ___       ___  ____  ___  _  _
//  /__/ /__/ /  / /__  /__/ /__    /   /_   / |/ /
// /    / \  /__/ ___/ ___/ ___/   /   /__  /    /  emulator
//
// ----------------------------------------------------------------------------
// Copyright 2005 Greg Stanton
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// ----------------------------------------------------------------------------
// LightGun.cpp - JnO, from lightgun support in Wii7800 by Raz0red
// ----------------------------------------------------------------------------
#include "LightGun.h"
#include "ProSystem.h"

byte lightgun_enabling = LIGHTGUN_AUTO;
bool lightgun_enabled = true;
int lightgun_x = 0;
int lightgun_y = 0;

static int lightgun_scanline;
static int lightgun_cycle;


// ----------------------------------------------------------------------------
// Resets whether the lightgun is activated or not
// ----------------------------------------------------------------------------
void lightgun_Reset()
{
  switch (lightgun_enabling) {
    case LIGHTGUN_AUTO:
      if ( cartridge_controller[0] & CARTRIDGE_CONTROLLER_LIGHTGUN ) {
        lightgun_enabled = true;
      }
      else {
        lightgun_enabled = false;
      }
      break;
    case LIGHTGUN_ENABLED:
      lightgun_enabled = true;
      break;
    case LIGHTGUN_DISABLED:
      lightgun_enabled = false;
      break;
  }
}

// ----------------------------------------------------------------------------
// Recalculate lightgun scanline and cycle when moving mouse
// ----------------------------------------------------------------------------
void lightgun_Refresh()
{
  lightgun_scanline = lightgun_y + cartridge_lightgun_dy +
      ( maria_visibleArea.top - maria_displayArea.top + 1 ) ;

  lightgun_cycle = lightgun_x + cartridge_lightgun_dx +
                              HBLANK_CYCLES + LG_CYCLES_INDENT ;

  if( lightgun_cycle > CYCLES_PER_SCANLINE ) {
    lightgun_scanline++;
    lightgun_cycle -= CYCLES_PER_SCANLINE;
  }
}

// ----------------------------------------------------------------------------
// Strobe based on the current lightgun location
// ----------------------------------------------------------------------------
void lightgun_Fire()
{
  if( ( ( maria_scanline >= lightgun_scanline ) &&
        ( maria_scanline <= ( lightgun_scanline + 3 ) ) ) &&
      ( prosystem_cycles >= ((int)lightgun_cycle ) - 1 ) )
  {
      memory_ram[INPT4] &= 0x7f;
  }
  else
  {
      memory_ram[INPT4] |= 0x80;
  }
  return ;
}

