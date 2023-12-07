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
 * Database.c
 * ----------------------------------------------------------------------------
 */
#include "Database.h"
#include "Cartridge.h"

#ifdef WIN32
  #define strcasecmp stricmp  
  #define strncasecmp strnicmp
#endif

#define MAPPER_EXRAM 0x0001  /* 16K ram */
#define MAPPER_EXRAM_A8 0x0002  /* 2K mirror */
#define MAPPER_EXRAM_M2 0x0004  /* halt ram */
#define MAPPER_EXROM 0x0008  /* extra 16K rom */
#define MAPPER_EXFIX 0x0010  /* lastbank-1 rom */
#define MAPPER_EXRAM_X2 0x0020  /* page ram */
#define MAPPER_BANKSET 0x0040  /* halt rom */

#define AUDIO_POKEY_440 0x0001
#define AUDIO_POKEY_450 0x0002
#define AUDIO_POKEY_440_450 0x0004
#define AUDIO_POKEY_800 0x0008
#define AUDIO_POKEY_4000 0x0010
#define AUDIO_ADPCM_420 0x0020
#define AUDIO_COVOX_430 0x0040
#define AUDIO_YM2151_460 0x0080
#define AUDIO_BUPCHIP 0x0100
#define AUDIO_POKEY1_IRQ 0x0200
#define AUDIO_POKEY2_IRQ 0x0400
#define AUDIO_YM2151_IRQ 0x0800

void database_Initialize(void) { }

typedef struct cartridge_db
{
   char digest[256];
   char title[256];
   uint8_t type;
   uint32_t mapper;
   uint32_t audio;
} cartridge_db_t;


/* auto-detect fail */
static const struct cartridge_db db_list[] = 
{
   {
      "8fc3a695eaea3984912d98ed4a543376",
      "Ballblazer",
      CARTRIDGE_TYPE_LINEAR,
      0,
      AUDIO_POKEY_4000
   },
   {
      "b558814d54904ce0582e2f6a801d03af",
      "Ballblazer (Europe)",
      CARTRIDGE_TYPE_LINEAR,
      0,
      AUDIO_POKEY_4000
   },
   {
      "2e8e28f6ad8b9b9267d518d880c73ebb",
      "Commando",
      CARTRIDGE_TYPE_SUPERGAME,
      0,
      AUDIO_POKEY_4000
   },
   {
      "55da6c6c3974d013f517e725aa60f48e",
      "Commando (Europe)",
      CARTRIDGE_TYPE_SUPERGAME,
      0,
      AUDIO_POKEY_4000
   },
   {
      "543484c00ba233736bcaba2da20eeea9",
      "Double Dragon",
      CARTRIDGE_TYPE_ACTIVISION,
   },
   {
      "de2ebafcf0e37aaa9d0e9525a7f4dd62",
      "Double Dragon (Europe)",
      CARTRIDGE_TYPE_ACTIVISION,
   },
   {
      "2251a6a0f3aec84cc0aff66fc9fa91e8",
      "F-18 Hornet",
      CARTRIDGE_TYPE_ABSOLUTE,
   },
   {
      "e7709da8e49d3767301947a0a0b9d2e6",
      "F-18 Hornet (Europe)",
      CARTRIDGE_TYPE_ABSOLUTE,
   },
   {
      "baebc9246c087e893dfa489632157180",
      "Impossible Mission",
      CARTRIDGE_TYPE_SUPERGAME,
      MAPPER_EXRAM
   },
   {
      "80dead01ea2db5045f6f4443faa6fce8",
      "Impossible Mission (Europe)",
      CARTRIDGE_TYPE_SUPERGAME,
      MAPPER_EXRAM
   },
   {
      "045fd12050b7f2b842d5970f2414e912",
      "Jinks",
      CARTRIDGE_TYPE_SUPERGAME,
      MAPPER_EXRAM
   },
   {
      "dfb86f4d06f05ad00cf418f0a59a24f7",
      "Jinks (Europe)",
      CARTRIDGE_TYPE_SUPERGAME,
      MAPPER_EXRAM
   },
   {
      "86546808dc60961cdb1b20e761c50ab1",
      "Plutos",
      CARTRIDGE_TYPE_SUPERGAME,
      MAPPER_EXRAM
   },
   {
      "1745feadabb24e7cefc375904c73fa4c",
      "Possible Mission",
      CARTRIDGE_TYPE_SUPERGAME,
      MAPPER_EXRAM
   },
   {
      "ac03806cef2558fc795a7d5d8dba7bc0",
      "Rampage",
      CARTRIDGE_TYPE_ACTIVISION
   },
   {
      "8f7eb10ad0bd75474abf0c6c36c08486",
      "Rescue On Fractalus",
      CARTRIDGE_TYPE_LINEAR,
      MAPPER_EXRAM_A8
   },
   {
      "592be737ce78a17a572d3bbd527c7a61",
      "Rikki & Vikki (R12)",
      CARTRIDGE_TYPE_SOUPER,
      0,
      AUDIO_BUPCHIP
   },
   {
      "79d3fb83577cd3fd8d1542f58353cfcd",
      "Rikki & Vikki (R13)",
      CARTRIDGE_TYPE_SOUPER,
      0,
      AUDIO_BUPCHIP
   },
   {
      "b3bc889e9cc498636990c5a4d980e85c",
      "Rikki & Vikki (R14)",
      CARTRIDGE_TYPE_SOUPER,
      0,
      AUDIO_BUPCHIP
   },
   {
      "2d643ac548c40e58c99d0fe433ba4ba0",
      "Sirius",
      CARTRIDGE_TYPE_SUPERGAME,
      MAPPER_EXRAM
   },
   {
      "cbb0746192540a13b4c7775c7ce2021f",
      "Summer Games",
      CARTRIDGE_TYPE_SUPERGAME,
      MAPPER_EXRAM
   },
   {
      "8d64763db3100aadc552db5e6868506a",
      "Tower Toppler",
      CARTRIDGE_TYPE_SUPERGAME,
      MAPPER_EXRAM
   },
   {
      "32a37244a9c6cc928dcdf02b45365aa8",
      "Tower Toppler (Europe)",
      CARTRIDGE_TYPE_SUPERGAME,
      MAPPER_EXRAM
   },
   {
      "3799d72f78dda2ee87b0ef8bf7b91186",
      "Winter Games",
      CARTRIDGE_TYPE_SUPERGAME,
      MAPPER_EXRAM
   },
};

