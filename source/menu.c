#include "menu.h"

#define IS_DEBUG_MENU_ENABLED 0

static u64 tick = 0;

u32 displayMenu(u32 key, volatile Settings *settings, volatile MenuState* menu)
{
  switch (menu->active_menu)
  {
    case 1:
      return displayDebugMenu(key, menu);
    case 2:
      return displaySpecialStatMenu(key, settings, menu);
    default:
      return displayMainMenu(key, settings, menu);
  }
}

static u16 drawBanner()
{ //return the row index of the next line
  
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
  
  return row;
}

static u32 displayMainMenu(u32 key, volatile Settings *settings, volatile MenuState* menu)
{  
  static const char* menu_options[] = {
      "Show overlay",
      "Language",
      "Show small monsters",
      "Show special stats",
      "Show size",
      "Show percentage",
      "Display location",
      "Background level",
      "Health bar width",
      "3D depth",
      "Configure special stats",
      "",
      "Search for monster list",
    };
  static const char* menu_states[][4] = {
      {"off", "on"},
      {"English", "Japanese"},
      {"off", "on"},
      {"off", "on"},
      {"off", "on"},
      {"numbers", "percentage"},
      {"BTM TOP-LEFT", "BTM BTM-LEFT", "TOP TOP-RIGHT", "TOP BTM_LEFT"},
      {},
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
      {"Show/hide monster size info", "", ""},
      {"Show numeric info, such as HP,", "in percentages instead of numbers", ""},
      {"Location of the overlay", "First part is top/bottom screen,", "and second part is screen corner"},
      {"Transparency of the overlay background", "Higher values are darker", "Set to 0 to disable background"},
      {"Length of the HP bar, in pixels", "Part bars will also scale", ""},
      {"Depth of 3D, if active", "Positive values make the display float", "Negative values make the display sink"},
      {"Adjust special stats display settings", "", "LEFT/RIGHT to activate sub-menu"},
      {"", "", ""},
      {"Try this if nothing displays", "Make sure you are in a quest", "Current location"},
    };
  static const u8 num_options = sizeof(menu_options) / sizeof(char*);
  static const u8 max_displayed_options = 14;
  static volatile u8 opp_state = 0; //used to index menu_states for non-settings operations
  static u8 index = 0;
  static u8 display_index_start = 0;
  static u8 display_index_end = 12;  //manually adjust this to 1 minus either num_options or max_displayed_options, whichever is smaller
  
  drawTransparentBlackRect(0, 0, SCREEN_HEIGHT, BTM_SCRN_WIDTH, 2);
  
  u16 row = drawBanner();
  
  //handle button input
  if (tick > svc_getSystemTick())
  {
    //do nothing, prevent button press bounce
  }
  else if (IS_DEBUG_MENU_ENABLED && key & BUTTON_R)
  {
    menu->active_menu = 1;
    tick = svc_getSystemTick() + BTN_WAIT_TICK_COUNT;
    return 1;
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
        settings->show_size = !settings->show_size;
        break;
      case 5:
        settings->show_percentage = !settings->show_percentage;
        break;
      case 6:
        settings->display_location = (settings->display_location == 0) ? 
          3 : settings->display_location - 1;
        break;
      case 7:
        settings->background_level--;
        break;
      case 8:
        settings->health_bar_width--;
        break;
      case 9:
        if (settings->parallax_offset > -15)
        {
          settings->parallax_offset--;
        }
        break;
      case 10:
        menu->active_menu = 2;
        tick = svc_getSystemTick() + BTN_WAIT_TICK_COUNT;
        return 1;
      case 11:
        //do nothing
        break;
      case 12:
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
        settings->show_size = !settings->show_size;
        break;
      case 5:
        settings->show_percentage = !settings->show_percentage;
        break;
      case 6:
        settings->display_location = (settings->display_location == 3) ? 
          0 : settings->display_location + 1;
        break;
      case 7:
        settings->background_level++;
        break;
      case 8:
        settings->health_bar_width++;
        break;
      case 9:
        if (settings->parallax_offset < 15)
        {
          settings->parallax_offset++;
        }
        break;
      case 10:
        menu->active_menu = 2;
        tick = svc_getSystemTick() + BTN_WAIT_TICK_COUNT;
        return 1;
      case 11:
        //do nothing
        break;
      case 12:
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
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->show_size]);
        break;
      case 5:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->show_percentage]);
        break;
      case 6:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->display_location]);
        break;
      case 7:
        xsprintf(msg, "%s: %u", menu_options[i], settings->background_level);
        break;
      case 8:
        xsprintf(msg, "%s: %u", menu_options[i], settings->health_bar_width);
        break;
      case 9:
        xsprintf(msg, "%s: %d", menu_options[i], settings->parallax_offset);
        break;
      case 10:
        xsprintf(msg, "%s -->", menu_options[i]);
        break;
      case 11:
        xsprintf(msg, "");
        break;
      case 12:
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
    if (index == 12 && i == 2)
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

