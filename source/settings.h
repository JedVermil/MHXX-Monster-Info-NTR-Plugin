#pragma once

#include "global.h"
#include "monster.h"

#ifdef GAME_REGION_JPN
#define CONFIG_PATH "/plugin/0004000000197100/config"
#endif


#pragma pack(push,1)

typedef struct
{
  u8 is_modified;
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

#pragma pack(pop)

extern FS_archive sdmcArchive;
extern Handle fsUserHandle;

void loadSettings(volatile Settings* settings);
void saveSettings(volatile Settings* settings);