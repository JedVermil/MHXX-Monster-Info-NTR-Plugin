#include "menu.h"

#define IS_DEBUG_MENU_ENABLED 0

static volatile u8 is_debug_menu_active = 0;
static u64 tick = 0;

u32 displayMenu(u32 key, volatile Settings *settings, volatile MenuState* menu)
{  
  static const char* menu_options[] = {
      "Show overlay",
      "Language",
      "Show small monsters",
      "Show special stats",
      "Show percentage",
      "Display location",
      "Background level",
      "Health bar width",
      "3D depth",
      "",
      "Search for monster list",
    };
  static const char* menu_states[][4] = {
      {"off", "on"},
      {"English", "Japanese"},
      {"off", "on"},
      {"off", "on"},
      {"numbers", "percentage"},
      {"BTM TOP-LEFT", "BTM BTM-LEFT", "TOP TOP-RIGHT", "TOP BTM_LEFT"},
      {},
      {},
      {},
      {},
      {"", "RUNNING", "SUCCESS", "FAILED"},
    };
  static const char* state_descriptions[][3] = {
      {"Enable/disable monster info overlay", "", ""},
      {"Monster info display language", "", ""},
      {"Show/hide small monster info", "", ""},
      {"Show/hide special attack damage,", "such as poison, paralysis ...etc", ""},
      {"Show numeric info, such as HP,", "in percentages instead of numbers", ""},
      {"Location of the overlay", "First part is top/bottom screen,", "and second part is screen corner"},
      {"Transparency of the overlay background", "Higher values are darker", "Set to 0 to disable background"},
      {"Length of the HP bar, in pixels", "Part bars will also scale", ""},
      {"Depth of 3D, if active", "Positive values make the display float", "Negative values make the display sink"},
      {"", "", ""},
      {"Try this if nothing displays", "Make sure you are in a quest", "Current location"},
    };
  static const u8 num_options = sizeof(menu_options) / sizeof(char*);
  static const u8 max_displayed_options = 14;
  static volatile u8 opp_state = 0; //used to index menu_states for non-settings operations
  static u8 index = 0;
  static u8 display_index_start = 0;
  static u8 display_index_end = 10;  //manually adjust this to 1 minus either num_options or max_displayed_options, whichever is smaller
  
  if (is_debug_menu_active)
    return displayDebugMenu(key, menu);
  
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
  else if (IS_DEBUG_MENU_ENABLED && key & BUTTON_R)
  {
    is_debug_menu_active = 1;
    tick = svc_getSystemTick() + BTN_WAIT_TICK_COUNT;
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
        settings->language = (settings->language == 0) ? 1 : settings->language - 1;
        break;
      case 2:
        settings->show_small_monsters = !settings->show_small_monsters;
        break;
      case 3:
        settings->show_special_stats = !settings->show_special_stats;
        break;
      case 4:
        settings->show_percentage = !settings->show_percentage;
        break;
      case 5:
        settings->display_location = (settings->display_location == 0) ? 
          3 : settings->display_location - 1;
        break;
      case 6:
        settings->background_level--;
        break;
      case 7:
        settings->health_bar_width--;
        break;
      case 8:
        if (settings->parallax_offset > -15)
        {
          settings->parallax_offset--;
        }
        break;
      case 9:
        //do nothing
        break;
      case 10:
        menu->is_busy = 1;
        opp_state = 1;
        opp_state = (findListPointer(settings)) ? 2 : 3;
        menu->is_busy = 0;
        break;
    }
    settings->is_modified = 1;
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
        settings->language = (settings->language == 1) ? 0 : settings->language + 1;
        break;
      case 2:
        settings->show_small_monsters = !settings->show_small_monsters;
        break;
      case 3:
        settings->show_special_stats = !settings->show_special_stats;
        break;
      case 4:
        settings->show_percentage = !settings->show_percentage;
        break;
      case 5:
        settings->display_location = (settings->display_location == 3) ? 
          0 : settings->display_location + 1;
        break;
      case 6:
        settings->background_level++;
        break;
      case 7:
        settings->health_bar_width++;
        break;
      case 8:
        if (settings->parallax_offset < 15)
        {
          settings->parallax_offset++;
        }
        break;
      case 9:
        //do nothing
        break;
      case 10:
        menu->is_busy = 1;
        opp_state = 1;
        opp_state = (findListPointer(settings)) ? 2 : 3;
        menu->is_busy = 0;
        break;
    }
    settings->is_modified = 1;
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
  char msg[BTM_SCRN_WIDTH/CHAR_WIDTH];
  for (u8 i = display_index_start; i <= display_index_end; i++)
  {
    switch (i)
    {
      case 0:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->show_overlay]);
        break;
      case 1:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->language]);
        break;
      case 2:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->show_small_monsters]);
        break;
      case 3:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->show_special_stats]);
        break;
      case 4:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->show_percentage]);
        break;
      case 5:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->display_location]);
        break;
      case 6:
        xsprintf(msg, "%s: %u", menu_options[i], settings->background_level);
        break;
      case 7:
        xsprintf(msg, "%s: %u", menu_options[i], settings->health_bar_width);
        break;
      case 8:
        xsprintf(msg, "%s: %d", menu_options[i], settings->parallax_offset);
        break;
      case 9:
        xsprintf(msg, "");
        break;
      case 10:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][opp_state]);
        break;
    }
    if (i == index)
    {
      drawString(row, 2 + CHAR_WIDTH, WHITE, ">");
    }
    drawString(row, 2 + CHAR_WIDTH*3, WHITE, msg);
    row += ROW_HEIGHT;
  }
  
  //display descriptions
  row = 5 + ROW_HEIGHT * 19;
  drawString(row, 2, WHITE, "Description--------------------------");
  row += ROW_HEIGHT;
  for (u8 i = 0; i < 3; i++)
  {
    //dynamic descriptions
    if (index == 10 && i == 2)
    {
      xsprintf(msg, "%s: %08X", state_descriptions[index][i], settings->pointer_list);
    }
    else
    {
      xsprintf(msg, "%s", state_descriptions[index][i]);  //ovDrawString doesn't like const pointers
    }
    
    drawString(row, 2, WHITE, msg);
    row += ROW_HEIGHT;
  }
  
  return 0;
}


