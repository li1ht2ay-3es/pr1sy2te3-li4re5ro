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
 * Maria.h
 * ----------------------------------------------------------------------------
 */
#ifndef MARIA_H
#define MARIA_H

#define MARIA_SURFACE_SIZE (320 * 293)
#define CYCLES_PER_SCANLINE 454


#include <stdint.h>
#include "Rect.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void maria_Reset(void);
extern void maria_Clear(void);
extern int maria_Run(void);
extern void maria_Scanline(void);

extern uint8_t maria_Read(uint16_t address);
extern void maria_Write(uint16_t address, uint8_t data);

extern void maria_SetRead(uint32_t start, uint32_t stop, uint8_t *chr);

extern void maria_LoadState(void);
extern void maria_SaveState(void);

extern rect maria_displayArea;
extern rect maria_visibleArea;
extern uint8_t maria_surface[MARIA_SURFACE_SIZE];
extern uint32_t maria_scanline;
extern int maria_draw;

extern uint8_t *maria_readmap[0x400];

#ifdef __cplusplus
}
#endif

#endif