static void fixup(int index)
{
   uint32_t flags;


   cartridge_type = db_list[index].type;


   flags = db_list[index].mapper;

   if (flags & MAPPER_BANKSET)
      cartridge_bankset = 1;

   if (flags & MAPPER_EXFIX)
      cartridge_exfix = 1;

   if (flags & MAPPER_EXROM)
      cartridge_exrom = 1;

   if (flags & MAPPER_EXRAM)
      cartridge_exram = 1;

   if (flags & MAPPER_EXRAM_M2)
      cartridge_exram_m2 = 1;

   if (flags & MAPPER_EXRAM_A8)
      cartridge_exram_a8 = 1;

   if (flags & MAPPER_EXRAM_X2)
      cartridge_exram_x2 = 1;



   flags = db_list[index].audio;

   if (flags & AUDIO_POKEY_440)
      cartridge_pokey = POKEY_AT_440;

   if (flags & AUDIO_POKEY_450)
      cartridge_pokey = POKEY_AT_450;

   if (flags & AUDIO_POKEY_440_450)
      cartridge_pokey = POKEY_AT_440_450;

   if (flags & AUDIO_POKEY_800)
      cartridge_pokey = POKEY_AT_800;

   if (flags & AUDIO_POKEY_4000)
      cartridge_pokey = POKEY_AT_4000;

   if (flags & AUDIO_YM2151_460)
      cartridge_ym2151 = YM2151_AT_460;

   if (flags & AUDIO_BUPCHIP)
      cartridge_bupchip = 1;


   if (strstr(db_list[index].title, "(PAL)") ||
	   strstr(db_list[index].title, "(Europe)"))
      cartridge_region = 1;
}

static void detect(void)
{
   if (strstr(cartridge_title, "(PAL)") ||
	   strstr(cartridge_title, "(Europe)"))
      cartridge_region = 1;


   if (cartridge_size < 0xe000)
   {
      cartridge_type = CARTRIDGE_TYPE_LINEAR;
      return;
   }


   cartridge_type = CARTRIDGE_TYPE_SUPERGAME;

   cartridge_exfix = 1;  /* common default */

   if (cartridge_size == 0x20000 + 0x4000)
      cartridge_exrom = 1;

   /* ignore exram map as causes problems enough times */
}

void database_Load(const char *digest)
{
   unsigned i;
   size_t len = sizeof(db_list) / sizeof(db_list[0]);

   for (i = 0; i < len; i++)
   {
      if (!strcasecmp(db_list[i].digest, digest))
      {
         fixup(i);
         return;
      }
   }

   detect();
}
