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
 * Sally.c
 * ----------------------------------------------------------------------------
 */
#include "ProSystem.h"
#include "Sally.h"
#include "Memory.h"
#include "Pair.h"
#include "Cartridge.h"

uint8_t* sally_readmap[0x400];
uint8_t* sally_writemap[0x400];

uint8_t sally_a;
uint8_t sally_x;
uint8_t sally_y;
uint8_t sally_p;
uint8_t sally_s;
pair sally_pc;

uint8_t sally_nmi;
uint8_t sally_irq;

static uint8_t sally_reset;
static uint32_t sally_cycles;
static uint32_t sally_slowcycles;

static uint8_t sally_opcode;
static pair sally_address;

struct Flag
{
   uint8_t C;
   uint8_t Z;
   uint8_t I;
   uint8_t D;
   uint8_t B;
   uint8_t R;
   uint8_t V;
   uint8_t N;
};

static const struct Flag SALLY_FLAG = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

struct Vector
{
   uint16_t H;
   uint16_t L;
};

static const struct Vector SALLY_RES = { 0xFFFD, 0xFFFC };
static const struct Vector SALLY_NMI = { 0xFFFB, 0xFFFA };
static const struct Vector SALLY_IRQ = { 0xFFFF, 0xFFFE }; 

static const uint8_t SALLY_CYCLES[256] = {
  7, 6, 0, 0, 2, 3, 5, 0, 3, 2, 2, 2, 0, 4, 6, 0,  /* 00 - 0F */
  2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,  /* 10 - 1F */
  6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 2, 4, 4, 6, 0,  /* 20 - 2F */
  2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,  /* 30 - 3F */
  6, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 2, 3, 4, 6, 0,  /* 40 - 4F */
  2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,  /* 50 - 5F */
  6, 6, 0, 0, 0, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0,  /* 60 - 6F */
  2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,  /* 70 - 7F */
  2, 6, 0, 0, 3, 3, 3, 0, 2, 0, 2, 0, 4, 4, 4, 0,  /* 80 - 8F */
  2, 6, 0, 0, 4, 4, 4, 4, 2, 5, 2, 0, 0, 5, 0, 0,  /* 90 - 9F */
  2, 6, 2, 0, 3, 3, 3, 0, 2, 2, 2, 0, 4, 4, 4, 0,  /* A0 - AF */
  2, 5, 0, 6, 4, 4, 4, 0, 2, 4, 2, 0, 4, 4, 4, 0,  /* B0 - BF */
  2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0,  /* C0 - CF */
  2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,  /* D0 - DF */
  2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0,  /* E0 - EF */
  2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,  /* F0 - FF */
};

static uint8_t read_mem(uint16_t address)
{
   uint16_t bank = address / 0x40;
   uint16_t offset = address % 0x40;

   if (sally_readmap[bank] > 0)
      return sally_readmap[bank][offset];

   else
   {
      sally_slowcycles += (address >= 0x000 && address <= 0x01F) ? 1 : 0;  /* TIA */
      sally_slowcycles += (address >= 0x280 && address <= 0x2FF) ? 1 : 0;  /* RIOT */

      return memory_Read(address);
   }
}

static void write_mem(uint16_t address, uint8_t data)
{
   uint16_t bank = address / 0x40;
   uint16_t offset = address % 0x40;

   if (sally_writemap[bank] > 0)
      sally_writemap[bank][offset] = data;

   else
   {
      sally_slowcycles += (address >= 0x000 && address <= 0x01F) ? 1 : 0;  /* TIA */
      sally_slowcycles += (address >= 0x280 && address <= 0x2FF) ? 1 : 0;  /* RIOT */

      memory_Write(address, data);
   }
}

void sally_SetRead(uint32_t start, uint32_t stop, uint8_t *prg)
{
   start /= 0x40;
   stop /= 0x40;

   while (start < stop)
   {
      sally_readmap[start++] = prg;

      if (prg)
         prg += 0x40;
   }
}

void sally_SetWrite(uint32_t start, uint32_t stop, uint8_t *prg)
{
   start /= 0x40;
   stop /= 0x40;

   while (start < stop)
   {
      sally_writemap[start++] = prg;

      if (prg)
         prg += 0x40;
   }
}

static void sally_Push(uint8_t data)
{
   if (sally_s >= 0x40)
      memory_ram[sally_s + 0x2100] = data;

   else
      memory_Write(sally_s, data);

   sally_s--;
}

static uint8_t sally_Pop(void)
{
   sally_s++;

   if (sally_s >= 0x40)
      return memory_ram[sally_s + 0x2100];

   else
      return memory_Read(sally_s);
}

