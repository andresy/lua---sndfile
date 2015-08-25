#include <sndfile.h>
#include <luaT.h>
#include <string.h>
#include "TH.h"
#include "format.h"
#include "buffer.h"

typedef struct SndFile__
{
  SNDFILE *file;
  SF_INFO info;
  struct byte_storage_buffer_t *buffer;
} SndFile;

static int sndfile_new(lua_State *L)
{
  int narg = lua_gettop(L);
  const char *path = NULL, *strmode = NULL;
  int hasinfo = 0;
  int mode = 0;
  SndFile *snd = NULL;
  THByteStorage *storage = NULL;

  if((narg == 1) && lua_isstring(L, 1) )
  {
    path = lua_tostring(L, 1);
    strmode = "r";
    hasinfo = 0;
  }
  else if((narg == 1) && luaT_isudata(L, 1, "torch.ByteStorage") )
  {
    storage = luaT_toudata(L, 1, "torch.ByteStorage");
    strmode = "r";
    hasinfo = 0;
  }
  else if((narg == 2) && lua_isstring(L, 1)  && lua_isstring(L, 2))
  {
    path = lua_tostring(L, 1);
    strmode = lua_tostring(L, 2);
    hasinfo = 0;
  }
  else if((narg == 2) && luaT_isudata(L, 1, "torch.ByteStorage") && lua_isstring(L, 2))
  {
    storage = luaT_toudata(L, 1, "torch.ByteStorage");
    strmode = lua_tostring(L, 2);
    hasinfo = 0;
  }
  else if((narg == 2) && lua_isstring(L, 1)  && lua_istable(L, 2))
  {
    path = lua_tostring(L, 1);
    strmode = "r";
    hasinfo = 2;
  }
  else if((narg == 2) && luaT_isudata(L, 1, "torch.ByteStorage") && lua_istable(L, 2))
  {
    storage = luaT_toudata(L, 1, "torch.ByteStorage");
    strmode = "r";
    hasinfo = 2;
  }
  else if((narg == 3) && lua_isstring(L, 1)  && lua_isstring(L, 2) && lua_istable(L, 3))
  {
    path = lua_tostring(L, 1);
    strmode = lua_tostring(L, 2);
    hasinfo = 3;
  }
  else if((narg == 3) && luaT_isudata(L, 1, "torch.ByteStorage") && lua_isstring(L, 2) && lua_istable(L, 3))
  {
    storage = luaT_toudata(L, 1, "torch.ByteStorage");
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
  snd->buffer = NULL;

  luaT_pushudata(L, snd, "sndfile.SndFile"); /* GCed if there is an error below */

  if(hasinfo)
  {
    lua_pushnil(L);
    while (lua_next(L, hasinfo) != 0)
    {
      const char *key;

      if(!lua_isstring(L, -2))
        luaL_error(L, "keys in info must be strings");
      key = lua_tostring(L, -2);

      if(!strcmp(key, "samplerate"))
      {
        if(!lua_isnumber(L, -1))
          luaL_error(L, "samplerate in info must be a number");
        snd->info.samplerate = (int)lua_tonumber(L, -1);
      }
      else if(!strcmp(key, "channels"))
      {
        if(!lua_isnumber(L, -1))
          luaL_error(L, "channels in info must be a number");
        snd->info.channels = (int)lua_tonumber(L, -1);
      }
      else if(!strcmp(key, "format"))
      {
        int format;

        if(!lua_isstring(L, -1))
          luaL_error(L, "format in info must be a string");

        format = sndfile_format_string2number(lua_tostring(L, -1));
        if(format >= 0)
          snd->info.format |= format;
        else
          luaL_error(L, "invalid format -- list of valid format:\n%s", sndfile_format_available);
      }
      else if(!strcmp(key, "subformat"))
      {
        int subformat;

        if(!lua_isstring(L, -1))
          luaL_error(L, "subformat in info must be a string");

        subformat = sndfile_subformat_string2number(lua_tostring(L, -1));
        if(subformat >= 0)
          snd->info.format |= subformat;
        else
          luaL_error(L, "invalid subformat -- list of valid subformat:\n%s", sndfile_subformat_available);
      }
      else if(!strcmp(key, "endian"))
      {
        int endian;

        if(!lua_isstring(L, -1))
          luaL_error(L, "endian in info must be a string");

        endian = sndfile_endian_string2number(lua_tostring(L, -1));
        if(endian >= 0)
          snd->info.format |= endian;
        else
          luaL_error(L, "invalid endian -- list of valid endian:\n%s", sndfile_endian_available);
      }
      else
        luaL_error(L, "invalid key <%s> (must be samplerate | channels | format | subformat | endian)", key);

      lua_pop(L, 1);
    }
  }

  if(!path && !storage)
    luaL_error(L, "please, provide a path or a valid ByteStorage");

  if(path) {
    if(!(snd->file = sf_open(path, mode, &snd->info)))
      luaL_error(L, "could not open file <%s> (%s)", path, sf_strerror(NULL));
  }
  else {
    struct byte_storage_buffer_t *buffer = luaT_alloc(L, sizeof(struct byte_storage_buffer_t));
    buffer->position = 0;
    buffer->storage = storage;
    THByteStorage_retain(storage);
    snd->buffer = buffer; /* buffer is GCed if there is an error */
    if(!(snd->file = sf_open_virtual(&sndfile_byte_storage_io, mode, &snd->info, buffer))) {
      luaL_error(L, "could not open virtual buffer");
    }
  }

  return 1;
}

static int sndfile_info(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else
    luaL_error(L, "expected arguments: SndFile");

  lua_newtable(L);
  lua_pushnumber(L, snd->info.frames);
  lua_setfield(L, -2, "frames");
  lua_pushnumber(L, snd->info.samplerate);
  lua_setfield(L, -2, "samplerate");
  lua_pushnumber(L, snd->info.channels);
  lua_setfield(L, -2, "channels");
  lua_pushstring(L, sndfile_format_number2string(snd->info.format));
  lua_setfield(L, -2, "format");
  lua_pushstring(L, sndfile_subformat_number2string(snd->info.format));
  lua_setfield(L, -2, "subformat");
  lua_pushstring(L, sndfile_endian_number2string(snd->info.format));
  lua_setfield(L, -2, "endian");
  lua_pushnumber(L, snd->info.sections);
  lua_setfield(L, -2, "sections");
  lua_pushboolean(L, snd->info.seekable);
  lua_setfield(L, -2, "seekable");
  return 1;
}

static int sndfile_free(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else
    luaL_error(L, "expected arguments: SndFile");

  if(snd->file)
    sf_close(snd->file);

  if(snd->buffer) {
    if(snd->buffer->storage)
      THByteStorage_free(snd->buffer->storage);
    free(snd->buffer);
  }

  luaT_free(L, snd);

  return 0;
}

static int sndfile_error(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else
    luaL_error(L, "expected arguments: SndFile");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  if(sf_error(snd->file))
  {
    lua_pushstring(L, sf_strerror(snd->file));
    return 1;
  }

  return 0;
}


static int sndfile_seek(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  long frames = 0;
  int whence = 0;

  if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_isnumber(L, 2))
  {
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
    frames = lua_tonumber(L, 2);
    whence = SEEK_SET;
  }
  else if((narg == 3) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_isnumber(L, 2) & lua_isnumber(L, 3))
  {
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
    frames = lua_tonumber(L, 2);
    whence = lua_tonumber(L, 3);
    luaL_argcheck(L, whence >= 0 && whence <= 2, 3, "whence parameter must be 0 (absolute), 1 (relative), 2 (to the end)");
  }
  else
    luaL_error(L, "expected arguments: SndFile #frame [whence (0,1,2)]");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  sf_seek(snd->file, frames, whence);

  return 0;
}

