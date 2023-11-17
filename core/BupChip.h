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
 * BupChip.h
 * ----------------------------------------------------------------------------
 */
#ifndef BUPCHIP_H
#define BUPCHIP_H

#include <stdint.h>
#include <stddef.h>

#include "Mixer.h"
#include "../bupboop/types.h"
#include "../bupboop/coretone/coretone.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int16_t bupchip_buffer[MAX_SOUND_SAMPLES];
extern int bupchip_outCount;
extern int bupchip_attenuation;

extern int bupchip_InitFromCDF(const char** cdf, size_t* cdfSize, const char *workingDir);
extern void bupchip_ProcessAudioCommand(unsigned char data);
extern void bupchip_Process(unsigned tick);
extern void bupchip_Release(void);

extern void bupchip_Frame(void);
extern void bupchip_ScanlineEnd(void);
extern void bupchip_Output(void);
extern void bupchip_Reset(void);
extern void bupchip_SetRate(void);

extern void bupchip_LoadState(void);
extern void bupchip_SaveState(void);

#ifdef __cplusplus
}
#endif

#endif