static void sally_Flags(uint8_t data)
{
   if(!data)
      sally_p |= SALLY_FLAG.Z;
   else
      sally_p &= ~SALLY_FLAG.Z;

   if(data & 0x80)
      sally_p |= SALLY_FLAG.N;
   else
      sally_p &= ~SALLY_FLAG.N;
}

static void sally_Branch(uint8_t branch)
{
   uint16_t carry = sally_pc.w;

   if (!branch)
      return;

   sally_pc.w += (int8_t) sally_address.b.l;
   sally_cycles += ((carry ^ sally_pc.w) & 0x100) ? 2 : 1;
}

static void sally_Delay(uint8_t delta)
{
   if (((uint16_t) sally_address.b.l + (uint16_t) delta) & 0xFF00)
      sally_cycles += 1;
}

static void sally_Absolute(void)
{
   sally_address.b.l = read_mem(sally_pc.w++);
   sally_address.b.h = read_mem(sally_pc.w++);
}

static void sally_AbsoluteX(void)
{
   sally_address.b.l = read_mem(sally_pc.w++);
   sally_address.b.h = read_mem(sally_pc.w++);
   sally_address.w += sally_x;
}

static void sally_AbsoluteY(void)
{
   sally_address.b.l = read_mem(sally_pc.w++);
   sally_address.b.h = read_mem(sally_pc.w++);
   sally_address.w += sally_y;
}

static void sally_Immediate(void)
{
   sally_address.w = sally_pc.w++;
}

static void sally_Indirect(void)
{
   pair base;
   base.b.l = read_mem(sally_pc.w++);
   base.b.h = read_mem(sally_pc.w++);
   sally_address.b.l = read_mem(base.w);
   sally_address.b.h = read_mem(base.w + 1);
}

static void sally_IndirectX(void)
{
   sally_address.b.l = read_mem(sally_pc.w++) + sally_x;
   sally_address.b.h = read_mem(sally_address.b.l + 1);
   sally_address.b.l = read_mem(sally_address.b.l);
}

static void sally_IndirectY(void)
{
   sally_address.b.l = read_mem(sally_pc.w++);
   sally_address.b.h = read_mem(sally_address.b.l + 1);
   sally_address.b.l = read_mem(sally_address.b.l);
   sally_address.w += sally_y;
}

static void sally_Relative(void)
{
   sally_address.w = read_mem(sally_pc.w++);
}

static void sally_ZeroPage(void)
{
   sally_address.w = read_mem(sally_pc.w++);
}

static void sally_ZeroPageX(void)
{
   sally_address.w = read_mem(sally_pc.w++);
   sally_address.b.l += sally_x;
}

static void sally_ZeroPageY(void)
{
   sally_address.w = read_mem(sally_pc.w++);
   sally_address.b.l += sally_y;
}

static void sally_ADC(void)
{
   uint8_t data = read_mem(sally_address.w);

   if(sally_p & SALLY_FLAG.D)
   {
      uint16_t al = (sally_a & 15) + (data & 15) + (sally_p & SALLY_FLAG.C);
      uint16_t ah = (sally_a >> 4) + (data >> 4);

      if(al > 9)
      {
         al += 6;
         ah++;
      }

      if(!(sally_a + data + (sally_p & SALLY_FLAG.C)))
         sally_p |= SALLY_FLAG.Z;
      else
         sally_p &= ~SALLY_FLAG.Z;

      if((ah & 8) != 0)
         sally_p |= SALLY_FLAG.N;      
      else
         sally_p &= ~SALLY_FLAG.N;

      if(~(sally_a ^ data) & ((ah << 4) ^ sally_a) & 0x80)
         sally_p |= SALLY_FLAG.V;
      else
         sally_p &= ~SALLY_FLAG.V;

      if(ah > 9)
         ah += 6;

      if(ah > 15)
         sally_p |= SALLY_FLAG.C;      
      else
         sally_p &= ~SALLY_FLAG.C;

      sally_a = (ah << 4) | (al & 15);
   }
   else
   {
      pair temp;
      temp.w = sally_a + data + (sally_p & SALLY_FLAG.C);

      if(temp.b.h)
         sally_p |= SALLY_FLAG.C;
      else
         sally_p &= ~SALLY_FLAG.C;

      if(~(sally_a ^ data) & (sally_a ^ temp.b.l) & 0x80)
         sally_p |= SALLY_FLAG.V;
      else
         sally_p &= ~SALLY_FLAG.V;

      sally_Flags(temp.b.l);
      sally_a = temp.b.l;
   }
}

