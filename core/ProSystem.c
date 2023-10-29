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
 * This library is free software; you can redistribute it and/or modify it   
 * under the terms of version 2 of the GNU Library General Public License    
 * as published by the Free Software Foundation.                             
 *                                                                           
 * This library is distributed in the hope that it will be useful, but       
 * WITHOUT ANY WARRANTY; without even the implied warranty of                
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library 
 * General Public License for more details.                                  
 * To obtain a copy of the GNU Library General Public License, write to the  
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.   
 *                                                                           
 * Any permitted reproduction of these routines, in whole or in part, must   
 * bear this legend.                                                         
 * ----------------------------------------------------------------------------
 * ProSystem.c
 * ----------------------------------------------------------------------------
 */
#include "ProSystem.h"
#include "Mixer.h"
#include "Bios.h"
#include "Cartridge.h"
#include "Maria.h"
#include "Memory.h"
#include "Region.h"
#include "Riot.h"
#include "Sally.h"
#include "Tia.h"
#include "Pokey.h"
#include "BupChip.h"

#define PRO_SYSTEM_STATE_HEADER "PRO-SYSTEM STATE"

#ifdef _WINDOWS
#include "../Win/resource.h"
#include "logger.h"
#include "archive.h"

bool prosystem_active = false;
bool prosystem_paused = false;
byte prosystem_frame = 0;
#endif

int prosystem_frequency = 60;
int prosystem_scanlines = 263;
int prosystem_cycles = 0;


void prosystem_Reset()
{
   if (!cartridge_IsLoaded())
      return;

   sally_Reset();
   region_Reset();
   memory_Reset();
   maria_Reset();
   riot_Reset();
   mixer_Reset();
   tia_Reset();
   pokey_Reset( );
   bupchip_Reset();
   /* cartridge_LoadHighScoreCart(); */

   if (bios_enabled)
      bios_Store();
   else
      cartridge_Store();

#ifdef _WINDOWS
   prosystem_active = true;
   prosystem_paused = false;
   prosystem_frame = 0;
#endif
}

void prosystem_SetRate(int rate)
{
   mixer_SetRate(rate);
   bupchip_SetRate(rate);
}

static void prosystem_FireLightGun()
{
/*
   if(((maria_scanline >= lightgun_scanline) && 
       (maria_scanline <= (lightgun_scanline + 3))) && 
       (prosystem_cycles >= ((int)lightgun_cycle ) - 1))
   {
      memory_ram[INPT4] &= 0x7f;                        
   } 
   else 
   {
      memory_ram[INPT4] |= 0x80;                        
   }
*/
}

void prosystem_ExecuteFrame(const uint8_t* input)
{
   int cycles_total = 0;

   riot_SetInput(input);

   mixer_Frame();
   tia_Frame();
   pokey_Frame();
   bupchip_Frame();

   for (maria_scanline = 0; maria_scanline < prosystem_scanlines; maria_scanline++)
   {
      int cycles;
      cycles_total -= prosystem_cycles;  /* debug */


	   /* 0 = hsync */
      {
         memory_ram[MSTAT] = (maria_scanline >= maria_displayArea.top && maria_scanline < maria_displayArea.bottom) ? 0x00 : 0x80;
         memory_ram[WSYNC] = false;

         pokey_Scanline();
      }


      /* 0-27 = pre-dma */
      while (prosystem_cycles < 28)
      {
         cycles = sally_Run();
         cycles += (!cycles) ? 28 - prosystem_cycles : 0;  /* wsync */

			riot_Run(cycles);
         tia_Run(cycles);
         pokey_Run(cycles);
         bupchip_Run(cycles);
         //prosystem_FireLightGun();


         cycles += sally_SlowCycles();  /* TIA + RIOT slow access */
         mixer_Run(cycles);

         prosystem_cycles += cycles;
      }


      /* 28-x = maria render */
      {
	      cycles = maria_Run();
         //if (cycles > CYCLES_PER_SCANLINE) cycles = CYCLES_PER_SCANLINE;

         riot_Run(cycles);
         tia_Run(cycles);
         pokey_Run(cycles);
         bupchip_Run(cycles);


         mixer_Run(cycles);
         prosystem_cycles += cycles;
		}


      /* x-453 = sally scanline */
      while (prosystem_cycles < CYCLES_PER_SCANLINE)
      {
         cycles = sally_Run();
         cycles += (!cycles) ? CYCLES_PER_SCANLINE - prosystem_cycles : 0;  /* wsync */

         riot_Run(cycles);
         tia_Run(cycles);
         pokey_Run(cycles);
         bupchip_Run(cycles);


         cycles += sally_SlowCycles();  /* TIA + RIOT slow access */
         mixer_Run(cycles);

         prosystem_cycles += cycles;
      }


		cycles_total += prosystem_cycles;  /* debug */
      prosystem_cycles -= CYCLES_PER_SCANLINE;  /* overflow */
	}


   mixer_FrameEnd();

#ifdef _WINDOWS
	prosystem_frame++;
#endif
}

