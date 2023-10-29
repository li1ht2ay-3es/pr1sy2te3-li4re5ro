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
 * Maria.c
 * ----------------------------------------------------------------------------
 */
#include "Maria.h"
#include "Equates.h"
#include "Pair.h"
#include "Memory.h"
#include "Sally.h"
#include "Cartridge.h"
#include "ProSystem.h"

#define MARIA_LINERAM_SIZE 160
#define MARIA_CYCLE_LIMIT 430

rect maria_displayArea = {0, 16, 319, 258};
rect maria_visibleArea = {0, 26, 319, 250};

uint8_t maria_surface[MARIA_SURFACE_SIZE] = {0};
uint32_t maria_scanline = 0;

static uint8_t maria_lineRAM[MARIA_LINERAM_SIZE];
static int maria_cycles;

static pair maria_dpp;
static pair maria_dp;
static pair maria_pp;

static uint8_t maria_horizontal;
static uint8_t maria_palette;
static int8_t maria_offset;
static uint16_t maria_holey;
static uint8_t maria_wmode;
static uint8_t maria_ctrl;
static uint8_t maria_nmi;
static uint8_t maria_dma;

static uint8_t maria_ReadByte(uint16_t address)
{
   uint32_t page, chrOffset;

   if ((address >= 0x20) && (address < 0x40))  /* internal MARIA register */
      return memory_ram[address];

   if (cartridge_type != CARTRIDGE_TYPE_SOUPER)
      return memory_ram[address];


   /* Souper */

   if (((cartridge_souper_mode & CARTRIDGE_SOUPER_MODE_MFT) == 0) || (address < 0x8000) ||
      (((cartridge_souper_mode & CARTRIDGE_SOUPER_MODE_CHR) == 0) && (address < 0xc000)))
      return memory_Read(address);

   if (address >= 0xC000) /* EXRAM */
      return memory_Read(address - 0x8000);

   if (address < 0xA000)  /* Fixed ROM */
      return memory_Read(address + 0x4000);


   page      = (uint16_t)cartridge_souper_chr_bank[((address & 0x80) != 0) ? 1 : 0];
   chrOffset = (((page & 0xFE) << 4) | (page & 1)) << 7;
   return cartridge_LoadROM((address & 0x0F7F) | chrOffset);
}

static void maria_StoreCell(uint8_t data)
{
   if (maria_horizontal < MARIA_LINERAM_SIZE)
   {
      if (data)
         maria_lineRAM[maria_horizontal] = maria_palette | data;

      else if (maria_ctrl & 0x04)  /* kangaroo */
         maria_lineRAM[maria_horizontal] = 0;
   }

   maria_horizontal++;
}

static void maria_StoreCell2(uint8_t high, uint8_t low)
{
   if (maria_horizontal < MARIA_LINERAM_SIZE)
   {
      if (low || high)
         maria_lineRAM[maria_horizontal] = (maria_palette & 0x10) | high | low;

      else if (maria_ctrl & 0x04)  /* kangaroo */
         maria_lineRAM[maria_horizontal] = 0;
   }

   maria_horizontal++;
}

static bool maria_IsHoleyDMA(uint16_t address)
{
   if (address & 0x8000)
   {
      if (address & maria_holey)  /* 800 (h08) - 1000 (h16) */
         return true;
   }

   return false;
}

static uint8_t maria_GetColor(uint8_t data)
{
   return maria_ReadByte(BACKGRND + ((data & 3) ? data : 0));
}

static void maria_StoreGraphic(uint8_t data, bool is_holey)
{
   if (!is_holey)
   {
      if (maria_wmode)
      {
         maria_StoreCell2((data & 0x0C) >> 0, (data & 0xC0) >> 6);
         maria_StoreCell2((data & 0x30) >> 4, (data & 0x03) << 2);
      }

      else
      {
         maria_StoreCell((data & 0xC0) >> 6);
         maria_StoreCell((data & 0x30) >> 4);
         maria_StoreCell((data & 0x0C) >> 2);
         maria_StoreCell((data & 0x03) >> 0);
      }
   }

   else  /* is_holey */
      maria_horizontal += maria_wmode ? 2 : 4;
}

