#include "menu.h"

u32 displayMenu(u32 key, volatile Settings *settings)
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
      {"BTM TOP-LEFT", "BTM BTM-LEFT", "TOP TOP-RIGHT", "TOP BTM_LEFT"},
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
  row += CHAR_HEIGHT;
  drawString(row, 2, WHITE, "  Press UP/DOWN to switch options");
  row += CHAR_HEIGHT;
  drawString(row, 2, WHITE, "        LEFT/RIGHT to switch state");
  row += CHAR_HEIGHT;
  drawString(row, 2, WHITE, "        B to exit");
  row += CHAR_HEIGHT;
  drawString(row, 2, WHITE, "-------------------------------------");
  row += CHAR_HEIGHT;
  
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
        settings->show_overlay = !settings->show_overlay;
        break;
      case 1:
        settings->show_small_monsters = !settings->show_small_monsters;
        break;
      case 2:
        settings->show_special_stats = !settings->show_special_stats;
        break;
      case 3:
        settings->show_percentage = !settings->show_percentage;
        break;
      case 4:
        settings->display_location = (settings->display_location == 0) ? 
          3 : settings->display_location - 1;
        break;
      case 5:
        settings->background_level--;
        break;
      case 6:
        settings->health_bar_width--;
        break;
    }
    tick = svc_getSystemTick() + BTN_WAIT_TICK_COUNT;
  }
  else if (key & BUTTON_DR)
  {
    switch (index)
    {
      case 0:
        settings->show_overlay = !settings->show_overlay;
        break;
      case 1:
        settings->show_small_monsters = !settings->show_small_monsters;
        break;
      case 2:
        settings->show_special_stats = !settings->show_special_stats;
        break;
      case 3:
        settings->show_percentage = !settings->show_percentage;
        break;
      case 4:
        settings->display_location = (settings->display_location == 3) ? 
          0 : settings->display_location + 1;
        break;
      case 5:
        settings->background_level++;
        break;
      case 6:
        settings->health_bar_width++;
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
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->show_overlay]);
        break;
      case 1:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->show_small_monsters]);
        break;
      case 2:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->show_special_stats]);
        break;
      case 3:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->show_percentage]);
        break;
      case 4:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->display_location]);
        break;
      case 5:
        xsprintf(msg, "%s: %u", menu_options[i], settings->background_level);
        break;
      case 6:
        xsprintf(msg, "%s: %u", menu_options[i], settings->health_bar_width);
        break;
    }
    if (i == index)
    {
      drawString(row, 2 + 8, WHITE, ">");
    }
    drawString(row, 2 + 8*3, WHITE, msg);
    row += CHAR_HEIGHT;
  }
  
  //display descriptions
  row = 5 + CHAR_HEIGHT * 19;
  drawString(row, 2, WHITE, "Description--------------------------");
  row += CHAR_HEIGHT;
  for (u8 i = 0; i < 3; i++)
  {
    xsprintf(msg, "%s", state_descriptions[index][i]);  //ovDrawString doesn't like const pointers
    drawString(row, 2, WHITE, msg);
    row += CHAR_HEIGHT;
  }
  
  return 0;
}