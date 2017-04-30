#pragma once

#include "3dstypes.h"
#include "constants.h"
#include <ctr/types.h>
#include <ctr/svc.h>
#include "xprintf.h"
#include "ov.h"
#include "settings.h"

#define MENU_ACTIVATE (BUTTON_L | BUTTON_SE)
#define MENU_DEACTIVATE (BUTTON_B)
#define BTN_WAIT_TICK_COUNT 30000000

u32 displayMenu(u32 key, volatile Settings *settings);