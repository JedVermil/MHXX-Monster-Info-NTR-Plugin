#include "global.h"
#include "ov.h"
#include "monster.h"
#include "color.h"

FS_archive sdmcArchive = { 0x9, (FS_path){ PATH_EMPTY, 1, (u8*)"" } };
Handle fsUserHandle = 0;
u32 IoBasePad = 0xFFFD4000;

static Handle ptmuHandle;

#define CALLBACK_OVERLAY (101)

extern u32 getCurrentProcessHandle();
extern void initSharedFunc();

typedef struct
{
  u16 max_stagger_hp;
  s16 max_break_hp;
} PartCache;

typedef struct
{
  Monster* m;       //set to 0 to deactivate
  PartCache p[8];
  u8 display_count; //for displaying only parts that are cuttable
  u16 break_hp_sum; //only sum displayable parts
} MonsterCache;

typedef struct
{
  u8 show_overlay;        //0 = off, 1 = on
  u8 show_small_monsters; //0 = off, 1 = on
  u8 show_special_stats;  //0 = off, 1 = on
  u8 show_percentage;     //0 = numbers, 1 = percentage
  u8 display_location;    //0 = bottom top-left, 1 = bottom bottom-left, 
                          //2 = top top-right, 3 = top bottom-left
  u8 background_level;
  u8 health_bar_width;
} Settings;

//constants
const u8 MAX_POINTERS_TO_CHECK = 10;
const u8 ROW_HEIGHT = 10;
const u8 BACKGROUND_BORDER = 2;
const u8 TEXT_BORDER = 4;
const u16 SCREEN_HEIGHT = 240;
const u16 TOP_SCRN_WIDTH = 400;
const u16 BTM_SCRN_WIDTH = 320;
const u32 MENU_ACTIVATE = BUTTON_L | BUTTON_SE;
const u32 MENU_DEACTIVATE = BUTTON_B;
const u64 BTN_WAIT_TICK_COUNT = 30000000;
const Color WHITE = {.r = 255, .g = 255, .b = 255};
const Color RED = {.r = 255, .g = 0, .b = 0};
const Color GREEN = {.r = 0, .g = 255, .b = 0};
const Color ORANGE = {.r = 255, .g = 220, .b = 0};
const Color PURPLE = {.r = 255, .g = 0, .b = 255};
const Color YELLOW = {.r = 255, .g = 255, .b = 0};
const Color CYAN = {.r = 0, .g = 255, .b = 255};
const Color GREY = {.r = 5, .g = 10, .b = 5};

//global vars
static MonsterPointerList* pointer_list = 0;
static MonsterCache m_cache[2]; //assume only 2 big monsters are active at a time
static volatile Settings settings = {
    .show_overlay = 1,
    .show_small_monsters = 0,
    .show_special_stats = 1,
    .show_percentage = 0,
    .display_location = 0,
    .background_level = 1,
    .health_bar_width = 100
  };
static volatile u8 is_menu_active = 0;

Result ptmuInit(void)
{
  Result res = srv_getServiceHandle(NULL, &ptmuHandle, "ptm:u");
  return res;
}

u32 getKey() 
{
	return (*(vu32*)(IoBasePad) ^ 0xFFF) & 0xFFF;
}

Color calculateColor(int hp, int max_hp)
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

void drawBorder(int row, int col, int height, Color c)
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
  
  Color c = calculateColor(hp, max_hp);
  u8 bar_length = calculatePercentage(hp, max_hp) * settings.health_bar_width / 100;
  
  drawBorder(row, col, 7, WHITE);
  drawRect(row+2, col+2, 3, bar_length, c);
}

void drawHealthBarWithParts(int row, int col, int hp, int max_hp, MonsterCache* cache)
{
  if (hp == 0)
    return;
  
  Color c = calculateColor(hp, max_hp);
  u8 bar_length = calculatePercentage(hp, max_hp) * settings.health_bar_width / 100;
  
  drawBorder(row, col, ROW_HEIGHT, WHITE);
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
    pointer_list = (MonsterPointerList*)offset;
    is_running = 0;
    return;
  }
  
  is_running = 0;
}