static int sndfile_sync(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else
    luaL_error(L, "expected arguments: SndFile");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  sf_write_sync(snd->file);

  return 0;
}

static int sndfile_string(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  const char *key = NULL;
  const char *value = NULL;

  if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_isstring(L, 2))
  {
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
    key = lua_tostring(L, 2);
  }
  else if((narg == 3) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_isstring(L, 2) && lua_isstring(L, 3))
  {
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
    key = lua_tostring(L, 2);
    value = lua_tostring(L, 3);
  }
  else
    luaL_error(L, "expected arguments: SndFile string [string]");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

#define SNDFILE_CHECK_STRING_KEY(NAME, CNAME)                           \
  if(!strcmp(key, NAME))                                                \
  {                                                                     \
    if(value)                                                           \
    {                                                                   \
      lua_pushboolean(L, sf_set_string(snd->file, SF_STR_##CNAME, value) == 0); \
      return 1;                                                         \
    }                                                                   \
    else                                                                \
    {                                                                   \
      lua_pushstring(L, sf_get_string(snd->file, SF_STR_##CNAME));      \
      return 1;                                                         \
    }                                                                   \
  }

  SNDFILE_CHECK_STRING_KEY("title", TITLE)
  else SNDFILE_CHECK_STRING_KEY("copyright", COPYRIGHT)
  else SNDFILE_CHECK_STRING_KEY("software", SOFTWARE)
  else SNDFILE_CHECK_STRING_KEY("artist", ARTIST)
  else SNDFILE_CHECK_STRING_KEY("comment", COMMENT)
  else SNDFILE_CHECK_STRING_KEY("date", DATE)
  else SNDFILE_CHECK_STRING_KEY("album", ALBUM)
  else SNDFILE_CHECK_STRING_KEY("license", LICENSE)
#ifdef SF_STR_TRACKNUMBER
  else SNDFILE_CHECK_STRING_KEY("tracknumber", TRACKNUMBER)
#endif
#ifdef SF_STR_GENRE
  else SNDFILE_CHECK_STRING_KEY("genre", GENRE)
#endif
  else
    luaL_error(L, "invalid string value (must be title|copyright|software|artist|comment|date|album|license|tracknumber|genre)");

  return 0;
}

static int sndfile_close(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else
    luaL_error(L, "expected arguments: SndFile");

  if(snd->file)
    sf_close(snd->file);

  snd->file = NULL;

  return 0;
}

#define SNDFILE_IMPLEMENT_READ(NAME, CNAME)                             \
  static int sndfile_read##CNAME(lua_State *L)                          \
  {                                                                     \
    int narg = lua_gettop(L);                                           \
    SndFile *snd = NULL;                                                \
    TH##CNAME##Tensor *tensor = NULL;                                   \
    long nframeread = -1;                                               \
                                                                        \
    if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_isnumber(L, 2)) \
    {                                                                   \
      long nframe = -1;                                                 \
      snd = luaT_toudata(L, 1, "sndfile.SndFile");                             \
      nframe = lua_tonumber(L, 2);                                      \
      luaL_argcheck(L, nframe > 0, 2, "the number of frames must be positive"); \
      tensor = TH##CNAME##Tensor_newWithSize2d(nframe, snd->info.channels); \
      luaT_pushudata(L, tensor, "torch." #CNAME "Tensor");              \
    }                                                                   \
    else if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && luaT_isudata(L, 2, "torch." #CNAME "Tensor")) \
    {                                                                   \
      snd = luaT_toudata(L, 1, "sndfile.SndFile");                             \
      tensor = luaT_toudata(L, 2, "torch." #CNAME "Tensor");            \
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
      nframeread = sf_readf_##NAME(snd->file, TH##CNAME##Tensor_data(tensor), tensor->size[0]); \
    else                                                                \
    {                                                                   \
      TH##CNAME##Tensor *tensorc = TH##CNAME##Tensor_newContiguous(tensor); \
      nframeread = sf_readf_##NAME(snd->file, TH##CNAME##Tensor_data(tensor), tensor->size[0]); \
      TH##CNAME##Tensor_copy(tensor, tensorc);                          \
      TH##CNAME##Tensor_free(tensorc);                                  \
    }                                                                   \
                                                                        \
    if(lua_isnumber(L, 2))                                              \
    {                                                                   \
      if(nframeread != tensor->size[0])                                 \
        TH##CNAME##Tensor_resize2d(tensor, nframeread, tensor->size[1]); \
    }                                                                   \
    else                                                                \
      lua_pushnumber(L, nframeread);                                    \
                                                                        \
    return 1;                                                           \
  }

SNDFILE_IMPLEMENT_READ(short, Short)
SNDFILE_IMPLEMENT_READ(int, Int)
SNDFILE_IMPLEMENT_READ(float, Float)
SNDFILE_IMPLEMENT_READ(double, Double)

#define SNDFILE_IMPLEMENT_WRITE(NAME, CNAME)                            \
  static int sndfile_write##CNAME(lua_State *L)                         \
  {                                                                     \
    int narg = lua_gettop(L);                                           \
    SndFile *snd = NULL;                                                \
    TH##CNAME##Tensor *tensor = NULL;                                   \
    long nframewrite = -1;                                              \
                                                                        \
    if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && luaT_isudata(L, 2, "torch." #CNAME "Tensor")) \
    {                                                                   \
      snd = luaT_toudata(L, 1, "sndfile.SndFile");                             \
      tensor = luaT_toudata(L, 2, "torch." #CNAME "Tensor");            \
      luaL_argcheck(L, tensor->nDimension == 2, 2, "the tensor must have 2 dimensions (nframe x channels)"); \
      luaL_argcheck(L, tensor->size[1] == snd->info.channels, 2, "dimension 2 size must be equal to the number of channels"); \
    }                                                                   \
    else                                                                \
      luaL_error(L, "expected arguments: SndFile Tensor");              \
                                                                        \
    if(!snd->file)                                                      \
      luaL_error(L, "trying to write in a closed file");                \
                                                                        \
    tensor = TH##CNAME##Tensor_newContiguous(tensor);                   \
    nframewrite = sf_writef_##NAME(snd->file, TH##CNAME##Tensor_data(tensor), tensor->size[0]); \
    lua_pushnumber(L, nframewrite);                                     \
    TH##CNAME##Tensor_free(tensor);                                     \
                                                                        \
    return 1;                                                           \
  }

SNDFILE_IMPLEMENT_WRITE(short, Short)
SNDFILE_IMPLEMENT_WRITE(int, Int)
SNDFILE_IMPLEMENT_WRITE(float, Float)
SNDFILE_IMPLEMENT_WRITE(double, Double)

static int sndfile_libversion(lua_State *L)
{
  long buffersize = 128;
  char *buffer = luaT_alloc(L, buffersize);
  long retsize;
  while(1)
  {
    retsize = sf_command(NULL, SFC_GET_LIB_VERSION, buffer, buffersize);
    if(buffersize <= retsize)
    {
      buffersize *= 2;
      luaT_free(L, buffer);
      buffer = luaT_alloc(L, buffersize);
    }
    else
      break;
  }
  lua_pushstring(L, buffer);
  return 1;
}

static int sndfile_loginfo(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  char buffer[2048];

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else
    luaL_error(L, "expected arguments: SndFile");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  sf_command(snd->file, SFC_GET_LOG_INFO, buffer, sizeof(buffer));
  lua_pushstring(L, buffer);
  return 1;
}

static int sndfile_signalmax(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  double max_val;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else
    luaL_error(L, "expected arguments: SndFile");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  if(!sf_command(snd->file, SFC_CALC_SIGNAL_MAX, &max_val, sizeof(max_val)))
  {
    lua_pushnumber(L, max_val);
    return 1;
  }
  return 0;
}

static int sndfile_normsignalmax(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  double max_val;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else
    luaL_error(L, "expected arguments: SndFile");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  if(!sf_command(snd->file, SFC_CALC_NORM_SIGNAL_MAX, &max_val, sizeof(max_val)))
  {
    lua_pushnumber(L, max_val);
    return 1;
  }

  return 0;
}

static int sndfile_maxallchannels(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  THDoubleTensor *max_val;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else
    luaL_error(L, "expected arguments: SndFile");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  max_val = THDoubleTensor_newWithSize1d(snd->info.channels);
  luaT_pushudata(L, max_val, "torch.DoubleTensor");

  if(!sf_command(snd->file, SFC_CALC_MAX_ALL_CHANNELS, THDoubleTensor_data(max_val), sizeof(double)*snd->info.channels))
    return 1;

  return 0;
}

static int sndfile_headersignalmax(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  double max_val;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else
    luaL_error(L, "expected arguments: SndFile");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  if(sf_command(snd->file, SFC_GET_SIGNAL_MAX, &max_val, sizeof(max_val)))
  {
    lua_pushnumber(L, max_val);
    return 1;
  }
  return 0;
}

static int sndfile_headermaxallchannels(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  THDoubleTensor *max_val;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else
    luaL_error(L, "expected arguments: SndFile");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  max_val = THDoubleTensor_newWithSize1d(snd->info.channels);
  luaT_pushudata(L, max_val, "torch.DoubleTensor");

  if(sf_command(snd->file, SFC_GET_MAX_ALL_CHANNELS, THDoubleTensor_data(max_val), sizeof(double)*snd->info.channels))
    return 1;

  return 0;
}

static int sndfile_normmaxallchannels(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  THDoubleTensor *max_val;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else
    luaL_error(L, "expected arguments: SndFile");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  max_val = THDoubleTensor_newWithSize1d(snd->info.channels);
  luaT_pushudata(L, max_val, "torch.DoubleTensor");

  if(!sf_command(snd->file, SFC_CALC_NORM_MAX_ALL_CHANNELS, THDoubleTensor_data(max_val), sizeof(double)*snd->info.channels))
    return 1;

  return 0;
}


static int sndfile_normfloat(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  int flag = 0;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_isboolean(L, 2))
  {
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
    flag = lua_toboolean(L, 2);
  }
  else
    luaL_error(L, "expected arguments: SndFile [boolean]");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  if(narg == 1)
    lua_pushboolean(L, sf_command(snd->file, SFC_GET_NORM_FLOAT, NULL, 0));
  else
    lua_pushboolean(L, sf_command(snd->file, SFC_SET_NORM_FLOAT, NULL, (flag ? SF_TRUE : SF_FALSE)));

  return 1;
}

static int sndfile_normdouble(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  int flag = 0;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_isboolean(L, 2))
  {
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
    flag = lua_toboolean(L, 2);
  }
  else
    luaL_error(L, "expected arguments: SndFile [boolean]");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  if(narg == 1)
    lua_pushboolean(L, sf_command(snd->file, SFC_GET_NORM_DOUBLE, NULL, 0));
  else
    lua_pushboolean(L, sf_command(snd->file, SFC_SET_NORM_DOUBLE, NULL, (flag ? SF_TRUE : SF_FALSE)));

  return 1;
}

static int sndfile_scalefloatintread(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  int flag = 0;

  if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_isboolean(L, 2))
  {
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
    flag = lua_toboolean(L, 2);
  }
  else
    luaL_error(L, "expected arguments: SndFile boolean");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  lua_pushboolean(L, sf_command(snd->file, SFC_SET_SCALE_FLOAT_INT_READ, NULL, (flag ? SF_TRUE : SF_FALSE)));
  return 1;
}

static int sndfile_scaleintfloatwrite(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  int flag = 0;

  if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_isboolean(L, 2))
  {
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
    flag = lua_toboolean(L, 2);
  }
  else
    luaL_error(L, "expected arguments: SndFile boolean");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  lua_pushboolean(L, sf_command(snd->file, SFC_SET_SCALE_INT_FLOAT_WRITE, NULL, (flag ? SF_TRUE : SF_FALSE)));
  return 1;
}

static int sndfile_addpeakchunk(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  int flag = 0;

  if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_isboolean(L, 2))
  {
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
    flag = lua_toboolean(L, 2);
  }
  else
    luaL_error(L, "expected arguments: SndFile boolean");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  lua_pushboolean(L, sf_command(snd->file, SFC_SET_ADD_PEAK_CHUNK, NULL, (flag ? SF_TRUE : SF_FALSE)));
  return 1;
}

static int sndfile_updateheadernow(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else
    luaL_error(L, "expected arguments: SndFile");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  sf_command(snd->file, SFC_UPDATE_HEADER_NOW, NULL, 0);
  return 0;
}

static int sndfile_updateheaderauto(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  int flag = 0;

  if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_isboolean(L, 2))
  {
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
    flag = lua_toboolean(L, 2);
  }
  else
    luaL_error(L, "expected arguments: SndFile boolean");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  lua_pushboolean(L, sf_command(snd->file, SFC_SET_UPDATE_HEADER_AUTO, NULL, (flag ? SF_TRUE : SF_FALSE)));
  return 1;
}

static int sndfile_filetruncate(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  sf_count_t frames = 0;

  if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_isnumber(L, 2))
  {
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
    frames = (sf_count_t)lua_tonumber(L, 2);
  }
  else
    luaL_error(L, "expected arguments: SndFile number");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  lua_pushboolean(L, sf_command(snd->file, SFC_FILE_TRUNCATE, &frames, sizeof(frames)) == 0);
  return 1;
}

static int sndfile_rawstartoffset(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  sf_count_t offset = 0;

  if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_isnumber(L, 2))
  {
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
    offset = (sf_count_t)lua_tonumber(L, 2);
  }
  else
    luaL_error(L, "expected arguments: SndFile number");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  lua_pushboolean(L, sf_command(snd->file, SFC_SET_RAW_START_OFFSET, &offset, sizeof(offset)) == 0);
  return 1;
}

