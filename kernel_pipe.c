#include "tinyos.h"
#include "kernel_pipe.h"
#include "kernel_cc.h"

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

// Initialize new Pipe Control Block 
Pipe_CB* pipe_init() {

	Pipe_CB* new_Pipe_CB = xmalloc(sizeof(Pipe_CB)); // Space allocation of the new pipe control block
	
	new_Pipe_CB->reader = NULL;
	new_Pipe_CB->writer = NULL;

	new_Pipe_CB->w_position = 0;
	new_Pipe_CB->r_position = 0;

	new_Pipe_CB->has_space = COND_INIT;
	new_Pipe_CB->has_data = COND_INIT;

	new_Pipe_CB->word_length = 0;

	return new_Pipe_CB;
}

// pipe_t return by value
int sys_Pipe(pipe_t* pipe)
{
	// Arguments for FCB_reserve
	Fid_t fid[2];
	FCB* fcb[2];

	/*	If reserve return 1 continue
	 	Make check and links PCB->Fidt and FCB 
	*/
	if(!FCB_reserve(2, fid, fcb)){
		return -1;
	}
	// Return read & write fid
	pipe->read = fid[0];
	pipe->write = fid[1];

	// Initialize new Pipe Control block
	Pipe_CB* new_pipe_cb = pipe_init();
	
	fcb[0]->streamobj = new_pipe_cb;
	fcb[1]->streamobj = new_pipe_cb;

	/*	?? The reader and writer of Pipe CB
		already pointed by streamobj of FCB

		We can call them by FCB[*]->streamobj->writer/reader

	*/ 	
	new_pipe_cb->reader = fcb[0];
	new_pipe_cb->writer = fcb[1];

	// Set the functions for read/write
	fcb[0]->streamfunc = &readOperations;
	fcb[1]->streamfunc = &writeOperations;
	return 0;
}

int pipe_write(void* pipecb_t, const char *buf, unsigned int size){
	Pipe_CB* pipe_CB = (Pipe_CB*)pipecb_t;
	
	if(pipe_CB==NULL || buf==NULL || size < 1 || pipe_CB->reader == NULL)
		return -1;

	int buffer_counter=0;

	while(buffer_counter < size){
		// If the previous stored byte was the last and not '\0' byte we store it.
		//if(buffer_counter == (size-1) && pipe_CB->buffer[(pipe_CB->w_position)-1] != '\0'){
		//	pipe_CB->buffer[pipe_CB->w_position] = '\0';
		//
		//} else{
		//	// Save the chars at current pos	
		//	pipe_CB->buffer[pipe_CB->w_position] = buf[buffer_counter];
		//}

		pipe_CB->BUFFER[pipe_CB->w_position] = buf[buffer_counter];
		
		// Inc w_position
		pipe_CB->w_position++;
		// Inc word length
		pipe_CB->word_length++;
		

		// If writer wrote all buffer and reader is sleeping 
		while(pipe_CB->word_length == (int)PIPE_BUFFER_SIZE)
			kernel_wait(&pipe_CB->has_space,SCHED_PIPE);

		// If buffer reached the end and need of store more data set w_position to 0 (Bounded Buffer)
		if(pipe_CB->w_position == ((int)PIPE_BUFFER_SIZE - 1) && buffer_counter < size /*&& pipe_CB->r_position!=0*/){
			pipe_CB->w_position = 0;

		}
		// Im full of data. Take them all ;)
		kernel_broadcast(&pipe_CB->has_data);
		// Inc buffer counter 
		buffer_counter++;
	}
	return buffer_counter;
}

// int pipe_write(void* pipecb_t, const char *buf, unsigned int size){
// 	Pipe_CB* pipe_CB = (Pipe_CB*)pipecb_t;
	
// 	if(pipe_CB==NULL || buf==NULL || size < 1 || pipe_CB->reader == NULL)
// 		return -1;

// 	// The stored buffers
// 	int buffer_counter;
// 	for(buffer_counter=0; buffer_counter <= size; buffer_counter++){	
		
// 		if(buffer_counter == size-1){
// 			pipe_CB->buffer[pipe_CB->w_position] = '\0';
// 		}
// 		else{
// 			// Save the chars at current pos	
// 			pipe_CB->buffer[pipe_CB->w_position] = buf[buffer_counter];
// 		}		
		
// 		// Inc w_position
// 		pipe_CB->w_position++;
// 		// Inc word length
// 		pipe_CB->word_length++;

// 		// If writer wrote all buffer and reader is sleeping 
// 		while(pipe_CB->word_length == (int)PIPE_BUFFER_SIZE)
// 			kernel_wait(&pipe_CB->has_space,SCHED_PIPE);

// 		// If buffer reached the end and need of store more data set w_position to 0 (Bounded Buffer)
// 		if(pipe_CB->w_position == ((int)PIPE_BUFFER_SIZE - 1) && buffer_counter < size /*&& pipe_CB->r_position!=0*/){
// 			pipe_CB->w_position = 0;
			
