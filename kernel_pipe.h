#include "tinyos.h"
#include "kernel_streams.h"

/**
  @brief Pipe Control Block.

  This structure holds all information pertaining to a Pipe.
 */
typedef struct pipe_control_block {
    FCB *reader, *writer;
} Pipe_cb;

int sys_Pipe(pipe_t* pipe);

int pipe_write(void* pipecb_t, const char *buf, unsigned int n);

int pipe_read(void* pipecb_t, char *buf, unsigned int n);

int pipe_writer_close(void* _pipecb);

int pipe_reader_close(void* _pipecb);