static int sndfile_clipping(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  int flag = 0;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_isboolean(L, 2))
  {
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
    flag = lua_toboolean(L, 2);
  }
  else
    luaL_error(L, "expected arguments: SndFile [boolean]");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  if(narg == 1)
    lua_pushboolean(L, sf_command(snd->file, SFC_GET_CLIPPING, NULL, 0));
  else
    lua_pushboolean(L, sf_command(snd->file, SFC_SET_CLIPPING, NULL, (flag ? SF_TRUE : SF_FALSE)));

  return 1;
}

static int sndfile_embedfileinfo(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  SF_EMBED_FILE_INFO info;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else
    luaL_error(L, "expected arguments: SndFile");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  if(!sf_command(snd->file, SFC_GET_EMBED_FILE_INFO, &info, sizeof(info)))
  {
    lua_pushnumber(L, info.offset);
    lua_pushnumber(L, info.length);
    return 2;
  }
  return 0;
}

static int sndfile_wavexambisonic(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  const char *str = NULL;
  int ret = 0;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_isstring(L, 2))
  {
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
    str = lua_tostring(L, 2);
  }
  else
    luaL_error(L, "expected arguments: SndFile [string (none/bformat)]");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  if(narg == 1)
    ret = sf_command(snd->file, SFC_WAVEX_GET_AMBISONIC, NULL, 0);
  else
  {
    if(!strcmp(str, "none"))
      ret = sf_command(snd->file, SFC_WAVEX_SET_AMBISONIC, NULL, SF_AMBISONIC_NONE);
    else if(!strcmp(str, "bformat"))
      ret = sf_command(snd->file, SFC_WAVEX_SET_AMBISONIC, NULL, SF_AMBISONIC_B_FORMAT);
    else
      luaL_error(L, "invalid format: must be none or bformat");
  }

  if(ret == SF_AMBISONIC_NONE)
    lua_pushstring(L, "none");
  else if(ret == SF_AMBISONIC_B_FORMAT)
    lua_pushstring(L, "bformat");
  else
    lua_pushstring(L, "unsupported");
  return 1;
}

