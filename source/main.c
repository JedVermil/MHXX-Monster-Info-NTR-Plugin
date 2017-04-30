#include "global.h"
#include "ov.h"
#include "monster.h"
#include "settings.h"
#include "menu.h"

//constants
const u8 BACKGROUND_BORDER = 2;
const u8 TEXT_BORDER = 4;

//global vars
//note: variables are required by libntrplg to function and must be named exactly this way
FS_archive sdmcArchive = { 0x9, (FS_path){ PATH_EMPTY, 1, (u8*)"" } };
Handle fsUserHandle = 0;
u32 IoBasePad = 0xFFFD4000;

//static vars
static volatile Settings settings = {
    .pointer_list = 0,
    .show_overlay = 1,
    .show_small_monsters = 0,
    .show_special_stats = 1,
    .show_percentage = 0,
    .display_location = 0,
    .background_level = 1,
    .health_bar_width = 100
  };
static volatile u8 is_menu_active = 0;

u32 getKey() 
{
	return (*(vu32*)(IoBasePad) ^ 0xFFF) & 0xFFF;
}

color calculateColor(int hp, int max_hp)
{
  if (hp == 0)
    return GREY;
  else if (hp * 5 < max_hp)  //20%
    return RED;
  else if (hp * 10 < max_hp * 3)  //30%
    return ORANGE;
  else
    return GREEN;
}

u16 calculatePercentage(int current_value, int max_value)
{
  int percentage = current_value * 100 / max_value;
  
  //always draw at least 1 pixel
  if (current_value != 0 && percentage == 0)
  {
    percentage = 1;
  }
  
  return percentage;
}

void drawBorder(int row, int col, int height, color c)
{
  //left
  drawRect(row, col, height, 2, c);
  //top
  drawRect(row, col, 2, 2+settings.health_bar_width+2, c);
  //right
  drawRect(row, col + settings.health_bar_width+2, height, 2, c);
  //bottom
  drawRect(row + height-2, col, 2, 2+settings.health_bar_width+2, c);
}

void drawHealthBar(int row, int col, int hp, int max_hp)
{
  if (hp == 0)
    return;
  
  color c = calculateColor(hp, max_hp);
  u8 bar_length = calculatePercentage(hp, max_hp) * settings.health_bar_width / 100;
  
  drawBorder(row, col, 7, WHITE);
  drawRect(row+2, col+2, 3, bar_length, c);
}

void drawHealthBarWithParts(int row, int col, int hp, int max_hp, MonsterCache* cache)
{
  if (hp == 0)
    return;
  
  color c = calculateColor(hp, max_hp);
  u8 bar_length = calculatePercentage(hp, max_hp) * settings.health_bar_width / 100;
  
  drawBorder(row, col, CHAR_HEIGHT, WHITE);
  drawRect(row+2, col+2, 3, bar_length, c);
  
  //center divide
  drawRect(row + 7-2, col, 1, 2+settings.health_bar_width+2, WHITE);

  u8 offset = 2;
  for (u8 i = 0; i < 8; i++)
  {
    if (cache->p[i].max_break_hp <= 5)
      continue;
    
    u8 part_bar_max_length = cache->p[i].max_break_hp * settings.health_bar_width / cache->break_hp_sum;
    
    //don't draw bar if it has been broken once and the part HP is at maximum
    //note: you can't tell the difference between a part that is broken and a part that has been partially broken once and has returned back to full health,
    //      but if it's broken for good then the part HP is fixed at max
    if (cache->m->parts[i].break_count == 0 || 
        cache->m->parts[i].break_hp < cache->p[i].max_break_hp)
    {
      c = calculateColor(cache->m->parts[i].break_hp, cache->p[i].max_break_hp);
      bar_length = calculatePercentage(cache->m->parts[i].break_hp, cache->p[i].max_break_hp) * 
        part_bar_max_length / 100; //scale to part bar
      if (bar_length == 0 && cache->m->parts[i].break_hp > 0)
      { //always show at least 1 pixel if hp is not 0
        bar_length = 1;
      }
      
      drawRect(row + 8-2, col + offset, 2, bar_length, c);
    }
    
    offset += part_bar_max_length;
    drawRect(row + 8-2, col + offset, 2, 1, WHITE);
    offset++;
  }
  
  //fill in any remaining pixels to the right
  if (offset < settings.health_bar_width+2)
  {
    drawRect(row + 8-2, col + offset, 2, settings.health_bar_width+2 - offset, WHITE);
  }
}

