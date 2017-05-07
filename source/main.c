#include "global.h"
#include "ov.h"
#include "monster.h"
#include "settings.h"
#include "menu.h"
#include "mem_ops.h"

#define STARTUP_BANNER_TIMEOUT_COUNT 60
#define BACKGROUND_BORDER 2
#define TEXT_BORDER 4

//global vars
//note: variables are required by libntrplg to function and must be named exactly this way
FS_archive sdmcArchive = { 0x9, (FS_path){ PATH_EMPTY, 1, (u8*)"" } };
Handle fsUserHandle = 0;

//static vars
static volatile Settings settings = {
    .is_modified = 0,
    .language = 0,
    .pointer_list = 0,
    .show_overlay = 1,
    .show_small_monsters = 0,
    .show_special_stats = 1,
    .show_percentage = 0,
    .display_location = 0,
    .background_level = 1,
    .health_bar_width = 100,
  };

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
  u16 bar_length = calculatePercentage(hp, max_hp) * settings.health_bar_width / 100;
  
  drawBorder(row, col, 7, WHITE);
  drawRect(row+2, col+2, 3, bar_length, c);
}

void drawHealthBarWithParts(int row, int col, int hp, int max_hp, MonsterCache* cache)
{
  if (hp == 0)
    return;
  
  color c = calculateColor(hp, max_hp);
  u16 bar_length = calculatePercentage(hp, max_hp) * settings.health_bar_width / 100;
  
  drawBorder(row, col, ROW_HEIGHT, WHITE);
  drawRect(row+2, col+2, 3, bar_length, c);
  
  //center divide
  drawRect(row + 7-2, col, 1, 2+settings.health_bar_width+2, WHITE);

  u16 offset = 2;
  for (u8 i = 0; i < MAX_PARTS_PER_MONSTER; i++)
  {
    if (cache->p[i].max_break_hp <= 5)
      continue;
    
    u16 part_bar_max_length = cache->p[i].max_break_hp * settings.health_bar_width / cache->break_hp_sum;
    
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

u32 debugListPointers()
{
  u16 row = ROW_HEIGHT;
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
  u16 row = ROW_HEIGHT;
  char msg[BTM_SCRN_WIDTH/CHAR_WIDTH];
  u8 drawn = 0;
      
  for (u8 i = 0; i < MAX_POINTERS_IN_LIST; i++)
  {
    Monster* m = settings.pointer_list->m[i];
    if (!m)
      continue;
    
    if (!drawn)
    {
      drawTransparentBlackRect(row-2, 2, 2 + ROW_HEIGHT*CHAR_WIDTH + 2, BTM_SCRN_WIDTH-4, 2);
      drawn = 1;
    }
    
    xsprintf(msg, "%u: %02X %02X", 
      m->hp, m->identifier1, m->identifier2);
    drawString(row, 2, WHITE, msg);
    row += ROW_HEIGHT;
    
  }
  
  return 0;
}

u32 debugBitChecker()
{
  static const u16 offsets[] = {0x6E0D, 0x0, 0x228, 0x1262, 0x1266, 0x1170, 0x1260, 0x1438, 0x1439, 0x143A, 0x3AD8, 0x6E08, 0x6E09, 0x6E30};
  static const u8 bits[] = {0xFF};

  /* if (sizeof(offsets) / sizeof(offsets[0]) != sizeof(bits) / sizeof(bits[0]))
  {
    drawString(10, 2, RED, "FAIL: offsets and bits do not match");
    return 0;
  } */
  
  u16 row = ROW_HEIGHT;
  char msg[BTM_SCRN_WIDTH/CHAR_WIDTH];
  u8 drawn = 0;
  
  for (u8 i = 0; i < MAX_POINTERS_IN_LIST; i++)
  {
    Monster* m = settings.pointer_list->m[i];
    if (!m || isSmallMonster(m))
      continue;
    
    if (!drawn)
    {
      drawTransparentBlackRect(row-2, 2, 2 + ROW_HEIGHT*CHAR_WIDTH + 2, BTM_SCRN_WIDTH-4, 1);
      drawn = 1;
    }
    
    if (i == 0)
    {
      xsprintf(msg, "MAP: %02X", *((u8*)((u32)m + offsets[0])));
      drawString(row, 2, WHITE, msg);
      row += ROW_HEIGHT;
    }
    
    u16 col = 2;
    for (u8 j = 0; j < sizeof(offsets) / sizeof(offsets[0]); j++)
    {
      if (j == 0)
        continue;
      if (j == 5)
      {
        col += CHAR_WIDTH;
      }
      /* if (j > 0 && j % 10 == 0)
      {
        col = 2;
        row += ROW_HEIGHT;
      } */
      /* if (j > 6)
      {
        u16 value = *((u16*)((u32)m + offsets[j]));
        xsprintf(msg, "%04X", value);
        drawString(row, col, WHITE, msg);
        col += CHAR_WIDTH*5;
        continue;
      } */
      u8 value = *((u8*)((u32)m + offsets[j]));
      //xsprintf(msg, "%02X", value & bits[j]);
      xsprintf(msg, "%02X", value);
      drawString(row, col, WHITE, msg);
      col += CHAR_WIDTH*3;
    }
    row += ROW_HEIGHT;
  }
  
  return 0;
}

u32 debugFindListPointer()
{
  char msg[BTM_SCRN_WIDTH/CHAR_WIDTH];
  
  if (findListPointer(&settings))
  {
    xsprintf(msg, "Found pointer: %08X", (u32)settings.pointer_list);
    drawString(ROW_HEIGHT, 2, WHITE, msg);
  }
  
  return 0;
}

u32 debugFileSystemTest()
{ 
  static char msg[100];
  static u8 run_once = 0;
  
  if (run_once)
  {
    drawString(ROW_HEIGHT*2, 2, WHITE, msg);
    return 0;
  }
  run_once = 1;
  
  Result r1 = 99;
  Handle infile = 0;
  Result r2 = FSUSER_OpenFileDirectly(fsUserHandle, &infile, sdmcArchive, FS_makePath(PATH_CHAR, "/ntr.bin"), FS_OPEN_READ, 0);
  
  xsprintf(msg, "%8X  %8X", r1, r2);
  
  return 0;
}

void displaySpecialStatHelper(u16 current_value, u16 max_value, u16 row, u16 col, color c)
{
  char msg[10];
  
  if (max_value == 0xFFFF)
    return;
  
  xsprintf(msg, "%4u", (settings.show_percentage) ? 
    calculatePercentage(max_value - current_value, max_value) : 
    max_value - current_value);
  drawString(row, col, c, msg);
}

u32 displayInfo(u8 is_3D_on, u8 is_right_buffer)
{
  u8 count = 0;
  u16 row = 0; //keep away from top edge of screen
  char msg[BTM_SCRN_WIDTH/CHAR_WIDTH];
  
  count = getMonsterCount(settings.pointer_list, settings.show_small_monsters);
  if (count == 0)
    return 1;
  
  updateMonsterCache(settings.pointer_list);
  
  //calculate display size of background
  u16 display_width = TEXT_BORDER-BACKGROUND_BORDER + CHAR_WIDTH*8 + TEXT_BORDER*2 + 2+settings.health_bar_width+2 + TEXT_BORDER-BACKGROUND_BORDER;
  if (settings.show_percentage)
  {
    display_width -= CHAR_WIDTH*2;
  }
  if (settings.show_special_stats)
  {
    display_width += CHAR_WIDTH*12 + TEXT_BORDER-BACKGROUND_BORDER;
  }
  u16 display_height = TEXT_BORDER-BACKGROUND_BORDER + ROW_HEIGHT*count*2 + TEXT_BORDER-BACKGROUND_BORDER;
  
  //calculate offsets to display location, not including background  
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
      col_offset = TOP_SCRN_WIDTH - display_width;
      //parallax adjustment
      //  if offset is positive, adjust right buffer
      //  if offset is negative, adjust left buffer
      col_offset -= abs(settings.parallax_offset) * 
        (is_3D_on && (is_right_buffer == (settings.parallax_offset > 0)));
      break;
    case 3: //top bottom-left
      row_offset = SCREEN_HEIGHT - display_height + TEXT_BORDER-BACKGROUND_BORDER;
      col_offset = TEXT_BORDER;
      //parallax adjustment
      //  if offset is positive, adjust left buffer
      //  if offset is negative, adjust right buffer
      col_offset += abs(settings.parallax_offset) * 
        (is_3D_on && (is_right_buffer == (settings.parallax_offset < 0)));
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
    if (!m || (!settings.show_small_monsters && isSmallMonster(m)))
      continue;
    
    //draw monster name
    MonsterInfo* m_info = getMonsterInfoFromDB(m);
    switch (settings.language)
    {
      case 1:
        drawMisakiString(row_offset + row+1, col, (m_info->is_hyper) ? RED : WHITE, m_info->jp_name);
        break;
      default:
        drawString(row_offset + row+1, col, (m_info->is_hyper) ? RED : WHITE, m_info->name);
        break;
    }
    col += CHAR_WIDTH*11;
    
    //draw dizzy/exhaust/blast/sleep
    if (settings.show_special_stats)
    {
      //right align the text
      col = col_offset + display_width - BACKGROUND_BORDER - CHAR_WIDTH*16 - TEXT_BORDER;
      
      displaySpecialStatHelper(m->jump, m->max_jump, row_offset + row+1, col, LIGHT_GREEN);
      displaySpecialStatHelper(m->exhaust, m->max_exhaust, row_offset + row+1, col + CHAR_WIDTH*4, LIGHT_BLUE);
      displaySpecialStatHelper(m->blast, m->max_blast, row_offset + row+1, col + CHAR_WIDTH*8, VIOLET);
      displaySpecialStatHelper(m->dizzy, m->max_dizzy, row_offset + row+1, col + CHAR_WIDTH*12, ORANGE);
    }
    
    //new row
    col = col_offset;
    row += ROW_HEIGHT;
    
    //draw main health bar
    if (settings.show_percentage)
    {
      xsprintf(msg, "HP:%3u", calculatePercentage(m->hp, m->max_hp));
      drawString(row_offset + row+1, col, WHITE, msg);
      col += CHAR_WIDTH*6 + TEXT_BORDER*2;
    }
    else
    {
      xsprintf(msg, "HP:%5u", m->hp);
      drawString(row_offset + row+1, col, WHITE, msg);
      col += CHAR_WIDTH*8 + TEXT_BORDER*2;
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
      displaySpecialStatHelper(m->poison, m->max_poison, row_offset + row+1, col, PURPLE);
      displaySpecialStatHelper(m->paralysis, m->max_paralysis, row_offset + row+1, col + CHAR_WIDTH*4, YELLOW);
      displaySpecialStatHelper(m->sleep, m->max_sleep, row_offset + row+1, col + CHAR_WIDTH*8, CYAN);
    }
    
    row += ROW_HEIGHT;
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
  static volatile MenuState menu = {0};
  
  if (isBottom == 1)
  {
    setState(addr, stride, format, BTM_SCRN_WIDTH);
    
    //menu activation/deactivation check
    u32 key = 0;
    if (!menu.is_busy)
    {
      key = getKey();
    }
    menu.is_active &= !(key & MENU_DEACTIVATE);
    if (menu.is_active || key == MENU_ACTIVATE)
    {
      if (!menu.is_active)
      {
        menu.is_active = 1;
        settings.is_modified = 0;
      }
      return displayMenu(key, &settings, &menu);
    }
    
    //settings save check
    if (settings.is_modified)
    {
      saveSettings(&settings);
    }
    
    //banner for startup
    if (count < STARTUP_BANNER_TIMEOUT_COUNT)
    {
      drawString(TEXT_BORDER, TEXT_BORDER, WHITE, "MHXX Overlay Plugin Active");
      count++;
    }
    
    if (!settings.pointer_list)
    {
      findListPointer(&settings);
    }
    else if (settings.show_overlay && settings.display_location < 2)
      return displayInfo(0, 0);
  }
  else if (settings.show_overlay && settings.display_location > 1) //should display in top screen
  {
    if (!settings.pointer_list)
    {
      findListPointer(&settings);
    }
    else
    {
      u8 is_3D_on = addrB && addr != addrB;
      
      setState(addr, stride, format, TOP_SCRN_WIDTH);
      u32 result = displayInfo(is_3D_on, 0); //adjust the left screen offset if 3D is on
      if (is_3D_on)
      {
        setState(addrB, stride, format, TOP_SCRN_WIDTH);
        result |= displayInfo(is_3D_on, 1);
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
    updateIoBasePad(plgGetIoBase(IO_BASE_PAD));
  }
  
  srv_getServiceHandle(0, &fsUserHandle, "fs:USER");
    //need this to open files, for the first parameter to FSUSER_~ calls
    //note: plgGetSharedServiceHandle("fs:USER", &fsUserHandle) doesn't work here as it's only for home menu
  
  initMonsterInfoDB();
  loadSettings(&settings);
  plgRegisterCallback(CALLBACK_OVERLAY, (void*) overlayCallback, 0);
  
  return 0;
}
