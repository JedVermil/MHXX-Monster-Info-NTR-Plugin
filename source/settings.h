#pragma once

#include "3dstypes.h"
#include "monster.h"

typedef struct
{
  MonsterPointerList* pointer_list;
  u8 show_overlay;        //0 = off, 1 = on
  u8 show_small_monsters; //0 = off, 1 = on
  u8 show_special_stats;  //0 = off, 1 = on
  u8 show_percentage;     //0 = numbers, 1 = percentage
  u8 display_location;    //0 = bottom top-left, 1 = bottom bottom-left, 
                          //2 = top top-right, 3 = top bottom-left
  u8 background_level;
  u8 health_bar_width;
} Settings;