#ifdef _WINDOWS
bool prosystem_Save(std::string filename, bool compress) {
  if(filename.empty( ) || filename.length( ) == 0) {
    logger_LogError(IDS_PROSYSTEM1,"");
    return false;
  }

  logger_LogInfo(IDS_PROSYSTEM2,filename);
  
  byte buffer[32829] = {0};
  uint size = 0;
  
  uint index;
  for(index = 0; index < 16; index++) {
    buffer[size + index] = PRO_SYSTEM_STATE_HEADER[index];
  }
  size += 16;
  
  buffer[size++] = 1;
  for(index = 0; index < 4; index++) {
    buffer[size + index] = 0;
  }
  size += 4;

  for(index = 0; index < 32; index++) {
    buffer[size + index] = cartridge_digest[index];
  }
  size += 32;

  buffer[size++] = sally_a;
  buffer[size++] = sally_x;
  buffer[size++] = sally_y;
  buffer[size++] = sally_p;
  buffer[size++] = sally_s;
  buffer[size++] = sally_pc.b.l;
  buffer[size++] = sally_pc.b.h;
  buffer[size++] = cartridge_bank;

  for(index = 0; index < 16384; index++) {
    buffer[size + index] = memory_ram[index];
  }
  size += 16384;
  
  if(cartridge_type == CARTRIDGE_TYPE_SUPERCART_RAM) {
    for(index = 0; index < 16384; index++) {
      buffer[size + index] = memory_ram[16384 + index];
    } 
    size += 16384;
  }
  
  if(!compress) {
    FILE* file = fopen(filename.c_str( ), "wb");
    if(file == NULL) {
      logger_LogError(IDS_PROSYSTEM3,filename);
      return false;
    }
  
    if(fwrite(buffer, 1, size, file) != size) {
      fclose(file);
      logger_LogError(IDS_PROSYSTEM4,filename);
      return false;
    }
  
    fclose(file);
  }
  else {
    if(!archive_Compress(filename.c_str( ), "Save.sav", buffer, size)) {
      logger_LogError(IDS_PROSYSTEM5, filename);
      return false;
    }
  }
  return true;
}