static int sndfile_vbrencodingquality(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  double quality = 0;

  if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_isnumber(L, 2))
  {
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
    quality = lua_tonumber(L, 2);
  }
  else
    luaL_error(L, "expected arguments: SndFile number");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  luaL_argcheck(L, quality >= 0 && quality <= 1, 2, "quality must be between 0 and 1");

  lua_pushboolean(L, sf_command(snd->file, SFC_SET_VBR_ENCODING_QUALITY, &quality, sizeof(quality)) == 0);
  return 1;
}

static void sndfile_broadcast_getstring(lua_State *L, int arg, const char *field, char *target, long maxsize)
{
  lua_getfield(L, arg, field);
  if(lua_isstring(L, -1))
  {
    size_t size;
    const char *str = lua_tolstring(L, -1, &size);
    memcpy(target, str, (size < maxsize ? size : maxsize));
    lua_pop(L, 1);
  }
  else if(lua_isnil(L, -1))
    lua_pop(L, 1);
  else
    luaL_error(L, "field %s should be a string", field);
}

static long sndfile_broadcast_getnumber(lua_State *L, int arg, const char *field)
{
  long res = 0;
  lua_getfield(L, arg, field);
  if(lua_isnumber(L, -1))
    res = (long)lua_tonumber(L, -1);
  else
    luaL_error(L, "filed %s should be a number", field);
  return res;
}