static void maria_WriteLineRAM(uint8_t* buffer)
{
   uint8_t rmode = maria_ctrl & 3;  /* screen mode */
   uint8_t colormask = (maria_ctrl & 0x80) ? 0x0F : 0xFF;  /* colorkill */


   if (rmode == 0 || rmode == 1)  /* 160 A-B */
   {
      int pixel = 0, index;

      for (index = 0; index < MARIA_LINERAM_SIZE; index += 4)
      {
         uint8_t color;

         color = maria_GetColor(maria_lineRAM[index + 0]) & colormask;
         buffer[pixel++] = color;
         buffer[pixel++] = color;

         color = maria_GetColor(maria_lineRAM[index + 1]) & colormask;
         buffer[pixel++] = color;
         buffer[pixel++] = color;

         color = maria_GetColor(maria_lineRAM[index + 2]) & colormask;
         buffer[pixel++] = color;
         buffer[pixel++] = color;

         color = maria_GetColor(maria_lineRAM[index + 3]) & colormask;
         buffer[pixel++] = color;
         buffer[pixel++] = color;
      }
   }

   else if (rmode == 2)  /* 320 B-D */
   {
      int pixel = 0, index;

      for (index = 0; index < MARIA_LINERAM_SIZE; index += 4)
      {
         buffer[pixel++] = maria_GetColor((maria_lineRAM[index + 0] & 16) | ((maria_lineRAM[index + 0] & 8) >> 3) | ((maria_lineRAM[index + 0] & 2) << 0)) & colormask;
         buffer[pixel++] = maria_GetColor((maria_lineRAM[index + 0] & 16) | ((maria_lineRAM[index + 0] & 4) >> 2) | ((maria_lineRAM[index + 0] & 1) << 1)) & colormask;

         buffer[pixel++] = maria_GetColor((maria_lineRAM[index + 1] & 16) | ((maria_lineRAM[index + 1] & 8) >> 3) | ((maria_lineRAM[index + 1] & 2) << 0)) & colormask;
         buffer[pixel++] = maria_GetColor((maria_lineRAM[index + 1] & 16) | ((maria_lineRAM[index + 1] & 4) >> 2) | ((maria_lineRAM[index + 1] & 1) << 1)) & colormask;

         buffer[pixel++] = maria_GetColor((maria_lineRAM[index + 2] & 16) | ((maria_lineRAM[index + 2] & 8) >> 3) | ((maria_lineRAM[index + 2] & 2) << 0)) & colormask;
         buffer[pixel++] = maria_GetColor((maria_lineRAM[index + 2] & 16) | ((maria_lineRAM[index + 2] & 4) >> 2) | ((maria_lineRAM[index + 2] & 1) << 1)) & colormask;

         buffer[pixel++] = maria_GetColor((maria_lineRAM[index + 3] & 16) | ((maria_lineRAM[index + 3] & 8) >> 3) | ((maria_lineRAM[index + 3] & 2) << 0)) & colormask;
         buffer[pixel++] = maria_GetColor((maria_lineRAM[index + 3] & 16) | ((maria_lineRAM[index + 3] & 4) >> 2) | ((maria_lineRAM[index + 3] & 1) << 1)) & colormask;
      }
   }
 
   else if (rmode == 3)  /* 320 A-C */
   {
      int pixel = 0, index;

      for (index = 0; index < MARIA_LINERAM_SIZE; index += 4)
      {
         buffer[pixel++] = maria_GetColor((maria_lineRAM[index + 0] & 0x1E)) & colormask;
         buffer[pixel++] = maria_GetColor((maria_lineRAM[index + 0] & 0x1C) | ((maria_lineRAM[index + 0] & 1) << 1)) & colormask;

         buffer[pixel++] = maria_GetColor((maria_lineRAM[index + 1] & 0x1E)) & colormask;
         buffer[pixel++] = maria_GetColor((maria_lineRAM[index + 1] & 0x1C) | ((maria_lineRAM[index + 1] & 1) << 1)) & colormask;

         buffer[pixel++] = maria_GetColor((maria_lineRAM[index + 2] & 0x1E)) & colormask;
         buffer[pixel++] = maria_GetColor((maria_lineRAM[index + 2] & 0x1C) | ((maria_lineRAM[index + 2] & 1) << 1)) & colormask;

         buffer[pixel++] = maria_GetColor((maria_lineRAM[index + 3] & 0x1E)) & colormask;
         buffer[pixel++] = maria_GetColor((maria_lineRAM[index + 3] & 0x1C) | ((maria_lineRAM[index + 3] & 1) << 1)) & colormask;
      }
   }
}

