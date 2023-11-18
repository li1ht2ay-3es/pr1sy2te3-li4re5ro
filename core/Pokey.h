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
 * Pokey.h
 * ----------------------------------------------------------------------------
 */
#ifndef POKEY_H
#define POKEY_H

#define POKEY_AUDF1  0x0
#define POKEY_AUDC1  0x1
#define POKEY_AUDF2  0x2
#define POKEY_AUDC2  0x3
#define POKEY_AUDF3  0x4
#define POKEY_AUDC3  0x5
#define POKEY_AUDF4  0x6
#define POKEY_AUDC4  0x7
#define POKEY_AUDCTL 0x8
#define POKEY_STIMER 0x9
#define POKEY_SKRES  0xa
#define POKEY_POTGO  0xb
#define POKEY_SEROUT 0xd
#define POKEY_IRQEN  0xe
#define POKEY_SKCTLS 0xf

#define POKEY_POT0   0x0
#define POKEY_POT1   0x1
#define POKEY_POT2   0x2
#define POKEY_POT3   0x3
#define POKEY_POT4   0x4
#define POKEY_POT5   0x5
#define POKEY_POT6   0x6
#define POKEY_POT7   0x7
#define POKEY_ALLPOT 0x8
#define POKEY_KBCODE 0x9
#define POKEY_RANDOM 0xa
#define POKEY_SERIN  0xd
#define POKEY_IRQST  0xe
#define POKEY_SKSTAT 0xf

#include <stdint.h>
#include "Mixer.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void pokey_SetLowpass(int rate);

extern void pokey_Reset(void);
extern uint8_t pokey_Read(uint16_t address);
extern void pokey_Write(uint16_t address, uint8_t value);

extern void pokey_Frame(void);
extern void pokey_Run(int cycles);
extern void pokey_Scanline(void);
extern void pokey_Output(void);

extern void pokey_LoadState(void);
extern void pokey_SaveState(void);

extern int16_t pokey_buffer[MAX_SOUND_SAMPLES];
extern int pokey_outCount;

#ifdef __cplusplus
}
#endif

#endif
