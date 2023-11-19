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
 * HighScore.c
 * ----------------------------------------------------------------------------
 */
#include <stdlib.h>

#include <streams/file_stream.h>
#include "HighScore.h"
#include "Mapper.h"
#include "Memory.h"
#include <string.h>


/* Forward declaration */
extern int64_t rfread(void* buffer, size_t elem_size, size_t elem_count, RFILE* stream);
extern int64_t rfwrite(void const* buffer, size_t elem_size, size_t elem_count, RFILE* stream);

bool highscore_enabled = false;

static uint8_t* highscore_data = NULL;
uint32_t highscore_size = 0;

bool highscore_Load(const char *filename)
{
   RFILE *file;
   if(!filename || filename[0] == '\0')
      return false;

   highscore_Release();

   file = filestream_open(filename,
		   RETRO_VFS_FILE_ACCESS_READ,
		   RETRO_VFS_FILE_ACCESS_HINT_NONE);
   if (!file)
      return false;

   highscore_size = filestream_get_size(file);
   highscore_data = (uint8_t*)malloc(highscore_size * sizeof(uint8_t));
   if(rfread(highscore_data, 1, highscore_size, file) != highscore_size && filestream_error(file))
   {
      filestream_close(file);
      highscore_Release();
      return false;
   }

   filestream_close(file);
   highscore_enabled = true;

   return true; 
}

bool highscore_ReadNvram(const char *filename)
{
   RFILE *file;
   if(!filename || filename[0] == '\0')
      return false;

   if (!highscore_enabled)
      return false;

   file = filestream_open(filename,
		   RETRO_VFS_FILE_ACCESS_READ,
		   RETRO_VFS_FILE_ACCESS_HINT_NONE);
   if (!file)
      return false;

   if(rfread(memory_nvram, 1, 0x800, file) != 0x800 && filestream_error(file))
   {
      filestream_close(file);
      return false;
   }

   filestream_close(file);

   return true; 
}

bool highscore_WriteNvram(const char *filename)
{
   RFILE *file;
   if(!filename || filename[0] == '\0')
      return false;

   if (!highscore_enabled)
      return false;

   file = filestream_open(filename,
		   RETRO_VFS_FILE_ACCESS_WRITE,
		   RETRO_VFS_FILE_ACCESS_HINT_NONE);
   if (!file)
      return false;

   if(rfwrite(memory_nvram, 1, 0x800, file) != 0x800 && filestream_error(file))
   {
      filestream_close(file);
      return false;
   }

   filestream_close(file);

   return true; 
}

void highscore_SetName(const char *name)
{
   uint8_t index;

   for (index = 0; index < strlen(name); index++)
   {
      if (name[index] == '_')
         memory_nvram[8 + index] = 0x1a;

      else if (name[index] == '.')
         memory_nvram[8 + index] = 0x1b;

      else if (name[index] == '-')
         memory_nvram[8 + index] = 0x1c;

      else if (name[index] == ' ')
         memory_nvram[8 + index] = 0x1d;

      else if (name[index] == '`')
         memory_nvram[8 + index] = 0x1e;

	  else
         memory_nvram[8 + index] = name[index] - 'A';
   }

   memory_nvram[8 + index] = 0x1f;
   memory_nvram[0x28] = strlen(name);
}

bool highscore_IsMapped(void)
{
   return (sally_readmap[0x3000 / 0x40] == highscore_data) ? true: false;
}

void highscore_Release(void)
{
   if (highscore_data)
      free(highscore_data);

   highscore_data = NULL;
   highscore_size = 0;
}

void highscore_Map(void)
{
   if (highscore_data != NULL && highscore_enabled)
   {
      sally_SetRead(0x1000, 0x1800, memory_nvram);
      maria_SetRead(0x1000, 0x1800, memory_nvram);
      sally_SetWrite(0x1000, 0x1800, memory_nvram);

      sally_SetRead(0x3000, 0x4000, highscore_data);
      maria_SetRead(0x3000, 0x4000, highscore_data);
   }
}