bool prosystem_Load(const std::string filename, bool fastsavestates = false) {
  if(filename.empty( ) || filename.length( ) == 0) {
    logger_LogError(IDS_PROSYSTEM1,"");    
    return false;
  }

 
  logger_LogInfo(IDS_PROSYSTEM6,filename);
  
  byte buffer[32829] = {0};
  uint size = archive_GetUncompressedFileSize(filename);
  if(size == 0) {
    FILE* file = fopen(filename.c_str( ), "rb");
    if(file == NULL) {
      logger_LogError(IDS_PROSYSTEM7,filename);
      return false;
    }

    if(fseek(file, 0, SEEK_END)) {
      fclose(file);
      logger_LogError(IDS_PROSYSTEM8,"");
      return false;
    }
  
    size = ftell(file);
    if(fseek(file, 0, SEEK_SET)) {
      fclose(file);
      logger_LogError(IDS_PROSYSTEM9,"");
      return false;
    }

    if(size != 16445 && size != 32829) {
      fclose(file);
      logger_LogError(IDS_PROSYSTEM10,"");
      return false;
    }
  
    if(fread(buffer, 1, size, file) != size && ferror(file)) {
      fclose(file);
      logger_LogError(IDS_PROSYSTEM11,"");
      return false;
    }
    fclose(file);
  }  
  else if(size == 16445 || size == 32829) {
    //archive_Uncompress(filename, buffer, size);
  }
  else {
    logger_LogError(IDS_PROSYSTEM12,"");
    return false;
  }

  uint offset = 0;
  uint index;
  for(index = 0; index < 16; index++) {
    if(buffer[offset + index] != PRO_SYSTEM_STATE_HEADER[index]) {
      logger_LogError(IDS_PROSYSTEM13,"");
      return false;
    }
  }
  offset += 16;
  byte version = buffer[offset++];
  
  uint date = 0;
  for(index = 0; index < 4; index++) {
  }
  offset += 4;
  
  prosystem_Reset( );
  
  char digest[33] = {0};
  for(index = 0; index < 32; index++) {
    digest[index] = buffer[offset + index];
  }
  offset += 32;
  if(cartridge_digest != std::string(digest)) {
    logger_LogError(IDS_PROSYSTEM14, "[" + std::string(digest) + "] [" + cartridge_digest + "].");
    return false;
  }
  
  sally_a = buffer[offset++];
  sally_x = buffer[offset++];
  sally_y = buffer[offset++];
  sally_p = buffer[offset++];
  sally_s = buffer[offset++];
  sally_pc.b.l = buffer[offset++];
  sally_pc.b.h = buffer[offset++];
  
  cartridge_StoreBank(buffer[offset++]);

  for(index = 0; index < 16384; index++) {
    memory_ram[index] = buffer[offset + index];
  }
  offset += 16384;

  if(cartridge_type == CARTRIDGE_TYPE_SUPERCART_RAM) {
    if(size != 32829) {
      logger_LogError(IDS_PROSYSTEM15,"");
      return false;
    }
    for(index = 0; index < 16384; index++) {
      memory_ram[16384 + index] = buffer[offset + index];
    }
    offset += 16384; 
  }  

  return true;
}
#endif

#ifdef _WINDOWS
void prosystem_Pause(bool pause)
{
   if(prosystem_active)
      prosystem_paused = pause;
}
#endif

void prosystem_Close(bool persistent_data)
{
#ifdef _WINDOWS  /* standalone */
   prosystem_active = false;
   prosystem_paused = false;
#endif

   bupchip_Release();
   cartridge_Release(persistent_data);
   maria_Reset();
   memory_Reset();
   tia_Reset();
}

uint32_t read_uint32_from_buffer(const char* buffer, uint32_t* offset)
{
    uint32_t index = *offset;
    *offset += 8;
    return (uint32_t)buffer[index]     << 28 |
           (uint32_t)buffer[index + 1] << 24 |
           (uint32_t)buffer[index + 2] << 20 |
           (uint32_t)buffer[index + 3] << 16 |
           (uint32_t)buffer[index + 4] << 12 |
           (uint32_t)buffer[index + 5] << 8  |
           (uint32_t)buffer[index + 6] << 4  |
           (uint32_t)buffer[index + 7];
}

void save_uint32_to_buffer(char* buffer, uint32_t* size, uint32_t data)
{
   int i;
   uint8_t shiftby = 32;
   uint32_t index = *size;
   *size += 8;
   for (i = 0; i < 8; i++)
      buffer[index++] = (data >> (shiftby -= 4)) & 0xF;
}
