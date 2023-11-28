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
 * LightGun.c
 * ----------------------------------------------------------------------------
 */
#include "ProSystem.h"
#include "Memory.h"
#include "Maria.h"
#include "LightGun.h"
#include "Cartridge.h"

#ifdef WIN32
  #define strcasecmp stricmp  
  #define strncasecmp strnicmp
#endif

int lightgun_enabled;

static int lightgun_x;
static int lightgun_y;

static int lightgun_xadj = 0x7fff;
static int lightgun_yadj;

/*
300 = 100
310 = 114
400 = 206
426 = 234

427 = 236
428 = 236
429 = 236
430 = 236

431 = 238
432 = 238
433 = 238
434 = 238

435 = 240
436 = 240
437 = 240
438 = 240

439 = x
440 = x
450 = x
452 = x

0 = 242 - 240 / 252 - 254
1 = 242
2 = 242

3 = 262
4 = 262 / 272

Barnyard
3 = 260, 254-267
7 = 266


Sentinel
0 = 246-261

3 = 264-279

7 = 270-285
11 = 272-287
15 = 276-291
19 = 280-295
23 = 284-299
27 = 286-301

31 = 300-315
35 = 302-317
39 = 304-319
43 = 306-321
47 = 308-323

51-192 = x

193 = 0-15

197 = 12-27
201 = 14-29
205 = 16-31
209 = 18-33
213 = 20-35
217 = 22-37

221 = 36-51
225 = 38-53
229 = 40-55
233 = 42-57
237 = 44-59

241 = 56-71
245 = 58-73
249 = 60-75
253 = 62-77
257 = 64-79
261 = 66-81

265 = 80-95
269 = 82-97
273 = 84-99
277 = 86-101
281 = 88-103

285 = 100-115
289 = 102-117
293 = 104-119
297 = 106-121
301 = 108-123
305 = 110-125

309 = 124-139
313 = 126-141
317 = 128-143
321 = 130-145
325 = 132-147

329 = 144-159
333 = 146-161
337 = 148-163
341 = 150-165
345 = 152-167
349 = 154-169

353 = 168-187
357 = 170-185
361 = 172-187
365 = 174-189
369 = 176-191

373 = 188-203
377 = 190-205
381 = 192-207
385 = 194-209
389 = 196-211
393 = 198-213

397 = 212-227
401 = 214-229
405 = 216-231
409 = 218-233
413 = 220-235

417 = 232-247
421 = 234-249
425 = 236-251
429 = 238-253
433 = 240-255
437 = 242-257

441-453 = x
*/


typedef struct lightgun_db
{
   char digest[256];
   char title[256];
   int xpos;
   int ypos;
} lightgun_db_t;

static const struct lightgun_db db_list[] = 
{
   {
      "DE3E9496CB7341F865F27E5A72C7F2F5",
      "Alien Brigade (Europe)",
      4, -14,
   },
   {
      "877DCC97A775ED55081864B2DBF5F1E2",
      "Alien Brigade (USA)",
      4, -14,
   },
   {
      "BABE2BC2976688BAFB8B23C192658126",
      "Barnyard Blaster (Europe)",
      16, -14,
   },
   {
      "42682415906C21C6AF80E4198403FFDA",
      "Barnyard Blaster (USA)",
      16, -14,
   },
   {
      "63DB371D67A98DAEC547B2ABD5E7AA95",
      "Crossbow (Europe)",
      0, -15,
   },
   {
      "A94E4560B6AD053A1C24E096F1262EBF",
      "Crossbow (USA)",
      0, -15,
   },
   {
      "C80155D7EEC9E3DCB79AA6B83C9CCD1E",
      "Meltdown (Europe)",
      16, -18,
   },
   {
      "BEDC30EC43587E0C98FC38C39C1EF9D0",
      "Meltdown (USA)",
      16, -18,
   },
   {
      "5469B4DE0608F23A5C4F98F331C9E75F",
      "Sentinel (Europe)",
      0, -2,
   },
   {
      "B697D9C2D1B9F6CB21041286D1BBFA7F",
      "Sentinel (USA)",
      0, -2,
   },
};


