/*
  u8x8_d_sdl_128x64.c
*/

#include "u8g2.h"
#include "EmulatorLog.h"
#ifndef NO_SDL
#include "SDL.h"
#endif
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define HEIGHT (64)
//#define WIDTH 128

#define W(x,w) (((x)*(w))/100)

#ifndef NO_SDL
SDL_Window *u8g_sdl_window;
SDL_Surface *u8g_sdl_screen;
SDL_Surface *u8g_sdl_framebuffer;
#endif

int u8g_sdl_multiple = 3;
uint32_t u8g_sdl_color[256];
int u8g_sdl_height, u8g_sdl_width;
static int u8g_sdl_inited = 0;

typedef struct
{
  int valid;
  int x;
  int y;
  int w;
  int h;
} u8g_sdl_window_state_t;

typedef struct
{
  int oled_off_checkerboard;
  int oled_off_checker_size;
  uint8_t window_bg_r;
  uint8_t window_bg_g;
  uint8_t window_bg_b;
  uint8_t oled_off_r;
  uint8_t oled_off_g;
  uint8_t oled_off_b;
  uint8_t oled_off_alt_r;
  uint8_t oled_off_alt_g;
  uint8_t oled_off_alt_b;
  uint8_t oled_dim1_r;
  uint8_t oled_dim1_g;
  uint8_t oled_dim1_b;
  uint8_t oled_dim2_r;
  uint8_t oled_dim2_g;
  uint8_t oled_dim2_b;
  uint8_t oled_on_r;
  uint8_t oled_on_g;
  uint8_t oled_on_b;
} u8g_sdl_theme_t;

static u8g_sdl_window_state_t u8g_sdl_window_state = {0, 0, 0, 0, 0};
static u8g_sdl_theme_t u8g_sdl_theme =
{
  1,
  7,
  0, 0, 0,
  0, 0, 0,
  30, 30, 30,
  127, 57, 26,
  203, 91, 42,
  254, 114, 53
};

static int u8g_sdl_get_sidecar_path(const char *filename, char *buf, size_t buf_size)
{
#ifndef NO_SDL
  char *base_path;

  if ( filename == NULL || buf == NULL || buf_size == 0 )
    return 0;

  base_path = SDL_GetBasePath();
  if ( base_path != NULL && base_path[0] != '\0' )
  {
    snprintf(buf, buf_size, "%s%s", base_path, filename);
    SDL_free(base_path);
    return 1;
  }
  if ( base_path != NULL )
    SDL_free(base_path);
#endif

  if ( filename == NULL || buf == NULL || buf_size == 0 )
    return 0;
  snprintf(buf, buf_size, "%s", filename);
  return 1;
}

static int u8g_sdl_get_window_state_path(char *buf, size_t buf_size)
{
  return u8g_sdl_get_sidecar_path("WouoUI.window.ini", buf, buf_size);
}

static int u8g_sdl_get_theme_path(char *buf, size_t buf_size)
{
  return u8g_sdl_get_sidecar_path("WouoUI.colors.ini", buf, buf_size);
}

static char *u8g_sdl_trim(char *s)
{
  char *end;

  if ( s == NULL )
    return NULL;

  while ( *s != '\0' && isspace((unsigned char)*s) )
    s++;

  if ( *s == '\0' )
    return s;

  end = s + strlen(s) - 1;
  while ( end >= s && isspace((unsigned char)*end) )
  {
    *end = '\0';
    end--;
  }
  return s;
}

static int u8g_sdl_ieq(const char *a, const char *b)
{
  if ( a == NULL || b == NULL )
    return 0;

  while ( *a != '\0' && *b != '\0' )
  {
    if ( tolower((unsigned char)*a) != tolower((unsigned char)*b) )
      return 0;
    a++;
    b++;
  }
  return *a == '\0' && *b == '\0';
}

static int u8g_sdl_hex_value(int c)
{
  if ( c >= '0' && c <= '9' )
    return c - '0';
  if ( c >= 'a' && c <= 'f' )
    return c - 'a' + 10;
  if ( c >= 'A' && c <= 'F' )
    return c - 'A' + 10;
  return -1;
}

