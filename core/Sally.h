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
 * Sally.h
 * ----------------------------------------------------------------------------
 */
#ifndef SALLY_H
#define SALLY_H

#include <stdint.h>
#include "Pair.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void sally_Reset(void);
extern void sally_SetNMI(void);
extern void sally_SetIRQ(void);

extern int sally_Run(void);
extern int sally_SlowCycles(void);

extern void sally_SetRead(uint32_t start, uint32_t stop, uint8_t *prg);
extern void sally_SetWrite(uint32_t start, uint32_t stop, uint8_t *prg);

extern void sally_LoadState(void);
extern void sally_SaveState(void);

extern uint8_t* sally_readmap[0x400];
extern uint8_t* sally_writemap[0x400];

#ifdef __cplusplus
}
#endif

#endif
