#include <stdio.h>
#include <sndfile.h>
#include <luaT.h>
#include <string.h>
#include "TH.h"

const void* sndfile_id;
const void* torch_ShortTensor_id;
const void* torch_IntTensor_id;
const void* torch_FloatTensor_id;
const void* torch_DoubleTensor_id;

typedef struct SndFile__
{
    SNDFILE *file;
    SF_INFO info;
} SndFile;

static int sndfile_new(lua_State *L)
{
  int narg = lua_gettop(L);
  const char *path = NULL, *strmode = NULL;
  int hasinfo = 0;
  int mode = 0;
  SndFile *snd = NULL;

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

static int sndfile_close(lua_State *L)
{
  SndFile *snd = luaT_checkudata(L, 1, sndfile_id);

  if(snd->file)
    sf_close(snd->file);

  snd->file = NULL;

  return 0;
}

#define SNDFILE_IMPLEMENT_READ(NAME, CNAME)                             \
  static int sndfile_read##CNAME(lua_State *L)                           \
  {                                                                     \
    int narg = lua_gettop(L);                                           \
    SndFile *snd = NULL;                                                \
    TH##CNAME##Tensor *tensor = NULL;                                   \
    long nframe = -1;                                                   \
    long nframeread = -1;                                               \
                                                                        \
    if((narg == 2) && luaT_isudata(L, 1, sndfile_id) && lua_isnumber(L, 2)) \
    {                                                                   \
      snd = luaT_toudata(L, 1, sndfile_id);                             \
      nframe = lua_tonumber(L, 2);                                      \
      luaL_argcheck(L, nframe > 0, 2, "the number of frames must be positive"); \
      tensor = TH##CNAME##Tensor_newWithSize2d(nframe, snd->info.channels); \
      luaT_pushudata(L, tensor, torch_##CNAME##Tensor_id);              \
    }                                                                   \
    else if((narg == 2) && luaT_isudata(L, 1, sndfile_id) && luaT_isudata(L, 2, torch_##CNAME##Tensor_id)) \
    {                                                                   \
      snd = luaT_toudata(L, 1, sndfile_id);                             \
      tensor = luaT_toudata(L, 2, torch_##CNAME##Tensor_id);            \
      luaL_argcheck(L, tensor->nDimension == 2, 2, "the tensor must have 2 dimensions (nframe x channels)"); \
      luaL_argcheck(L, tensor->size[1] == snd->info.channels, 2, "dimension 2 size must be equal to the number of channels"); \
    }                                                                   \
    else                                                                \
      luaL_error(L, "expected arguments: SndFile ( Tensor | nframe )"); \
                                                                        \
    if(!snd->file)                                                      \
      luaL_error(L, "trying to read in a closed file");                 \
                                                                        \
    if(TH##CNAME##Tensor_isContiguous(tensor))                          \
      nframeread = sf_read_##NAME(snd->file, TH##CNAME##Tensor_data(tensor), tensor->size[0]*tensor->size[1]); \
    else                                                                \
    {                                                                   \
      TH##CNAME##Tensor *tensorc = TH##CNAME##Tensor_newContiguous(tensor); \
      nframeread = sf_read_##NAME(snd->file, TH##CNAME##Tensor_data(tensor), tensor->size[0]*tensor->size[1]); \
      TH##CNAME##Tensor_copy(tensor, tensorc);                          \
      TH##CNAME##Tensor_free(tensorc);                                  \
    }                                                                   \
                                                                        \
    if(nframeread != tensor->size[0])                                   \
      TH##CNAME##Tensor_resize2d(tensor, nframeread, tensor->size[1]);  \
                                                                        \
    return 1;                                                           \
  }

SNDFILE_IMPLEMENT_READ(short, Short)
SNDFILE_IMPLEMENT_READ(int, Int)
SNDFILE_IMPLEMENT_READ(float, Float)
SNDFILE_IMPLEMENT_READ(double, Double)

static const struct luaL_Reg sndfile_SndFile__ [] = {
  {"info", sndfile_info},
  {"close", sndfile_close},
  {"readShort", sndfile_readShort},
  {"readInt", sndfile_readInt},
  {"readFloat", sndfile_readFloat},
  {"readDouble", sndfile_readDouble},
  {NULL, NULL}
};

DLL_EXPORT int luaopen_libsndfile(lua_State *L)
{
  lua_newtable(L);
  lua_pushvalue(L, -1);
  lua_setfield(L, LUA_GLOBALSINDEX, "sndfile");

  torch_ShortTensor_id = luaT_checktypename2id(L, "torch.ShortTensor");
  torch_IntTensor_id = luaT_checktypename2id(L, "torch.IntTensor");
  torch_FloatTensor_id = luaT_checktypename2id(L, "torch.FloatTensor");
  torch_DoubleTensor_id = luaT_checktypename2id(L, "torch.DoubleTensor");

  sndfile_id = luaT_newmetatable(L, "sndfile.SndFile", NULL, sndfile_new, sndfile_free, NULL);
  luaL_register(L, NULL, sndfile_SndFile__);
  lua_pop(L, 1);

  return 1;
}
