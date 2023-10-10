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
 * ProSystem.h
 * ----------------------------------------------------------------------------
 */
#ifndef PRO_SYSTEM_H
#define PRO_SYSTEM_H

#include "Equates.h"

#include <stdint.h>
#include <boolean.h>
#include <string.h>
#include <retro_inline.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void prosystem_Reset();
extern void prosystem_ExecuteFrame(const uint8_t* input);
extern void prosystem_Close(bool persistent_data);
extern void prosystem_SetRate(int rate);
extern void prosystem_Run(int cycles);

extern bool prosystem_LoadState(const uint8_t *buffer, bool fast_saves);
extern int prosystem_SaveState(uint8_t *buffer, bool fast_saves);

extern uint8_t prosystem_ReadState8(void);
extern uint16_t prosystem_ReadState16(void);
extern uint32_t prosystem_ReadState32(void);
extern void prosystem_ReadStatePtr(uint8_t *ptr, uint32_t size);

extern void prosystem_WriteState8(uint8_t val);
extern void prosystem_WriteState16(uint16_t val);
extern void prosystem_WriteState32(uint32_t val);
extern void prosystem_WriteStatePtr(uint8_t *ptr, uint32_t size);

extern int prosystem_frequency;
extern int prosystem_scanlines;
extern int prosystem_cycles;
extern uint8_t *prosystem_statePtr;
extern int prosystem_fastsaves;

#ifdef __cplusplus
}
#endif

#endif