static void maria_StoreLineRAM(void)
{
   uint32_t index;
   uint8_t cwidth = maria_ctrl & 0x10;
   uint8_t chigh = maria_ReadByte(CHARBASE) + maria_offset;
   pair list_dp = maria_dp;


   for (index = 0; index < MARIA_LINERAM_SIZE; index++)
      maria_lineRAM[index] = 0;


   if ((maria_ctrl & 0x60) != 0x40) return;  /* dma off */


   maria_cycles += 16;  /* dma startup + shutdown time */

   while (maria_cycles < MARIA_CYCLE_LIMIT)  /* render list */
   {
      uint8_t width, mode;
      uint8_t indirect = 0;
      bool skip_holey = false;


      maria_pp.b.l = maria_ReadByte(list_dp.w++);
      mode = maria_ReadByte(list_dp.w++);

      if ((mode & 0x5F) == 0) break;  /* list end */

      maria_pp.b.h = maria_ReadByte(list_dp.w++);


      if ((mode & 0x1F) == 0)  /* extended header */
      { 
         indirect = mode & 0x20;
         maria_wmode = mode & 0x80;

         mode = maria_ReadByte(list_dp.w++);
         maria_cycles += 2;
      }


      maria_pp.b.h += (!indirect) ? maria_offset : 0;  /* direct graphics mode */

      maria_palette = (mode & 0xE0) >> 3;
      width = ((mode ^ 0xFF) & 0x1F) + 1;

      maria_horizontal = maria_ReadByte(list_dp.w++);
      maria_cycles += 8;


      for (index = 0; index < width; index++)
      {
         uint8_t data1, data2;
         bool is_holey;


			if (maria_cycles >= MARIA_CYCLE_LIMIT) break;


         if (!indirect)
         {
				data1 = maria_ReadByte(maria_pp.w);
            is_holey = maria_IsHoleyDMA(maria_pp.w++);
         }

         else  /* indirect load */
         {
            pair new_pp;

            new_pp.b.l = maria_ReadByte(maria_pp.w++);  /* load real tile address */
            new_pp.b.h = chigh;

            is_holey = maria_IsHoleyDMA(new_pp.w);

            data1 = maria_ReadByte(new_pp.w++);
            data2 = cwidth ? maria_ReadByte(new_pp.w++) : 0;

            maria_cycles += (!is_holey) ? 3 : 0;  /* memory read penalty */
         }


         if (is_holey)
         {
            maria_cycles += (!skip_holey) ? 3 : 0;  /* 1-time zone skip penalty */
            skip_holey = true;
         }


         maria_StoreGraphic(data1, is_holey);
         maria_cycles += (!is_holey) ? 3 : 0;


         if (indirect && cwidth)  /* 2 data bytes per map byte */
			{
            maria_StoreGraphic(data2, is_holey);
            maria_cycles += (!is_holey) ? 3 : 0;
         }
      }
   }
}

static void maria_LoadDisplayList(void)
{
   uint8_t mode;

   if (!maria_dma) return;


   if (maria_dpp.w == 0)  /* list start */
	{
      maria_dpp.b.h = maria_ReadByte(DPPH);
      maria_dpp.b.l = maria_ReadByte(DPPL);
	}


   mode = maria_ReadByte(maria_dpp.w++);

   maria_holey = (mode & 0x60) << 6;
   maria_offset = mode & 0x0F;
   maria_nmi = mode & 0x80;

   maria_dp.b.h = maria_ReadByte(maria_dpp.w++);
   maria_dp.b.l = maria_ReadByte(maria_dpp.w++);


   if (maria_nmi)  /* display list interrupt */
   {
      maria_cycles += 1;

      sally_SetNMI();
   }
}

static int maria_RenderScanline(void)
{
   maria_ctrl = maria_ReadByte(CTRL);
   maria_dma = (maria_ctrl & 0x60) == 0x40;

   maria_cycles = 0;


	if (maria_scanline <= maria_displayArea.top-1)  /* vblank top */
   {
      maria_dpp.w = 0;  /* reset */
		maria_offset = 0;
      return 0;
   }

	else if (maria_scanline >= maria_displayArea.bottom+1)  /* vblank bottom */
      return 0;

	else
	{
      if (maria_scanline >= maria_displayArea.top)  /* draw buffered line to screen */
         maria_WriteLineRAM(maria_surface + ((maria_scanline - maria_displayArea.top) * Rect_GetLength(&maria_displayArea)));

	   else if (maria_dpp.w == 0)  /* dma start */
         maria_LoadDisplayList();


		maria_StoreLineRAM();  /* build next line */


      if (!maria_dma || maria_scanline > maria_displayArea.bottom)
         return 0;


      if (!maria_offset--)  /* zone finished */
      {
         maria_cycles += 6;  /* extra shutdown time */

         maria_LoadDisplayList();
      }
   }

   return (maria_cycles > MARIA_CYCLE_LIMIT) ? MARIA_CYCLE_LIMIT : maria_cycles;
}

void maria_Reset(void)
{
   maria_Clear();
}

int maria_Run(void)
{
   return maria_RenderScanline();
}

void maria_Clear(void)
{
   int index;

   for (index = 0; index < MARIA_SURFACE_SIZE; index++)
      maria_surface[index] = 0;
}
