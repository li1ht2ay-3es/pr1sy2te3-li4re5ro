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
 * BupChip.c
 * ----------------------------------------------------------------------------
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "libretro.h"
#include "BupChip.h"
#include "ProSystem.h"
#include "Cartridge.h"
#include "Maria.h"
#include "ProSystem.h"
#include "../bupboop/coretone/coretone.h"

#define BUPCHIP_FLAGS_PLAYING   1
#define BUPCHIP_FLAGS_PAUSED   2

struct BupchipFileContents
{
   uint8_t *data;
   size_t size;
};

typedef struct BupchipFileContents BupchipFileContents;

uint8_t *bupchip_sample_data;
uint8_t *bupchip_instrument_data;
BupchipFileContents bupchip_songs[32];
uint8_t bupchip_song_count;

uint8_t bupchip_flags;
uint8_t bupchip_volume;
uint8_t bupchip_current_song;

int bupchip_attenuation = 1;

static int out_rate;
static int out_tick;
static int out_tick2;
static int out_clock;
static int out_clock2;

int16_t bupchip_buffer[MAX_SOUND_SAMPLES];
int bupchip_outCount;


static void bupchip_ReplaceChar(char *string, char character, char replacement)
{
   while(true)
   {
      if (!(string = strchr(string, character)))
         return;
      *string = replacement;
      string++;
   }
}

int bupchip_InitFromCDF(const char** cdf, size_t* cdfSize, const char *workingDir)
{
   size_t fileIndex;
   char *line;
   BupchipFileContents fileData[34];
   size_t songIndex       = 0;
   uint32_t fileDataCount = 0;

   while(fileDataCount < sizeof(fileData) / sizeof(fileData[0]) &&
      (line = cartridge_GetNextNonemptyLine(cdf, cdfSize)) != NULL)
   {
#ifndef _WIN32
      /* CDF files always use Windows paths. 
         Convert to Unix-style if necessary. */
      bupchip_ReplaceChar(line, '\\', '/');
#endif

      if(!cartridge_ReadFile(&fileData[fileDataCount].data, &fileData[fileDataCount].size, line, workingDir))
      {
         free(line);
         goto err;
      }
      free(line);
      fileDataCount++;
   }

   if(fileDataCount < 2)
      goto err;

   bupchip_sample_data     = fileData[0].data;
   bupchip_instrument_data = fileData[1].data;
   if(ct_init(bupchip_sample_data, bupchip_instrument_data) != 0)
      goto err;
   for(songIndex = 0; songIndex < fileDataCount - 2; songIndex++)
      bupchip_songs[songIndex] = fileData[songIndex + 2];
   bupchip_song_count = (uint8_t)(fileDataCount - 2);

   return 1;

err:
   for(fileIndex = 0; fileIndex < fileDataCount; fileIndex++)
   {
      free(fileData[fileIndex].data);
      fileData[fileIndex].data = NULL;
   }
   bupchip_song_count      = 0;
   bupchip_instrument_data = NULL;
   bupchip_sample_data     = NULL;
   return 0;
}

void bupchip_Stop(void)
{
   bupchip_flags &= ~BUPCHIP_FLAGS_PLAYING;
   ct_stopMusic();
}

void bupchip_Play(unsigned char song)
{
   if(song >= bupchip_song_count)
   {
      bupchip_Stop();
      return;
   }
   bupchip_flags |= BUPCHIP_FLAGS_PLAYING;
   bupchip_current_song = song;
   ct_playMusic(bupchip_songs[bupchip_current_song].data);
}

void bupchip_Pause(void)
{
   bupchip_flags |= BUPCHIP_FLAGS_PAUSED;
   ct_pause();
}

void bupchip_Resume(void)
{
   bupchip_flags &= ~BUPCHIP_FLAGS_PAUSED;
   ct_resume();
}

void bupchip_SetVolume(uint8_t volume)
{
   int attenuation;

   /* This matches BupSystem. */
   attenuation = volume << 2;
   if((volume & 1) != 0)
      attenuation += 0x3;

   bupchip_volume = attenuation;
   ct_attenMusic(attenuation);
}

void bupchip_ProcessAudioCommand(unsigned char data)
{
   switch(data & 0xc0)
   {
   case 0x00:
      switch(data)
      {
      case 0:
         bupchip_flags  = 0;
         bupchip_volume = 0x7f;
         ct_stopAll();
         ct_resume();
         ct_attenMusic(127);
         break;

      case 2:
         bupchip_Resume();
         break;

      case 3:
         bupchip_Pause();
         break;
      }
      break;

   case 0x40:
      bupchip_Stop();
      break;

   case 0x80:
      bupchip_Play(data & 0x1f);
      bupchip_Resume();
      break;

   case 0xc0:
      bupchip_SetVolume(data);
      break;
   }
}

void bupchip_Tick(unsigned tick)
{
   ct_update(&bupchip_buffer[tick * CORETONE_BUFFER_LEN]);
}

void bupchip_Release(void)
{
   int i;
   for(i = 0; i < bupchip_song_count; i++)
   {
      free(bupchip_songs[i].data);
      bupchip_songs[i].data = NULL;
   }

   free(bupchip_instrument_data);
   bupchip_instrument_data = NULL;

   free(bupchip_sample_data);
   bupchip_sample_data     = NULL;
}

void bupchip_ScanlineEnd()
{
   out_clock--;
   if (out_clock > 0)  /* 240 Hz ~ 1/4 frame */
      return;


   bupchip_Tick(bupchip_outCount);
   bupchip_outCount++;


   out_clock = out_tick;
   if (out_tick2 > 0)
   {
      out_clock2 -= out_tick2;
      if (out_clock2 <= 0)
      {
         out_clock2 += out_rate;
         out_clock++;
         return;
      }
   }
}

void bupchip_Output()
{
   /* stream decoder */

   /* note: seems to play at 1/2 unscaled volume (7f max) */
}

void bupchip_Frame(void)
{
   bupchip_outCount = 0;
}

void bupchip_SetRate()
{
   int clock = prosystem_scanlines * prosystem_frequency;

   out_rate = CORETONE_DECODE_RATE;  /* 240 Hz = default */

   out_tick  = clock / out_rate;
   out_tick2 = clock % out_rate;


   ct_setrate(mixer_rate);
}

void bupchip_Reset(void)
{
   out_clock = out_tick;
   out_clock2 = out_rate;

   memset(&bupchip_buffer, 0, sizeof(bupchip_buffer));
}

void bupchip_LoadState(void)
{
   uint8_t new_song;
   uint8_t new_volume;
   uint8_t new_flags;

   new_song = prosystem_ReadState8();
   new_volume = prosystem_ReadState8();
   new_flags = prosystem_ReadState8();


   if (new_song != bupchip_current_song)
      bupchip_Play(new_song);

   bupchip_volume = new_volume;
   ct_attenMusic(bupchip_volume);

   if ((new_flags & BUPCHIP_FLAGS_PLAYING) == 0)
   {
      bupchip_Pause();
      bupchip_Stop();
   }

   else
      (new_flags & BUPCHIP_FLAGS_PAUSED) ? ct_pause() : ct_resume();
}

void bupchip_SaveState(void)
{
   prosystem_WriteState8(bupchip_current_song);
   prosystem_WriteState8(bupchip_volume);
   prosystem_WriteState8(bupchip_flags);
}