static int sndfile_broadcastinfo(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  SF_BROADCAST_INFO info;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_istable(L, 2))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else
    luaL_error(L, "expected arguments: SndFile [table]");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  if(narg == 1)
  {
    if(sf_command(snd->file, SFC_GET_BROADCAST_INFO, &info, sizeof(info)))
    {
      lua_newtable(L);
      lua_pushlstring(L, info.description, 256);
      lua_setfield(L, -2, "description");
      lua_pushlstring(L, info.originator, 32);
      lua_setfield(L, -2, "originator");
      lua_pushlstring(L, info.originator_reference, 32);
      lua_setfield(L, -2, "originatorreference");
      lua_pushlstring(L, info.origination_date, 10);
      lua_setfield(L, -2, "originationdate");
      lua_pushlstring(L, info.origination_time, 8);
      lua_setfield(L, -2, "originationtime");
      lua_pushnumber(L, info.time_reference_low);
      lua_setfield(L, -2, "timereferencelow");
      lua_pushnumber(L, info.time_reference_high);
      lua_setfield(L, -2, "timereferencehigh");
      lua_pushnumber(L, info.version);
      lua_setfield(L, -2, "version");
      lua_pushlstring(L, info.umid, 64);
      lua_setfield(L, -2, "umid");
      lua_pushlstring(L, info.reserved, 190);
      lua_setfield(L, -2, "reserved");
      lua_pushnumber(L, info.coding_history_size);
      lua_setfield(L, -2, "codinghistorysize");
      lua_pushlstring(L, info.coding_history, 256);
      lua_setfield(L, -2, "codinghistory");
      return 1;
    }
    else
      return 0;
  }
  else
  {
    sndfile_broadcast_getstring(L, 2, "description", info.description, 256);
    sndfile_broadcast_getstring(L, 2, "originator", info.originator, 32);
    sndfile_broadcast_getstring(L, 2, "originatorreference", info.originator_reference, 32);
    sndfile_broadcast_getstring(L, 2, "originationdate", info.origination_date, 10);
    sndfile_broadcast_getstring(L, 2, "originationtime", info.origination_time, 8);
    info.time_reference_low = sndfile_broadcast_getnumber(L, 2, "timereferencelow");
    info.time_reference_high = sndfile_broadcast_getnumber(L, 2, "timereferencehigh");
    info.version = sndfile_broadcast_getnumber(L, 2, "version");
    sndfile_broadcast_getstring(L, 2, "umid", info.umid, 64);
    sndfile_broadcast_getstring(L, 2, "reserved", info.reserved, 190);
    info.coding_history_size = sndfile_broadcast_getnumber(L, 2, "codinghistorysize");
    info.coding_history_size = sndfile_broadcast_getnumber(L, 2, "codinghistorysize");
    sndfile_broadcast_getstring(L, 2, "codinghistory", info.coding_history, 256);
    lua_pushboolean(L, sf_command(snd->file, SFC_SET_BROADCAST_INFO, &info, sizeof(info)));
  }

  return 0;
}