static int u8g_sdl_parse_hex_byte(const char *s, uint8_t *value)
{
  int hi;
  int lo;

  if ( s == NULL || value == NULL )
    return 0;

  hi = u8g_sdl_hex_value((unsigned char)s[0]);
  lo = u8g_sdl_hex_value((unsigned char)s[1]);
  if ( hi < 0 || lo < 0 )
    return 0;

  *value = (uint8_t)((hi << 4) | lo);
  return 1;
}

static int u8g_sdl_parse_color(const char *value, uint8_t *r, uint8_t *g, uint8_t *b)
{
  const char *p = value;

  if ( p == NULL || r == NULL || g == NULL || b == NULL )
    return 0;

  if ( p[0] == '#' )
    p++;
  else if ( p[0] == '0' && (p[1] == 'x' || p[1] == 'X') )
    p += 2;

  if ( strlen(p) != 6 )
    return 0;

  if ( u8g_sdl_parse_hex_byte(p, r) == 0 )
    return 0;
  if ( u8g_sdl_parse_hex_byte(p + 2, g) == 0 )
    return 0;
  if ( u8g_sdl_parse_hex_byte(p + 4, b) == 0 )
    return 0;
  return 1;
}

static void u8g_sdl_set_theme_color(const char *key, const char *value)
{
  uint8_t r;
  uint8_t g;
  uint8_t b;

  if ( u8g_sdl_ieq(key, "oled_off_mode") )
  {
    if ( u8g_sdl_ieq(value, "checkerboard") || u8g_sdl_ieq(value, "checker") )
      u8g_sdl_theme.oled_off_checkerboard = 1;
    else
      u8g_sdl_theme.oled_off_checkerboard = 0;
    return;
  }
  if ( u8g_sdl_ieq(key, "oled_off_checker_size") )
  {
    int size = atoi(value);
    if ( size < 1 )
      size = 1;
    if ( size > 16 )
      size = 16;
    u8g_sdl_theme.oled_off_checker_size = size;
    return;
  }

  if ( u8g_sdl_parse_color(value, &r, &g, &b) == 0 )
    return;

  if ( u8g_sdl_ieq(key, "window_bg") )
  {
    u8g_sdl_theme.window_bg_r = r;
    u8g_sdl_theme.window_bg_g = g;
    u8g_sdl_theme.window_bg_b = b;
  }
  else if ( u8g_sdl_ieq(key, "oled_off") )
  {
    u8g_sdl_theme.oled_off_r = r;
    u8g_sdl_theme.oled_off_g = g;
    u8g_sdl_theme.oled_off_b = b;
  }
  else if ( u8g_sdl_ieq(key, "oled_off_alt") )
  {
    u8g_sdl_theme.oled_off_alt_r = r;
    u8g_sdl_theme.oled_off_alt_g = g;
    u8g_sdl_theme.oled_off_alt_b = b;
  }
  else if ( u8g_sdl_ieq(key, "oled_dim1") )
  {
    u8g_sdl_theme.oled_dim1_r = r;
    u8g_sdl_theme.oled_dim1_g = g;
    u8g_sdl_theme.oled_dim1_b = b;
  }
  else if ( u8g_sdl_ieq(key, "oled_dim2") )
  {
    u8g_sdl_theme.oled_dim2_r = r;
    u8g_sdl_theme.oled_dim2_g = g;
    u8g_sdl_theme.oled_dim2_b = b;
  }
  else if ( u8g_sdl_ieq(key, "oled_on") )
  {
    u8g_sdl_theme.oled_on_r = r;
    u8g_sdl_theme.oled_on_g = g;
    u8g_sdl_theme.oled_on_b = b;
  }
}