u32 displayDebugMenu(u32 key, volatile MenuState* menu)
{  
  static const char* menu_options[] = {
      "SOME_TEXT",
      "R",
      "G",
      "B",
      "",
    };
  static const char* menu_states[][4] = {
      {},
      {},
      {},
      {},
      {},
    };
  static const char* state_descriptions[][3] = {
      {"", "", ""},
      {"", "", ""},
      {"", "", ""},
      {"", "", ""},
      {"", "", ""},
    };
  static const u8 num_options = sizeof(menu_options) / sizeof(char*);
  static const u8 max_displayed_options = 23;
  static volatile u8 opp_state = 0; //used to index menu_states for non-settings operations
  static u8 index = 0;
  static u8 display_index_start = 0;
  static u8 display_index_end = 3;  //manually adjust this to 1 minus either num_options or max_displayed_options, whichever is smaller
  
  static color text_color = {.r = 255, .g = 255, .b = 255};
  static u8 m_info_index = 0;
  
  drawTransparentBlackRect(0, 0, SCREEN_HEIGHT, BTM_SCRN_WIDTH, 2);
  
  //banner
  u16 row = 5;
  
  //handle button input
  if (tick > svc_getSystemTick())
  {
    //do nothing, prevent button press bounce
  }
  else if (key & BUTTON_R)
  {
    is_debug_menu_active = 0;
    tick = svc_getSystemTick() + BTN_WAIT_TICK_COUNT;
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
        break;
      case 1:
        text_color.r--;
        break;
      case 2:
        text_color.g--;
        break;
      case 3:
        text_color.b--;
        break;
      case 4:
        m_info_index = (m_info_index == 0) ? 175 : m_info_index - 1;
        break;
    }
    tick = svc_getSystemTick() + BTN_WAIT_TICK_COUNT;
  }
  else if (key & BUTTON_DR)
  {
    switch (index)
    {
      case 0:
        break;
      case 1:
        text_color.r++;
        break;
      case 2:
        text_color.g++;
        break;
      case 3:
        text_color.b++;
        break;
      case 4:
        m_info_index = (m_info_index == 175) ? 0 : m_info_index + 1;
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
  char msg[BTM_SCRN_WIDTH/CHAR_WIDTH];
  for (u8 i = display_index_start; i <= display_index_end; i++)
  {
    u8 is_drawn = 0;
    MonsterInfo* m_info = 0;
    switch (i)
    {
      case 0:
        xsprintf(msg, "%s", menu_options[i]);
        break;
      case 1:
        xsprintf(msg, "%u", text_color.r);
        break;
      case 2:
        xsprintf(msg, "%u", text_color.g);
        break;
      case 3:
        xsprintf(msg, "%u", text_color.b);
        break;
      case 4:
        m_info = getMonsterInfoByIndex(m_info_index);
        drawMisakiString(row, 2 + CHAR_WIDTH*3, (m_info->is_hyper) ? RED : WHITE, m_info->jp_name);
        is_drawn = 1;
    }
    if (i == index)
    {
      drawString(row, 2 + CHAR_WIDTH, WHITE, ">");
    }
    if (!is_drawn)
    {
      drawString(row, 2 + CHAR_WIDTH*3, (i == 0) ? text_color : WHITE, msg);
    }
    row += ROW_HEIGHT;
  }
  
  return 0;
}