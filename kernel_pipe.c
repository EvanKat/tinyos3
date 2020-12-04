#include "tinyos.h"
#include "kernel_pipe.h"
#include "kernel_streams.h"

static file_ops readOperations = {
	.Open = NULL,
	.Read = pipe_read,
	.Write = NULL,
	.Close = pipe_reader_close
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

	new_Pipe_CB->wordlength = 0;

	new_Pipe_CB->has_space = COND_INIT;
	new_Pipe_CB->has_data = COND_INIT; 

	return new_Pipe_CB;
}


// a function to Initialise a pipe, and its connections with the 
// corresponding FCB, PCB(through FIDT) and file_ops struct
int sys_Pipe(pipe_t* pipe)
{
	//These to will go as arguments in the fcb_reserve
	Fid_t fid_pipe[2];  // *convention*-> first: writer, second: reader 
	FCB* pipe_FCB[2];

	//we give as arguments two pointers to arrays, and each of one will be filled
	//and we can use them afterwards
	int reserve_return_value;
	reserve_return_value = FCB_reserve(2,fid_pipe,pipe_FCB); //TODO: check if it returns error

	if(reserve_return_value==-1){
		return -1;  // in case FCB_reserve failed, return error code
	}

	//now the above arrays have the reserved FCB's pointers from the FT,
	//and the reserved fid_t pointers from the F

	//give the "Return" values to the pipe_t struct 
	pipe->read = fid_pipe[1];
	pipe->write = fid_pipe[0];

	//give values to the Pipe_CB
	//initialise the Pipe_CB
	Pipe_CB* new_pipe = pipe_init();

	//link the FCB to the pipe(returned above) through the streamobj
	pipe_FCB[0]->streamobj = new_pipe;  // FCB---Pipe_CB
	pipe_FCB[1]->streamobj = new_pipe;

	//link the FCB to the corresponding file_ops structure
	pipe_FCB[0]->streamfunc = &writeOperations;  //FCB---file_ops(----write() and close() )
	pipe_FCB[1]->streamfunc = &readOperations;   

	return 0;

}



  /**Write operation.

    Write up to 'size' bytes from 'buf' to the stream 'this'.
    If it is not possible to write any data (e.g., a buffer is full),
    the thread will block. 
    return -> number of bytes copied from buf
    return -> -1 on error

    Possible errors are:
    - There was a I/O runtime problem.
  */
int pipe_write(void* pipecb_t, const char *buf, unsigned int size){
	//cast pipecb_t to Pipe_CB
	Pipe_CB* pipe_cb = (Pipe_CB*)pipecb_t;

	//check if we can write to the buffer

	//pass 1 by 1 the buffer elements?
		//case when there is no reader(return -1)
		//case where writer closed
		//reached MAX_BUFFER_SIZE(write_pointer = 0 again)

	//broadkast to readers
	//return how many bytes where written
}


int pipe_read(void* pipecb_t, char *buf, unsigned int size){
	return -1;
}

int pipe_writer_close(void* _pipecb){
	return -1;
}

int pipe_reader_close(void* _pipecb){
	return -1;
}
