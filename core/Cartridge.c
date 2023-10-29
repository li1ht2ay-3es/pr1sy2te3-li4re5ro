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
 * Cartridge.c
 * ----------------------------------------------------------------------------
 */
#include "Cartridge.h"
#include "Equates.h"
#include "Memory.h"
#include "Hash.h"
#include "Pokey.h"
#include "BupChip.h"
#include <streams/file_stream.h>
#include <stdlib.h>
#include <string.h>

char cartridge_digest[33];
int cartridge_type;
int cartridge_region;
int cartridge_pokey;
int cartridge_controller[2];
int cartridge_bank;
int cartridge_flags;
int cartridge_bupchip;

/* SOUPER-specific stuff, used for "Rikki & Vikki" */
int cartridge_souper_chr_bank[2];
int cartridge_souper_mode;
int cartridge_souper_ram_page_bank[2];

uint8_t* cartridge_buffer      = NULL;
static uint32_t cartridge_size = 0;

uint8_t ex_ram_buffer[0x8000];
uint8_t banksets_memory[64*1024];

char* cartridge_GetNextNonemptyLine(const char **stream, size_t* size)
{
   while(*size != 0)
   {
      char *line_buffer;
      const char *end;
      const char* line = *stream;
      while(*size > 0 && **stream != '\r' && **stream != '\n')
      {
         (*stream)++;
         (*size)--;
      }

      /* Skip CR/LF. */
      end = *stream;
      while(*size > 0 && (**stream == '\r' || **stream == '\n'))
      {
         (*stream)++;
         (*size)--;
      }

      if(line == end || line[0] == '\n' || line[0] == '\r')
         continue;

      line_buffer = (char*)malloc(end - line + 1);
      memcpy(line_buffer, line, end - line);
      line_buffer[end - line] = '\0';
      return line_buffer;
   }

   return NULL;
}

bool cartridge_ReadFile(uint8_t** outData, size_t* outSize, const char* subpath, const char* relativeTo)
{
   int64_t len    = 0;
   size_t pathLen = strlen(subpath) + strlen(relativeTo) + 1;
   char* path     = (char*)malloc(pathLen + 1);
#ifdef _WIN32
   char pathSeparator = '\\';
#else
   char pathSeparator = '/';
#endif
   sprintf(path, "%s%c%s", relativeTo, pathSeparator, subpath);

   filestream_read_file(path, (void**)outData, &len);
   *outSize = (size_t)len;
   return len > 0;
}

static bool cartridge_HasHeader(const uint8_t* header)
{
   unsigned index;
   const char HEADER_ID[ ] = {"ATARI7800"};

   for (index = 0; index < 9; index++)
   {
      if(HEADER_ID[index] != header[index + 1])
         return false;
   }
   return true;
}

/* ----------------------------------------------------------------------------
 * Header for CC2 hack
 * ----------------------------------------------------------------------------
 */
static bool cartridge_CC2(const uint8_t* header)
{
   unsigned index;
   const char HEADER_ID[ ] = {">>"};

   for (index = 0; index < 2; index++)
   {
      if(HEADER_ID[index] != header[index+1])
         return false;
   }
   return true;
}

static uint32_t cartridge_GetBankOffset(uint8_t bank)
{
   if (
         (
          cartridge_type == CARTRIDGE_TYPE_SUPERCART || 
          cartridge_type == CARTRIDGE_TYPE_SUPERCART_ROM || 
          cartridge_type == CARTRIDGE_TYPE_SUPERCART_RAM) && cartridge_size <= 0x10000
      )
   {
      /* for some of these carts, there are only 4 banks. in this case we ignore bit 3
       * previously, games of this type had to be doubled. The first 4 banks needed to be duplicated at the end of the ROM */
      return (bank & 3) * 0x4000;
   }

   return bank * 0x4000;
}

static void cartridge_WriteBank(uint16_t address, uint8_t bank)
{
  uint32_t offset = cartridge_GetBankOffset(bank);

  if (offset < cartridge_size)
  {
    memory_WriteROM(address, 0x4000, cartridge_buffer + offset);
    cartridge_bank = bank;
  }
}