static void u8g_sdl_write_default_theme_file(void)
{
  char path[1024];
  FILE *fp;

  if ( u8g_sdl_get_theme_path(path, sizeof(path)) == 0 )
    return;

  fp = fopen(path, "rb");
  if ( fp != NULL )
  {
    fclose(fp);
    return;
  }

  fp = fopen(path, "wb");
  if ( fp == NULL )
    return;

  fprintf(fp, "# WouoUI emulator colors\n");
  fprintf(fp, "# Format: key=#RRGGBB\n");
  fprintf(fp, "# oled_off_mode=solid or checkerboard\n");
  fprintf(fp, "# oled_off_checker_size=1 for classic small checker\n");
  fprintf(fp, "window_bg=#%02X%02X%02X\n", u8g_sdl_theme.window_bg_r, u8g_sdl_theme.window_bg_g, u8g_sdl_theme.window_bg_b);
  fprintf(fp, "oled_off_mode=%s\n", u8g_sdl_theme.oled_off_checkerboard ? "checkerboard" : "solid");
  fprintf(fp, "oled_off_checker_size=%d\n", u8g_sdl_theme.oled_off_checker_size);
  fprintf(fp, "oled_off=#%02X%02X%02X\n", u8g_sdl_theme.oled_off_r, u8g_sdl_theme.oled_off_g, u8g_sdl_theme.oled_off_b);
  fprintf(fp, "oled_off_alt=#%02X%02X%02X\n", u8g_sdl_theme.oled_off_alt_r, u8g_sdl_theme.oled_off_alt_g, u8g_sdl_theme.oled_off_alt_b);
  fprintf(fp, "oled_dim1=#%02X%02X%02X\n", u8g_sdl_theme.oled_dim1_r, u8g_sdl_theme.oled_dim1_g, u8g_sdl_theme.oled_dim1_b);
  fprintf(fp, "oled_dim2=#%02X%02X%02X\n", u8g_sdl_theme.oled_dim2_r, u8g_sdl_theme.oled_dim2_g, u8g_sdl_theme.oled_dim2_b);
  fprintf(fp, "oled_on=#%02X%02X%02X\n", u8g_sdl_theme.oled_on_r, u8g_sdl_theme.oled_on_g, u8g_sdl_theme.oled_on_b);
  fclose(fp);
}

static void u8g_sdl_load_theme(void)
{
  char path[1024];
  FILE *fp;
  char line[256];

  u8g_sdl_write_default_theme_file();

  if ( u8g_sdl_get_theme_path(path, sizeof(path)) == 0 )
    return;

  fp = fopen(path, "rb");
  if ( fp == NULL )
    return;

  while ( fgets(line, sizeof(line), fp) != NULL )
  {
    char *key;
    char *value;
    char *sep;

    key = u8g_sdl_trim(line);
    if ( key == NULL || *key == '\0' || *key == '#' || *key == ';' )
      continue;

    sep = strchr(key, '=');
    if ( sep == NULL )
      continue;

    *sep = '\0';
    value = u8g_sdl_trim(sep + 1);
    key = u8g_sdl_trim(key);
    if ( value == NULL || key == NULL || *key == '\0' || *value == '\0' )
      continue;

    u8g_sdl_set_theme_color(key, value);
  }

  fclose(fp);
}

static void u8g_sdl_apply_theme(void)
{
#ifndef NO_SDL
  int x;
  int y;
  int size;
  uint32_t *row;

  if ( u8g_sdl_framebuffer == NULL )
    return;

  u8g_sdl_color[0] = SDL_MapRGB(u8g_sdl_framebuffer->format, u8g_sdl_theme.oled_off_r, u8g_sdl_theme.oled_off_g, u8g_sdl_theme.oled_off_b);
  u8g_sdl_color[5] = SDL_MapRGB(u8g_sdl_framebuffer->format, u8g_sdl_theme.oled_off_alt_r, u8g_sdl_theme.oled_off_alt_g, u8g_sdl_theme.oled_off_alt_b);
  u8g_sdl_color[1] = SDL_MapRGB(u8g_sdl_framebuffer->format, u8g_sdl_theme.oled_dim1_r, u8g_sdl_theme.oled_dim1_g, u8g_sdl_theme.oled_dim1_b);
  u8g_sdl_color[2] = SDL_MapRGB(u8g_sdl_framebuffer->format, u8g_sdl_theme.oled_dim2_r, u8g_sdl_theme.oled_dim2_g, u8g_sdl_theme.oled_dim2_b);
  u8g_sdl_color[3] = SDL_MapRGB(u8g_sdl_framebuffer->format, u8g_sdl_theme.oled_on_r, u8g_sdl_theme.oled_on_g, u8g_sdl_theme.oled_on_b);
  u8g_sdl_color[4] = SDL_MapRGB(u8g_sdl_framebuffer->format, u8g_sdl_theme.window_bg_r, u8g_sdl_theme.window_bg_g, u8g_sdl_theme.window_bg_b);

  if ( u8g_sdl_theme.oled_off_checkerboard == 0 )
  {
    SDL_FillRect(u8g_sdl_framebuffer, NULL, u8g_sdl_color[0]);
    return;
  }

  size = u8g_sdl_theme.oled_off_checker_size;
  if ( size < 1 )
    size = 1;

  for ( y = 0; y < u8g_sdl_height; y++ )
  {
    row = (uint32_t *)(((uint8_t *)(u8g_sdl_framebuffer->pixels)) + y * u8g_sdl_framebuffer->pitch);
    for ( x = 0; x < u8g_sdl_width; x++ )
      row[x] = ((((x / size) + (y / size)) & 1) != 0) ? u8g_sdl_color[5] : u8g_sdl_color[0];
  }
#endif
}