static void sally_AND(void)
{
   sally_a &= read_mem(sally_address.w);
   sally_Flags(sally_a);
}

static void sally_ASLA(void)
{
   if(sally_a & 0x80)
      sally_p |= SALLY_FLAG.C;
   else
      sally_p &= ~SALLY_FLAG.C;

   sally_a <<= 1;
   sally_Flags(sally_a);
}

static void sally_ASL(void)
{
   uint8_t data = read_mem(sally_address.w);

   if(data & 0x80)
      sally_p |= SALLY_FLAG.C;
   else
      sally_p &= ~SALLY_FLAG.C;

   data <<= 1;
   write_mem(sally_address.w, data);
   sally_Flags(data);
}

static void sally_BCC(void)
{
   sally_Branch(!(sally_p & SALLY_FLAG.C));
}

static void sally_BCS(void)
{
   sally_Branch(sally_p & SALLY_FLAG.C);
}

static void sally_BEQ(void)
{
   sally_Branch(sally_p & SALLY_FLAG.Z);
}

static void sally_BIT(void)
{
   uint8_t data = read_mem(sally_address.w);
   sally_p &= ~(SALLY_FLAG.V | SALLY_FLAG.N | SALLY_FLAG.Z);

   if(!(data & sally_a)) 
   {
      sally_p |= SALLY_FLAG.Z;
   }
   sally_p |= data & SALLY_FLAG.V;
   sally_p |= data & SALLY_FLAG.N;
}

static void sally_BMI(void)
{
   sally_Branch(sally_p & SALLY_FLAG.N);
}

static void sally_BNE(void)
{
   sally_Branch(!(sally_p & SALLY_FLAG.Z));
}

static void sally_BPL(void)
{
   sally_Branch(!(sally_p & SALLY_FLAG.N));
}

static void sally_BRK(void)
{
   sally_pc.w++;
   sally_p |= SALLY_FLAG.B;

   sally_Push(sally_pc.b.h);
   sally_Push(sally_pc.b.l);
   sally_Push(sally_p);

   sally_p |= SALLY_FLAG.I;
   sally_pc.b.l = read_mem(SALLY_IRQ.L);
   sally_pc.b.h = read_mem(SALLY_IRQ.H);
}

static void sally_BVC(void)
{
   sally_Branch(!(sally_p & SALLY_FLAG.V));
}

static void sally_BVS(void)
{
   sally_Branch(sally_p & SALLY_FLAG.V);
}

static void sally_CLC(void)
{
   sally_p &= ~SALLY_FLAG.C;
}

static void sally_CLD(void) {
   sally_p &= ~SALLY_FLAG.D;
}

static void sally_CLI(void) {
   sally_p &= ~SALLY_FLAG.I;
}

static void sally_CLV(void) {
   sally_p &= ~SALLY_FLAG.V;
}

static void sally_CMP(void) {
   uint8_t data = read_mem(sally_address.w);

   if(sally_a >= data) {
      sally_p |= SALLY_FLAG.C;
   }
   else {
      sally_p &= ~SALLY_FLAG.C;
   }

   sally_Flags(sally_a - data);
}

static void sally_CPX(void)
{
   uint8_t data = read_mem(sally_address.w);

   if(sally_x >= data)
      sally_p |= SALLY_FLAG.C;  
   else
      sally_p &= ~SALLY_FLAG.C;

   sally_Flags(sally_x - data);
}

static void sally_CPY(void)
{
   uint8_t data = read_mem(sally_address.w);

   if(sally_y >= data)
      sally_p |= SALLY_FLAG.C;
   else
      sally_p &= ~SALLY_FLAG.C;

   sally_Flags(sally_y - data);
}

static void sally_DEC(void)
{
   uint8_t data = read_mem(sally_address.w);
   write_mem(sally_address.w, --data);
   sally_Flags(data);
}

static void sally_DEX(void)
{
   sally_Flags(--sally_x);
}

static void sally_DEY(void)
{
   sally_Flags(--sally_y);
}

static void sally_EOR(void)
{
   sally_a ^= read_mem(sally_address.w);
   sally_Flags(sally_a);
}

static void sally_INC(void)
{
   uint8_t data = read_mem(sally_address.w);
   write_mem(sally_address.w, ++data);
   sally_Flags(data);
}

static void sally_INX(void)
{
   sally_Flags(++sally_x);
}

static void sally_INY(void)
{
   sally_Flags(++sally_y);
}

static void sally_JMP(void)
{
   sally_pc = sally_address;
}