static void cartridge_souper_StoreChrBank(uint8_t page, uint8_t bank)
{
   if (page < 2)
      cartridge_souper_chr_bank[page] = bank;
}

static void cartridge_souper_SetMode(uint8_t data)
{
   cartridge_souper_mode = data;
}

static void cartridge_souper_SetRamPageBank(uint8_t which, uint8_t data)
{
   if (which < 2)
      cartridge_souper_ram_page_bank[which] = data & 7;
}

static void cartridge_ReadHeader(const uint8_t* header)
{
   uint16_t cardtype;

   cartridge_size  = header[49] << 24;
   cartridge_size |= header[50] << 16;
   cartridge_size |= header[51] << 8;
   cartridge_size |= header[52];

/*
   -------------------------------------------------------
   A78 Card Type is a 16-bit word at header offset 53+54
   -------------------------------------------------------
   bit 0     = pokey at $4000
   bit 1     = supergame bank switched
   bit 2     = ram at $4000
   bit 3     = rom at $4000
    
   bit 4     = second to last bank at $4000
   bit 5     = banked ram
   bit 6     = pokey at $450
   bit 7     = mirror ram at $4000
  
   bit 8     = activision banking
   bit 9     = absolute banking
   bit 10    = pokey at $440
   bit 11    = ym2151 at $460/$461
    
   bit 12    = souper
   bit 13    = banksets
   bit 14    = halt banked ram
   bit 15    = pokey at $800
*/

   cardtype = (header[53] << 8) | header[54];

   cartridge_type = (cartridge_size <= 0x10000) ? CARTRIDGE_TYPE_NORMAL : CARTRIDGE_TYPE_SUPERCART;

   if(cardtype & 0x0002) cartridge_type  = CARTRIDGE_TYPE_SUPERCART;
   if(cardtype & 0x0004) cartridge_type  = ((cardtype & 0x0002) ? CARTRIDGE_TYPE_SUPERCART_RAM : CARTRIDGE_TYPE_FLAT_WITH_RAM);
   if(cardtype & 0x0008) cartridge_type  = CARTRIDGE_TYPE_SUPERCART_LARGE;
   if(cardtype & 0x0010) cartridge_type  = CARTRIDGE_TYPE_SUPERCART_ROM;
   if(cardtype & 0x0020) cartridge_type  = CARTRIDGE_TYPE_SUPERCART_RAMX2;
   if(cardtype & 0x0100) cartridge_type  = CARTRIDGE_TYPE_ACTIVISION;
   if(cardtype & 0x0200) cartridge_type  = CARTRIDGE_TYPE_ABSOLUTE;
   if(cardtype & 0x1000) cartridge_type  = CARTRIDGE_TYPE_SOUPER;
   if(cardtype & 0x2000)  /* Banksets */
   {
      cartridge_type = CARTRIDGE_TYPE_BANKSETS;                                   /* Default to Banksets (no RAM) */
      if(cardtype & 0x0004) cartridge_type  = CARTRIDGE_TYPE_BANKSETS_RAM;        /* RAM @ 4000 enabled */
      if(cardtype & 0x4000) cartridge_type  = CARTRIDGE_TYPE_BANKSETS_HALTRAM;    /* Banked Halt RAM @ 4000 enabled */
   }

   cartridge_pokey = POKEY_NONE;
   if(cardtype & 0x0001) cartridge_pokey = POKEY_AT_4000;
   if(cardtype & 0x0040) cartridge_pokey = POKEY_AT_450;
   if(cardtype & 0x8000) cartridge_pokey = POKEY_AT_800;

/*
    // ========================
    // 0 = none
    // 1 = 7800 joystick
    // 2 = lightgun
    // 3 = paddle
    // 4 = trakball
    // 5 = 2600 joystick
    // 6 = 2600 driving
    // 7 = 2600 keypad
    // 8 = ST mouse
    // 9 = Amiga mouse
    // 10 = AtariVox/SaveKey
    // 11 = SNES2Atari
    // ========================
  switch(header[55])
  {
      case 1:
          myCartInfo.cardctrl1 = CARTRIDGE_CONTROLLER_JOYSTICK;
          break;
      case 2:
          myCartInfo.cardctrl1 = CARTRIDGE_CONTROLLER_LIGHTGUN;
          break;
      case 3:
          myCartInfo.cardctrl1 = CARTRIDGE_CONTROLLER_PADDLES;
          break;
      case 11:
          myCartInfo.cardctrl1 = CARTRIDGE_CONTROLLER_SNES2ATARI;
          break;
      default:
          myCartInfo.cardctrl1 = CARTRIDGE_CONTROLLER_JOYSTICK;
          break;
  }

  switch(header[56])
  {
      case 1:
          myCartInfo.cardctrl2 = CARTRIDGE_CONTROLLER_JOYSTICK;
          break;
      case 2:
          myCartInfo.cardctrl2 = CARTRIDGE_CONTROLLER_LIGHTGUN;
          break;
      case 3:
          myCartInfo.cardctrl2 = CARTRIDGE_CONTROLLER_PADDLES;
          break;
      case 11:
          myCartInfo.cardctrl2 = CARTRIDGE_CONTROLLER_SNES2ATARI;
          break;
      default:
          myCartInfo.cardctrl2 = CARTRIDGE_CONTROLLER_JOYSTICK;
          break;
  }
    
  myCartInfo.region = header[57] & 1;
  myCartInfo.hsc = ((header[58] & 1) ? HSC_YES:HSC_NO);
  last_bank = 255;
  last_ex_ram_bank = 0;
  ex_ram_bank = 0;
  last_ex_ram_bank_df = 0;
  write_only_pokey_at_4000 = false;
  ex_ram_bank_df = 0;
*/

   cartridge_controller[0] = header[55];
   cartridge_controller[1] = header[56];
   cartridge_region        = header[57];
   cartridge_flags         = 0;
   cartridge_bupchip       = cartridge_type == CARTRIDGE_TYPE_SOUPER;
}