void findListPointer()
{
  //single instance only, needed because this can take some time
  static volatile u8 is_running = 0;
  if (is_running)
    return;
  
  //avoid running too frequently
  //note: avoids tearing in opening and drop in framerate before first quest
  static volatile u64 tick = 0;
  if (tick > svc_getSystemTick())
    return;
  tick = svc_getSystemTick() + 400000000;
  
  is_running = 1;
  
  const u32 search_start = 0x08200000;
  const u32 search_end = 0x08300000;
  const u32 valid_pointer_min = 0x30000000;
  const u32 valid_pointer_max = 0x30200000; //this is just a guess
  
  u32 offset = search_start;
  while (offset < search_end)
  {
    MonsterPointerList* l = (MonsterPointerList*)offset;
    u8 skip = 0;  //flag for whether we should skip to next offset (for inner loops)
    
    //check the 15 bytes of unused
    //note: the boundary condition is wrong, should be i >= 0; however, this causes the game to fail to load for some reason
    //      So, leaving this "error" in for now; it seems to work OK
    for (u8 i = 14; i > 0; i--)
    {
      if (l->unused[i] > 1)
      {
        offset += i + 2;
        skip = 1;
        break;
      }
      else if (l->unused[i] == 1)
      {
        offset += i + 1;
        skip = 1;
        break;
      }
    }
    if (skip)
    { 
      //only advance even numbers
      if (offset % 2)
      {
        offset++;
      }
      continue;
    }
    
    //check the first fixed byte
    if (l->fixed != 1)
    {
      offset += 16;
      continue;
    }
    
    //check the monster pointers:
    // 1. should be within valid pointer min and max
    // 2. if one is 0, the rest must be 0
    // 3. should add up to count
    u8 my_count = 0;
    u8 should_be_0 = 0;
    for (u8 i = 0; i < 8; i++)
    {
      u32 p = (u32)(l->m[i]);
      
      if (p == 0)
      {
        if (i == 0)
        { //must have at least 1 monster to be sure it's valid
          skip = 1;
          break;
        }
        else
        {
          should_be_0 = 1;
        }
      }
      else if (should_be_0)
      { //there shouldn't be null pointers in between entries in the list
        skip = 1;
        break;
      }
      else if (p < valid_pointer_min || p > valid_pointer_max)
      { //make some assumptions about where the pointer should live in
        skip = 1;
        break;
      }
      else
      {
        my_count++;
      }
    }
    if (skip || my_count != l->count)
    { //only skip the fixed and unused bytes
      offset += 16;
      continue;
    }
    
    //we found it!!!
    settings.pointer_list = (MonsterPointerList*)offset;
    is_running = 0;
    return;
  }
  
  is_running = 0;
}

u32 debugListPointers()
{
  u16 row = CHAR_HEIGHT;
  char msg[100];
  
  drawString(row, 4, WHITE, "DEBUG: List Pointers");
  row += 10;
  
  for (u8 i = 0; i < MAX_POINTERS_IN_LIST; i++)
  {
    Monster* m = settings.pointer_list->m[i];
    if (!m)
      continue;
    
    xsprintf(msg, "%X", (u32)m);
    drawString(row, 4, WHITE, msg);
    row += 10;
  }
  
  return 0;
}

