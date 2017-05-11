#pragma once

#include "global.h"
#include "ov.h"
#include "settings.h"
#include "mem_ops.h"
#include "monster.h"
#include "misc_funcs.h"

#define MENU_ACTIVATE (BUTTON_L | BUTTON_SE)
#define MENU_DEACTIVATE (BUTTON_B)
#define BTN_WAIT_TICK_COUNT 35000000

typedef struct
{
  u8 is_active;
  u8 is_busy;
  u8 active_menu; //1 = debug, 2 = special stat, default = main menu
} MenuState;

u32 displayMenu(u32 key, volatile Settings* settings, volatile MenuState* menu);

static u16 drawBanner();
static u32 displayMainMenu(u32 key, volatile Settings* settings, volatile MenuState* menu);
static u32 displaySpecialStatMenu(u32 key, volatile Settings* settings, volatile MenuState* menu);
static u32 displayDebugMenu(u32 key, volatile MenuState* menu);