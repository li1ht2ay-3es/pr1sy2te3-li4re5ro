#ifndef _MSC_VER
#include <sched.h>
#endif
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <boolean.h>
#include <streams/file_stream.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#pragma pack(1)
#endif

#include <libretro.h>
#include "libretro_core_options.h"

#include "Cartridge.h"
#include "Maria.h"
#include "Pokey.h"
#include "ProSystem.h"
#include "Tia.h"
#include "Memory.h"
#include "Mixer.h"
#include "Database.h"
#include "Palette.h"
#include "Bios.h"
#include "Region.h"
#include "HighScore.h"
#include "LightGun.h"

#ifdef _3DS
extern void* linearMemAlign(size_t size, size_t alignment);
extern void linearFree(void* mem);
#endif

#define VIDEO_BUFFER_SIZE (320 * 292 * 4)
static uint8_t *videoBuffer            = NULL;
static uint8_t videoPixelBytes         = 2;
static int videoWidth                  = 320;
static int videoHeight                 = 224;
static uint32_t display_palette32[256] = {0};
static uint16_t display_palette16[256] = {0};
static uint8_t keyboard_data[17]       = {0};

static bool persistent_data            = false;

#define GAMEPAD_ANALOG_THRESHOLD 0x4000
static bool gamepad_dual_stick_hack    = false;

/* Low pass audio filter */
static bool low_pass_enabled           = false;
static int32_t low_pass_range          = 0;
static int32_t low_pass_prev           = 0; /* Previous sample */

/* Save state info */
#define SAVE_STATE_SIZE                0x10000
#define FAST_SAVE_STATE_SIZE           0x30000
static bool fast_savestates;

static bool first_frame = true;

static int audio_rate = 48000;
static int console_region = REGION_AUTO;
static int display_aspect = 0;

static int left_difficulty = 1;  /* beginner */
static int right_difficulty = 0;  /* advanced */
static int left_difficulty_hold = 0;
static int right_difficulty_hold = 0;

static int highscore_save = 1;  /* global */
static int highscore_name = 2;  /* global */

static char highscore_globalname[33];

static int bios_startup = 1;

static int lightgun_trigger = 0;
static int lightgun_x = 0;
static int lightgun_y = 0;
static int lightgun_detect = 0;

static unsigned port_devices[2];

static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;

static bool libretro_supports_bitmasks = false;

void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

static char osd_message[4096];

void retro_print_message(char *str)
{
   struct retro_message msg = { osd_message, 100 };

   strcpy(osd_message, str);
   environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
}

void retro_set_environment(retro_environment_t cb)
{
   struct retro_vfs_interface_info vfs_iface_info;

   static const struct retro_system_content_info_override content_overrides[] = {
      {
         "a78|bin|cdf", /* extensions */
         false,         /* need_fullpath */
         true           /* persistent_data */
      },
      { NULL, false, false }
   };


   static const struct retro_controller_description port_1[] = {
      { "None", RETRO_DEVICE_NONE },
      { "Joypad", RETRO_DEVICE_JOYPAD },
      { "Lightgun", RETRO_DEVICE_LIGHTGUN },
   };

   static const struct retro_controller_description port_2[] = {
      { "None", RETRO_DEVICE_NONE },
      { "Joypad", RETRO_DEVICE_JOYPAD },
   };

   static const struct retro_controller_info ports[] = {
      { port_1, 3 },
      { port_2, 2 },
   };

   environ_cb = cb;
   libretro_set_core_options(environ_cb);
   /* Request a persistent content data buffer */
   environ_cb(RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE,
         (void*)content_overrides);

   vfs_iface_info.required_interface_version = 1;
   vfs_iface_info.iface                      = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
      filestream_vfs_init(&vfs_iface_info);

   environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);
}

#define BLIT_VIDEO_BUFFER(typename_t, src, palette, width, height, pitch, dst) \
   {                                                                           \
      typename_t *surface = (typename_t*)dst;                                  \
      uint32_t x, y;                                                           \
                                                                               \
      for(y = 0; y < height; y++)                                              \
      {                                                                        \
         typename_t *surface_ptr = surface;                                    \
         const uint8_t *src_ptr  = src;                                        \
                                                                               \
         for(x = 0; x < width; x++)                                            \
            *(surface_ptr++) = *(palette + *(src_ptr++));                      \
                                                                               \
         surface += pitch;                                                     \
         src     += width;                                                     \
      }                                                                        \
   }

