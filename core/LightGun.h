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
// LightGun.h
// ----------------------------------------------------------------------------
#ifndef LIGHTGUN_H
#define LIGHTGUN_H

#define LIGHTGUN_DISABLED 0
#define LIGHTGUN_ENABLED 1
#define LIGHTGUN_AUTO 2

#define LG_CYCLES_INDENT 52
#define LG_CYCLES_PER_SCANLINE 318

typedef unsigned char byte;

extern byte lightgun_enabling;
extern bool lightgun_enabled;
extern int lightgun_x;
extern int lightgun_y;

extern void lightgun_Reset();
extern void lightgun_Refresh();
extern void lightgun_Fire();

#endif