static int u8g_sdl_get_off_pixel_color(int x, int y)
{
  int size;

  if ( u8g_sdl_theme.oled_off_checkerboard == 0 )
    return 0;
  size = u8g_sdl_theme.oled_off_checker_size;
  if ( size < 1 )
    size = 1;
  return ((((x / size) + (y / size)) & 1) != 0) ? 5 : 0;
}

static void u8g_sdl_load_window_state(void)
{
  char path[1024];
  FILE *fp;
  int x, y, w, h;

  if ( u8g_sdl_get_window_state_path(path, sizeof(path)) == 0 )
    return;

  fp = fopen(path, "rb");
  if ( fp == NULL )
    return;

  if ( fscanf(fp, "%d %d %d %d", &x, &y, &w, &h) == 4 )
  {
    if ( w >= u8g_sdl_width && h >= u8g_sdl_height )
    {
      u8g_sdl_window_state.valid = 1;
      u8g_sdl_window_state.x = x;
      u8g_sdl_window_state.y = y;
      u8g_sdl_window_state.w = w;
      u8g_sdl_window_state.h = h;
    }
  }
  fclose(fp);
}

static void u8g_sdl_save_window_state(void)
{
#ifndef NO_SDL
  char path[1024];
  FILE *fp;
  int x, y, w, h;

  if ( u8g_sdl_window == NULL )
    return;
  if ( u8g_sdl_get_window_state_path(path, sizeof(path)) == 0 )
    return;

  SDL_GetWindowPosition(u8g_sdl_window, &x, &y);
  SDL_GetWindowSize(u8g_sdl_window, &w, &h);
  if ( w < u8g_sdl_width || h < u8g_sdl_height )
    return;

  fp = fopen(path, "wb");
  if ( fp == NULL )
    return;

  fprintf(fp, "%d %d %d %d\n", x, y, w, h);
  fclose(fp);
#endif
}

static void u8g_sdl_shutdown(void)
{
#ifndef NO_SDL
  if ( u8g_sdl_window != NULL )
    u8g_sdl_save_window_state();
  if ( u8g_sdl_framebuffer != NULL )
  {
    SDL_FreeSurface(u8g_sdl_framebuffer);
    u8g_sdl_framebuffer = NULL;
  }
  u8g_sdl_screen = NULL;
  if ( u8g_sdl_window != NULL )
  {
    SDL_DestroyWindow(u8g_sdl_window);
    u8g_sdl_window = NULL;
  }
  SDL_Quit();
#endif
}

static void u8g_sdl_set_pixel(int x, int y, int idx)
{
  uint32_t *ptr;
  
  if ( y >= u8g_sdl_height )
    return;
  if ( y < 0 )
    return;
  if ( x >= u8g_sdl_width )
    return;
  if ( x < 0 )
    return;
  
#ifndef NO_SDL
      if ( u8g_sdl_framebuffer == NULL )
        return;
      ptr = (uint32_t *)(((uint8_t *)(u8g_sdl_framebuffer->pixels)) + y * u8g_sdl_framebuffer->pitch);
      ptr += x;
      *ptr = u8g_sdl_color[idx];
#endif
}

