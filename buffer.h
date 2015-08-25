#include <sndfile.h>
#include "TH.h"

struct byte_storage_buffer_t {
  THByteStorage *storage;
  long position;
};

SF_VIRTUAL_IO byte_storage_io;
