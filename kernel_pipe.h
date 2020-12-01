#include "tinyos.h"
#include "kernel_streams.h"

/**
  @brief Pipe Control Block.

  This structure holds all information pertaining to a Pipe.
 */
typedef struct pipe_control_block {
    FCB *reader, *writer;
} Pipe_cb;