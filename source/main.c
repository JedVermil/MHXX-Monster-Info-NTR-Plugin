#include "global.h"
#include "ov.h"
#include "monster.h"

FS_archive sdmcArchive = { 0x9, (FS_path){ PATH_EMPTY, 1, (u8*)"" } };
Handle fsUserHandle = 0;

typedef struct
{
  u32 r;
  u32 g;
  u32 b;
} Color;

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

static Handle ptmuHandle;

#define CALLBACK_OVERLAY (101)

extern u32 getCurrentProcessHandle();
extern void initSharedFunc();

#ifdef GAME_REGION_JPN
#define MONSTER_POINTER_LIST_ADDR 0x082B9674
#endif

//get a list of pointers that points to the monster structs
static Monster** pointer_list = (Monster**)MONSTER_POINTER_LIST_ADDR;

const u8 MAX_POINTERS_TO_CHECK = 10;
const u8 ROW_HEIGHT = 10;
const u16 BTM_SCRN_WIDTH = 320;
const Color WHITE = {.r = 255, .g = 255, .b = 255};

//assume only 2 big monsters are active at a time
static MonsterCache m_cache[2];

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

u8 calculatePercentage(int hp, int max_hp)
{
  int percentage = hp*100 / max_hp;
  
  //always draw at least 1 pixel
  if (hp != 0 && percentage == 0)
  {
    percentage = 1;
  }
  
  return percentage;
}

void drawBorder(u32 addr, u32 stride, u32 format, int row, int col, int height, Color c)
{
  //left
  ovDrawRect(addr, stride, format, row, col, height, 2, c.r, c.g, c.b);
  //top
  ovDrawRect(addr, stride, format, row, col, 2, 104, c.r, c.g, c.b);
  //right
  ovDrawRect(addr, stride, format, row, col+102, height, 2, c.r, c.g, c.b);
  //bottom
  ovDrawRect(addr, stride, format, row + height-2, col, 2, 104, c.r, c.g, c.b);
}

void drawHealthBar(u32 addr, u32 stride, u32 format, int row, int col, int hp, int max_hp)
{
  if (hp == 0)
    return;
  
  Color c = calculateColor(hp, max_hp);
  u8 percentage = calculatePercentage(hp, max_hp);
  
  drawBorder(addr, stride, format, row, col, 7, WHITE);
  ovDrawRect(addr, stride, format, row+2, col+2, 3, percentage, c.r, c.g, c.b);
}

void drawHealthBarWithParts(u32 addr, u32 stride, u32 format, int row, int col, int hp, int max_hp, MonsterCache* cache)
{
  if (hp == 0)
    return;
  
  Color c = calculateColor(hp, max_hp);
  u8 percentage = calculatePercentage(hp, max_hp);
  
  drawBorder(addr, stride, format, row, col, ROW_HEIGHT, WHITE);
  ovDrawRect(addr, stride, format, row+2, col+2, 3, percentage, c.r, c.g, c.b);
  
  //center divide
  ovDrawRect(addr, stride, format, row + 7-2, col, 1, 104, WHITE.r, WHITE.g, WHITE.b);

  u8 offset = 2;
  for (u8 i = 0; i < 8; i++)
  {
    if (cache->p[i].max_break_hp < 1)
      continue;
    
    u8 bar_length = cache->p[i].max_break_hp * 100 / cache->break_hp_sum;
    
    //don't draw bar if it has been broken once and the part HP is at maximum
    //note: you can't tell the difference between a part that is broken and a part that has been partially broken once and has returned back to full health,
    //      but if it's broken for good then the part HP is fixed at max
    if (cache->m->parts[i].break_count == 0 || 
        cache->m->parts[i].break_hp < cache->p[i].max_break_hp)
    {
      c = calculateColor(cache->m->parts[i].break_hp, cache->p[i].max_break_hp);
      percentage = calculatePercentage(cache->m->parts[i].break_hp, cache->p[i].max_break_hp);
      percentage = percentage * bar_length / 100; //scale to part bar
      if (percentage == 0 && cache->m->parts[i].break_hp > 0)
      { //always show at least 1 pixel if hp is not 0
        percentage = 1;
      }
      
      ovDrawRect(addr, stride, format, row + 8-2, col + offset, 2, percentage, c.r, c.g, c.b);
    }
    
    offset += bar_length;
    ovDrawRect(addr, stride, format, row + 8-2, col + offset, 2, 1, WHITE.r, WHITE.g, WHITE.b);
    offset++;
  }
  
  //fill in any remaining pixels to the right
  if (offset < 102)
  {
    ovDrawRect(addr, stride, format, row + 8-2, col + offset, 2, 102 - offset, WHITE.r, WHITE.g, WHITE.b);
  }
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
    if (!pointer_list[i] || pointer_list[i]->max_hp < 375)
      continue;
    
    if (pointer_list[i] == m_cache[0].m)
    {
      keep_m1 = 1;
    }
    else if (pointer_list[i] == m_cache[1].m)
    {
      keep_m2 = 1;
    }
    else if (new_m1 == 0)
    { 
      //save new monster pointer so we can add parts info later
      new_m1 = pointer_list[i];
    }
    else if (new_m2 == 0)
    {
      new_m2 = pointer_list[i];
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
    if (pointer_list[i])
      count++;
  }
  
  return count;
}

