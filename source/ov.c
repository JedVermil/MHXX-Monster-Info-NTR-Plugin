#include "ov.h"

static u32 addr_cache;
static u32 stride_cache;
static u32 format_cache;
static u32 screen_width_cache;

void setState(u32 addr, u32 stride, u32 format, u32 screen_width)
{
  addr_cache = addr;
  stride_cache = stride;
  format_cache = format;
  screen_width_cache = screen_width;
}

void drawTransparentBlackRect(int r, int c, int h, int w, int level)
{
  ovDrawTranspartBlackRect(addr_cache, stride_cache, format_cache, 
    r, c, h, w, level);
}

void drawRect(int posR, int posC, int h, int w, color c)
{
  ovDrawRect(addr_cache, stride_cache, format_cache, 
    posR, posC, h, w, c.r, c.g, c.b);
}

void drawString(int posR, int posC, color c, u8* buffer)
{
  ovDrawString(addr_cache, stride_cache, format_cache, screen_width_cache,
    posR, posC, c.r, c.g, c.b, buffer);
}

void drawMisakiString(int y, int x, color c, u8* buffer)
{
  if (y + ROW_HEIGHT > SCREEN_HEIGHT)
    return;
  
  s16 x_ = (s16)x;
  u8 is_kanji = 0;
  
  for (u8 i = 0; ; i++)
  {
    if (x_ + CHAR_WIDTH > screen_width_cache)
      return;
      
    switch (buffer[i])
    {
      case 0xFF:  //terminator
        return;
      case 0xF0:  //hiragana/katagana
        is_kanji = 0;
        break;
      case 0xF2:  //kanji
        is_kanji = 1;
        break;
      default:
        if (x_ >= 0)
        {
          drawMisakiChar(y, x_, c, (is_kanji) ? misaki2 : misaki1, buffer[i]);
        }
        x_ += CHAR_WIDTH;
        break;
    }
  }
}

static void drawMisakiChar(int y, int x, color c, const unsigned char font[][8], u8 letter)
{
  //note: misaki font is organized differently than font.h
  //      each byte for a character is a column, not a row
  for (u8 i = 0; i < CHAR_WIDTH-1; i++) //skip last column since none of the chars used are non-zero
  {
    for (u8 j = 0; j < CHAR_HEIGHT; j++)
    {
      if ((0x1 << j) & font[letter][i])
      {
        ovDrawPixel(addr_cache, stride_cache, format_cache, y+j, x+i, c.r, c.g, c.b);
      }
    }
  }
}

static void ovDrawTranspartBlackRect(u32 addr, u32 stride, u32 format, int r, int c, int h, int w, u8 level) {
	format &= 0x0f;
  u16 posC = max((s16)c, 0);
  u16 max_posC = min((s16)c + w, screen_width_cache);
	/* u16 posC = (c >= screen_width_cache) ? 0 : c;
  u16 max_posC = min(posC + w, screen_width_cache); */
	for (; posC < max_posC; posC++)
  {
		if (format == 2)
    {
			u16* sp = (u16*)(addr + stride * posC + SCREEN_HEIGHT * 2 - 2 * (r + h - 1));
			u16* spEnd = sp + h;
			while (sp < spEnd)
      {
				u16 pix = *sp;
				u16 r = (pix >> 11) & 0x1f;
				u16 g = (pix >> 5) & 0x3f;
				u16 b = (pix & 0x1f);
				pix = ((r >> level) << 11) | ((g >> level) << 5) | (b >> level);
				*sp = pix;
				sp++;
			}
    } 
    else if (format == 1)
    {
			u8* sp = (u8*)(addr + stride * posC + SCREEN_HEIGHT * 3 - 3 * (r + h - 1));
			u8* spEnd = sp +  3 * h;
			while (sp < spEnd) 
      {
				sp[0] >>= level;
				sp[1] >>= level;
				sp[2] >>= level;
				sp += 3;
			}
		}
	}
}

static void ovDrawPixel(u32 addr, u32 stride, u32 format, int posR, int posC, u32 r, u32 g, u32 b) {
	format &= 0x0f;	
	if (format == 2) {
		u16 pix =  ((r ) << 11) | ((g ) << 5) | (b );
		*(u16*)(addr + stride * posC + SCREEN_HEIGHT * 2 -2 * posR) = pix;
	} else {
		u8* sp = (u8*)(addr + stride * posC + SCREEN_HEIGHT * 3 - 3 * posR);
		sp[0] = b;
		sp[1] = g;
		sp[2] = r;
	}
}

static void ovDrawRect(u32 addr, u32 stride, u32 format, int posR, int posC, int h, int w, u32 r, u32 g, u32 b) {
  int r_;
  u16 c_ = max((s16)posC, 0);
  u16 max_c = min((s16)posC + w, screen_width_cache);
  /* u16 c_ = (posC >= screen_width_cache) ? 0 : posC;
  u16 max_c = min(c_ + w, screen_width_cache); */
	for (; c_ < max_c; c_++)
  {
		for (r_ = posR; r_ < posR + h; r_++)
    {
			ovDrawPixel(addr, stride, format, r_, c_, r, g, b);
		}
	}
}

static void ovDrawChar(u32 addr, u32 stride, u32 format, u8 letter,int y, int x, u32 r, u32 g, u32 b){
  int i;
  int k;
  int c;
  unsigned char mask;
  unsigned char l; 

  //magic value due to commenting in font.h
	if ((letter < 32) || (letter > 127)) {
		letter = '?';
	}

  //magic value to calculate first byte of char
  //note: would have been much easier to just use a matrix IMO
  c=(letter-32)*CHAR_WIDTH;

  for (i = 0; i < CHAR_HEIGHT; i++){
    mask = 0b10000000;
    l = font[i+c];
    for (k = 0; k < CHAR_WIDTH; k++){
      if ((mask >> k) & l){
        ovDrawPixel(addr, stride, format, i+y, k+x ,r,g,b);
      }     
    }
  }
}

static void ovDrawString(u32 addr, u32 stride, u32 format, u32 scrnWidth, int posR, int posC, u32 r, u32 g, u32 b, u8* buf) {
    if (posR + ROW_HEIGHT > SCREEN_HEIGHT)
      return;
    
    s16 c = (s16)posC;
    
    while(*buf)
    {
      if (c + CHAR_WIDTH > scrnWidth)
        return;
      
      if (c >= 0)
      {
        ovDrawChar(addr, stride, format, *buf, posR, c, r, g, b);
      }
      buf++;
      c += CHAR_WIDTH;
    }
} 