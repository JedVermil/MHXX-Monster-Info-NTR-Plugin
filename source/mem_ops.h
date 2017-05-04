#pragma once

#include "global.h"
#include "monster.h"
#include "settings.h"

#define TIMEOUT_TICK_COUNT 400000000
#define SEARCH_START_OFFSET 0x08200000
#define SEARCH_END_OFFSET 0x08300000
#define VALID_POINTER_RANGE 0x400000

static u32 IoBasePad = 0xFFFD4000;

void updateIoBasePad(u32 value);
u32 getKey();
u8 findListPointer(volatile Settings* settings);