static void sally_JSR(void)
{
   sally_pc.w--;
   sally_Push(sally_pc.b.h);
   sally_Push(sally_pc.b.l);

   sally_pc = sally_address;
}

static void sally_LDA(void)
{
   sally_a = read_mem(sally_address.w);
   sally_Flags(sally_a);
}

static void sally_LDX(void)
{
   sally_x = read_mem(sally_address.w);
   sally_Flags(sally_x);
}

static void sally_LDY(void)
{
   sally_y = read_mem(sally_address.w);
   sally_Flags(sally_y);
}

static void sally_LSRA(void)
{
   sally_p &= ~SALLY_FLAG.C;
   sally_p |= sally_a & 1;

   sally_a >>= 1;
   sally_Flags(sally_a);
}

static void sally_LSR(void)
{
   uint8_t data = read_mem(sally_address.w);

   sally_p &= ~SALLY_FLAG.C;
   sally_p |= data & 1;

   data >>= 1;
   write_mem(sally_address.w, data);
   sally_Flags(data);
}

static void sally_NOP(void)
{
}

static void sally_ORA(void)
{
   sally_a |= read_mem(sally_address.w);
   sally_Flags(sally_a);
}

static void sally_PHA(void)
{
   sally_Push(sally_a);   
}

static void sally_PHP(void)
{
   sally_Push(sally_p);
}

static void sally_PLA(void)
{
   sally_a = sally_Pop();
   sally_Flags(sally_a);
}

static void sally_PLP(void)
{
   sally_p = sally_Pop();
}

static void sally_ROLA(void)
{
   uint8_t temp = sally_p;

   if(sally_a & 0x80)
      sally_p |= SALLY_FLAG.C; 
   else
      sally_p &= ~SALLY_FLAG.C;

   sally_a <<= 1;
   sally_a |= temp & SALLY_FLAG.C;
   sally_Flags(sally_a);
}

static void sally_ROL(void)
{
   uint8_t data = read_mem(sally_address.w);
   uint8_t temp = sally_p;

   if(data & 0x80)
      sally_p |= SALLY_FLAG.C;
   else
      sally_p &= ~SALLY_FLAG.C;

   data <<= 1;
   data |= temp & 1;
   write_mem(sally_address.w, data);
   sally_Flags(data);
}

static void sally_RORA(void)
{
   uint8_t temp = sally_p;

   sally_p &= ~SALLY_FLAG.C;
   sally_p |= sally_a & 1;

   sally_a >>= 1;
   if(temp & SALLY_FLAG.C)
      sally_a |= 0x80;

   sally_Flags(sally_a);
}

static void sally_ROR(void)
{
   uint8_t data = read_mem(sally_address.w);
   uint8_t temp = sally_p;

   sally_p &= ~SALLY_FLAG.C;
   sally_p |= data & 1;

   data >>= 1;
   if(temp & 1) {
      data |= 0x80;
   }

   write_mem(sally_address.w, data);
   sally_Flags(data);
}

static void sally_RTI(void)
{
   sally_p      = sally_Pop();
   sally_pc.b.l = sally_Pop();
   sally_pc.b.h = sally_Pop();
}

static void sally_RTS(void)
{
   sally_pc.b.l = sally_Pop();
   sally_pc.b.h = sally_Pop();
   sally_pc.w++;
}

static void sally_SBC(void)
{
   uint8_t data = read_mem(sally_address.w);

   if(sally_p & SALLY_FLAG.D)
   {
      pair temp;
      uint16_t al = (sally_a & 15) - (data & 15) - !(sally_p & SALLY_FLAG.C);
      uint16_t ah = (sally_a >> 4) - (data >> 4);

      if(al > 9) {
         al -= 6;
         ah--;
      }

      if(ah > 9) {
         ah -= 6;
      }

      temp.w = sally_a - data - !(sally_p & SALLY_FLAG.C);

      if(!temp.b.h) {
         sally_p |= SALLY_FLAG.C;
      }
      else {
         sally_p &= ~SALLY_FLAG.C;
      }

      if((sally_a ^ data) & (sally_a ^ temp.b.l) & 0x80) {
         sally_p |= SALLY_FLAG.V;
      }
      else {
         sally_p &= ~SALLY_FLAG.V;     
      }

      sally_Flags(temp.b.l);
      sally_a = (ah << 4) | (al & 15);
   }
   else
   {
      pair temp;
      temp.w = sally_a - data - !(sally_p & SALLY_FLAG.C);

      if(!temp.b.h) {
         sally_p |= SALLY_FLAG.C;
      }
      else {
         sally_p &= ~SALLY_FLAG.C;
      }

      if((sally_a ^ data) & (sally_a ^ temp.b.l) & 0x80) {
         sally_p |= SALLY_FLAG.V;
      }
      else {
         sally_p &= ~SALLY_FLAG.V;
      }

      sally_Flags(temp.b.l);
      sally_a = temp.b.l;
   }
}

