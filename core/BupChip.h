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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "Mixer.h"
#include "../bupboop/types.h"
#include "../bupboop/coretone/coretone.h"

extern unsigned char bupchip_flags;
extern unsigned char bupchip_volume;
extern unsigned char bupchip_current_song;

extern int bupchip_InitFromCDF(const char** cdf, size_t* cdfSize, const char *workingDir);
extern void bupchip_ProcessAudioCommand(unsigned char data);
extern void bupchip_Process(unsigned tick);
extern void bupchip_Release(void);
extern void bupchip_StateLoaded(void);
extern void bupchip_Frame(void);
extern void bupchip_Run(int cycles);
extern int bupchip_Output(void);
extern int16_t *bupchip_GetBuffer(void);
extern void bupchip_Reset(void);
extern void bupchip_SetRate(int rate);

#ifdef __cplusplus
}
#endif

#endif
