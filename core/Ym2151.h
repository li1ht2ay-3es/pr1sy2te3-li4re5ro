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
 * Ym2151.h
 * ----------------------------------------------------------------------------
 */
#ifndef YM2151_H
#define YM2151_H

#include <stdint.h>
#include <stddef.h>

#include "Mixer.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void ym2151_Reset(void);
extern void ym2151_Frame(void);
extern void ym2151_Run(int cycles);
extern void ym2151_Output(void);

extern uint8_t ym2151_Read(uint16_t address);
extern void ym2151_Write(uint16_t address, uint8_t data);

extern int16_t ym2151_buffer[MAX_SOUND_SAMPLES * 2];
extern int ym2151_outCount;

#ifdef __cplusplus
}
#endif

#endif