static void display_ResetPalette(void)
{
   unsigned index;

   for(index = 0; index < 256; index++)
   {
      uint32_t r = palette_data[(index * 3) + 0] << 16;
      uint32_t g = palette_data[(index * 3) + 1] << 8;
      uint32_t b = palette_data[(index * 3) + 2];

      display_palette32[index] = r | g | b;
      display_palette16[index] = ((r & 0xF80000) >> 8) |
                                 ((g & 0x00F800) >> 5) |
                                 ((b & 0x0000F8) >> 3);
   }
}

static void draw_lightgun_cursor(int16_t x, int16_t y, uint8_t color)
{
   int ypos, xpos;
   int x_start = x - 2;  /* pixel center */
   int x_end  = x + 2;
   int y_start = y - 2;
   int y_end = y + 2;

   uint8_t *ptr = maria_surface + (maria_visibleArea.top - maria_displayArea.top) * Rect_GetLength(&maria_visibleArea);
   ptr += maria_visibleArea.left - maria_displayArea.left;

   if ((x == 0x7fff) | (y == 0x7fff))
      return;

   for (ypos = y_start; ypos <= y_end; ypos++)  /* draw crosshair */
   {
      if (ypos < 0) continue;
      if (ypos >= 224) continue;

      for (xpos = x_start; xpos <= x_end; xpos++)
      {
         if (xpos < 0) continue;
         if (xpos >= 320) continue;

         ptr[ypos * 320 + xpos] = ((xpos | ypos) & 1) ? color : 0xff;
      }
   }
}

static void process_lightgun(int port)
{
   int btn = input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_TRIGGER) ? 1 : 0;
   int x = input_state_cb(port, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
   int y = input_state_cb(port, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);

   x = ((x + 0x7FFF) * 320) / 0xFFFF;  /* scale + clamp */

   if (x < 0)
      x = 0;
   else if (x >= 320)
      x = 320 - 1;


   y = ((y + 0x7FFF) * 224) / 0xFFFF;

   if (y < 0)
      y = 0;
   else if (y >= 224)
      y = 224 - 1;


   if (input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN))
   {
      x = 0x7FFF;
      y = 0x7FFF;
   }


   if (btn)
      lightgun_trigger++;
   else
      lightgun_trigger = 0;


   lightgun_x = x;
   lightgun_y = y;

   lightgun_Cursor(x, y);
}