uint8_t cartridge_LoadROM(uint32_t address)
{
   if(address >= cartridge_size)
      return 0;
   return cartridge_buffer[address];
}

bool cartridge_LoadFromCDF(const char* data, size_t size,
		const char *workingDir)
{
   static const char *cartridgeTypes[ ] = {
      "EMPTY",
      "SUPER",
      NULL,
      NULL,
      NULL,
      "ABSOLUTE",
      "ACTIVISION",
      "SOUPER",
   };
   int i;
   char* line;
   size_t cart_size;
   if((line = cartridge_GetNextNonemptyLine(&data, &size)) == NULL)
      return false;
   if (strcmp(line, "ProSystem") != 0)
      return false;
   free(line);

   if((line = cartridge_GetNextNonemptyLine(&data, &size)) == NULL)
      return false;
   for(i = 0; i < sizeof(cartridgeTypes) / sizeof(cartridgeTypes[0]); i++)
   {
      if(cartridgeTypes[i] != NULL && strcmp(line, cartridgeTypes[i]) == 0)
      {
         cartridge_type = i;
         break;
      }
   }
   free(line);

   if((line = cartridge_GetNextNonemptyLine(&data, &size)) == NULL)
      return false;
   /* Just ignore the cartridge title in `libretro`. */
   free(line);

   /* Read binary file. */
   if((line = cartridge_GetNextNonemptyLine(&data, &size)) == NULL)
      return false;
   if(!cartridge_ReadFile(&cartridge_buffer, &cart_size, line, workingDir))
      return false;
   free(line);

   cartridge_size = (uint32_t)cart_size;
   hash_Compute(cartridge_digest, cartridge_buffer, cart_size);

   cartridge_bupchip = false;
   if((line = cartridge_GetNextNonemptyLine(&data, &size)))
   {
      cartridge_bupchip = strcmp(line, "CORETONE") == 0;
      free(line);
   }

   if(cartridge_bupchip)
   {
      if(!bupchip_InitFromCDF(&data, &size, workingDir))
      {
         free(cartridge_buffer);
         return false;
      }
   }

   if (!cartridge_Load(false, cartridge_buffer, cart_size))
   {
      free(cartridge_buffer);
      return false;
   }

   return true;
}