static u32 displaySpecialStatMenu(u32 key, volatile Settings *settings, volatile MenuState* menu)
{  
  static const char* menu_options[] = {
      "Poison",
      "color",
      "",
      "Paralysis",
      "color",
      "",
      "Sleep",
      "color",
      "",
      "Dizzy",
      "color",
      "",
      "Exhaust",
      "color",
      "",
      "Blast",
      "color",
      "",
      "Jump",
      "color",
      "",
      "Return"
    };
  static const char* menu_states[][4] = {
      {"off", "on"},
      {},
      {},
      {"off", "on"},
      {},
      {},
      {"off", "on"},
      {},
      {},
      {"off", "on"},
      {},
      {},
      {"off", "on"},
      {},
      {},
      {"off", "on"},
      {},
      {},
      {"off", "on"},
      {},
      {},
      {},
    };
  static const char* state_descriptions[][3] = {
      {"Show/hide poison stat", "", ""},
      {"Change display color for poison", "", ""},
      {"", "", ""},
      {"Show/hide paralysis stat", "", ""},
      {"Change display color for paralysis", "", ""},
      {"", "", ""},
      {"Show/hide sleep stat", "", ""},
      {"Change display color for sleep", "", ""},
      {"", "", ""},
      {"Show/hide dizzy stat", "", ""},
      {"Change display color for dizzy", "", ""},
      {"", "", ""},
      {"Show/hide exhaust stat", "", ""},
      {"Change display color for exhaust", "", ""},
      {"", "", ""},
      {"Show/hide blast stat", "", ""},
      {"Change display color for blast", "", ""},
      {"", "", ""},
      {"Show/hide jump stat", "", ""},
      {"Change display color for jump", "", ""},
      {"", "", ""},
      {"Go back to main menu", "", ""},
    };
  static const u8 num_options = sizeof(menu_options) / sizeof(char*);
  static const u8 max_displayed_options = 14;
  static u8 index = 0;
  static u8 color_index = 0;  //0 = list, 1 = r, 2 = g, 3 = b
  static u8 display_index_start = 0;
  static u8 display_index_end = 13;  //manually adjust this to 1 minus either num_options or max_displayed_options, whichever is smaller
  
  static volatile u8* p_color = NULL;
  
  drawTransparentBlackRect(0, 0, SCREEN_HEIGHT, BTM_SCRN_WIDTH, 2);
  
  u16 row = drawBanner();
  
  //handle button input
  if (tick > svc_getSystemTick())
  {
    //do nothing, prevent button press bounce
  }
  else if (key & BUTTON_DU)
  {
    if (p_color)
    { //we are in color adjust mode
      (*p_color)++;
      settings->is_modified = 1;
    }
    else if (index == 0)
    {
      index = num_options - 1;
    }
    else if (isEmptyString(menu_options[index - 1]))
    { //if next option is empty string, skip
      index -= 2;
    }
    else
    {
      index--;
    }
    
    tick = svc_getSystemTick() + BTN_WAIT_TICK_COUNT;
  }
  else if (key & BUTTON_DD)
  {
    if (p_color)
    { //we are in color adjust mode
      (*p_color)--;
      settings->is_modified = 1;
    }
    else if (index == num_options - 1)
    {
      index = 0;
    }
    else if (isEmptyString(menu_options[index + 1]))
    { //if next option is empty string, skip
      index += 2;
    }
    else
    {
      index++;
    }
    
    tick = svc_getSystemTick() + BTN_WAIT_TICK_COUNT;
  }
  else if (key & BUTTON_DL)
  {
    s8 stat_index = -1; //used to index into stat settings, where -1 means don't do anything
    switch (index)
    {
      case 0:
        settings->stat_enabled[0] = !settings->stat_enabled[0];
        settings->is_modified = 1;
        break;
      case 1:
        stat_index = 0;
        color_index = (color_index == 0) ? 3 : color_index - 1;
        break;
      case 3:
        settings->stat_enabled[1] = !settings->stat_enabled[1];
        settings->is_modified = 1;
        break;
      case 4:
        stat_index = 1;
        color_index = (color_index == 0) ? 3 : color_index - 1;
        break;
      case 6:
        settings->stat_enabled[2] = !settings->stat_enabled[2];
        settings->is_modified = 1;
        break;
      case 7:
        stat_index = 2;
        color_index = (color_index == 0) ? 3 : color_index - 1;
        break;
      case 9:
        settings->stat_enabled[3] = !settings->stat_enabled[3];
        settings->is_modified = 1;
        break;
      case 10:
        stat_index = 3;
        color_index = (color_index == 0) ? 3 : color_index - 1;
        break;
      case 12:
        settings->stat_enabled[4] = !settings->stat_enabled[4];
        settings->is_modified = 1;
        break;
      case 13:
        stat_index = 4;
        color_index = (color_index == 0) ? 3 : color_index - 1;
        break;
      case 15:
        settings->stat_enabled[5] = !settings->stat_enabled[5];
        settings->is_modified = 1;
        break;
      case 16:
        stat_index = 5;
        color_index = (color_index == 0) ? 3 : color_index - 1;
        break;
      case 18:
        settings->stat_enabled[6] = !settings->stat_enabled[6];
        settings->is_modified = 1;
        break;
      case 19:
        stat_index = 6;
        color_index = (color_index == 0) ? 3 : color_index - 1;
        break;
      case 21:
        menu->active_menu = 0;
        tick = svc_getSystemTick() + BTN_WAIT_TICK_COUNT;
        return 1;
      default:
        //do nothing
        break;
    }
    switch (color_index)
    {
      case 0:
        p_color = NULL;
        break;
      case 1:
        p_color = &(settings->stat_color[stat_index].r);
        break;
      case 2:
        p_color = &(settings->stat_color[stat_index].g);
        break;
      case 3:
        p_color = &(settings->stat_color[stat_index].b);
        break;
    }
    tick = svc_getSystemTick() + BTN_WAIT_TICK_COUNT;
  }
  else if (key & BUTTON_DR)
  {
    s8 stat_index = -1;
    switch (index)
    {
      case 0:
        settings->stat_enabled[0] = !settings->stat_enabled[0];
        settings->is_modified = 1;
        break;
      case 1:
        stat_index = 0;
        color_index = (color_index == 3) ? 0 : color_index + 1;
        break;
      case 3:
        settings->stat_enabled[1] = !settings->stat_enabled[1];
        settings->is_modified = 1;
        break;
      case 4:
        stat_index = 1;
        color_index = (color_index == 3) ? 0 : color_index + 1;
        break;
      case 6:
        settings->stat_enabled[2] = !settings->stat_enabled[2];
        settings->is_modified = 1;
        break;
      case 7:
        stat_index = 2;
        color_index = (color_index == 3) ? 0 : color_index + 1;
        break;
      case 9:
        settings->stat_enabled[3] = !settings->stat_enabled[3];
        settings->is_modified = 1;
        break;
      case 10:
        stat_index = 3;
        color_index = (color_index == 3) ? 0 : color_index + 1;
        break;
      case 12:
        settings->stat_enabled[4] = !settings->stat_enabled[4];
        settings->is_modified = 1;
        break;
      case 13:
        stat_index = 4;
        color_index = (color_index == 3) ? 0 : color_index + 1;
        break;
      case 15:
        settings->stat_enabled[5] = !settings->stat_enabled[5];
        settings->is_modified = 1;
        break;
      case 16:
        stat_index = 5;
        color_index = (color_index == 3) ? 0 : color_index + 1;
        break;
      case 18:
        settings->stat_enabled[6] = !settings->stat_enabled[6];
        settings->is_modified = 1;
        break;
      case 19:
        stat_index = 6;
        color_index = (color_index == 3) ? 0 : color_index + 1;
        break;
      case 21:
        menu->active_menu = 0;
        tick = svc_getSystemTick() + BTN_WAIT_TICK_COUNT;
        return 1;
      default:
        //do nothing
        break;
    }
    switch (color_index)
    {
      case 0:
        p_color = NULL;
        break;
      case 1:
        p_color = &(settings->stat_color[stat_index].r);
        break;
      case 2:
        p_color = &(settings->stat_color[stat_index].g);
        break;
      case 3:
        p_color = &(settings->stat_color[stat_index].b);
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
  if (display_index_end != num_options - 1 && 
      isEmptyString(menu_options[display_index_end + 1]))
  { //scroll so that color selection cursor is always visible
    display_index_start++;
    display_index_end++;
  }
  while (index < display_index_start)
  {
    display_index_start--;
    display_index_end--;
  }  
  if (display_index_start != 0 && 
      isEmptyString(menu_options[display_index_start - 1]))
  { //scroll so that color selection cursor is always visible
    display_index_start--;
    display_index_end--;
  }
    
  //display settings
  char msg[BTM_SCRN_WIDTH/CHAR_WIDTH];
  for (u8 i = display_index_start; i <= display_index_end; i++)
  {
    s8 stat_index = -1;
    switch (i)
    {
      case 0:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->stat_enabled[0]]);
        break;
      case 1:
        xsprintf(msg, "%s:", menu_options[i]);
        stat_index = 0;
        break;
      case 2:
        xsprintf(msg, "");
        break;
      case 3:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->stat_enabled[1]]);
        break;
      case 4:
        xsprintf(msg, "%s:", menu_options[i]);
        stat_index = 1;
        break;
      case 5:
        xsprintf(msg, "");
        break;
      case 6:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->stat_enabled[2]]);
        break;
      case 7:
        xsprintf(msg, "%s:", menu_options[i]);
        stat_index = 2;
        break;
      case 8:
        xsprintf(msg, "");
        break;
      case 9:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->stat_enabled[3]]);
        break;
      case 10:
        xsprintf(msg, "%s:", menu_options[i]);
        stat_index = 3;
        break;
      case 11:
        xsprintf(msg, "");
        break;
      case 12:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->stat_enabled[4]]);
        break;
      case 13:
        xsprintf(msg, "%s:", menu_options[i]);
        stat_index = 4;
        break;
      case 14:
        xsprintf(msg, "");
        break;
      case 15:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->stat_enabled[5]]);
        break;
      case 16:
        xsprintf(msg, "%s:", menu_options[i]);
        stat_index = 5;
        break;
      case 17:
        xsprintf(msg, "");
        break;
      case 18:
        xsprintf(msg, "%s: %s", menu_options[i], menu_states[i][settings->stat_enabled[6]]);
        break;
      case 19:
        xsprintf(msg, "%s:", menu_options[i]);
        stat_index = 6;
        break;
      case 20:
        xsprintf(msg, "");
        break;
      case 21:
        xsprintf(msg, "%s", menu_options[i]);
        break;
    }
    
    //cursor
    if (i == index && color_index == 0)
    {
      drawString(row, 2 + CHAR_WIDTH, WHITE, ">");
    }
    else if (i == index + 1)
    {
      switch (color_index)
      {
        case 1:
          drawString(row, 2 + CHAR_WIDTH*11, WHITE, "^");
          break;
        case 2:
          drawString(row, 2 + CHAR_WIDTH*15, WHITE, "^");
          break;
        case 3:
          drawString(row, 2 + CHAR_WIDTH*19, WHITE, "^");
          break;
        default:
          break;
      }
    }
    
    //option label
    drawString(row, 2 + CHAR_WIDTH*3, WHITE, msg);
    if (stat_index > -1)
    {
      xsprintf(msg, "%3u", settings->stat_color[stat_index].r);
      drawString(row, 2 + CHAR_WIDTH*10, RED, msg);
      xsprintf(msg, "%3u", settings->stat_color[stat_index].g);
      drawString(row, 2 + CHAR_WIDTH*14, GREEN, msg);
      xsprintf(msg, "%3u", settings->stat_color[stat_index].b);
      drawString(row, 2 + CHAR_WIDTH*18, BLUE, msg);
      
      //color preview box
      u8 preview_box[] = {0xF0, 0x2E, 0xFF};
      drawMisakiString(row, 2 + CHAR_WIDTH*23, settings->stat_color[stat_index], preview_box);
    }
    
    row += ROW_HEIGHT;
  }
  
  //display descriptions
  row = 5 + ROW_HEIGHT * 19;
  drawString(row, 2, WHITE, "Description--------------------------");
  row += ROW_HEIGHT;
  for (u8 i = 0; i < 3; i++)
  {
    xsprintf(msg, "%s", state_descriptions[index][i]);
    drawString(row, 2, WHITE, msg);
    row += ROW_HEIGHT;
  }
  
  return 0;
}

static u32 displayDebugMenu(u32 key, volatile MenuState* menu)
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
    menu->active_menu = 0;
    tick = svc_getSystemTick() + BTN_WAIT_TICK_COUNT;
    return 1;
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