u32 debugListStructs()
{
  u16 row = CHAR_HEIGHT;
  char msg[BTM_SCRN_WIDTH/8];
  u8 drawn = 0;
  
  updateMonsterCache(settings.pointer_list);
  
  drawTransparentBlackRect(row-2, 2, 2 + CHAR_HEIGHT*8 + 2, BTM_SCRN_WIDTH-4, 2);
  for (u8 j = 0; j < 8; j++)
  {
    Monster* m1 = settings.pointer_list->m[0];
    Monster* m2 = settings.pointer_list->m[1];
    if (!m1)
      continue;
    else if (!m2)
    {
      xsprintf(msg, "%4u/%4u %4d/%4d", 
        m1->parts[j].stagger_hp, getCachedMonsterByIndex(0)->p[j].max_stagger_hp,
        m1->parts[j].break_hp, getCachedMonsterByIndex(0)->p[j].max_break_hp);
    }
    else
    {
      xsprintf(msg, "%4u/%4u %3d/%3d  %4u/%4u %3d/%3d", 
        m1->parts[j].stagger_hp, getCachedMonsterByIndex(0)->p[j].max_stagger_hp,
        m1->parts[j].break_hp, getCachedMonsterByIndex(0)->p[j].max_break_hp,
        m2->parts[j].stagger_hp, getCachedMonsterByIndex(1)->p[j].max_stagger_hp,
        m2->parts[j].break_hp, getCachedMonsterByIndex(1)->p[j].max_break_hp);
    }
    
    drawString(row, 2, WHITE, msg);
    row += CHAR_HEIGHT;
  }
  
  return 0;
  
  for (u8 i = 0; i < MAX_POINTERS_IN_LIST; i++)
  {
    Monster* m = settings.pointer_list->m[i];
    if (!m)
      continue;
    if (m->identifier3 == 0 || m->identifier3 == 0x80)
      continue;
    
    if (!drawn)
    {
      drawTransparentBlackRect(row-2, 2, 2 + CHAR_HEIGHT*8 + 2, BTM_SCRN_WIDTH-4, 2);
      drawn = 1;
    }
    
    /* xsprintf(msg, "%u: %02X %02X %02X", 
      m->hp, m->identifier1, m->identifier2, m->identifier3);
    drawString(row, 2, WHITE, msg);
    row += CHAR_HEIGHT; */
    /* xsprintf(msg, "%4u %4u  %4u %4u  %4u %4u", 
      m->poison, m->max_poison, m->paralysis, m->max_paralysis, m->sleep, m->max_sleep);
    drawString(row, 2, WHITE, msg);
    row += CHAR_HEIGHT;
    xsprintf(msg, "%4u  %4u  %4u %4u  %4u %4u", 
      m->dizzy, m->exhaust, m->jump, m->max_jump, m->blast, m->max_blast);
    drawString(row, 2, WHITE, msg);
    row += CHAR_HEIGHT;
    xsprintf(msg, "%2u  %2u  %2u  %2u  %2u", 
      m->is_asleep, m->jump_counter, m->ride_counter, m->blast_counter, m->status);
    drawString(row, 2, WHITE, msg);
    row += CHAR_HEIGHT; */
    
  }
  
  return 0;
}