static int sndfile_loopinfo(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  SF_LOOP_INFO info;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else
    luaL_error(L, "expected arguments: SndFile");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  if(sf_command(snd->file, SFC_GET_LOOP_INFO, &info, sizeof(info)))
  {
    lua_newtable(L);
    lua_pushnumber(L, info.time_sig_num);
    lua_setfield(L, -2, "timesignum");
    lua_pushnumber(L, info.time_sig_den);
    lua_setfield(L, -2, "timesigden");
    lua_pushnumber(L, info.loop_mode);
    lua_setfield(L, -2, "loopmode");
    lua_pushnumber(L, info.num_beats);
    lua_setfield(L, -2, "numbeats");
    lua_pushnumber(L, info.bpm);
    lua_setfield(L, -2, "bpm");
    lua_pushnumber(L, info.root_key);
    lua_setfield(L, -2, "rootkey");
    return 1;
  }
  else
    return 0;

  return 0;
}

static int sndfile_instrument(lua_State *L)
{
  int narg = lua_gettop(L);
  SndFile *snd = NULL;
  SF_INSTRUMENT info;

  if((narg == 1) && luaT_isudata(L, 1, "sndfile.SndFile"))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else if((narg == 2) && luaT_isudata(L, 1, "sndfile.SndFile") && lua_istable(L, 2))
    snd = luaT_toudata(L, 1, "sndfile.SndFile");
  else
    luaL_error(L, "expected arguments: SndFile [table]");

  if(!snd->file)
    luaL_error(L, "trying to operate on a closed file");

  if(narg == 1)
  {
    if(sf_command(snd->file, SFC_GET_INSTRUMENT, &info, sizeof(info)))
    {
      int i;

      lua_newtable(L);
      lua_pushnumber(L, info.gain);
      lua_setfield(L, -2, "gain");
      lua_pushnumber(L, info.basenote);
      lua_setfield(L, -2, "basenote");
      lua_pushnumber(L, info.detune);
      lua_setfield(L, -2, "detune");
      lua_pushnumber(L, info.velocity_lo);
      lua_setfield(L, -2, "velocitylo");
      lua_pushnumber(L, info.velocity_hi);
      lua_setfield(L, -2, "velocityhi");

      lua_newtable(L); /* loops table */
      for(i = 0; i < info.loop_count; i++)
      {
        lua_newtable(L);
        switch(info.loops[i].mode)
        {
          case(SF_LOOP_NONE):
            lua_pushstring(L, "none");
            break;
          case(SF_LOOP_FORWARD):
            lua_pushstring(L, "forward");
            break;
          case(SF_LOOP_BACKWARD):
            lua_pushstring(L, "backward");
            break;
          case(SF_LOOP_ALTERNATING):
            lua_pushstring(L, "alternating");
            break;
          default:
            lua_pushstring(L, "unknown");
        }
        lua_setfield(L, -2, "mode");
        lua_pushnumber(L, info.loops[i].start);
        lua_setfield(L, -2, "start");
        lua_pushnumber(L, info.loops[i].end);
        lua_setfield(L, -2, "end");
        lua_pushnumber(L, info.loops[i].count);
        lua_setfield(L, -2, "count");

        lua_pushnumber(L, i);
        lua_rawset(L, -3);
      }
      lua_setfield(L, -2, "loops");
      return 1;
    }
    else
      return 0;
  }
  else
  {
    luaL_error(L, "not implemented yet");
  }

  return 0;
}