static void sally_SEC(void)
{
   sally_p |= SALLY_FLAG.C; 
}

static void sally_SED(void)
{
   sally_p |= SALLY_FLAG.D;
}

static void sally_SEI(void)
{
   sally_p |= SALLY_FLAG.I;
}

static void sally_STA(void)
{
   write_mem(sally_address.w, sally_a);
}

static void sally_STX(void)
{
   write_mem(sally_address.w, sally_x);
}

static void sally_STY(void)
{
   write_mem(sally_address.w, sally_y);
}

static void sally_TAX(void)
{
   sally_x = sally_a;
   sally_Flags(sally_x);
}

static void sally_TAY(void)
{
   sally_y = sally_a;
   sally_Flags(sally_y);
}

static void sally_TSX(void)
{
   sally_x = sally_s;
   sally_Flags(sally_x);
}

static void sally_TXA(void)
{
   sally_a = sally_x;
   sally_Flags(sally_a);
}

static void sally_TXS(void)
{
   sally_s = sally_x;
}

static void sally_TYA(void)
{
   sally_a = sally_y;
   sally_Flags(sally_a);
}

void sally_Reset(void)
{
   sally_a = 0;
   sally_x = 0;
   sally_y = 0;
   sally_p = SALLY_FLAG.R;
   sally_s = 0;
   sally_pc.w = 0;

   sally_nmi = 0;
   sally_irq = 0;

   sally_reset = 1;

   memset(sally_readmap, 0, sizeof(sally_readmap));
   memset(sally_writemap, 0, sizeof(sally_writemap));
}

static uint32_t sally_ExecuteRES(void)
{
   sally_p      = SALLY_FLAG.I | SALLY_FLAG.R | SALLY_FLAG.Z;
   sally_pc.b.l = read_mem(SALLY_RES.L);
   sally_pc.b.h = read_mem(SALLY_RES.H);

   return 6;
}

static uint32_t sally_ExecuteNMI(void)
{
   sally_Push(sally_pc.b.h);
   sally_Push(sally_pc.b.l);
   sally_p &= ~SALLY_FLAG.B;
   sally_Push(sally_p);
   sally_p |= SALLY_FLAG.I;
   sally_pc.b.l = read_mem(SALLY_NMI.L);
   sally_pc.b.h = read_mem(SALLY_NMI.H);

   return 7;
}

static uint32_t sally_ExecuteIRQ(void)
{
   sally_Push(sally_pc.b.h);
   sally_Push(sally_pc.b.l);
   sally_p &= ~SALLY_FLAG.B;
   sally_Push(sally_p);
   sally_p |= SALLY_FLAG.I;
   sally_pc.b.l = read_mem(SALLY_IRQ.L);
   sally_pc.b.h = read_mem(SALLY_IRQ.H);

   return 7;
}

