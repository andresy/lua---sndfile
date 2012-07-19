#include <stdio.h>
#include <sndfile.h>
#include <luaT.h>
#include <string.h>

const void* sndfile_id;

typedef struct SndFile__
{
    SNDFILE *file;
    SF_INFO info;
} SndFile;

static int sndfile_new(lua_State *L)
{
  int narg = lua_gettop(L);
  const char *path, *strmode;
  int hasinfo;
  int mode;
  SndFile *snd;

  if((narg == 1) && lua_isstring(L, 1) )
  {
    path = lua_tostring(L, 1);
    strmode = "r";
    hasinfo = 0;
  }
  else if((narg == 2) && lua_isstring(L, 1)  && lua_isstring(L, 2))
  {
    path = lua_tostring(L, 1);
    strmode = lua_tostring(L, 2);
    hasinfo = 0;
  }
  else if((narg == 2) && lua_isstring(L, 1)  && lua_istable(L, 2))
  {
    path = lua_tostring(L, 1);
    strmode = "r";
    hasinfo = 2;
  }
  else if((narg == 3) && lua_isstring(L, 1)  && lua_isstring(L, 2) && lua_istable(L, 3))
  {
    path = lua_tostring(L, 1);
    strmode = lua_tostring(L, 2);
    hasinfo = 3;
  }
  else
    luaL_error(L, "invalid arguments: filename [mode] [info table] expected");

  if(!strcmp(strmode, "r"))
    mode = SFM_READ;
  else if(!strcmp(strmode, "w"))
    mode = SFM_WRITE;
  else if(!strcmp(strmode, "rw"))
    mode = SFM_RDWR;
  else
    luaL_error(L, "invalid mode (must be 'r', 'w', or 'rw')");


  snd = luaT_alloc(L, sizeof(SndFile));
  snd->file = NULL;
  snd->info.frames = 0;
  snd->info.samplerate = 0;
  snd->info.channels = 0;
  snd->info.format = 0;
  snd->info.sections = 0;
  snd->info.seekable = 0;

  luaT_pushudata(L, snd, sndfile_id);

  if(hasinfo)
  {
    lua_pushnil(L);
    while (lua_next(L, hasinfo) != 0)
    {
      const char *key;
      int value;

      if(!lua_isstring(L, -2) || !lua_isnumber(L, -1))
        luaL_error(L, "invalid key/value (must be string/number) in info table");

      key = lua_tostring(L, -2);
      value = (int)lua_tonumber(L, -3);

      if(!strcmp(key, "samplerate"))
        snd->info.samplerate = value;
      else if(!strcmp(key, "channels"))
        snd->info.channels = value;
      else if(!strcmp(key, "format"))
        snd->info.format = value;
      else
        luaL_error(L, "invalid key <%s> (must be samplerate | channels | format)", key);
      lua_pop(L, 1);
    }
  }

  if( ((mode == SFM_WRITE) || (mode == SFM_RDWR)) && !sf_format_check(&snd->info) )
    luaL_error(L, "invalid info table");

  if(!(snd->file = sf_open(path, mode, &snd->info)))
    luaL_error(L, "could not open file <%s>", path);
  
  return 1;
}

static int sndfile_info(lua_State *L)
{  
  SndFile *snd = luaT_checkudata(L, 1, sndfile_id);
  lua_newtable(L);
  lua_pushnumber(L, snd->info.frames);
  lua_setfield(L, -2, "frames");
  lua_pushnumber(L, snd->info.samplerate);
  lua_setfield(L, -2, "samplerate");
  lua_pushnumber(L, snd->info.channels);
  lua_setfield(L, -2, "channels");
  lua_pushnumber(L, snd->info.format);
  lua_setfield(L, -2, "format");
  lua_pushnumber(L, snd->info.sections);
  lua_setfield(L, -2, "sections");
  lua_pushboolean(L, snd->info.seekable);
  lua_setfield(L, -2, "seekable");
  return 1;
}

static int sndfile_free(lua_State *L)
{
  SndFile *snd = luaT_checkudata(L, 1, sndfile_id);

  if(snd->file)
    sf_close(snd->file);

  luaT_free(L, snd);

  return 0;
}

static const struct luaL_Reg sndfile_SndFile__ [] = {
  {"info", sndfile_info},
  {NULL, NULL}
};

DLL_EXPORT int luaopen_libsndfile(lua_State *L)
{
  lua_newtable(L);
  lua_pushvalue(L, -1);
  lua_setfield(L, LUA_GLOBALSINDEX, "sndfile");

  sndfile_id = luaT_newmetatable(L, "sndfile.SndFile", NULL, sndfile_new, sndfile_free, NULL);
  luaL_register(L, NULL, sndfile_SndFile__);
  lua_pop(L, 1);

  return 1;
}