static const struct luaL_Reg sndfile_SndFile__ [] = {
  {"error", sndfile_error},
  {"info", sndfile_info},
  {"string", sndfile_string},
  {"close", sndfile_close},
  {"seek", sndfile_seek},
  {"sync", sndfile_sync},
  {"readShort", sndfile_readShort},
  {"readInt", sndfile_readInt},
  {"readFloat", sndfile_readFloat},
  {"readDouble", sndfile_readDouble},
  {"writeShort", sndfile_writeShort},
  {"writeInt", sndfile_writeInt},
  {"writeFloat", sndfile_writeFloat},
  {"writeDouble", sndfile_writeDouble},
  {"loginfo", sndfile_loginfo},
  {"signalmax", sndfile_signalmax},
  {"normsignalmax", sndfile_normsignalmax},
  {"maxallchannels", sndfile_maxallchannels},
  {"normmaxallchannels", sndfile_normmaxallchannels},
  {"headersignalmax", sndfile_headersignalmax},
  {"headermaxallchannels", sndfile_headermaxallchannels},
  {"normfloat", sndfile_normfloat},
  {"normdouble", sndfile_normdouble},
  {"scalefloatintread", sndfile_scalefloatintread},
  {"scaleintfloatwrite", sndfile_scaleintfloatwrite},
  {"addpeakchunk", sndfile_addpeakchunk},
  {"updateheadernow", sndfile_updateheadernow},
  {"updateheaderauto", sndfile_updateheaderauto},
  {"filetruncate", sndfile_filetruncate},
  {"rawstartoffset", sndfile_rawstartoffset},
  {"clipping", sndfile_clipping},
  {"embedfileinfo", sndfile_embedfileinfo},
  {"wavexambisonic", sndfile_wavexambisonic},
  {"vbrencodingquality", sndfile_vbrencodingquality},
  {"broadcastinfo", sndfile_broadcastinfo},
  {"loopinfo", sndfile_loopinfo},
  {"instrument", sndfile_instrument},
  {NULL, NULL}
};