u32 debugListPointers(u32 addr, u32 stride, u32 format)
{
  u8 row = ROW_HEIGHT;
  char msg[100];
  
  for (u8 i = 0; i < MAX_POINTERS_TO_CHECK; i++)
  {
    Monster* m = pointer_list[i];
    if (!m)
      continue;
    
    xsprintf(msg, "%X", (u32)m);
    ovDrawString(addr, stride, format, BTM_SCRN_WIDTH, row, 4, 255, 255, 255, msg);
    row += 10;
  }
  
  return 0;
}

u32 debugListStructs(u32 addr, u32 stride, u32 format)
{
  u8 row = ROW_HEIGHT;
  char msg[BTM_SCRN_WIDTH/8];
  
  for (u8 i = 0; i < MAX_POINTERS_TO_CHECK; i++)
  {
    Monster* m = pointer_list[i];
    if (!m)
      continue;
    
    ovDrawTranspartBlackRect(addr, stride, format, row-2, 2, 2 + ROW_HEIGHT*8 + 2, BTM_SCRN_WIDTH-4, 1);
    
    for (u8 j = 0; j < 8; j++)
    {
      xsprintf(msg, "%04X %08X | %u %u %4u %4d",
        m->parts[j].unknown1, m->parts[j].fixed, m->parts[j].stagger_count,
        m->parts[j].break_count, m->parts[j].stagger_hp, m->parts[j].break_hp);
      ovDrawString(addr, stride, format, BTM_SCRN_WIDTH, row, 2, 255, 255, 255, msg);
      row += 10;
    }
    
    break;
    
    /* xsprintf(msg, "%08X %08X %08X %08X", 
      m->is_despawned, m->optional2[0], m->optional2[1], m->test1);
    ovDrawString(addr, stride, format, BTM_SCRN_WIDTH, row, 2, 255, 255, 255, msg);
    row += 10;
    xsprintf(msg, "%08X %08X %02X %02X", 
      m->test2, m->test3, m->test4, m->test5);
    ovDrawString(addr, stride, format, BTM_SCRN_WIDTH, row, 2, 255, 255, 255, msg);
    row += 14; */
  }
  
  return 0;
}

u32 displayInfo(u32 addr, u32 stride, u32 format)
{
  u8 count = 0;
  u8 row = ROW_HEIGHT; //keep away from top edge of screen
  char msg[100];
  
  count = getMonsterCount();
  if (count == 0)
    return 1;
  
  updateMonsterCache();
  
  //draw background
  ovDrawTranspartBlackRect(addr, stride, format, row-2, 2, 2 + ROW_HEIGHT*count + 2, 2 + 8*10 + 104 + 2, 1);
  
  for (u8 i = 0; i < MAX_POINTERS_TO_CHECK; i++)
  {
    Monster* m = pointer_list[i];
    if (!m)
      continue;
    
    //draw this monster
    xsprintf(msg, "HP: %u", m->hp);
    ovDrawString(addr, stride, format, BTM_SCRN_WIDTH, row+1, 4, 255, 255, 255, msg);
    if (m == m_cache[0].m)
    {
      drawHealthBarWithParts(addr, stride, format, row, 4 + 8*10, m->hp, m->max_hp, &m_cache[0]);
    }
    else if (m == m_cache[1].m)
    {
      drawHealthBarWithParts(addr, stride, format, row, 4 + 8*10, m->hp, m->max_hp, &m_cache[1]);      
    }
    else
    {
      drawHealthBar(addr, stride, format, row, 4 + 8*10, m->hp, m->max_hp);
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