static void update_input(void)
{
   unsigned i,j;
   unsigned joypad_bits[2];
   unsigned j2_override_right = 0;
   unsigned j2_override_left  = 0;
   unsigned j2_override_down  = 0;
   unsigned j2_override_up    = 0;
   int port;

    
   /*
    * ----------------------------------------------------------------------------
    * SetInput
    * +----------+--------------+-------------------------------------------------
    * | Offset   | Controller   | Control
    * +----------+--------------+-------------------------------------------------
    * | 00       | Joystick 1   | Right
    * | 01       | Joystick 1   | Left
    * | 02       | Joystick 1   | Down
    * | 03       | Joystick 1   | Up
    * | 04       | Joystick 1   | Button 1
    * | 05       | Joystick 1   | Button 2
    * | 06       | Joystick 2   | Right
    * | 07       | Joystick 2   | Left
    * | 08       | Joystick 2   | Down
    * | 09       | Joystick 2   | Up
    * | 10       | Joystick 2   | Button 1
    * | 11       | Joystick 2   | Button 2
    * | 12       | Console      | Reset
    * | 13       | Console      | Select
    * | 14       | Console      | Pause
    * | 15       | Console      | Left Difficulty
    * | 16       | Console      | Right Difficulty
    * +----------+--------------+-------------------------------------------------
    */

   input_poll_cb();

   if (libretro_supports_bitmasks)
   {
      for (j = 0; j < 2; j++)
         joypad_bits[j] = input_state_cb(j, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
   }
   else
   {
      for (j = 0; j < 2; j++)
      {
         joypad_bits[j] = 0;
         for (i = 0; i < (RETRO_DEVICE_ID_JOYPAD_R3+1); i++)
            joypad_bits[j] |= input_state_cb(j, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;
      }
   }

   /* If dual stick controller hack is enabled,
    * fetch overrides for player 2's joystick
    * right/left/down/up values */
   if (gamepad_dual_stick_hack)
   {
      int analog_x = input_state_cb(0, RETRO_DEVICE_ANALOG,
            RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
      int analog_y = input_state_cb(0, RETRO_DEVICE_ANALOG,
            RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);

      if (analog_x >= GAMEPAD_ANALOG_THRESHOLD)
         j2_override_right = 1;
      else if (analog_x <= -GAMEPAD_ANALOG_THRESHOLD)
         j2_override_left  = 1;

      if (analog_y >= GAMEPAD_ANALOG_THRESHOLD)
         j2_override_down  = 1;
      else if (analog_y <= -GAMEPAD_ANALOG_THRESHOLD)
         j2_override_up    = 1;
   }

   keyboard_data[0]  = (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT))  ? 1 : 0;
   keyboard_data[1]  = (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT))   ? 1 : 0;
   keyboard_data[2]  = (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN))   ? 1 : 0;
   keyboard_data[3]  = (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_UP))     ? 1 : 0;
   keyboard_data[4]  = (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_B))      ? 1 : 0;
   keyboard_data[5]  = (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_A))      ? 1 : 0;

   keyboard_data[6]  = (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT))  ? 1 : j2_override_right;
   keyboard_data[7]  = (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT))   ? 1 : j2_override_left;
   keyboard_data[8]  = (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN))   ? 1 : j2_override_down;
   keyboard_data[9]  = (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_UP))     ? 1 : j2_override_up;
   keyboard_data[10] = (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_B))      ? 1 : 0;
   keyboard_data[11] = (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_A))      ? 1 : 0;

   keyboard_data[12] = (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_X))      ? 1 : 0;
   keyboard_data[13] = (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT)) ? 1 : 0;
   keyboard_data[14] = (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_START))  ? 1 : 0;

   if (!left_difficulty_hold && (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L)))
   {
      left_difficulty ^= 1;
      left_difficulty_hold = 1;

      retro_print_message(left_difficulty ? "Left switch: Left (B)" : "Left switch: Right (A)");
   }
   else if (left_difficulty_hold && !(joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L)))
      left_difficulty_hold = 0;

   if (!right_difficulty_hold && (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R)))
   {
      right_difficulty ^= 1;
      right_difficulty_hold = 1;

      retro_print_message(right_difficulty ? "Right switch: Left (B)" : "Right switch: Right (A)");
   }
   else if (right_difficulty_hold && !(joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R)))
      right_difficulty_hold = 0;

   keyboard_data[15] = left_difficulty;
   keyboard_data[16] = right_difficulty;


   lightgun_enabled = 0;

   for (port = 0; port < 2; port++)
   {
      switch (port_devices[port])
      {
	  case RETRO_DEVICE_LIGHTGUN:
         process_lightgun(port);

         lightgun_enabled = 1;
         keyboard_data[3] = (lightgun_trigger >= 4) ? 0 : 1;  /* inverted */
		 break;
	  }
   }
}

/************************************
 * libretro implementation
 ************************************/

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name = "ProSystem";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version  = "1.3g" GIT_VERSION;
   info->need_fullpath    = false;
   info->valid_extensions = "a78|bin|cdf";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   memset(info, 0, sizeof(*info));
   info->timing.fps            = (cartridge_region == REGION_NTSC) ? 60 : 50;
   info->timing.sample_rate    = audio_rate;
   info->geometry.base_width   = videoWidth;
   info->geometry.base_height  = videoHeight;
   info->geometry.max_width    = 320;
   info->geometry.max_height   = 272;

   switch (display_aspect)
   {
   default: /* Native */
      info->geometry.aspect_ratio = (float) videoWidth / (float) videoHeight;
      break;

   case 1:  /* Pixel Aspect @ 320 */
      info->geometry.aspect_ratio = (float) videoWidth * ((cartridge_region == REGION_NTSC) ? 6.0f / 7.0f : 1.040f) / (float) videoHeight;
      break;

   case 2:  /* TV */
      info->geometry.aspect_ratio = 4.0f / 3.0f;
      break;
   }
}

static void update_geometry(void)
{
   struct retro_system_av_info info;
   retro_get_system_av_info(&info);
   environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &info);
}