static const struct luaL_Reg sndfile_global__ [] = {
  {"formatlist", sndfile_formatlist},
  {"subformatlist", sndfile_subformatlist},
  {"endianlist", sndfile_endianlist},
  {"libversion", sndfile_libversion},
  {NULL, NULL}
};

#if LUA_VERSION_NUM == 501
static void luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup)
{
  luaL_checkstack(L, nup+1, "too many upvalues");
  for (; l->name != NULL; l++) {  /* fill the table with given functions */
    int i;
    lua_pushstring(L, l->name);
    for (i = 0; i < nup; i++)  /* copy upvalues to the top */
      lua_pushvalue(L, -(nup+1));
    lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
    lua_settable(L, -(nup + 3));
  }
  lua_pop(L, nup);  /* remove upvalues */
}
#endif

DLL_EXPORT int luaopen_libsndfile(lua_State *L)
{
  lua_newtable(L);
  luaL_setfuncs(L, sndfile_global__, 0);
  lua_pushvalue(L, -1);
  lua_setglobal(L, "sndfile");

  luaT_newmetatable(L, "sndfile.SndFile", NULL, sndfile_new, sndfile_free, NULL);
  luaL_setfuncs(L, sndfile_SndFile__, 0);
  lua_pop(L, 1);

  return 1;
}
