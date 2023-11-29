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

rect maria_displayArea = {0, 16, 319, 257};
rect maria_visibleArea = {0, 27, 319, 250};

uint8_t maria_surface[MARIA_SURFACE_SIZE] = {0};
uint32_t maria_scanline = 0;
int maria_draw;

static uint8_t maria_lineRAM[MARIA_LINERAM_SIZE];
static int maria_cycles;

static pair maria_dpp;
static pair maria_dp;
static pair maria_pp;

static uint8_t maria_horizontal;
static uint8_t maria_palette;
static uint8_t maria_offset;
static uint16_t maria_holey;
static uint8_t maria_wmode;
static uint8_t maria_ctrl;
static uint8_t maria_nmi;
static uint8_t maria_dma;

uint8_t *maria_readmap[0x400];

void maria_SetRead(uint32_t start, uint32_t stop, uint8_t *chr)
{
   start /= 0x40;
   stop /= 0x40;

   while (start < stop)
   {
      maria_readmap[start++] = chr;

      if (chr)
         chr += 0x40;
   }
}

uint8_t maria_Read(uint16_t address)
{
   switch (address)
   {
   case MSTAT:
      return memory_ram[MSTAT];
   }

   return memory_ReadOpenBus(address);
}

void maria_Write(uint16_t address, uint8_t data)
{
   switch (address)
   {
   case MSTAT:
      break;

   case WSYNC:
      memory_ram[WSYNC] = 1;
      break;

   default:
      memory_ram[address] = data;
      break;
   }
}

static void maria_AddCycles(int cycles)
{
   /* future use */

   maria_cycles += cycles;
}

static uint8_t maria_ReadByte(uint16_t address)
{
   uint16_t bank = address / 0x40;
   uint16_t offset = address % 0x40;

   if (maria_readmap[bank] > 0)
      return maria_readmap[bank][offset];

   else
      return memory_Read(address);
}

static void maria_StoreCell(uint8_t data)
{
   if (maria_horizontal < MARIA_LINERAM_SIZE)
   {
      if (data & 3)  /* non-transparent */
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
      if ((low | high) & 3)
         maria_lineRAM[maria_horizontal] = (maria_palette & 0x10) | high | low;

      else if (maria_ctrl & 0x04)
         maria_lineRAM[maria_horizontal] = (maria_palette & 0x10) | high | low;
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
   return memory_ram[BACKGRND + ((data & 3) ? data : 0)];
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

static void maria_DrawLineRAM(uint8_t* buffer)
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

static void maria_LoadDisplayList(void)
{
   uint8_t mode;

   if (!maria_dma)
      return;

		  
   if (maria_dpp.w == 0)  /* list start */
   {
      maria_dpp.b.h = memory_ram[DPPH];
      maria_dpp.b.l = memory_ram[DPPL];
   }


   mode = maria_ReadByte(maria_dpp.w++);

   maria_holey = (mode & 0x60) << 6;
   maria_offset = mode & 0x0F;
   maria_nmi = mode & 0x80;

   maria_dp.b.h = maria_ReadByte(maria_dpp.w++);
   maria_dp.b.l = maria_ReadByte(maria_dpp.w++);


   if (maria_nmi)  /* display list interrupt */
   {
      maria_AddCycles(1);

      sally_SetNMI();
   }
}

static void maria_WriteLineRAM(void)
{
   uint32_t index;
   uint8_t cwidth = maria_ctrl & 0x10;
   uint8_t chigh = memory_ram[CHARBASE] + maria_offset;
   pair list_dp = maria_dp;


   memset(maria_lineRAM, 0, sizeof(maria_lineRAM));

   if (!maria_dma || !maria_dpp.w)
      return;


   maria_AddCycles(16);  /* dma startup + shutdown time */

   while (maria_cycles < MARIA_CYCLE_LIMIT)  /* render list */
   {
      uint8_t width, mode;
      uint8_t indirect = 0;
      bool skip_holey = false;


      maria_pp.b.l = maria_ReadByte(list_dp.w++);
      mode = maria_ReadByte(list_dp.w++);

      if ((mode & 0x5F) == 0)  /* list end */
         break;

      maria_pp.b.h = maria_ReadByte(list_dp.w++);


      if ((mode & 0x1F) == 0)  /* extended header */
      { 
         indirect = mode & 0x20;
         maria_wmode = mode & 0x80;

         mode = maria_ReadByte(list_dp.w++);
         maria_AddCycles(2);
      }


      maria_pp.b.h += (!indirect) ? maria_offset : 0;  /* direct graphics mode */

      maria_palette = (mode & 0xE0) >> 3;
      width = ((mode ^ 0xFF) & 0x1F) + 1;

      maria_horizontal = maria_ReadByte(list_dp.w++);
      maria_AddCycles(8);


      for (index = 0; index < width; index++)
      {
         uint8_t data1, data2;
         bool is_holey;

         if (maria_cycles >= MARIA_CYCLE_LIMIT)
            break;


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

            maria_AddCycles((!is_holey) ? 3 : 0);  /* memory read penalty */
         }


         if (is_holey)
         {
            maria_AddCycles((!skip_holey) ? 3 : 0);  /* 1-time zone skip penalty */
            skip_holey = true;
         }


         maria_StoreGraphic(data1, is_holey);
         maria_AddCycles((!is_holey) ? 3 : 0);


         if (indirect && cwidth)  /* 2 data bytes per map byte */
         {
            maria_StoreGraphic(data2, is_holey);
            maria_AddCycles((!is_holey) ? 3 : 0);
         }
      }
   }


   if (!maria_offset--)  /* zone finished */
   {
      maria_AddCycles(6+4);  /* extra shutdown time */

      maria_LoadDisplayList();
   }
}

static int maria_RenderScanline(void)
{
   maria_ctrl = memory_ram[CTRL];
   maria_dma = (maria_ctrl & 0x60) == 0x40;

   maria_cycles = 0;


   if (maria_scanline == maria_displayArea.top)
   {
      maria_dpp.w = 0;
      maria_draw = maria_dma;

      maria_LoadDisplayList();
   }


   if (maria_scanline >= maria_visibleArea.top && maria_scanline <= maria_visibleArea.bottom)  /* draw to screen */
      maria_DrawLineRAM(maria_surface + ((maria_scanline - maria_displayArea.top) * Rect_GetLength(&maria_displayArea)));

   if (maria_scanline > maria_displayArea.top && maria_scanline <= maria_displayArea.bottom)
      maria_WriteLineRAM();

   return (maria_cycles > MARIA_CYCLE_LIMIT) ? MARIA_CYCLE_LIMIT : maria_cycles;
}

void maria_Reset(void)
{
   memset(memory_ram + 0x20, 0, 0x20);

   memset(maria_readmap, 0, sizeof(maria_readmap));
}

int maria_Run(void)
{
   return maria_RenderScanline();
}

void maria_Scanline(void)
{
   memory_ram[MSTAT] = (maria_scanline < maria_displayArea.top || maria_scanline > maria_displayArea.bottom) ? 0x80 : 0x00;
   memory_ram[WSYNC] = false;
}

void maria_Clear(void)
{
}

void maria_LoadState(void)
{
   prosystem_ReadStatePtr(memory_ram + 0x20, 0x20);
}

void maria_SaveState(void)
{
   prosystem_WriteStatePtr(memory_ram + 0x20, 0x20);
}
