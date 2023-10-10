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
 * Mixer.h
 * ----------------------------------------------------------------------------
 */
#ifndef MIXER_H
#define MIXER_H

#include <stdint.h>
#include "Equates.h"


#ifdef __cplusplus
extern "C" {
#endif


extern void mixer_Reset(void);
extern void mixer_Frame(void);
extern void mixer_Run(int cycles);
extern void mixer_SetRate(void);
extern void mixer_FrameEnd(void);

extern void mixer_SetMixerVolume(uint16_t volume);
extern void mixer_SetTiaVolume(uint16_t volume);
extern void mixer_SetPokeyVolume(uint16_t volume);
extern void mixer_SetBupchipVolume(uint16_t volume);
extern void mixer_SetCovoxVolume(uint16_t volume);

extern void mixer_SetFilter(int limit);
extern void mixer_SetTiaFilter(int limit);

#define MAX_SOUND_SAMPLES (3 * 2 * 192000 / 50)  /* stereo + overflow */

extern int16_t mixer_buffer[MAX_SOUND_SAMPLES];
extern int mixer_outCount;
extern int mixer_rate;

#ifdef __cplusplus
}
#endif

#endif
