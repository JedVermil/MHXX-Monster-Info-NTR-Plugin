#include "mem_ops.h"

void updateIoBasePad(u32 value)
{
  IoBasePad = value;
}

u32 getKey() 
{
  return (*(vu32*)(IoBasePad) ^ 0xFFF) & 0xFFF;
}

u8 findListPointer(volatile Settings* settings)
{
  //single instance only, needed because this can take some time
  static volatile u8 is_running = 0;
  if (is_running)
    return 0;
  
  //avoid running too frequently
  //note: avoids tearing in opening and drop in framerate before first quest
  static volatile u64 tick = 0;
  if (tick > svc_getSystemTick())
    return 0;
  tick = svc_getSystemTick() + TIMEOUT_TICK_COUNT;
  
  is_running = 1;
  
  u32 offset = SEARCH_START_OFFSET;
  while (offset < SEARCH_END_OFFSET)
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
    // 4. should be strictly increasing
    u8 my_count = 0;
    u8 should_be_0 = 0;
    u32 valid_pointer_min = 0;
    u32 valid_pointer_max = 0;
    for (u8 i = 0; i < MAX_POINTERS_IN_LIST; i++)
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
      else if (valid_pointer_min == 0)
      { //calculate a valid range, using the first pointer
        valid_pointer_min = p & 0xFF000000; //take only the most-significant byte
        valid_pointer_max = valid_pointer_min + VALID_POINTER_RANGE;
        my_count++;
      }
      else if (p < valid_pointer_min || p > valid_pointer_max)
      { //make some assumptions about where the pointer should live in
        skip = 1;
        break;
      }
      else
      {
        valid_pointer_min = p;  //pointer list needs to be strictly increasing
        my_count++;
      }
    }
    if (skip || my_count != l->count)
    { //only skip the fixed and unused bytes
      offset += 16;
      continue;
    }
    
    //we found it!!!
    settings->pointer_list = (MonsterPointerList*)offset;
    settings->is_modified = 1;
    is_running = 0;
    return 1;
  }
  
  is_running = 0;
  
  return 0;
}