static void u8g_sdl_present(void)
{
#ifndef NO_SDL
  int window_w;
  int window_h;
  float scale_x;
  float scale_y;
  float scale;
  SDL_Rect dst;

  if ( u8g_sdl_window == NULL || u8g_sdl_framebuffer == NULL )
    return;

  SDL_PumpEvents();
  u8g_sdl_screen = SDL_GetWindowSurface(u8g_sdl_window);
  if ( u8g_sdl_screen == NULL )
    return;

  SDL_GetWindowSize(u8g_sdl_window, &window_w, &window_h);
  if ( window_w <= 0 || window_h <= 0 )
    return;

  SDL_FillRect(u8g_sdl_screen, NULL, SDL_MapRGB(u8g_sdl_screen->format, u8g_sdl_theme.window_bg_r, u8g_sdl_theme.window_bg_g, u8g_sdl_theme.window_bg_b));

  scale_x = (float)window_w / (float)u8g_sdl_width;
  scale_y = (float)window_h / (float)u8g_sdl_height;
  scale = scale_x < scale_y ? scale_x : scale_y;
  if ( scale <= 0.0f )
    scale = 1.0f;

  dst.w = (int)((float)u8g_sdl_width * scale + 0.5f);
  dst.h = (int)((float)u8g_sdl_height * scale + 0.5f);
  if ( dst.w < 1 )
    dst.w = 1;
  if ( dst.h < 1 )
    dst.h = 1;
  dst.x = (window_w - dst.w) / 2;
  dst.y = (window_h - dst.h) / 2;

  SDL_BlitScaled(u8g_sdl_framebuffer, NULL, u8g_sdl_screen, &dst);
  SDL_UpdateWindowSurface(u8g_sdl_window);
#endif
}

static void u8g_sdl_set_8pixel(int x, int y, uint8_t pixel)
{
  int cnt = 8;
  while( cnt > 0 )
  {
    if ( (pixel & 1) == 0 )
    {
      u8g_sdl_set_pixel(x,y,u8g_sdl_get_off_pixel_color(x, y));
    }
    else
    {
      u8g_sdl_set_pixel(x,y,3);
    }
    pixel >>= 1;
    y++;
    cnt--;
  }
}

static void u8g_sdl_set_multiple_8pixel(int x, int y, int cnt, uint8_t *pixel)
{
  uint8_t b;
  while( cnt > 0 )
  {
    b = *pixel;
    u8g_sdl_set_8pixel(x, y, b);
    x++;
    pixel++;
    cnt--;
  }
}