bool cartridge_Load(bool persistent_data, const uint8_t* data, uint32_t size)
{
   int index;
   uint32_t offset     = 0;
   uint8_t header[128] = {0};

   /* Cartridge data is invalid. */
   if(size <= 128)
      return false;

   for(index = 0; index < 128; index++)
      header[index] = data[index];

   /* Prosystem doesn't support CC2 hacks. */
   if (cartridge_CC2(header))
      return false;

   if(cartridge_HasHeader(header))
   {
      cartridge_ReadHeader(header);
      size   -= 128;
      offset  = 128;
   }

   cartridge_size      = size;
   if (persistent_data)
      cartridge_buffer = (uint8_t*)data + offset;
   else
   {
      cartridge_buffer = (uint8_t*)malloc(cartridge_size * sizeof(uint8_t));

      for(index = 0; index < cartridge_size; index++)
         cartridge_buffer[index] = data[index + offset];
   }

   hash_Compute(cartridge_digest, cartridge_buffer, cartridge_size);

   return true;
}

/*
------------------------------------------------------------------------------------------
Here are the main 7800 Bankswitching schemes (ignoring Absolute, Activistion and Fractalus):

NORMAL           Anything 48K or less... fits into memory (0xffff downwards) without switching.
SUPERCART        Games that are 128+K in size with nothing mapped in at 0x4000
SUPERCART_LARGE  Games that are 144+K in size with the extra 16K bank 0 fixed at 0x4000
SUPERCART_RAM    Games that are 128+K in size with extra 16K of RAM at 0x4000
SUPERCART_ROM    Games that are 128+K in size with the second-to-last bank fixed at 0x4000

For the "Super Carts" the 16K at 0xC000 is the last bank in the ROM.
For the "Super Carts" the 16K at 0x8000 is the bankswapping bank and is switched by writing
the bank # to any address in that region.  For Supercart "Large" there are actually two
chips (16K fixed and 128K bankswapped) and the bank is relative to the 128K chip so emulators
will use (bank+1) to compensate for the extra 16K fixed bank 0 at 0x4000.

In theory, since we can write any bank number 0-255 that would allow up to 255 banks of 16k
which is a whopping 4096K (4 Megabytes) of ROM but in practice carts seem to limit to 512K
or less for practical reasons with a few outstanding tech-demos reaching 1024K. 
------------------------------------------------------------------------------------------
*/

