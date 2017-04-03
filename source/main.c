#include "global.h"
#include "ov.h"

FS_archive sdmcArchive = { 0x9, (FS_path){ PATH_EMPTY, 1, (u8*)"" } };
Handle fsUserHandle = 0;

#pragma pack(push,1)

//Monster info (incomplete)
typedef struct
{
  u8 unknown1;      //78 for big, 40 for small
  u16 unknown2;     //monster id
  u8 unknown3;      //alive?
  u32 unused1[2];
  u8 fixed1;        //22
  u8 unknown4;      //another monster id
  u8 unknown5;      //alive?
  u8 optional1;
  u32 fixed2;       //0F 08 00 00
  u32 optional2[2];
  u8 big_only1;     //only big monsters have this filled
  u16 unused2;
  u8 big_only2;     //only big mosnters have this filled
  u8 ignore[0x13F8];
  u32 hp;
  u32 max_hp;
} Monster;

#pragma pack(pop)

typedef struct
{
  u32 r;
  u32 g;
  u32 b;
} Color;

static Handle ptmuHandle;

#define CALLBACK_OVERLAY (101)

extern u32 getCurrentProcessHandle();
extern void initSharedFunc();

#ifdef GAME_REGION_JPN
#define MONSTER_POINTER_LIST_ADDR 0x082B9674
#endif

const u8 MAX_POINTERS_TO_CHECK = 10;
const u8 MIN_WIDTH = 10;  //keep away from screen border
const u16 BOTTOM_SCREEN_WIDTH = 320;

//get a list of pointers that points to the monster structs
static Monster** pointer_list = (Monster**)MONSTER_POINTER_LIST_ADDR;

Result ptmuInit(void)
{
  Result res = srv_getServiceHandle(NULL, &ptmuHandle, "ptm:u");
  return res;
}

Color calculateColor(int hp, int max_hp)
{
  Color c = {.r = 255, .g = 255, .b = 255};
  
  if (hp == 0)
  {
    c.r = 192;
    c.g = 192;
    c.b = 192;
  }
  else if (hp * 5 < max_hp)  //20%
  {
    c.g = 0;
    c.b = 0;
  }
  else if (hp * 10 < max_hp * 3)  //30%
  {
    c.b = 0;
  }
  else
  {
    c.r = 0;
    c.b = 0;
  }
  
  return c;
}

u32 debugListPointers(u32 addr, u32 stride, u32 format)
{
  u32 posR = MIN_WIDTH;
  char msg[100];
  
  for (u32 i = 0; i < MAX_POINTERS_TO_CHECK; i++)
  {
    Monster* m = pointer_list[i];
    if (!m)
      continue;
    
    xsprintf(msg, "%X", (u32)m);
    ovDrawString(addr, stride, format, BOTTOM_SCREEN_WIDTH, posR, 4, 255, 255, 255, msg);
    posR += 10;
  }
  
  return 0;
}

u32 displayInfo(u32 addr, u32 stride, u32 format)
{
  u32 count = 0;
  u32 posR = MIN_WIDTH;
  char msg[100];
  Color c;
  
  for (u32 i = 0; i < 10; i++)
  {
    Monster* m = pointer_list[i];
    if (!m)
      continue;
    
    c = calculateColor(m->hp, m->max_hp);
    xsprintf(msg, "%u/%u", m->hp, m->max_hp);
    ovDrawString(addr, stride, format, BOTTOM_SCREEN_WIDTH, posR, 4, 255, 255, 255, "HP:");
    ovDrawString(addr, stride, format, BOTTOM_SCREEN_WIDTH, posR, 36, c.r, c.g, c.b, msg);
    posR += 10;
    count++;
  }
  
  if (count == 0)
  {
    return 1;
  }
  
  return 0;
}

/*
Overlay Callback.
isBottom: 1 for bottom screen, 0 for top screen.
addr: writable cached framebuffer virtual address, should flush data cache after modifying.
addrB: right-eye framebuffer for top screen, undefined for bottom screen.
stride: framebuffer stride(pitch) in bytes, at least 240*bytes_per_pixel.
format: framebuffer format, see https://www.3dbrew.org/wiki/GPU/External_Registers for details.

return 0 on success. return 1 when nothing in framebuffer was modified.
*/

u32 overlayCallback(u32 isBottom, u32 addr, u32 addrB, u32 stride, u32 format)
{
  if (isBottom == 1)
  {
     return displayInfo(addr, stride, format);
  }
  return 1;
}

int main()
{
  initSharedFunc();
  initSrv();
  ptmuInit();
  plgRegisterCallback(CALLBACK_OVERLAY, (void*) overlayCallback, 0);
  return 0;
}