void updateMonsterCache()
{
  Monster* new_m1 = 0;
  Monster* new_m2 = 0;
  u8 keep_m1 = 0;
  u8 keep_m2 = 0;

  //check all monsters, excluding small ones
  for (u8 i = 0; i < MAX_POINTERS_TO_CHECK; i++)
  {
    //small monsters have 0x0 or 0x80 for identifier3
    if (!pointer_list->m[i] || 
        pointer_list->m[i]->identifier3 == 0 || 
        pointer_list->m[i]->identifier3 == 0x80)
      continue;
    
    if (pointer_list->m[i] == m_cache[0].m)
    {
      keep_m1 = 1;
    }
    else if (pointer_list->m[i] == m_cache[1].m)
    {
      keep_m2 = 1;
    }
    else if (new_m1 == 0)
    { 
      //save new monster pointer so we can add parts info later
      new_m1 = pointer_list->m[i];
    }
    else if (new_m2 == 0)
    {
      new_m2 = pointer_list->m[i];
    }
  }

  //remove expired monster parts
  if (!keep_m1)
  {
    m_cache[0].m = 0;
    m_cache[0].display_count = 0;
    m_cache[0].break_hp_sum = 0;
  }
  if (!keep_m2)
  {
    m_cache[1].m = 0;
    m_cache[1].display_count = 0;
    m_cache[1].break_hp_sum = 0;
  }

  //add new monster info
  //note: assume new_m2 will never be assigned before new_m1
  //note: only display parts that have more than 1 break_hp; for non-breakable parts it is typically negative but it can be fixed to 1 if there are special critereas involved
  if (new_m1)
  {
    if (!m_cache[0].m)
    {
      m_cache[0].m = new_m1;
      
      for (u8 i = 0; i < 8; i++)
      {
        m_cache[0].p[i].max_stagger_hp = new_m1->parts[i].stagger_hp;
        m_cache[0].p[i].max_break_hp = new_m1->parts[i].break_hp;
        
        if (m_cache[0].p[i].max_break_hp > 1)
        {
          m_cache[0].display_count++;
          m_cache[0].break_hp_sum += m_cache[0].p[i].max_break_hp;
        }
      }
    }
    else
    {
      m_cache[1].m = new_m1;
      
      for (u8 i = 0; i < 8; i++)
      {
        m_cache[1].p[i].max_stagger_hp = new_m1->parts[i].stagger_hp;
        m_cache[1].p[i].max_break_hp = new_m1->parts[i].break_hp;
        
        if (m_cache[1].p[i].max_break_hp > 1)
        {
          m_cache[1].display_count++;
          m_cache[1].break_hp_sum += m_cache[1].p[i].max_break_hp;
        }
      }
    }
  }
  if (new_m2)
  {
    if (!m_cache[0].m)
    {
      m_cache[0].m = new_m2;
      
      for (u8 i = 0; i < 8; i++)
      {
        m_cache[0].p[i].max_stagger_hp = new_m2->parts[i].stagger_hp;
        m_cache[0].p[i].max_break_hp = new_m2->parts[i].break_hp;
        
        if (m_cache[0].p[i].max_break_hp > 1)
        {
          m_cache[0].display_count++;
          m_cache[0].break_hp_sum += m_cache[0].p[i].max_break_hp;
        }
      }
    }
    else
    {
      m_cache[1].m = new_m2;
      
      for (u8 i = 0; i < 8; i++)
      {
        m_cache[1].p[i].max_stagger_hp = new_m2->parts[i].stagger_hp;
        m_cache[1].p[i].max_break_hp = new_m2->parts[i].break_hp;
        
        if (m_cache[1].p[i].max_break_hp > 1)
        {
          m_cache[1].display_count++;
          m_cache[1].break_hp_sum += m_cache[1].p[i].max_break_hp;
        }
      }
    }
  }
}

u8 getMonsterCount()
{
  u8 count = 0;
  
  for (u8 i = 0; i < MAX_POINTERS_TO_CHECK; i++)
  {
    //small monsters have 0x0 or 0x80 for identifier3
    if (pointer_list->m[i] && 
        (settings.show_small_monsters || 
         pointer_list->m[i]->identifier3 != 0 && 
         pointer_list->m[i]->identifier3 != 0x80))
      count++;
  }
  
  return count;
}