void cartridge_Store(void)
{
   uint32_t offset, lastBank, codesize;

   switch(cartridge_type)
   {
      case CARTRIDGE_TYPE_NORMAL:
         memory_WriteROM(0x10000 - cartridge_size, cartridge_size, cartridge_buffer);
         break;

      case CARTRIDGE_TYPE_FLAT_WITH_RAM:
         memory_WriteROM(0x10000 - cartridge_size, cartridge_size, cartridge_buffer);
         memory_ClearROM(0x4000, 0x4000);
         break;
          
      case CARTRIDGE_TYPE_SUPERCART:
         offset = cartridge_size - 0x4000;
         memory_WriteROM(0xc000, 0x4000, cartridge_buffer + offset);

      case CARTRIDGE_TYPE_SUPERCART_LARGE:
         offset = cartridge_size - 0x4000;
         memory_WriteROM(0xc000, 0x4000, cartridge_buffer + offset);
         memory_WriteROM(0x4000, 0x4000, cartridge_buffer + cartridge_GetBankOffset(0));
         break;

      case CARTRIDGE_TYPE_SUPERCART_RAM:
         offset = cartridge_size - 0x4000;
         memory_WriteROM(0xc000, 16384, cartridge_buffer + offset);
         memory_ClearROM(0x4000, 0x4000);
         break;

      case CARTRIDGE_TYPE_SUPERCART_RAMX2:
         offset = cartridge_size - 0x4000;
         memory_WriteROM(0xc000, 0x4000, cartridge_buffer + offset);
         memory_ClearROM(0x4000, 0x4000);
         break;          
          
      case CARTRIDGE_TYPE_SUPERCART_ROM:
         offset = cartridge_size - 0x4000;
         lastBank = (cartridge_size/0x4000)-1;
         memory_WriteROM(0xc000, 0x4000, cartridge_buffer + offset);        
         memory_WriteROM(0x4000, 0x4000, cartridge_buffer + cartridge_GetBankOffset(lastBank-1));
         break;

      case CARTRIDGE_TYPE_ABSOLUTE:
         memory_WriteROM(0x4000, 0x4000, cartridge_buffer);
         memory_WriteROM(0x8000, 0x8000, cartridge_buffer + cartridge_GetBankOffset(2));
         break;

      case CARTRIDGE_TYPE_ACTIVISION:
         memory_WriteROM(0xa000, 0x4000, cartridge_buffer);
         memory_WriteROM(0x4000, 0x2000, cartridge_buffer + 0x1a000);
         memory_WriteROM(0x6000, 0x2000, cartridge_buffer + 0x18000);
         memory_WriteROM(0x8000, 0x2000, cartridge_buffer + 0x1e000);
         memory_WriteROM(0xe000, 0x2000, cartridge_buffer + 0x1c000);
         break;

      case CARTRIDGE_TYPE_SOUPER:
         memory_WriteROM(0xc000, 0x4000, cartridge_buffer + cartridge_GetBankOffset(31));
         memory_WriteROM(0x8000, 0x4000, cartridge_buffer + cartridge_GetBankOffset(0));
         memory_ClearROM(0x4000, 0x4000);
         break;

      case CARTRIDGE_TYPE_FRACTALUS:
         memory_WriteROM(0x10000 - cartridge_size, cartridge_size, cartridge_buffer);
         memory_ClearROM(0x4000, 0x4000);
         break;
          
      case CARTRIDGE_TYPE_BANKSETS:
         codesize = cartridge_size / 2;
         if (codesize <= (52 * 1024))
         {
            offset = 0;
            memory_WriteROM(0x10000 - codesize, codesize, cartridge_buffer + offset);

            memcpy(&banksets_memory[0x10000 - codesize], cartridge_buffer + codesize, codesize);
            if (codesize >= (48*1024))
               cartridge_pokey = POKEY_AT_4000_W;
         }
         else
         {
            offset = codesize - 0x4000;
            memory_WriteROM(0xc000, 0x4000, cartridge_buffer + offset);
            memcpy(&banksets_memory[0xc000], &cartridge_buffer[cartridge_size - 0x4000], 0x4000);
         }
         break;
          
      case CARTRIDGE_TYPE_BANKSETS_RAM:
      case CARTRIDGE_TYPE_BANKSETS_HALTRAM:
         codesize = cartridge_size / 2;
         if (codesize <= (52 * 1024))
         {
            memory_WriteROM(0x10000 - codesize, codesize, cartridge_buffer);
            memcpy(&banksets_memory[0x10000 - codesize], cartridge_buffer + codesize, codesize);
            if (codesize >= (48*1024))
               cartridge_pokey = POKEY_AT_4000_W;
         }
         else
         {
            memory_WriteROM(0xc000, 0x4000, cartridge_buffer + codesize - 0x4000);
            memcpy(&banksets_memory[0xc000], cartridge_buffer + cartridge_size - 0x4000, 0x4000);
         }
         memory_ClearROM(0x4000, 0x4000);
         memset(&banksets_memory[0x4000], 0x00, 0x4000);
         break;
   }
}

