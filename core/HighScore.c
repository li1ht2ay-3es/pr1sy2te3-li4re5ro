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
extern int64_t rfseek(RFILE* stream, int64_t offset, int origin);


bool highscore_enabled = false;

static uint8_t* highscore_bios = NULL;
static uint32_t highscore_size = 0;

static void highscore_initram(void)
{
   memset(memory_nvram + 0, 0, 0x29);  /* name */
   memset(memory_nvram + 0x29, 0xff, 0x6e - 0x29);  /* game id - table 1 */
   memset(memory_nvram + 0x6e, 0xff, 0xb3 - 0x6e);  /* game id - table 2 */
   memset(memory_nvram + 0xb3, 0x7f, 0xf8 - 0xb3);  /* difficulty */
   memset(memory_nvram + 0xf8, 0x7f, 0x13d - 0xf8);  /* index */
   memset(memory_nvram + 0x13d, 0, 0x780 - 0x13d);  /* data */   
   memset(memory_nvram + 0x790, 0, 0x7a0 - 0x790);  /* data */   
   memset(memory_nvram + 0x7a0, 0, 0x800 - 0x7a0);  /* data */   
}

bool highscore_IsValid(void)
{
   if (memory_nvram[2] != 0x68) return false;
   if (memory_nvram[3] != 0x83) return false;
   if (memory_nvram[4] != 0xaa) return false;
   if (memory_nvram[5] != 0x55) return false;
   if (memory_nvram[6] != 0x9c) return false;

   return true;
}

bool highscore_Load(const char *filename)
{
   RFILE *file;
   if(!filename || filename[0] == '\0')
      return false;

   highscore_Release();
   highscore_initram();


   file = filestream_open(filename,
		   RETRO_VFS_FILE_ACCESS_READ,
		   RETRO_VFS_FILE_ACCESS_HINT_NONE);
   if (!file)
      return false;

   highscore_size = filestream_get_size(file);
   highscore_bios = (uint8_t*)malloc(highscore_size * sizeof(uint8_t));
   if(rfread(highscore_bios, 1, highscore_size, file) != highscore_size && filestream_error(file))
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

bool highscore_ReadNvramName(const char *filename, char *buffer)
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

   if(rfseek(file, 0x800, SEEK_SET) && filestream_error(file))
      return false;

   if(rfseek(file, 8, SEEK_SET) && filestream_error(file))
      return false;

   if(rfread(buffer, 1, 33, file) != 33 && filestream_error(file))
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

bool highscore_WriteNvramName(const char *filename, const char *buffer)
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

   if(rfseek(file, 0x800, SEEK_SET) && filestream_error(file))
      return false;

   if(rfseek(file, 8, SEEK_SET) && filestream_error(file))
      return false;

   if(rfwrite(buffer, 1, 33, file) != 33 && filestream_error(file))
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

   memory_nvram[0] = 0;
   memory_nvram[1] = 0;
   memory_nvram[2] = 0x68;
   memory_nvram[3] = 0x83;
   memory_nvram[4] = 0xaa;
   memory_nvram[5] = 0x55;
   memory_nvram[6] = 0x9c;
   memory_nvram[7] = 0x00;

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
   return (sally_readmap[0x3000 / 0x40] == highscore_bios) ? true: false;
}

void highscore_Release(void)
{
   if (highscore_bios)
      free(highscore_bios);

   highscore_bios = NULL;
   highscore_size = 0;
}

void highscore_Map(void)
{
   if (highscore_enabled)
   {
      sally_SetRead(0x1000, 0x1800, memory_nvram);
      maria_SetRead(0x1000, 0x1800, memory_nvram);
      sally_SetWrite(0x1000, 0x1800, memory_nvram);

      if (highscore_bios)
	  {
         sally_SetRead(0x3000, 0x4000, highscore_bios);
         maria_SetRead(0x3000, 0x4000, highscore_bios);
	  }
   }
}
