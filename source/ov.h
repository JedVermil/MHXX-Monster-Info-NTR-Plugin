#pragma once

#include "color.h"

void setState(u32 addr, u32 stride, u32 format, u32 screen_width);
void drawTransparentBlackRect(int r, int c, int h, int w, int level);
void drawRect(int posR, int posC, int h, int w, Color c);
void drawString(int posR, int posC, Color c, u8* buffer);

void ovDrawTranspartBlackRect(
	u32 addr, 
	u32 stride, 
	u32 format, 
	int r, 
	int c, 
	int h, 
	int w, 
	u8 level) ;
void ovDrawPixel(
	u32 addr, 
	u32 stride, 
	u32 format, 
	int posR, 
	int posC, 
	u32 r, 
	u32 g, 
	u32 b);
void ovDrawRect(
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
void ovDrawChar(
	u32 addr, 
	u32 stride,
	u32 format,
	u8 letter,
	int y, 
	int x, 
	u32 r, 
	u32 g, 
	u32 b);
void ovDrawString(
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
