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
 * HighScore.h
 * ----------------------------------------------------------------------------
 */
#ifndef HIGHSCORE_H
#define HIGHSCORE_H

#include <stdint.h>
#include <boolean.h>

#ifdef __cplusplus
extern "C" {
#endif

extern bool highscore_Load(const char *filename);
extern void highscore_Release(void);
extern bool highscore_IsValid(void);

extern bool highscore_ReadNvram(const char *filename);
extern bool highscore_ReadNvramName(const char *filename, char *buffer);
extern bool highscore_WriteNvram(const char *filename);
extern bool highscore_WriteNvramName(const char *filename, const char *buffer);

extern void highscore_SetName(const char *name);
extern bool highscore_IsMapped(void);
extern void highscore_Map(void);

extern bool highscore_enabled;

#ifdef __cplusplus
}
#endif

#endif
