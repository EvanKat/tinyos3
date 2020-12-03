#include "tinyos.h"
#include "kernel_pipe.h"


static file_ops readOperations = {
	.Open = NULL,
	.Read = pipe_read,
	.Write = NULL,
	.Close = pipe_reader_close;
};

static file_ops writeOperations = {
	.Open = NULL,
	.Read = NULL,
	.Write = pipe_write,
	.Close = pipe_writer_close
};

Pipe_CB* pipe_init() {

	Pipe_CB* new_Pipe_CB = xmalloc(sizeof(Pipe_CB)); // Space allocation of the new pipe control block
	
	new_Pipe_CB->reader = NULL;
	new_Pipe_CB->writer = NULL;

	new_Pipe_CB->w_position = 0;
	new_Pipe_CB->r_position = 0;

	new_Pipe_CB->has_space = COND_INIT;
	new_Pipe_CB->has_data = COND_INIT;

	return new_Pipe_CB;
}


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
