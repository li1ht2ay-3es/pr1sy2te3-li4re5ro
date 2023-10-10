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
// ProSystem.h
// ----------------------------------------------------------------------------
#ifndef PRO_SYSTEM_H
#define PRO_SYSTEM_H

#include "Equates.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int uint;

extern void prosystem_Reset( );
extern void prosystem_ExecuteFrame(const byte* input);
extern void prosystem_Close(int persistent_data);
extern int prosystem_frequency;
extern int prosystem_scanlines;
extern int prosystem_cycles;

#ifdef __cplusplus
}
#endif



#ifdef _WINDOWS
#include <String>

extern bool prosystem_active;
extern bool prosystem_paused;

extern void prosystem_Pause(bool pause);
extern byte prosystem_frame;

extern bool prosystem_Save(std::string filename, bool compress);
extern bool prosystem_Load(std::string filename, bool fast_savestates);
#endif

#endif