static uint32_t sally_ExecuteInstruction(void)
{
   sally_opcode = read_mem(sally_pc.w++);
   sally_cycles = SALLY_CYCLES[sally_opcode];

   switch (sally_opcode)
   {
   case 0x00:
      sally_BRK();
      break;

   case 0x01:
      sally_IndirectX();
      sally_ORA();
      break;

   case 0x05:
      sally_ZeroPage();
      sally_ORA();
      break;

   case 0x06:
      sally_ZeroPage();
      sally_ASL();
      break;

   case 0x08:
      sally_PHP();
      break;

   case 0x09:
      sally_Immediate();
      sally_ORA();
      break;       

   case 0x0a:
      sally_ASLA();
      break;       

   case 0x0d:
      sally_Absolute();
      sally_ORA();
      break;

   case 0x0e:
      sally_Absolute();
      sally_ASL();
      break;

   case 0x10:
      sally_Relative();
      sally_BPL();
      break;       

   case 0x11:
      sally_IndirectY();
      sally_ORA();
      sally_Delay(sally_y);
      break;

   case 0x15:
      sally_ZeroPageX();
      sally_ORA();
      break;

   case 0x16:
      sally_ZeroPageX();
      sally_ASL();
      break;

   case 0x18:
      sally_CLC();
      break;

   case 0x19:
      sally_AbsoluteY();
      sally_ORA();
      sally_Delay(sally_y);
      break;

   case 0x1d:
      sally_AbsoluteX();
      sally_ORA();
      sally_Delay(sally_x);
      break;

   case 0x1e:
      sally_AbsoluteX();
      sally_ASL();
      break;

   case 0x20:
      sally_Absolute();
      sally_JSR();
      break;

   case 0x21:
      sally_IndirectX();
      sally_AND();
      break;

   case 0x24:
      sally_ZeroPage();
      sally_BIT();
      break;

   case 0x25:
      sally_ZeroPage();
      sally_AND();
      break;

   case 0x26:
      sally_ZeroPage();
      sally_ROL();
      break;

   case 0x28:
      sally_PLP();
      break;

   case 0x29:
      sally_Immediate();
      sally_AND();
      break;

   case 0x2a:
      sally_ROLA();
      break;

   case 0x2c:
      sally_Absolute();
      sally_BIT();
      break;

   case 0x2d:
      sally_Absolute();
      sally_AND();
      break;

   case 0x2e:
      sally_Absolute();
      sally_ROL();
      break;

   case 0x30:
      sally_Relative();
      sally_BMI();
      break;

   case 0x31:
      sally_IndirectY();
      sally_AND();
      sally_Delay(sally_y);
      break;

   case 0x35:
      sally_ZeroPageX();
      sally_AND();
      break;

   case 0x36:
      sally_ZeroPageX();
      sally_ROL();
      break;

   case 0x38:
      sally_SEC();
      break;

   case 0x39:
      sally_AbsoluteY();
      sally_AND();
      sally_Delay(sally_y);
      break;

   case 0x3d:
      sally_AbsoluteX();
      sally_AND();
      sally_Delay(sally_x);
      break;

   case 0x3e:
      sally_AbsoluteX();
      sally_ROL();
      break;

   case 0x40:
      sally_RTI();
      break;

   case 0x41:
      sally_IndirectX();
      sally_EOR();
      break;

   case 0x45:
      sally_ZeroPage();
      sally_EOR();
      break;

   case 0x46:
      sally_ZeroPage();
      sally_LSR();
      break;

   case 0x48:
      sally_PHA();
      break;

   case 0x49:
      sally_Immediate();
      sally_EOR();
      break; 

   case 0x4a:
      sally_LSRA();
      break;

   case 0x4c:
      sally_Absolute();
      sally_JMP();
      break;

   case 0x4d:
      sally_Absolute();
      sally_EOR();
      break;

   case 0x4e:
      sally_Absolute();
      sally_LSR();
      break;

   case 0x50:
      sally_Relative();
      sally_BVC();
      break;

   case 0x51:
      sally_IndirectY();
      sally_EOR();
      sally_Delay(sally_y);
      break;     

   case 0x55:
      sally_ZeroPageX();
      sally_EOR();
      break;

   case 0x56:
      sally_ZeroPageX();
      sally_LSR();
      break;

   case 0x58:
      sally_CLI();
      break;

   case 0x59:
      sally_AbsoluteY();
      sally_EOR();
      sally_Delay(sally_y);
      break;

   case 0x5d:
      sally_AbsoluteX();
      sally_EOR();
      sally_Delay(sally_x);
      break;

   case 0x5e:
      sally_AbsoluteX();
      sally_LSR();
      break;

   case 0x60:
      sally_RTS();
      break;

   case 0x61:
      sally_IndirectX();
      sally_ADC();
      break;

   case 0x65:
      sally_ZeroPage();
      sally_ADC();
      break;

   case 0x66:
      sally_ZeroPage();
      sally_ROR();
      break;

   case 0x68:
      sally_PLA();
      break;

   case 0x69:
      sally_Immediate();
      sally_ADC();
      break;

   case 0x6a:
      sally_RORA();
      break;

   case 0x6c:
      sally_Indirect();
      sally_JMP();
      break;

   case 0x6d:
      sally_Absolute();
      sally_ADC();
      break;

   case 0x6e:
      sally_Absolute();
      sally_ROR();
      break;

   case 0x70:
      sally_Relative(); 
      sally_BVS();
      break;

   case 0x71:
      sally_IndirectY();
      sally_ADC();
      sally_Delay(sally_y);
      break;

   case 0x75:
      sally_ZeroPageX();
      sally_ADC();
      break;

   case 0x76:
      sally_ZeroPageX();
      sally_ROR();
      break;

   case 0x78:
      sally_SEI();
      break;

   case 0x79:
      sally_AbsoluteY();
      sally_ADC();
      sally_Delay(sally_y);
      break;

   case 0x7d:
      sally_AbsoluteX();
      sally_ADC();
      sally_Delay(sally_x);
      break;

   case 0x7e:
      sally_AbsoluteX();
      sally_ROR();
      break;

   case 0x81:
      sally_IndirectX();
      sally_STA();
      break;

   case 0x84:
      sally_ZeroPage();
      sally_STY();
      break;

   case 0x85:
      sally_ZeroPage();
      sally_STA();
      break;

   case 0x86:
      sally_ZeroPage();
      sally_STX();
      break;

   case 0x88:
      sally_DEY();
      break;

   case 0x8a:
      sally_TXA();
      break;

   case 0x8c:
      sally_Absolute();
      sally_STY();
      break;

   case 0x8d:
      sally_Absolute();
      sally_STA();
      break;

   case 0x8e:
      sally_Absolute();
      sally_STX();
      break;

   case 0x90:
      sally_Relative();
      sally_BCC();
      break;

   case 0x91:
      sally_IndirectY();
      sally_STA();
      break;

   case 0x94:
      sally_ZeroPageX();
      sally_STY();
      break;

   case 0x95:
      sally_ZeroPageX();
      sally_STA();
      break;

   case 0x96:
      sally_ZeroPageY();
      sally_STX();
      break;

   case 0x98:
      sally_TYA();
      break;

   case 0x99:
      sally_AbsoluteY();
      sally_STA();
      break;

   case 0x9a:
      sally_TXS();
      break;

   case 0x9d:
      sally_AbsoluteX();
      sally_STA();
      break;

   case 0xa0:
      sally_Immediate();
      sally_LDY();
      break;

   case 0xa1:
      sally_IndirectX();
      sally_LDA();
      break;

   case 0xa2:
      sally_Immediate();
      sally_LDX();
      break;

   case 0xa4:
      sally_ZeroPage();
      sally_LDY();
      break;

   case 0xa5:
      sally_ZeroPage();
      sally_LDA();
      break;

   case 0xa6: 
      sally_ZeroPage();
      sally_LDX();
      break;

   case 0xa8: 
      sally_TAY();
      break;

   case 0xa9: 
      sally_Immediate();
      sally_LDA();
      break;

   case 0xaa:
      sally_TAX();
      break;

   case 0xac:
      sally_Absolute();
      sally_LDY();
      break;

   case 0xad:
      sally_Absolute();
      sally_LDA();
      break;

   case 0xae: 
      sally_Absolute();
      sally_LDX();
      break;

   case 0xb0:
      sally_Relative();
      sally_BCS();
      break;

   case 0xb1:
      sally_IndirectY();
      sally_LDA();
      sally_Delay(sally_y);
      break;

   case 0xb4:
      sally_ZeroPageX();
      sally_LDY();
      break;

   case 0xb5:
      sally_ZeroPageX();
      sally_LDA();
      break;

   case 0xb6:
      sally_ZeroPageY();
      sally_LDX();
      break;

   case 0xb8:
      sally_CLV();
      break;

   case 0xb9:
      sally_AbsoluteY();
      sally_LDA();
      sally_Delay(sally_y);
      break;

   case 0xba:
      sally_TSX();
      break;

   case 0xbc:
      sally_AbsoluteX();
      sally_LDY();
      sally_Delay(sally_x);
      break;

   case 0xbd:
      sally_AbsoluteX();
      sally_LDA();
      sally_Delay(sally_x);
      break;

   case 0xbe:
      sally_AbsoluteY();
      sally_LDX();
      sally_Delay(sally_y);
      break;

   case 0xc0:
      sally_Immediate();
      sally_CPY();
      break;

   case 0xc1:
      sally_IndirectX();
      sally_CMP();
      break;

   case 0xc4:
      sally_ZeroPage();
      sally_CPY();
      break;

   case 0xc5:
      sally_ZeroPage();
      sally_CMP();
      break;

   case 0xc6:
      sally_ZeroPage();
      sally_DEC();
      break;

   case 0xc8:
      sally_INY();
      break;

   case 0xc9:
      sally_Immediate();
      sally_CMP();
      break;

   case 0xca:
      sally_DEX();
      break;

   case 0xcc:
      sally_Absolute();
      sally_CPY();
      break;

   case 0xcd:
      sally_Absolute();
      sally_CMP();
      break;

   case 0xce:
      sally_Absolute();
      sally_DEC();
      break;

   case 0xd0:
      sally_Relative();
      sally_BNE();
      break;        

   case 0xd1:
      sally_IndirectY();
      sally_CMP();
      sally_Delay(sally_y);
      break;

   case 0xd5:
      sally_ZeroPageX();
      sally_CMP();
      break;

   case 0xd6:
      sally_ZeroPageX();
      sally_DEC();
      break;

   case 0xd8:
      sally_CLD();
      break;

   case 0xd9:
      sally_AbsoluteY();
      sally_CMP();
      sally_Delay(sally_y);
      break;

   case 0xdd:
      sally_AbsoluteX();
      sally_CMP();
      sally_Delay(sally_x);
      break;

   case 0xde:
      sally_AbsoluteX();
      sally_DEC();
      break;

   case 0xe0:
      sally_Immediate();
      sally_CPX();
      break;

   case 0xe1:
      sally_IndirectX();
      sally_SBC();
      break;

   case 0xe4:
      sally_ZeroPage();
      sally_CPX();
      break;

   case 0xe5:
      sally_ZeroPage();
      sally_SBC();
      break;

   case 0xe6:
      sally_ZeroPage();
      sally_INC();
      break;

   case 0xe8:
      sally_INX();
      break;

   case 0xe9:
      sally_Immediate();
      sally_SBC();
      break;

   case 0xea:
      sally_NOP();
      break;

   case 0xec:
      sally_Absolute();
      sally_CPX();
      break;

   case 0xed:
      sally_Absolute();
      sally_SBC();
      break;

   case 0xee:
      sally_Absolute();
      sally_INC();
      break;

   case 0xf0:
      sally_Relative();
      sally_BEQ();
      break;

   case 0xf1:
      sally_IndirectY();
      sally_SBC();
      sally_Delay(sally_y);
      break;

   case 0xf5:
      sally_ZeroPageX();
      sally_SBC();
      break;

   case 0xf6:
      sally_ZeroPageX();
      sally_INC();
      break;

   case 0xf8:
      sally_SED();
      break;

   case 0xf9:
      sally_AbsoluteY();
      sally_SBC();
      sally_Delay(sally_y);
      break;

   case 0xfd:
      sally_AbsoluteX();
      sally_SBC();
      sally_Delay(sally_x);
      break;

   case 0xfe:
      sally_AbsoluteX();
      sally_INC();
      break;

   case 0x0b:  /* ANC */
   case 0x2b:
      sally_Immediate();
      sally_AND();
      if (sally_a & 0x80)
         sally_p |= SALLY_FLAG.C;
      else
         sally_p &= ~SALLY_FLAG.C;
      break;

   case 0x4b:  /* ALR - ASR */
      sally_Immediate();
      sally_AND();
      sally_LSRA();
      break;

   case 0x97:  /* SAX */
      sally_ZeroPageY();
      sally_PHP();
      sally_PHA();
      sally_STX();
      sally_AND();
      sally_STA();
      sally_PLA();
      sally_PLP();
      break;

   case 0xb3:  /* LAX */
      sally_IndirectY();
      sally_LDA();
      sally_TAX();
      break;
   }

   return sally_cycles;
}

