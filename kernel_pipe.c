#include "tinyos.h"

int sys_Pipe(pipe_t* pipe)
{
	return -1;
}
int pipe_write(void* pipecb_t, const char *buf, unsigned int n){
	return -1;
}
int pipe_read(void* pipecb_t, char *buf, unsigned int n){
	return -1;
}

int pipe_writer_close(void* _pipecb){
	return -1;
}

int pipe_reader_close(void* _pipecb){
	return -1;
}