static void u8g_sdl_init(int width, int height)
{
  emulator_log("[sdl] init request %dx%d", width, height);
  if ( u8g_sdl_inited != 0 )
  {
    emulator_log("[sdl] already initialized");
    return;
  }

  u8g_sdl_height = height;
  u8g_sdl_width = width;
  
#ifndef NO_SDL
  
  if (SDL_Init(SDL_INIT_VIDEO) != 0) 
  {
    emulator_log("[sdl] SDL_Init failed: %s", SDL_GetError());
    printf("Unable to initialize SDL:  %s\n", SDL_GetError());
    exit(1);
  }
  emulator_log("[sdl] SDL_Init ok");

  u8g_sdl_load_window_state();
  
  u8g_sdl_window = SDL_CreateWindow("Simulator",
    u8g_sdl_window_state.valid ? u8g_sdl_window_state.x : SDL_WINDOWPOS_CENTERED,
    u8g_sdl_window_state.valid ? u8g_sdl_window_state.y : SDL_WINDOWPOS_CENTERED,
    u8g_sdl_window_state.valid ? u8g_sdl_window_state.w : (u8g_sdl_width * u8g_sdl_multiple),
    u8g_sdl_window_state.valid ? u8g_sdl_window_state.h : (u8g_sdl_height * u8g_sdl_multiple),
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

  if ( u8g_sdl_window == NULL )
  {
    emulator_log("[sdl] SDL_CreateWindow failed: %s", SDL_GetError());
    printf("Couldn't create window: %s\n", SDL_GetError());
    exit(1);
  }
  emulator_log("[sdl] SDL_CreateWindow ok");

  SDL_ShowWindow(u8g_sdl_window);
  SDL_RaiseWindow(u8g_sdl_window);
  SDL_SetWindowMinimumSize(u8g_sdl_window, u8g_sdl_width, u8g_sdl_height);

  u8g_sdl_framebuffer = SDL_CreateRGBSurfaceWithFormat(0, u8g_sdl_width, u8g_sdl_height, 32, SDL_PIXELFORMAT_ARGB8888);
  if ( u8g_sdl_framebuffer == NULL )
  {
    emulator_log("[sdl] SDL_CreateRGBSurfaceWithFormat failed: %s", SDL_GetError());
    printf("Couldn't create framebuffer: %s\n", SDL_GetError());
    exit(1);
  }

  u8g_sdl_screen = SDL_GetWindowSurface(u8g_sdl_window);
  if ( u8g_sdl_screen == NULL )
  {
    emulator_log("[sdl] SDL_GetWindowSurface failed: %s", SDL_GetError());
    printf("Couldn't create screen: %s\n", SDL_GetError());
    exit(1);
  }
  emulator_log("[sdl] SDL_GetWindowSurface ok");
  
  printf("%d bits-per-pixel mode\n", u8g_sdl_framebuffer->format->BitsPerPixel);
  printf("%d bytes-per-pixel mode\n", u8g_sdl_framebuffer->format->BytesPerPixel);

  u8g_sdl_load_theme();
  u8g_sdl_apply_theme();

  u8g_sdl_present();

  atexit(u8g_sdl_shutdown);
#endif  
  u8g_sdl_inited = 1;
  emulator_log("[sdl] init done");
  return;
}



/*
void main(void)
{
  u8g_sdl_init();
  u8g_sdl_set_pixel(0,0,3);
  u8g_sdl_set_pixel(0,1,3);
  u8g_sdl_set_pixel(0,2,3);
  u8g_sdl_set_pixel(1,1,3);
  u8g_sdl_set_pixel(2,2,3);
  while( u8g_sdl_get_key() < 0 )
    ;
}
*/

static const u8x8_display_info_t u8x8_sdl_128x64_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 0,
  /* pre_chip_disable_wait_ns = */ 0,
  /* reset_pulse_width_ms = */ 0, 
  /* post_reset_wait_ms = */ 0, 
  /* sda_setup_time_ns = */ 0,		
  /* sck_pulse_width_ns = */ 0,
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 1,		
  /* i2c_bus_clock_100kHz = */ 0,
  /* data_setup_time_ns = */ 0,
  /* write_pulse_width_ns = */ 0,
  /* tile_width = */ 16,
  /* tile_hight = */ 8,
  /* default_x_offset = */ 0,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 128,
  /* pixel_height = */ 64
};

static const u8x8_display_info_t u8x8_sdl_128x128_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 0,
  /* pre_chip_disable_wait_ns = */ 0,
  /* reset_pulse_width_ms = */ 0,
  /* post_reset_wait_ms = */ 0,
  /* sda_setup_time_ns = */ 0,
  /* sck_pulse_width_ns = */ 0,
  /* sck_clock_hz = */ 4000000UL,
  /* spi_mode = */ 1,
  /* i2c_bus_clock_100kHz = */ 0,
  /* data_setup_time_ns = */ 0,
  /* write_pulse_width_ns = */ 0,
  /* tile_width = */ 16,
  /* tile_hight = */ 16,
  /* default_x_offset = */ 0,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 128,
  /* pixel_height = */ 128
};

static const u8x8_display_info_t u8x8_sdl_240x160_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 0,
  /* pre_chip_disable_wait_ns = */ 0,
  /* reset_pulse_width_ms = */ 0, 
  /* post_reset_wait_ms = */ 0, 
  /* sda_setup_time_ns = */ 0,		
  /* sck_pulse_width_ns = */ 0,
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 1,		
  /* i2c_bus_clock_100kHz = */ 0,
  /* data_setup_time_ns = */ 0,
  /* write_pulse_width_ns = */ 0,
  /* tile_width = */ 30,		/* width of 30*8=240 pixel */
  /* tile_hight = */ 20,		/* height: 160 pixel */
  /* default_x_offset = */ 0,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 240,
  /* pixel_height = */ 160
};


