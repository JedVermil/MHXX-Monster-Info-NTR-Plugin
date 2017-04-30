#include "settings.h"

void loadSettings(volatile Settings* settings)
{
  Handle infile = 0;
  Result res = FSUSER_OpenFileDirectly(fsUserHandle, &infile, sdmcArchive, FS_makePath(PATH_CHAR, CONFIG_PATH), FS_OPEN_READ, 0);
  if (res == 0)
  {
    u64 size = 0;
    FSFILE_GetSize(infile, &size);
    u32 bytesRead = 0;
    FSFILE_Read(infile, &bytesRead, 0, (u32*)settings, size);
  }
  else
  {
    //file does not exist, so create a blank file
    FSUSER_OpenFileDirectly(fsUserHandle, &infile, sdmcArchive, FS_makePath(PATH_CHAR, CONFIG_PATH), FS_OPEN_READ | FS_OPEN_CREATE, 0);
  }
  FSFILE_Close(infile);
}

void saveSettings(volatile Settings* settings)
{
  Handle outfile = 0;
  Result res = FSUSER_OpenFileDirectly(fsUserHandle, &outfile, sdmcArchive, FS_makePath(PATH_CHAR, CONFIG_PATH), FS_OPEN_WRITE, 0);
  if (res != 0)
    return; //we failed, do nothing
  
  settings->is_modified = 0;
  u32 bytesWritten = 0;
  FSFILE_Write(outfile, &bytesWritten, 0, (u32*)settings, sizeof(Settings), FS_WRITE_FLUSH);
  FSFILE_Close(outfile);
}