static void update_timing(void)
{
   struct retro_system_av_info info;
   retro_get_system_av_info(&info);
   environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
}

static void check_variables(bool first_run)
{
   struct retro_variable var = {0};

   /* Read dual stick controller setting */
   var.key   = "prosystem_gamepad_dual_stick_hack";
   var.value = NULL;

   gamepad_dual_stick_hack = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      if (strcmp(var.value, "enabled") == 0)
         gamepad_dual_stick_hack = true;


   var.key   = "prosystem_aspect_ratio";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int old_aspect = display_aspect;

	  if (strcmp(var.value, "PAR") == 0)
	     display_aspect = 1;

	  else if (strcmp(var.value, "4:3") == 0)
	     display_aspect = 2;

	  else
         display_aspect = 0;

      if (display_aspect != old_aspect)
         update_geometry();
   }


   var.key   = "prosystem_tia_lowpass";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int val = atoi(var.value);

      tia_SetLowpass(val);
   }


   var.key   = "prosystem_pokey_lowpass";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int val = atoi(var.value);

      pokey_SetLowpass(val);
   }


   var.key   = "prosystem_mixer_volume";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      mixer_SetMixerVolume(strtol(var.value, NULL, 10));


   var.key   = "prosystem_tia_volume";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      mixer_SetTiaVolume(strtol(var.value, NULL, 10));


   var.key   = "prosystem_pokey_volume";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      mixer_SetPokeyVolume(strtol(var.value, NULL, 10));


   var.key   = "prosystem_bupchip_volume";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      mixer_SetBupchipVolume(strtol(var.value, NULL, 10));


   var.key   = "prosystem_bios_startup";
   var.value = NULL;

   bios_startup = 1;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
	  if (strcmp(var.value, "disabled") == 0)
         bios_startup = 0;
   }


   var.key   = "prosystem_low_pass_filter";
   var.value = NULL;

   low_pass_enabled = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      if (strcmp(var.value, "enabled") == 0)
         low_pass_enabled = true;


   var.key   = "prosystem_low_pass_range_mixer";
   var.value = NULL;

   mixer_SetFilter(0);

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int val = atoi(var.value);

      if (low_pass_enabled)
         mixer_SetFilter(val);
   }


   var.key   = "prosystem_low_pass_range";
   var.value = NULL;

   mixer_SetTiaFilter(0);

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int val = atoi(var.value);

      if (low_pass_enabled)
         mixer_SetTiaFilter(val);
   }


   if (!first_run)
      return;

   /* Only read colour depth option on first run */
   var.key   = "prosystem_color_depth";
   var.value = NULL;

   /* Set 16bpp by default */
   videoPixelBytes = 2;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      if (strcmp(var.value, "24bit") == 0)
         videoPixelBytes = 4;


   var.key   = "prosystem_audio_rate";
   var.value = NULL;

   audio_rate = 48000;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      audio_rate = atoi(var.value);


   var.key   = "prosystem_console_region";
   var.value = NULL;

   region_type = REGION_AUTO;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
	  if (strcmp(var.value, "NTSC") == 0)
	     region_type = REGION_NTSC;

	  else if (strcmp(var.value, "PAL") == 0)
	     region_type = REGION_PAL;
   }


   var.key   = "prosystem_highscore_save";
   var.value = NULL;

   highscore_save = 1;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
	  if (strcmp(var.value, "Disabled") == 0)
	     highscore_save = 0;

	  else if (strcmp(var.value, "Per-Game") == 0)
	     highscore_save = 2;
   }


   var.key   = "prosystem_highscore_name";
   var.value = NULL;

   highscore_name = 1;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
	  if (strcmp(var.value, "None") == 0)
         highscore_name = 0;

	  else if (strcmp(var.value, "Per-Game") == 0)
	     highscore_name = 2;

	  else if (strcmp(var.value, "HSC") == 0)
         highscore_name = 3;

	  else if (strcmp(var.value, "Prosystem") == 0)
         highscore_name = 4;
   }
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   if (port >= 2)
      return;

   switch (device)
   {
      case RETRO_DEVICE_JOYPAD:
         port_devices[port] = RETRO_DEVICE_JOYPAD;
         break;

      case RETRO_DEVICE_MOUSE:
         port_devices[port] = RETRO_DEVICE_MOUSE;
         break;

      case RETRO_DEVICE_LIGHTGUN:
         port_devices[port] = RETRO_DEVICE_LIGHTGUN;
         break;

      case RETRO_DEVICE_NONE:
         port_devices[port] = RETRO_DEVICE_NONE;
         break;
   }
}