static uint8_t u8x8_d_sdl_gpio(u8x8_t *u8x8, uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr)
{
	static int debounce_cnt = 0;
	static int curr_msg = 0;
	static int db_cnt = 10;
	int event;
  
	if ( curr_msg > 0 )
	{
	  if ( msg == curr_msg )
	  {
		  u8x8_SetGPIOResult(u8x8, 0);
		  if ( debounce_cnt == 0 )
		    curr_msg = 0;
		  else
		    debounce_cnt--;
		  return 1;
	  }
	  
	}
	else
	{
	  event = u8g_sdl_get_key();
	  
	  switch(event)
	  {
		  case 273:
			  curr_msg = U8X8_MSG_GPIO_MENU_UP;
			  debounce_cnt = db_cnt;
			  break;
		  case 274:
			  curr_msg = U8X8_MSG_GPIO_MENU_DOWN;
			  debounce_cnt = db_cnt;
			  break;
		  case 275:
			  curr_msg = U8X8_MSG_GPIO_MENU_NEXT;
			  debounce_cnt = db_cnt;
			  break;
		  case 276:
			  curr_msg = U8X8_MSG_GPIO_MENU_PREV;
			  debounce_cnt = db_cnt;
			  break;
		  case 's':
			  curr_msg = U8X8_MSG_GPIO_MENU_SELECT;
			  debounce_cnt = db_cnt;
			  break;
		  case 'q':
			  curr_msg = U8X8_MSG_GPIO_MENU_HOME;
			  debounce_cnt = db_cnt;
			  break;
	  }
	}
	u8x8_SetGPIOResult(u8x8, 1);
	return 1;
}

uint8_t u8x8_d_sdl_128x64(u8x8_t *u8g2, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, y, c;
  uint8_t *ptr;
  switch(msg)
  {
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:
      u8x8_d_helper_display_setup_memory(u8g2, &u8x8_sdl_128x64_info);
      break;
    case U8X8_MSG_DISPLAY_INIT:
      u8g_sdl_init(128, 64);
      u8x8_d_helper_display_init(u8g2);
      break;
    case U8X8_MSG_DISPLAY_SET_POWER_SAVE:
      break;
    case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
      break;
    case U8X8_MSG_DISPLAY_SET_CONTRAST:
      break;
    case U8X8_MSG_DISPLAY_DRAW_TILE:
      x = ((u8x8_tile_t *)arg_ptr)->x_pos;
      x *= 8;
      x += u8g2->x_offset;
    
      y = ((u8x8_tile_t *)arg_ptr)->y_pos;
      y *= 8;
    
      do
      {
        c = ((u8x8_tile_t *)arg_ptr)->cnt;
        ptr = ((u8x8_tile_t *)arg_ptr)->tile_ptr;
        u8g_sdl_set_multiple_8pixel(x, y, c*8, ptr);
        arg_int--;
	x+=c*8;
      } while( arg_int > 0 );
      
      u8g_sdl_present();
      break;
    default:
      return 0;
  }
  return 1;
}

uint8_t u8x8_d_sdl_240x160(u8x8_t *u8g2, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, y, c;
  uint8_t *ptr;
  switch(msg)
  {
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:
      u8x8_d_helper_display_setup_memory(u8g2, &u8x8_sdl_240x160_info);
      break;
    case U8X8_MSG_DISPLAY_INIT:
      u8g_sdl_init(240, 160);
      u8x8_d_helper_display_init(u8g2);
      break;
    case U8X8_MSG_DISPLAY_SET_POWER_SAVE:
      break;
    case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
      break;
    case U8X8_MSG_DISPLAY_SET_CONTRAST:
      break;
    case U8X8_MSG_DISPLAY_DRAW_TILE:
      x = ((u8x8_tile_t *)arg_ptr)->x_pos;
      x *= 8;
      x += u8g2->x_offset;
    
      y = ((u8x8_tile_t *)arg_ptr)->y_pos;
      y *= 8;
    
      do
      {
        c = ((u8x8_tile_t *)arg_ptr)->cnt;
        ptr = ((u8x8_tile_t *)arg_ptr)->tile_ptr;
        u8g_sdl_set_multiple_8pixel(x, y, c*8, ptr);
        arg_int--;
      } while( arg_int > 0 );

      u8g_sdl_present();
      break;
    default:
      return 0;
  }
  return 1;
}