void cartridge_Write(uint16_t address, uint8_t data)
{
   switch(cartridge_type)
   {
      case CARTRIDGE_TYPE_SUPERCART:
      case CARTRIDGE_TYPE_SUPERCART_RAM:
      case CARTRIDGE_TYPE_SUPERCART_ROM:
      case CARTRIDGE_TYPE_SUPERCART_RAMX2:
         if ((address & 0xC000) == 0x8000) // Is this a bankswitching write?
            cartridge_StoreBank(data);

         else if (address == 0xFFFF)
            0; //cartridge_SwapRAM_DragonFlyStyle(data); // For the Dragonfly way of RAM banking
         break;

      case CARTRIDGE_TYPE_SUPERCART_LARGE:
         if(address >= 0x8000 && address < 0xc000 && data < 9)
            cartridge_StoreBank(data + 1);
         break;

      case CARTRIDGE_TYPE_ABSOLUTE:
         if(address == 0x8000 && (data == 1 || data == 2))
            cartridge_StoreBank(data - 1);
         break;

      case CARTRIDGE_TYPE_ACTIVISION:
         if(address >= 0xff80)
            cartridge_StoreBank(address & 7);
         break;

      case CARTRIDGE_TYPE_SOUPER:
         if(address >= 0x4000 && address < 0x8000)
         {
            memory_souper_ram[memory_souper_GetRamAddress(address)] = data;
            break;
         }

         switch(address)
         {
            case CARTRIDGE_SOUPER_BANK_SEL:
               cartridge_StoreBank(data & 31);
               break;

            case CARTRIDGE_SOUPER_CHR_A_SEL:
               cartridge_souper_StoreChrBank(0, data);
               break;

            case CARTRIDGE_SOUPER_CHR_B_SEL:
               cartridge_souper_StoreChrBank(1, data);
               break;

            case CARTRIDGE_SOUPER_MODE_SEL:
               cartridge_souper_SetMode(data);
               break;

            case CARTRIDGE_SOUPER_EXRAM_V_SEL:
               cartridge_souper_SetRamPageBank(0, data);
               break;

            case CARTRIDGE_SOUPER_EXRAM_D_SEL:
               cartridge_souper_SetRamPageBank(1, data);
               break;

            case CARTRIDGE_SOUPER_AUDIO_CMD:
               bupchip_ProcessAudioCommand(data);
               break;
         }
         break;

      case CARTRIDGE_TYPE_BANKSETS:
      case CARTRIDGE_TYPE_BANKSETS_RAM:
         if ((address & 0xc000) == 0x8000) // Is this a bankswitching write?
         {
            // We need to swap in the main Sally memory...
            cartridge_StoreBank(data);

			// And also swap in the Maria memory... this ROM starts half-way up the main cartridge_buffer[]
            uint32_t offset = (cartridge_size/2) + (data*0x4000);
            memcpy(&banksets_memory[0x8000], &cartridge_buffer[offset], 0x4000);
         }
         break;
          
      case CARTRIDGE_TYPE_BANKSETS_HALTRAM:
         if ((address & 0xc000) == 0x8000) // Is this a bankswitching write?
         {
            // We need to swap in the main Sally memory...
            cartridge_StoreBank(data);

            // And also swap in the Maria memory... this ROM starts half-way up the main cartridge_buffer[]
            uint32_t offset = (cartridge_size/2) + (data*0x4000);
            memcpy(&banksets_memory[0x8000], &cartridge_buffer[offset], 0x4000);
         }
         else if ((address & 0xc000) == 0xc000) // Are we writing to MARIA HALT RAM?
         {
            // Write the data into the 0x4000-0x7FFF region - for Sally, this is write only but will be seen by Maria
            banksets_memory[0x4000 + (address & 0x3FFF)] = data;
         }
         break;
   }
}

void cartridge_StoreBank(uint8_t bank)
{
   switch(cartridge_type)
   {
      case CARTRIDGE_TYPE_SUPERCART:
      case CARTRIDGE_TYPE_SUPERCART_RAM:
      case CARTRIDGE_TYPE_SUPERCART_ROM:
      case CARTRIDGE_TYPE_SUPERCART_RAMX2:
      case CARTRIDGE_TYPE_SUPERCART_LARGE:
         cartridge_WriteBank(0x8000, bank);
         break;

      case CARTRIDGE_TYPE_ABSOLUTE:
         cartridge_WriteBank(0x4000, bank);
         break;

      case CARTRIDGE_TYPE_ACTIVISION:
         cartridge_WriteBank(0xa000, bank);
         break;

      case CARTRIDGE_TYPE_SOUPER:
         cartridge_WriteBank(0x8000, bank);
         break;

      case CARTRIDGE_TYPE_BANKSETS:
      case CARTRIDGE_TYPE_BANKSETS_RAM:
      case CARTRIDGE_TYPE_BANKSETS_HALTRAM:
         cartridge_WriteBank(0x8000, bank);
         break;
   }
}

bool cartridge_IsLoaded(void)
{
  return (cartridge_buffer != NULL) ? true: false;
}

void cartridge_Release(bool persistent_data)
{
   if (!persistent_data)
   {
      if (cartridge_buffer)
         free(cartridge_buffer);
   }
   cartridge_buffer = NULL;
   cartridge_size   = 0;
}