bool get_fast_savestates(void)
{
   int result = -1;
   bool okay = false;
   okay = environ_cb(RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE, &result);
   if (okay)
   {
      return 0 != (result & 4);
   }
   else
   {
      return 0;
   }
}

size_t retro_serialize_size(void) 
{
   fast_savestates = get_fast_savestates();
   if (fast_savestates)
      return FAST_SAVE_STATE_SIZE;
   else
      return SAVE_STATE_SIZE;
}

bool retro_serialize(void *data, size_t size)
{
   fast_savestates = get_fast_savestates();
   if ((fast_savestates && size != FAST_SAVE_STATE_SIZE) || (!fast_savestates && size != SAVE_STATE_SIZE))
      return false;

   return prosystem_SaveState((uint8_t*)data, fast_savestates);
}

bool retro_unserialize(const void *data, size_t size)
{
   fast_savestates = get_fast_savestates();
   if ((fast_savestates && size != FAST_SAVE_STATE_SIZE) || (!fast_savestates && size != SAVE_STATE_SIZE))
      return false;

   return prosystem_LoadState((const uint8_t*)data, fast_savestates);
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

bool retro_load_game(const struct retro_game_info *info)
{
   enum retro_pixel_format fmt;
   char biospath[512];
   const char *system_directory_c             = NULL;
   const struct retro_game_info_ext *info_ext = NULL;
#ifdef _WIN32
   char slash = '\\';
#else
   char slash = '/';
#endif

   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "1" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "2" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Console Reset" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Console Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Console Pause" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Left Difficulty" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Right Difficulty" },
      { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "(Dual Stick) P2 X-Axis" },
      { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "(Dual Stick) P2 Y-Axis" },

      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "1" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "2" },

      { 0 },
   };

   if (!info)
      return false;

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

   /* Set color depth */
   check_variables(true);

   if (videoPixelBytes == 4)
   {
      fmt = RETRO_PIXEL_FORMAT_XRGB8888;
      if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
      {
         if (log_cb)
            log_cb(RETRO_LOG_INFO, "[ProSystem]: XRGB8888 is not supported - trying RGB565...\n");

         /* Fallback to RETRO_PIXEL_FORMAT_RGB565 */
         videoPixelBytes = 2;
      }
   }

   if (videoPixelBytes == 2)
   {
      fmt = RETRO_PIXEL_FORMAT_RGB565;
      if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
      {
         if (log_cb)
            log_cb(RETRO_LOG_INFO, "[ProSystem]: RGB565 is not supported.\n");
         return false;
      }
   }

   /* Difficulty switches: 
    * Left position = (B)eginner, Right position = (A)dvanced
    * Left difficulty switch defaults to left position, "(B)eginner"
    */
   left_difficulty = 1;

   /* Right difficulty switch defaults to right position,
    * "(A)dvanced", which fixes Tower Toppler
    */
   right_difficulty = 0;

   if (environ_cb(RETRO_ENVIRONMENT_GET_GAME_INFO_EXT, &info_ext) &&
       info_ext->persistent_data)
      persistent_data = true;

   if (info_ext->name)
      strcpy(cartridge_title, info_ext->name);

   prosystem_SetRate(audio_rate);

   if (info->size >= 10 && memcmp(info->data, "ProSystem", 9) == 0)
   {
      /* CDF file. */
      int ok;
      char* lastSlash = (char *) strrchr(info->path, slash);
      size_t baseSize = (lastSlash == NULL) ? strlen(info->path) : (size_t) (lastSlash - info->path);
      char* workingDir = (char *) malloc(baseSize + 1);
      memcpy(workingDir, info->path, baseSize);
      workingDir[baseSize] = '\0';

      ok = cartridge_LoadFromCDF((const char *) info->data, info->size, workingDir);

      free(workingDir);

      if (ok == 0)
         return false;
   }
   else if (!cartridge_Load(persistent_data,
            (const uint8_t*)info->data, info->size))
      return false;

   environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_directory_c);

   /* BIOS is optional */
   if (cartridge_region == REGION_PAL)
      sprintf(biospath, "%s%c%s", system_directory_c, slash, "7800 BIOS (E).rom");
   else
      sprintf(biospath, "%s%c%s", system_directory_c, slash, "7800 BIOS (U).rom");
   
   if (bios_Load(biospath))
   {
      bios_enabled = true;

      if (log_cb)
         log_cb(RETRO_LOG_INFO, "[ProSystem]: System BIOS loaded\n");
   }

   /* High Score Cart is optional */
   if (cartridge_region == REGION_PAL)
      sprintf(biospath, "%s%c%s", system_directory_c, slash, "7800 Highscore (E).rom");
   else
      sprintf(biospath, "%s%c%s", system_directory_c, slash, "7800 Highscore (U).rom");

   if (highscore_Load(biospath))
   {
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "[ProSystem]: High Score Cart loaded\n");

      sprintf(biospath, "%s%c%s", system_directory_c, slash, "7800 Highscore.sav");

      if (highscore_ReadNvram(biospath))  /* per core overwrites later */
	  {
         if (log_cb)
            log_cb(RETRO_LOG_INFO, "[ProSystem]: High Score file found\n");

         memcpy(highscore_globalname, memory_nvram + 8, 33);
	  }
   }

   retro_reset();

   display_ResetPalette();

   if (log_cb)
   {
      log_cb(RETRO_LOG_INFO, "[ProSystem]: Cartridge info\n");

      switch (cartridge_type)
	  {
      case CARTRIDGE_TYPE_LINEAR: log_cb(RETRO_LOG_INFO, "- Mapper = Linear\n"); break;
      case CARTRIDGE_TYPE_SUPERGAME: log_cb(RETRO_LOG_INFO, "- Mapper = SuperGame\n"); break;
      case CARTRIDGE_TYPE_ACTIVISION: log_cb(RETRO_LOG_INFO, "- Mapper = Activision\n"); break;
      case CARTRIDGE_TYPE_ABSOLUTE: log_cb(RETRO_LOG_INFO, "- Mapper = Absolute\n"); break;
      case CARTRIDGE_TYPE_SOUPER: log_cb(RETRO_LOG_INFO, "- Mapper = Souper\n"); break;
      }

      if (cartridge_exram)
         log_cb(RETRO_LOG_INFO, "- Exram = 16 KB @ 4000\n");

      if (cartridge_exram_a8)
         log_cb(RETRO_LOG_INFO, "- Exram = 2 KB @ 4000, Mirrored @ A8\n");

      if (cartridge_exram_x2)
         log_cb(RETRO_LOG_INFO, "- Paging RAM @ 4000, Not supported\n");

      if (cartridge_exrom)
         log_cb(RETRO_LOG_INFO, "- Exrom = 16 KB @ 4000\n");

      if (cartridge_exfix)
         log_cb(RETRO_LOG_INFO, "- Exfix = bank6 @ 4000\n");

      if (cartridge_bankset)
         log_cb(RETRO_LOG_INFO, "- Bankset ROM\n");

      if (cartridge_exram_m2)
         log_cb(RETRO_LOG_INFO, "- Bankset RAM\n");

      switch (cartridge_pokey)
	  {
	  case POKEY_AT_4000: log_cb(RETRO_LOG_INFO, "- Audio = POKEY @ 4000\n"); break;
	  case POKEY_AT_450: log_cb(RETRO_LOG_INFO, "- Audio = POKEY @ 450\n"); break;
	  case POKEY_AT_440: log_cb(RETRO_LOG_INFO, "- Audio = POKEY @ 440\n"); break;
	  case POKEY_AT_800: log_cb(RETRO_LOG_INFO, "- Audio = POKEY @ 800\n"); break;
	  }

      if (cartridge_ym2151)
         log_cb(RETRO_LOG_INFO, "- Audio = YM2151 @ 460\n");

      if (cartridge_bupchip)
         log_cb(RETRO_LOG_INFO, "- Audio = Bupchip\n");

      if (cartridge_region == 1)
         log_cb(RETRO_LOG_INFO, "- Region = PAL\n");
	  else
         log_cb(RETRO_LOG_INFO, "- Region = NTSC\n");
   }

   return true;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
   (void)game_type;
   (void)info;
   (void)num_info;
   return false;
}