uint8_t u8x8_d_sdl_128x128(u8x8_t *u8g2, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, y, c;
  uint8_t *ptr;
  switch(msg)
  {
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:
      u8x8_d_helper_display_setup_memory(u8g2, &u8x8_sdl_128x128_info);
      break;
    case U8X8_MSG_DISPLAY_INIT:
      u8g_sdl_init(128, 128);
      u8x8_d_helper_display_init(u8g2);
      break;
    case U8X8_MSG_DISPLAY_SET_POWER_SAVE:
      break;
    case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
      break;
    case U8X8_MSG_DISPLAY_SET_CONTRAST:
      break;
    case U8X8_MSG_DISPLAY_DRAW_TILE:
      x = ((u8x8_tile_t *)arg_ptr)->x_pos;
      x *= 8;
      x += u8g2->x_offset;

      y = ((u8x8_tile_t *)arg_ptr)->y_pos;
      y *= 8;

      do
      {
        c = ((u8x8_tile_t *)arg_ptr)->cnt;
        ptr = ((u8x8_tile_t *)arg_ptr)->tile_ptr;
        u8g_sdl_set_multiple_8pixel(x, y, c*8, ptr);
        arg_int--;
        x += c*8;
      } while( arg_int > 0 );

      u8g_sdl_present();
      break;
    default:
      return 0;
  }
  return 1;
}


void u8x8_Setup_SDL_128x64(u8x8_t *u8x8)
{
  /* setup defaults */
  u8x8_SetupDefaults(u8x8);
  
  /* setup specific callbacks */
  u8x8->display_cb = u8x8_d_sdl_128x64;
	
  u8x8->gpio_and_delay_cb = u8x8_d_sdl_gpio;

  /* setup display info */
  u8x8_SetupMemory(u8x8);  
}

void u8g2_SetupBuffer_SDL_128x64(u8g2_t *u8g2, const u8g2_cb_t *u8g2_cb)
{
  
  static uint8_t buf[128*8];
  
  u8x8_Setup_SDL_128x64(u8g2_GetU8x8(u8g2));
  u8g2_SetupBuffer(u8g2, buf, 8, u8g2_ll_hvline_vertical_top_lsb, u8g2_cb);
}


void u8g2_SetupBuffer_SDL_128x64_4(u8g2_t *u8g2, const u8g2_cb_t *u8g2_cb)
{
  
  static uint8_t buf[128*3];
  
  u8x8_Setup_SDL_128x64(u8g2_GetU8x8(u8g2));
  u8g2_SetupBuffer(u8g2, buf, 3, u8g2_ll_hvline_vertical_top_lsb, u8g2_cb);
}

void u8g2_SetupBuffer_SDL_128x64_1(u8g2_t *u8g2, const u8g2_cb_t *u8g2_cb)
{
  
  static uint8_t buf[128];
  
  u8x8_Setup_SDL_128x64(u8g2_GetU8x8(u8g2));
  u8g2_SetupBuffer(u8g2, buf, 1, u8g2_ll_hvline_vertical_top_lsb, u8g2_cb);
}


void u8x8_Setup_SDL_128x128(u8x8_t *u8x8)
{
  /* setup defaults */
  u8x8_SetupDefaults(u8x8);

  /* setup specific callbacks */
  u8x8->display_cb = u8x8_d_sdl_128x128;

  u8x8->gpio_and_delay_cb = u8x8_d_sdl_gpio;

  /* setup display info */
  u8x8_SetupMemory(u8x8);
}

void u8g2_SetupBuffer_SDL_128x128(u8g2_t *u8g2, const u8g2_cb_t *u8g2_cb)
{
  static uint8_t buf[128*16];

  u8x8_Setup_SDL_128x128(u8g2_GetU8x8(u8g2));
  u8g2_SetupBuffer(u8g2, buf, 16, u8g2_ll_hvline_vertical_top_lsb, u8g2_cb);
}


void u8x8_Setup_SDL_240x160(u8x8_t *u8x8)
{
  /* setup defaults */
  u8x8_SetupDefaults(u8x8);
  
  /* setup specific callbacks */
  u8x8->display_cb = u8x8_d_sdl_240x160;
	
  u8x8->gpio_and_delay_cb = u8x8_d_sdl_gpio;

  /* setup display info */
  u8x8_SetupMemory(u8x8);  
}

void u8g2_SetupBuffer_SDL_240x160(u8g2_t *u8g2, const u8g2_cb_t *u8g2_cb)
{
  
  static uint8_t buf[240*20];
  
  u8x8_Setup_SDL_240x160(u8g2_GetU8x8(u8g2));
  u8g2_SetupBuffer(u8g2, buf, 20, u8g2_ll_hvline_vertical_top_lsb, u8g2_cb);
}