// 		}
// 		// Im full of data. Take them all ;)
// 		kernel_broadcast(&pipe_CB->has_data);
// 	}
// 	//pipe_CB->buffer[size-1] = '\0'; //EOD or 0 or NULL
// 	// Inc w_position
// 	//pipe_CB->w_position++;
// 	// Inc word length
	
// 	//pipe_CB->word_length++;
// 	return buffer_counter-1;
// }


// // TODO: WTF IM writing
// int pipe_read(void* pipecb_t, char *buf, unsigned int size){
// 	Pipe_CB* pipe_CB = (Pipe_CB*)pipecb_t;
	
// 	if(pipe_CB==NULL || buf==NULL || size<1)
// 		return -1;
	
// 	int buffer_counter;
// 	for(buffer_counter=0; buffer_counter <size; buffer_counter++){
		
// 		// No data to Read
// 		while(pipe_CB->word_length==0)
// 			kernel_wait(&pipe_CB->has_data, SCHED_PIPE);

// 		// Store the data that read
// 		buf[buffer_counter] = pipe_CB->buffer[pipe_CB->r_position];
// 		// next read position
// 		pipe_CB->r_position++;
// 		// word length is less
// 		pipe_CB->word_length--;

// 		// If buffer reached the end and need of store more data set w_position to 0 (Bounded Buffer)
// 		// posible check wordlenght
// 		if(pipe_CB->r_position == ((int)PIPE_BUFFER_SIZE - 1)/*&& pipe_CB->r_position!=0*/){
// 			pipe_CB->r_position = 0;
// 		}
// 		// END Of File
// 		if(buf[buffer_counter] == '\0'){
// 			kernel_broadcast(&pipe_CB->has_space);
// 			buffer_counter++;
// 			break;
// 		}
// 		if(pipe_CB->word_length<PIPE_BUFFER_SIZE)
// 			kernel_broadcast(&pipe_CB->has_space);	
// 	}
	
// 	return buffer_counter;
// }

int pipe_read(void* pipecb_t, char *buf, unsigned int size){
	Pipe_CB* pipe_CB = (Pipe_CB*)pipecb_t;
	
	if(pipe_CB==NULL || buf==NULL || size<1)
		return -1;
	
	int buffer_counter=0;
	while(buffer_counter < size){
		/***/
    /***/
    /***/
		// No data to Read
		while(pipe_CB->word_length==0){
			if(pipe_CB->writer == NULL)
				 return buffer_counter;
			kernel_wait(&pipe_CB->has_data, SCHED_PIPE);
		}

		// Store the data that read
		buf[buffer_counter] = pipe_CB->BUFFER[pipe_CB->r_position];
		// next read position
		pipe_CB->r_position++;
		// word length is less
		pipe_CB->word_length--;
		// If buffer reached the end and need of store more data set w_position to 0 (Bounded Buffer)
		// posible check wordlenght
		if(pipe_CB->r_position == ((int)PIPE_BUFFER_SIZE - 1)/*&& pipe_CB->r_position!=0*/){
			pipe_CB->r_position = 0;
		}
		// // END Of File
		// if(buf[buffer_counter] == '\0'){
		// 	kernel_broadcast(&pipe_CB->has_space);
		// 	break;
		// }
		// If word_length is lesser than PIPE_BUFFER_SIZE then there is space to write new data
		// If no no need of write
		if(pipe_CB->word_length<PIPE_BUFFER_SIZE)
			kernel_broadcast(&pipe_CB->has_space);	
		
		// END Of File
		if(buf[buffer_counter] == '\0'){
			// Inc buffer_counter
			buffer_counter++;
			// kernel_broadcast(&pipe_CB->has_space);
			break;
		}
		// Inc buffer_counter
		buffer_counter++;
	}
	
	return buffer_counter;
	
}



int pipe_writer_close(void* pipecb){
	Pipe_CB* pipe_CB = (Pipe_CB*)pipecb;
	
	// Cases of failure 
	// Possible no need of FCB writer check
	 if(pipe_CB == NULL || pipe_CB->writer == NULL)
		 return -1;
	
	// Set writer FCB to NULL
	pipe_CB->writer = NULL;
	// For reader to read the remaining data
	kernel_broadcast(&pipe_CB->has_data);

	// If reader fCB is NULL free pipe control block
	if (pipe_CB->reader == NULL)
		free(pipe_CB);
	return 0;
}

int pipe_reader_close(void* pipecb){
	Pipe_CB* pipe_CB = (Pipe_CB*)pipecb;

	// Cases of failure 
	// Possible no need of FCB writer check
	if(pipe_CB == NULL || pipe_CB->reader == NULL)
		return -1;

	// set reader FCB to null
	pipe_CB->reader = NULL;

	// say to others that reader is closed
	// So wake the writers
	kernel_broadcast(&pipe_CB->has_space);

	// Unalocate the Pipe Control Bock if both reader-writer are closed
	if(pipe_CB->writer == NULL)
		free(pipe_CB);

	return 0;
}
