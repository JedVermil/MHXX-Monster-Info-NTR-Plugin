#pragma once

#include "global.h"
#include "font.h"
#include "misaki_font.h"

#define max(x,y) ( \
    { __auto_type __x = (x); __auto_type __y = (y); \
      __x > __y ? __x : __y; })
#define min(x,y) ( \
    { __auto_type __x = (x); __auto_type __y = (y); \
      __x < __y ? __x : __y; })

#define SCREEN_HEIGHT 240
#define TOP_SCRN_WIDTH 400
#define BTM_SCRN_WIDTH 320
#define ROW_HEIGHT 10
#define CHAR_HEIGHT 8
#define CHAR_WIDTH 8
#define WHITE (color){.r = 255, .g = 255, .b = 255}
#define RED (color){.r = 255, .g = 0, .b = 0}
#define GREEN (color){.r = 0, .g = 255, .b = 0}
#define ORANGE (color){.r = 255, .g = 220, .b = 0}
#define PURPLE (color){.r = 255, .g = 0, .b = 255}
#define YELLOW (color){.r = 255, .g = 255, .b = 0}
#define CYAN (color){.r = 0, .g = 255, .b = 255}
#define GREY (color){.r = 5, .g = 10, .b = 5}
#define LIGHT_BLUE (color){.r = 210, .g = 180, .b = 250}
#define VIOLET (color){.r = 20, .g = 20, .b = 255}
#define LIGHT_GREEN (color){.r = 0, .g = 255, .b = 10}

void setState(u32 addr, u32 stride, u32 format, u32 screen_width);
void drawTransparentBlackRect(int r, int c, int h, int w, int level);
void drawRect(int posR, int posC, int h, int w, color c);
void drawString(int posR, int posC, color c, u8* buffer);
void drawMisakiString(int y, int x, color c, u8* buffer);
static void drawMisakiChar(int y, int x, color c, const unsigned char font[][8], u8 letter);

static void ovDrawTranspartBlackRect(
	u32 addr, 
	u32 stride, 
	u32 format, 
	int r, 
	int c, 
	int h, 
	int w, 
	u8 level) ;
static void ovDrawPixel(
	u32 addr, 
	u32 stride, 
	u32 format, 
	int posR, 
	int posC, 
	u32 r, 
	u32 g, 
	u32 b);
static void ovDrawRect(
	u32 addr, 
	u32 stride, 
	u32 format, 
	int posR, 
	int posC, 
	int h, 
	int w, 
	u32 r, 
	u32 g, 
	u32 b);
static void ovDrawChar(
	u32 addr, 
	u32 stride,
	u32 format,
	u8 letter,
	int y, 
	int x, 
	u32 r, 
	u32 g, 
	u32 b);
static void ovDrawString(
	u32 addr, 
	u32 stride, 
	u32 format, 
	u32 scrnWidth,
	int posR, 
	int posC, 
	u32 r, 
	u32 g, 
	u32 b, 
	u8* buf);