void sally_SetNMI(void)
{
   sally_nmi = 2;  /* 2-cycle delay */
}

void sally_SetIRQ(void)
{
   sally_irq = 2;  /* 2-cycle delay */
}

static int sally_RunOnce(void)
{
   sally_slowcycles = 0;


   if (sally_reset)
      return (--sally_reset) | sally_ExecuteRES();


   if (memory_ram[WSYNC])  /* halt */
      return 0;


   if (sally_nmi)  /* pending or ack */
	   return (--sally_nmi) ? sally_ExecuteInstruction() : sally_ExecuteNMI();


   if (sally_irq && !(sally_p & SALLY_FLAG.I))  /* pending or ack */
	   return (--sally_irq) ? sally_ExecuteInstruction() : sally_ExecuteIRQ();


   return sally_ExecuteInstruction();
}

int sally_Run(void)
{
   return sally_RunOnce() * 4;
}

int sally_SlowCycles(void)
{
   return sally_slowcycles * 2;
}

void sally_LoadState(void)
{
   sally_a = prosystem_ReadState8();
   sally_x = prosystem_ReadState8();
   sally_y = prosystem_ReadState8();
   sally_p = prosystem_ReadState8();
   sally_s = prosystem_ReadState8();
   sally_pc.w = prosystem_ReadState16();

   sally_nmi = prosystem_ReadState8();
   sally_irq = prosystem_ReadState8();
}

void sally_SaveState(void)
{
   prosystem_WriteState8(sally_a);
   prosystem_WriteState8(sally_x);
   prosystem_WriteState8(sally_y);
   prosystem_WriteState8(sally_p);
   prosystem_WriteState8(sally_s);
   prosystem_WriteState16(sally_pc.w);

   prosystem_WriteState8(sally_nmi);
   prosystem_WriteState8(sally_irq);
}
