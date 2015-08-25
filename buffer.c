#include "buffer.h"

static sf_count_t bt_vio_get_filelen(void *user_data)
{
  struct byte_storage_buffer_t *buffer = user_data;
  return (sf_count_t)buffer->storage->size;
}

static sf_count_t bt_vio_seek(sf_count_t offset, int whence, void *user_data)
{
  struct byte_storage_buffer_t *buffer = user_data;
  switch(whence) {
    case SEEK_CUR:
      if(buffer->position+offset < 0 || buffer->position+offset > buffer->storage->size)
        THError("out of bounds");
      buffer->position += offset;
      break;
    case SEEK_END:
      if(buffer->storage->size+offset < 0 || buffer->storage->size+offset > buffer->storage->size)
        THError("out of bounds");
      buffer->position = buffer->storage->size+offset;
      break;
    case SEEK_SET:
      if(offset < 0 || offset > buffer->storage->size)
        THError("out of bounds");
      buffer->position = offset;
      break;
    default:
        THError("invalid seek command");
  }

  return (sf_count_t)buffer->position;
}

static sf_count_t bt_vio_read(void *ptr, sf_count_t count, void *user_data)
{
  struct byte_storage_buffer_t *buffer = user_data;
  long i;
  if(count < 0 || buffer->position+count > buffer->storage->size)
    THError("out of bounds");
  memcpy(ptr, buffer->storage->data+buffer->position, count);
  buffer->position += count;
  return count;
}

static sf_count_t bt_vio_write(const void *ptr, sf_count_t count, void *user_data)
{
  struct byte_storage_buffer_t *buffer = user_data;
  long i;
  if(count < 0)
    THError("out of bounds");
  if(buffer->position+count > buffer->storage->size)
    THByteStorage_resize(buffer->storage, buffer->position+count);
  memcpy(buffer->storage->data+buffer->position, ptr, count);
  buffer->position += count;
  return count;
}

static sf_count_t bt_vio_get_tell(void *user_data)
{
  struct byte_storage_buffer_t *buffer = user_data;
  return (sf_count_t)buffer->position;
}

SF_VIRTUAL_IO byte_storage_io = {
  bt_vio_get_filelen,
  bt_vio_seek,
  bt_vio_read,
  bt_vio_write,
  bt_vio_get_tell
};