void retro_unload_game(void) 
{
   char biospath[512];
   const char *system_directory_c             = NULL;
#ifdef _WIN32
   char slash = '\\';
#else
   char slash = '/';
#endif

   environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_directory_c);

   prosystem_Close(persistent_data);

   if (highscore_enabled)
   {
      sprintf(biospath, "%s%c%s", system_directory_c, slash, "7800 Highscore.sav");

      if (highscore_save == 1)  /* global */
         highscore_WriteNvram(biospath);

	  else if (highscore_name == 1)  /* global */
         highscore_WriteNvramName(biospath, memory_nvram + 8);
   }

   persistent_data = false;
}

unsigned retro_get_region(void)
{
    return (cartridge_region == REGION_NTSC) ? RETRO_REGION_NTSC : RETRO_REGION_PAL;
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void *retro_get_memory_data(unsigned id)
{
   void* data = NULL;

   switch(id)
   {
   case RETRO_MEMORY_SAVE_RAM:
      if (highscore_enabled && highscore_save == 2)  /* per game */
         data = memory_nvram;
      break;

   case RETRO_MEMORY_SYSTEM_RAM:
      data = memory_ram;
      break;
   }

   return data;
}

size_t retro_get_memory_size(unsigned id)
{
   switch(id)
   {
   case RETRO_MEMORY_SAVE_RAM:
      if (highscore_enabled && highscore_save == 2)  /* per game */
         return 0x800;
      break;

   case RETRO_MEMORY_SYSTEM_RAM:
      return 0x1000;
   }

   return 0;
}

void retro_init(void)
{
   struct retro_log_callback log;
   unsigned level = 5;

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;

   environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;

#ifdef _3DS
   videoBuffer = (uint8_t*)linearMemAlign(VIDEO_BUFFER_SIZE * sizeof(uint8_t), 128);
#else
   videoBuffer = (uint8_t*)malloc(VIDEO_BUFFER_SIZE * sizeof(uint8_t));
#endif
}

void retro_deinit(void)
{
   libretro_supports_bitmasks = false;
   gamepad_dual_stick_hack    = false;
   low_pass_enabled           = false;
   low_pass_prev              = 0;

   if (videoBuffer)
   {
#ifdef _3DS
      linearFree(videoBuffer);
#else
      free(videoBuffer);
#endif
      videoBuffer = NULL;
   }
}

void retro_reset(void)
{
   prosystem_Reset();

   if (!bios_startup)
      tia_Write(INPTCTRL, 22);

   first_frame = true;
   lightgun_detect = true;
}

void retro_run(void)
{
   const uint8_t *buffer = NULL;
   uint32_t video_pitch  = 320;
   bool options_updated  = false;


   if (first_frame)
   {
      first_frame = false;

      if (highscore_enabled)
	  {
         if (highscore_name == 0)
            highscore_SetName(" ");

	     else if (highscore_name == 1)
            memcpy(memory_nvram + 8, highscore_globalname, 33);

         else if (highscore_name == 3)
            highscore_SetName("HSC");

         else if (highscore_name == 4)
            highscore_SetName("PROSYSTEM");
	  }
   }

   videoWidth  = Rect_GetLength(&maria_visibleArea);
   videoHeight = Rect_GetHeight(&maria_visibleArea);

   /* Core options */
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &options_updated) && options_updated)
      check_variables(false);

   update_input();

   prosystem_ExecuteFrame(keyboard_data); /* wants input */

   if (port_devices[0] == RETRO_DEVICE_LIGHTGUN || port_devices[1] == RETRO_DEVICE_LIGHTGUN)
      draw_lightgun_cursor(lightgun_x, lightgun_y, 255);

   buffer = maria_surface + ((maria_visibleArea.top - maria_displayArea.top) * Rect_GetLength(&maria_visibleArea));
   if (videoPixelBytes == 2)
   {
      BLIT_VIDEO_BUFFER(uint16_t, buffer, display_palette16, videoWidth, videoHeight, video_pitch, videoBuffer);
   }
   else
   {
      BLIT_VIDEO_BUFFER(uint32_t, buffer, display_palette32, videoWidth, videoHeight, video_pitch, videoBuffer);
   }

   video_cb(videoBuffer, videoWidth, videoHeight, videoWidth * videoPixelBytes);
   audio_batch_cb(mixer_buffer, mixer_outCount);
}
