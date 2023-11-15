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
 * Mapper.h
 * ----------------------------------------------------------------------------
 */
#ifndef MAPPER_H
#define MAPPER_H

#include "Cartridge.h"
#include "Sally.h"
#include "Maria.h"

extern void mapper_Reset(void);
extern void mapper_Map(void);
extern void mapper_MapBios(void);

extern uint8_t mapper_Read(uint16_t address);
extern void mapper_Write(uint16_t address, uint8_t data);

extern void mapper_LoadState(void);
extern void mapper_SaveState(void);

#endif