u32 debugBitChecker()
{
  static const u16 offsets[] = {0x68FC, 0x68FD, 0x68FE, 0x68FF, 0x6900, 0x6901};
  static const u8 bits[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  if (sizeof(offsets) / sizeof(offsets[0]) != sizeof(bits) / sizeof(bits[0]))
  {
    drawString(10, 2, RED, "FAIL: offsets and bits do not match");
    return 0;
  }
  
  u16 row = CHAR_HEIGHT;
  char msg[BTM_SCRN_WIDTH/8];
  u8 drawn = 0;
  
  for (u8 i = 0; i < MAX_POINTERS_IN_LIST; i++)
  {
    Monster* m = settings.pointer_list->m[i];
    if (!m || m->max_hp < 550)
      continue;
    
    if (!drawn)
    {
      drawTransparentBlackRect(row-2, 2, 2 + CHAR_HEIGHT*8 + 2, BTM_SCRN_WIDTH-4, 1);
      drawn = 1;
    }
    
    u16 col = 2;
    for (u8 j = 0; j < sizeof(offsets) / sizeof(offsets[0]); j++)
    {
      if (j == 2 || j == 4)
      {
        col += 8;
      }
      /* if (j > 0 && j % 10 == 0)
      {
        col = 2;
        row += CHAR_HEIGHT;
      } */
      u8 value = *((u8*)((u32)m + offsets[j]));
      xsprintf(msg, "%02X", value & bits[j]);
      drawString(row, col, WHITE, msg);
      col += 8*3;
    }
    row += CHAR_HEIGHT;
  }
  
  return 0;
}

u32 debugFindListPointer()
{
  char msg[BTM_SCRN_WIDTH/8];
  
  findListPointer();
  
  if (settings.pointer_list)
  {
    xsprintf(msg, "Found pointer: %08X", (u32)settings.pointer_list);
    drawString(CHAR_HEIGHT, 2, WHITE, msg);
  }
  
  return 0;
}

u32 debugFileSystemTest()
{ 
  static char msg[100];
  static u8 run_once = 0;
  
  if (run_once)
  {
    drawString(CHAR_HEIGHT*2, 2, WHITE, msg);
    return 0;
  }
  run_once = 1;
  
  Result r1 = 99;
  Handle infile = 0;
  Result r2 = FSUSER_OpenFileDirectly(fsUserHandle, &infile, sdmcArchive, FS_makePath(PATH_CHAR, "/ntr.bin"), FS_OPEN_READ, 0);
  
  xsprintf(msg, "%8X  %8X", r1, r2);
  
  return 0;
}

u32 displayInfo()
{
  u8 count = 0;
  u16 row = 0; //keep away from top edge of screen
  char msg[BTM_SCRN_WIDTH/8];
  
  count = getMonsterCount(settings.pointer_list, settings.show_small_monsters);
  if (count == 0)
    return 1;
  
  updateMonsterCache(settings.pointer_list);
  
  //calculate offsets to display location
  //note: top/bottom screen separation is done by overlayCallback()
  //note: display size includes background, but offsets do not
  u16 display_width = TEXT_BORDER-BACKGROUND_BORDER + 8*8 + TEXT_BORDER*2 + 2+settings.health_bar_width+2 + TEXT_BORDER-BACKGROUND_BORDER;
  if (settings.show_percentage)
  {
    display_width -= 8*2;
  }
  if (settings.show_special_stats)
  {
    display_width += 8*12 + TEXT_BORDER-BACKGROUND_BORDER;
  }
  u16 display_height = TEXT_BORDER-BACKGROUND_BORDER + CHAR_HEIGHT*count + TEXT_BORDER-BACKGROUND_BORDER;
  u16 row_offset, col_offset;
  switch (settings.display_location)
  {
    case 0: //bottom top-left
      row_offset = TEXT_BORDER;
      col_offset = TEXT_BORDER;
      break;
    case 1: //bottom bottom-left
      row_offset = SCREEN_HEIGHT - display_height + TEXT_BORDER-BACKGROUND_BORDER;
      col_offset = TEXT_BORDER;
      break;
    case 2: //top top-right
      row_offset = TEXT_BORDER * 4;
      col_offset = TOP_SCRN_WIDTH - display_width + TEXT_BORDER-BACKGROUND_BORDER;
      break;
    case 3: //top bottom-left
      row_offset = SCREEN_HEIGHT - display_height + TEXT_BORDER-BACKGROUND_BORDER;
      col_offset = TEXT_BORDER;
      break;
  }
  
  //draw background
  drawTransparentBlackRect(
    row_offset - (TEXT_BORDER-BACKGROUND_BORDER), 
    col_offset - (TEXT_BORDER-BACKGROUND_BORDER), 
    display_height, display_width, settings.background_level);
  
  for (u8 i = 0; i < MAX_POINTERS_IN_LIST; i++)
  {
    u16 col = col_offset;
    Monster* m = settings.pointer_list->m[i];
    if (!m ||
        (!settings.show_small_monsters && (m->identifier3 == 0 || m->identifier3 == 0x80)))
      continue;
    
    //draw main health bar
    if (settings.show_percentage)
    {
      xsprintf(msg, "HP:%3u", calculatePercentage(m->hp, m->max_hp));
      drawString(row_offset + row+1, col, WHITE, msg);
      col += 8*6 + TEXT_BORDER*2;
    }
    else
    {
      xsprintf(msg, "HP:%5u", m->hp);
      drawString(row_offset + row+1, col, WHITE, msg);
      col += 8*8 + TEXT_BORDER*2;
    }
    MonsterCache* cache = getCachedMonsterByPointer(m);
    if (cache)
    {
      drawHealthBarWithParts(row_offset + row, col, m->hp, m->max_hp, cache);
    }
    else
    {
      drawHealthBar(row_offset + row, col, m->hp, m->max_hp);
    }
    col += 2+settings.health_bar_width+2;
    
    //draw poison/paralysis/sleep
    if (settings.show_special_stats)
    {
      xsprintf(msg, "%4u", (settings.show_percentage) ? 
        calculatePercentage(m->max_poison - m->poison, m->max_poison) : 
        m->max_poison - m->poison);
      drawString(row_offset + row+1, col, PURPLE, msg);
      xsprintf(msg, "%4u", (settings.show_percentage) ? 
        calculatePercentage(m->max_paralysis - m->paralysis, m->max_paralysis) : 
        m->max_paralysis - m->paralysis);
      drawString(row_offset + row+1, col + 8*4, YELLOW, msg);
      xsprintf(msg, "%4u", (settings.show_percentage) ? 
        calculatePercentage(m->max_sleep - m->sleep, m->max_sleep) : 
        m->max_sleep - m->sleep);
      drawString(row_offset + row+1, col + 8*8, CYAN, msg);
    }
    
    row += CHAR_HEIGHT;
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
  static u16 count = 0;
  
  if (isBottom == 1)
  {
    setState(addr, stride, format, BTM_SCRN_WIDTH);
    
    u32 key = getKey();
    is_menu_active &= !(key & MENU_DEACTIVATE);
    if (is_menu_active || key == MENU_ACTIVATE)
    {
      is_menu_active = 1;
      return displayMenu(key, &settings);
    }
    
    if (count < 60)
    {
      drawString(TEXT_BORDER, TEXT_BORDER, WHITE, "MHXX Overlay Plugin Active");
      count++;
    }
    
    if (!settings.pointer_list)
    {
      findListPointer();
    }
    else if (settings.show_overlay && settings.display_location < 2)
      return displayInfo();
  }
  else if (settings.show_overlay && settings.display_location > 1) //should display in top screen
  {
    if (!settings.pointer_list)
    {
      findListPointer();
    }
    else
    {
      setState(addr, stride, format, TOP_SCRN_WIDTH);
      u32 result = displayInfo();
      if (addrB && addr != addrB)
      {
        setState(addrB, stride, format, TOP_SCRN_WIDTH);
        result |= displayInfo();
      }
      
      return result;
    }
  }
  
  return 1;
}

#define CALLBACK_OVERLAY (101)

int main()
{
  initSrv();  //needed for srv_getServiceHandle
  initSharedFunc(); //needed for plg~ functions
  
  if (((NS_CONFIG*)(NS_CONFIGURE_ADDR))->sharedFunc[8])
  {
    IoBasePad = plgGetIoBase(IO_BASE_PAD);
  }
  
  srv_getServiceHandle(0, &fsUserHandle, "fs:USER");
    //need this to open files, for the first parameter to FSUSER_~ calls
    //note: plgGetSharedServiceHandle("fs:USER", &fsUserHandle) is broken
  
  plgRegisterCallback(CALLBACK_OVERLAY, (void*) overlayCallback, 0);
  
  return 0;
}