void lightgun_Reset(void)
{
   int index;
   int len = sizeof(db_list) / sizeof(db_list[0]);

   lightgun_xadj = 0;
   lightgun_yadj = 0;

   for (index = 0; index < len; index++)
   {
      if (!strcasecmp(db_list[index].digest, cartridge_digest))
      {
         lightgun_xadj = db_list[index].xpos;
         lightgun_yadj = db_list[index].ypos;
         break;
      }
   }
}

void lightgun_Cursor(int x, int y)
{
   if (lightgun_xadj == 0x7fff)
      lightgun_Reset();


/*
193 = 0-15
*/
   if (x < 16)
      x = 195;

/*
197 = 12-27
201 = 14-29
205 = 16-31
209 = 18-33
213 = 20-35
217 = 22-37
*/
   else if (x < 38)
      x = 197 + x - 14;

/*
221 = 36-51
225 = 38-53
229 = 40-55
233 = 42-57
237 = 44-59
*/
   else if (x < 60)
      x = 221 + x - 40;

/*
241 = 56-71
245 = 58-73
249 = 60-75
253 = 62-77
257 = 64-79
261 = 66-81
*/
   else if (x < 82)
      x = 241 + x - 58;

/*
265 = 80-95
269 = 82-97
273 = 84-99
277 = 86-101
281 = 88-103
*/
   else if (x < 104)
      x = 265 + x - 84;

/*
285 = 100-115
289 = 102-117
293 = 104-119
297 = 106-121
301 = 108-123
305 = 110-125
*/
   else if (x < 126)
      x = 285 + x - 102;

/*
309 = 124-139
313 = 126-141
317 = 128-143
321 = 130-145
325 = 132-147
*/
   else if (x < 148)
      x = 309 + x - 128;

/*
329 = 144-159
333 = 146-161
337 = 148-163
341 = 150-165
345 = 152-167
349 = 154-169
*/
   else if (x < 170)
      x = 329 + x - 146;

/*
353 = 168-187
357 = 170-185
361 = 172-187
365 = 174-189
369 = 176-191
*/
   else if (x < 192)
      x = 353 + x - 172;

/*
373 = 188-203
377 = 190-205
381 = 192-207
385 = 194-209
389 = 196-211
393 = 198-213
*/
   else if (x < 214)
      x = 373 + x - 190;

/*
397 = 212-227
401 = 214-229
405 = 216-231
409 = 218-233
413 = 220-235
*/
   else if (x < 236)
      x = 397 + x - 216;

/*
417 = 232-247
421 = 234-249
425 = 236-251
429 = 238-253
433 = 240-255
437 = 242-257
*/
   else if (x < 256)
      x = 417 + x - 234;

/*
441-453 = x
*/

/*
0 = 246-261
*/
   else if (x < 262)
      x = 2;

/*
3 = 264-279
*/
   else if (x < 278)
      x = 5;

/*
 7 = 270-285
11 = 272-287
*/
   else if (x < 288)
      x = 7 + x - 278;

/*
15 = 276-291
19 = 280-295
*/
   else if (x < 296)
      x = 15 + x - 288;

/*
23 = 284-299
27 = 286-301
*/
   else if (x < 302)
      x = 23 + x - 295;

/*
31 = 300-315
35 = 302-317
39 = 304-319
43 = 306-321
47 = 308-323
*/
   else if (x < 324)
      x = 31 + x - 300;

/*
51-192 = x
*/

   lightgun_x = x + lightgun_xadj + maria_visibleArea.left;
   lightgun_y = y + lightgun_yadj + maria_visibleArea.top;
}

uint8_t lightgun_Strobe(void)
{
   uint8_t data = memory_ram[INPT4] | 0x80;  /* not lit */


   if (maria_draw)
      return data;


   if (maria_scanline < lightgun_y)
      return data;

   if (prosystem_cycles < lightgun_x)
      return data;


   return data & 0x7f;
}