u32 debugListPointers()
{
  u16 row = ROW_HEIGHT;
  char msg[100];
  
  drawString(row, 4, WHITE, "DEBUG: List Pointers");
  row += 10;
  
  for (u8 i = 0; i < MAX_POINTERS_TO_CHECK; i++)
  {
    Monster* m = pointer_list->m[i];
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
  char msg[BTM_SCRN_WIDTH/8];
  u8 drawn = 0;
  
  updateMonsterCache();
  
  drawTransparentBlackRect(row-2, 2, 2 + ROW_HEIGHT*8 + 2, BTM_SCRN_WIDTH-4, 2);
  for (u8 j = 0; j < 8; j++)
  {
    Monster* m1 = pointer_list->m[0];
    Monster* m2 = pointer_list->m[1];
    if (!m1)
      continue;
    else if (!m2)
    {
      xsprintf(msg, "%4u/%4u %4d/%4d", 
        m1->parts[j].stagger_hp, m_cache[0].p[j].max_stagger_hp,
        m1->parts[j].break_hp, m_cache[0].p[j].max_break_hp);
    }
    else
    {
      xsprintf(msg, "%4u/%4u %3d/%3d  %4u/%4u %3d/%3d", 
        m1->parts[j].stagger_hp, m_cache[0].p[j].max_stagger_hp,
        m1->parts[j].break_hp, m_cache[0].p[j].max_break_hp,
        m2->parts[j].stagger_hp, m_cache[1].p[j].max_stagger_hp,
        m2->parts[j].break_hp, m_cache[1].p[j].max_break_hp);
    }
    
    drawString(row, 2, WHITE, msg);
    row += ROW_HEIGHT;
  }
  
  return 0;
  
  for (u8 i = 0; i < MAX_POINTERS_TO_CHECK; i++)
  {
    Monster* m = pointer_list->m[i];
    if (!m)
      continue;
    if (m->identifier3 == 0 || m->identifier3 == 0x80)
      continue;
    
    if (!drawn)
    {
      drawTransparentBlackRect(row-2, 2, 2 + ROW_HEIGHT*8 + 2, BTM_SCRN_WIDTH-4, 2);
      drawn = 1;
    }
    
    /* xsprintf(msg, "%u: %02X %02X %02X", 
      m->hp, m->identifier1, m->identifier2, m->identifier3);
    drawString(row, 2, WHITE, msg);
    row += ROW_HEIGHT; */
    /* xsprintf(msg, "%4u %4u  %4u %4u  %4u %4u", 
      m->poison, m->max_poison, m->paralysis, m->max_paralysis, m->sleep, m->max_sleep);
    drawString(row, 2, WHITE, msg);
    row += ROW_HEIGHT;
    xsprintf(msg, "%4u  %4u  %4u %4u  %4u %4u", 
      m->dizzy, m->exhaust, m->jump, m->max_jump, m->blast, m->max_blast);
    drawString(row, 2, WHITE, msg);
    row += ROW_HEIGHT;
    xsprintf(msg, "%2u  %2u  %2u  %2u  %2u", 
      m->is_asleep, m->jump_counter, m->ride_counter, m->blast_counter, m->status);
    drawString(row, 2, WHITE, msg);
    row += ROW_HEIGHT; */
    
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
  
  u16 row = ROW_HEIGHT;
  char msg[BTM_SCRN_WIDTH/8];
  u8 drawn = 0;
  
  for (u8 i = 0; i < MAX_POINTERS_TO_CHECK; i++)
  {
    Monster* m = pointer_list->m[i];
    if (!m || m->max_hp < 550)
      continue;
    
    if (!drawn)
    {
      drawTransparentBlackRect(row-2, 2, 2 + ROW_HEIGHT*8 + 2, BTM_SCRN_WIDTH-4, 1);
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
        row += ROW_HEIGHT;
      } */
      u8 value = *((u8*)((u32)m + offsets[j]));
      xsprintf(msg, "%02X", value & bits[j]);
      drawString(row, col, WHITE, msg);
      col += 8*3;
    }
    row += ROW_HEIGHT;
  }
  
  return 0;
}

u32 debugFindListPointer()
{
  char msg[BTM_SCRN_WIDTH/8];
  
  findListPointer();
  
  if (pointer_list)
  {
    xsprintf(msg, "Found pointer: %08X", (u32)pointer_list);
    drawString(ROW_HEIGHT, 2, WHITE, msg);
  }
  
  return 0;
}

u32 displayInfo()
{
  u8 count = 0;
  u16 row = 0; //keep away from top edge of screen
  char msg[BTM_SCRN_WIDTH/8];
  
  count = getMonsterCount();
  if (count == 0)
    return 1;
  
  updateMonsterCache();
  
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
  u16 display_height = TEXT_BORDER-BACKGROUND_BORDER + ROW_HEIGHT*count + TEXT_BORDER-BACKGROUND_BORDER;
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
  
  for (u8 i = 0; i < MAX_POINTERS_TO_CHECK; i++)
  {
    u16 col = col_offset;
    Monster* m = pointer_list->m[i];
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
    if (m == m_cache[0].m)
    {
      drawHealthBarWithParts(row_offset + row, col, m->hp, m->max_hp, &m_cache[0]);
    }
    else if (m == m_cache[1].m)
    {
      drawHealthBarWithParts(row_offset + row, col, m->hp, m->max_hp, &m_cache[1]);      
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
    
    row += ROW_HEIGHT;
  }
  
  return 0;
}

u32 displayMenu(u32 key, volatile Settings *my_settings)
{  
  static const char* menu_options[] = {
      "Show overlay",
      "Show small monsters",
      "Show special stats",
      "Show percentage",
      "Display location",
      "Background level",
      "Health bar width",
    };
  static const char* menu_states[][4] = {
      {"off", "on"},
      {"off", "on"},
      {"off", "on"},
      {"numbers", "percentage"},
      {"BTM TOP-LFT", "BTM BTM-LFT", "TOP TOP-RHT", "TOP BTM_LFT"},
      {},
      {},
    };
  static const char* state_descriptions[][3] = {
      {"Enable/disable monster info overlay", "", ""},
      {"Show/hide small monster info", "", ""},
      {"Show/hide special attack damage,", "such as poison, paralysis ...etc", ""},
      {"Show numeric info, such as HP,", "in percentages instead of numbers", ""},
      {"Location of the overlay", "First part is top/bottom screen,", "and second part is screen corner"},
      {"Transparency of the overlay background", "Higher values are darker", "Set to 0 to disable background"},
      {"Length of the HP bar, in pixels", "Part bars will also scale", ""},
    };
  static const u8 num_options = sizeof(menu_options) / sizeof(char*);
  static const u8 max_displayed_options = 14;
  static u8 index = 0;
  static u8 display_index_start = 0;
  static u8 display_index_end = 6;  //manually adjust this to 1 minus either num_options or max_displayed_options, whichever is smaller
  static u64 tick = 0;
  
  drawTransparentBlackRect(0, 0, SCREEN_HEIGHT, BTM_SCRN_WIDTH, 2);
  
  //banner
  u16 row = 5;
  drawString(row, 2, WHITE, "MHXX Overlay Plugin  by Setsu-BHMT");
  row += ROW_HEIGHT;
  drawString(row, 2, WHITE, "  Press UP/DOWN to switch options");
  row += ROW_HEIGHT;
  drawString(row, 2, WHITE, "        LEFT/RIGHT to switch state");
  row += ROW_HEIGHT;
  drawString(row, 2, WHITE, "        B to exit");
  row += ROW_HEIGHT;
  drawString(row, 2, WHITE, "-------------------------------------");
  row += ROW_HEIGHT;
  
  //handle button input
  if (tick > svc_getSystemTick())
  {
    //do nothing, prevent button press bounce
  }
  else if (key & BUTTON_DU)
  {
    index = (index == 0) ? num_options - 1: index - 1;
    tick = svc_getSystemTick() + BTN_WAIT_TICK_COUNT;
  }
  else if (key & BUTTON_DD)
  {
    index = (index == num_options - 1) ? 0 : index + 1;
    tick = svc_getSystemTick() + BTN_WAIT_TICK_COUNT;
  }
  else if (key & BUTTON_DL)
  {
    switch (index)
    {
      case 0:
        my_settings->show_overlay = !my_settings->show_overlay;
        break;
      case 1:
        my_settings->show_small_monsters = !my_settings->show_small_monsters;
        break;
      case 2:
        my_settings->show_special_stats = !my_settings->show_special_stats;
        break;
      case 3:
        my_settings->show_percentage = !my_settings->show_percentage;
        break;
      case 4:
        my_settings->display_location = (my_settings->display_location == 0) ? 
          3 : my_settings->display_location - 1;
        break;
      case 5:
        my_settings->background_level--;
        break;
      case 6:
        my_settings->health_bar_width--;
        break;
    }
    tick = svc_getSystemTick() + BTN_WAIT_TICK_COUNT;
  }
  else if (key & BUTTON_DR)
  {
    switch (index)
    {
      case 0:
        my_settings->show_overlay = !my_settings->show_overlay;
        break;
      case 1:
        my_settings->show_small_monsters = !my_settings->show_small_monsters;
        break;
      case 2:
        my_settings->show_special_stats = !my_settings->show_special_stats;
        break;
      case 3:
        my_settings->show_percentage = !my_settings->show_percentage;
        break;
      case 4:
        my_settings->display_location = (my_settings->display_location == 3) ? 
          0 : my_settings->display_location + 1;
        break;
      case 5:
        my_settings->background_level++;
        break;
      case 6:
        my_settings->health_bar_width++;
        break;
    }
    tick = svc_getSystemTick() + BTN_WAIT_TICK_COUNT;
  }
  else
  {
    //user didn't press anything we care about
    //so we guard against system tick overflow
    tick = 0;
  }
  
  //scroll the settings to be displayed
  while (index > display_index_end)
  {
    display_index_start++;
    display_index_end++;
  }
  while (index < display_index_start)
  {
    display_index_start--;
    display_index_end--;
  }  
    
  //display settings
  char msg[BTM_SCRN_WIDTH/8];
  for (u8 i = display_index_start; i <= display_index_end; i++)
  {
    switch (i)
    {
      case 0:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][my_settings->show_overlay]);
        break;
      case 1:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][my_settings->show_small_monsters]);
        break;
      case 2:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][my_settings->show_special_stats]);
        break;
      case 3:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][my_settings->show_percentage]);
        break;
      case 4:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][my_settings->display_location]);
        break;
      case 5:
        xsprintf(msg, "%s: %u", menu_options[i], my_settings->background_level);
        break;
      case 6:
        xsprintf(msg, "%s: %u", menu_options[i], my_settings->health_bar_width);
        break;
    }
    if (i == index)
    {
      drawString(row, 2 + 8, WHITE, ">");
    }
    drawString(row, 2 + 8*3, WHITE, msg);
    row += ROW_HEIGHT;
  }
  
  //display descriptions
  row = 5 + ROW_HEIGHT * 19;
  drawString(row, 2, WHITE, "Description--------------------------");
  row += ROW_HEIGHT;
  for (u8 i = 0; i < 3; i++)
  {
    xsprintf(msg, "%s", state_descriptions[index][i]);  //ovDrawString doesn't like const pointers
    drawString(row, 2, WHITE, msg);
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
    
    if (!pointer_list)
    {
      findListPointer();
    }
    else if (settings.show_overlay && settings.display_location < 2)
      return displayInfo();
  }
  else if (settings.show_overlay && settings.display_location > 1) //should display in top screen
  {
    if (!pointer_list)
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

int main()
{
  initSharedFunc();
  initSrv();
  ptmuInit();
  
  if (((NS_CONFIG*)(NS_CONFIGURE_ADDR))->sharedFunc[8])
  {
    IoBasePad = plgGetIoBase(IO_BASE_PAD);
  }
  
  //plgGetSharedServiceHandle("fs:USER", &fsUserHandle);
  plgRegisterCallback(CALLBACK_OVERLAY, (void*) overlayCallback, 0);
  
  return 0;
}
