#include "monster.h"

extern inline u8 isSmallMonster();

static u8 num_db_entries = 0;

static int compareMonsterInfo(const void* p1, const void* p2)
{
  MonsterInfo* entry1 = (MonsterInfo*)p1;
  MonsterInfo* entry2 = (MonsterInfo*)p2;
  
  if (entry1->id == entry2->id)
    return 0;
  else
    return (entry1->id > entry2->id) ? 1 : -1;
}

void initMonsterInfoDB()
{
  num_db_entries = sizeof(database)/sizeof(MonsterInfo);
  qsort(database, num_db_entries, sizeof(MonsterInfo), compareMonsterInfo);
}

void updateMonsterCache(MonsterPointerList* list)
{
  Monster* new_m1 = 0;
  Monster* new_m2 = 0;
  u8 keep_m1 = 0;
  u8 keep_m2 = 0;

  //check all monsters, excluding small ones
  for (u8 i = 0; i < MAX_POINTERS_IN_LIST; i++)
  {
    Monster* m = list->m[i];
    if (!m || isSmallMonster(m))
      continue;
    
    if (m == m_cache[0].m)
    {
      keep_m1 = 1;
    }
    else if (m == m_cache[1].m)
    {
      keep_m2 = 1;
    }
    else if (new_m1 == 0)
    { 
      //save new monster pointer so we can add parts info later
      new_m1 = m;
    }
    else if (new_m2 == 0)
    {
      new_m2 = m;
    }
  }

  //remove expired monster parts
  if (!keep_m1)
  {
    m_cache[0].m = 0;
    m_cache[0].break_hp_sum = 0;
  }
  if (!keep_m2)
  {
    m_cache[1].m = 0;
    m_cache[1].break_hp_sum = 0;
  }

  //add new monster stats
  //note: assume new_m2 will never be assigned before new_m1
  //note: only display parts that have more than 2 break_hp; for non-breakable parts it is typically negative but it can be fixed to 1 if there are special critereas involved
  if (new_m1)
  {
    if (!m_cache[0].m)
    {
      m_cache[0].m = new_m1;
      
      for (u8 i = 0; i < 8; i++)
      {
        m_cache[0].p[i].max_stagger_hp = new_m1->parts[i].stagger_hp;
        m_cache[0].p[i].max_break_hp = new_m1->parts[i].break_hp;
        
        if (m_cache[0].p[i].max_break_hp > 2)
        {
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
        
        if (m_cache[1].p[i].max_break_hp > 2)
        {
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
        
        if (m_cache[0].p[i].max_break_hp > 2)
        {
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
        
        if (m_cache[1].p[i].max_break_hp > 2)
        {
          m_cache[1].break_hp_sum += m_cache[1].p[i].max_break_hp;
        }
      }
    }
  }
}

u8 getMonsterCount(MonsterPointerList* list, u8 show_small_monsters)
{
  u8 count = 0;
  
  for (u8 i = 0; i < MAX_POINTERS_IN_LIST; i++)
  {
    Monster* m = list->m[i];
    
    if (m && (show_small_monsters || !isSmallMonster(m)))
      count++;
  }
  
  return count;
}

MonsterInfo* getMonsterInfoFromDB(Monster* m)
{
  u32 id;

  id = m->identifier1;
  id <<= 8;
  id += m->identifier2;
  
  void* result = bsearch(&id, database, num_db_entries, sizeof(MonsterInfo), compareMonsterInfo);
    
  return (result == NULL) ? &unknown : (MonsterInfo*)result;
}
MonsterInfo* getMonsterInfoByIndex(u8 index)
{
  //this is for debugging only, nothing is checked
  return &(database[index]);
}

MonsterCache* getCachedMonsterByIndex(u8 index)
{
  return (index > 1) ? NULL : &(m_cache[index]);
}

MonsterCache* getCachedMonsterByPointer(Monster* m)
{
  if (m_cache[0].m == m)
    return &(m_cache[0]);
  else if (m_cache[1].m == m)
    return &(m_cache[1]);
  else
    return 0;
}