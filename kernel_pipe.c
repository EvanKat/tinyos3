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

/**
@brief Inisialize the Pipe Control Bock.

@returns The Initialized Pipe Control Block
*/ 
Pipe_CB* pipe_init() {

    Pipe_CB* new_Pipe_CB = xmalloc(sizeof(Pipe_CB)); // Space allocation of the new pipe control block
    
    // Reader, Writer FCB's
    new_Pipe_CB->reader = NULL;
    new_Pipe_CB->writer = NULL;

    // Reader and writer possition of the buffer
    new_Pipe_CB->w_position = 0;
    new_Pipe_CB->r_position = 0;

    // Condiotion variables initialized (for waiting). 
    new_Pipe_CB->has_space = COND_INIT;
    new_Pipe_CB->has_data = COND_INIT;

    // Current word length
    new_Pipe_CB->word_length = 0;

    return new_Pipe_CB;
}

/**
    @brief @brief Construct and return a pipe id.

    Firstly acquire a number of FCBs and corresponding fids by calling FCB_reserve().
    After the succeded reservation, the fid's of the given pipe are set to the reserved 
    fid's. Then the pipe is initialized, streams are connected to the pipe control block (@c streamobj)
   
    @param pipe a pointer to a pipe_t structure for storing the file ids.
    @returns 0 on success, or -1 on error. Possible reasons for error:
        - the available file ids for the process are exhausted.
*/
int sys_Pipe(pipe_t* pipe)
{
    // Arguments for FCB_reserve
    Fid_t fid[2];
    FCB* fcb[2];

    //  If reserve return 1 then connection successfully created
    // In case of error return -1 
    if(!FCB_reserve(2, fid, fcb)){
        return -1;
    }

    // Return read & write fid
    pipe->read = fid[0];
    pipe->write = fid[1];

    // Initialize new Pipe Control block
    Pipe_CB* new_pipe_cb = pipe_init();

    // Set streams
    fcb[0]->streamobj = new_pipe_cb;
    fcb[1]->streamobj = new_pipe_cb;

    // Save the read and write FCBs
    new_pipe_cb->reader = fcb[0];
    new_pipe_cb->writer = fcb[1];

    // Set the functions for read/write
    fcb[0]->streamfunc = &readOperations;
    fcb[1]->streamfunc = &writeOperations;
    return 0;
}


/**
    @brief Function to write at a Pipe Control Block .
    
    Firstly make the following checks in order to continue:\n
    1) Pipe Control Block exists.\n
    2) Source Buffer exists.\n
    3) The given size is valid.\n
    4) The reader is activated.\n
    5) The writer is activated in order to proceed (sockets).\n

    The "size" bytes of source buffer are stored into the PipeCB buffer one by one. At every entry 
    the word length counter and writer possition increments.
    If word length = length of pipe buffer the pipe is waiting until space is available
    
    The pipe buffer is bounded.

    At the end broadcast that data are available to read. (wake up reader) 
    
    @param pipecb_t A pipe id.
    @param *buff The buffer to read from the data
    @param size The max size to write at the pipe's buffer
    @returns The number of stored bytes
*/
int pipe_write(void* pipecb_t, const char *buf, unsigned int size){
    Pipe_CB* pipe_CB = (Pipe_CB*)pipecb_t;
    
    // Checks
    if(pipe_CB==NULL || buf==NULL || size < 1 || pipe_CB->writer == NULL || pipe_CB->reader == NULL)
        return -1;

    // Initialize buffer counter  
    int buffer_counter=0;


    while(buffer_counter < size && pipe_CB->reader != NULL){
        // Store byte
        pipe_CB->buffer[pipe_CB->w_position] = buf[buffer_counter];
        
        // Inc w_position
        pipe_CB->w_position++;
        // Inc word length
        pipe_CB->word_length++;
        

        // If writer wrote all buffer and reader is sleeping 
        while(pipe_CB->word_length == (int)PIPE_BUFFER_SIZE)
            kernel_wait(&pipe_CB->has_space,SCHED_PIPE);

        // If buffer reached the end and need of store more data set w_position to 0 (Bounded Buffer)
        if(pipe_CB->w_position == ((int)PIPE_BUFFER_SIZE - 1)){
            pipe_CB->w_position = 0;
        }
        // Signal the reader 
        kernel_broadcast(&pipe_CB->has_data);
        // Inc buffer counter 
        buffer_counter++;

    }
    ////////////////////////////////////////////////////////////
    if(pipe_CB->reader == NULL && pipe_CB->word_length == size)
        return -1;
    return buffer_counter;
}


/**
    @brief Function to read from a Pipe Control Block .
    
    Firstly make the following checks in order to continue:\n
    1) Pipe Control Block exists.\n
    2) Destination Buffer exists.\n
    3) The given size is valid.\n
    4) The reader is activated in order to proceed (sockets).\n

    The "size" bytes of pipe's buffer are stored into the given buffer one by one. At every entry 
    the word length counter decrements and writer possition increments.
    If word length = 0 in the pipe buffer the pipe reader is waiting until data are available
    
    If 
    The pipe buffer is bounded.

    At the end broadcast that new space is available to write. (wake up writer) 
    
    @param pipecb_t A pipe id to read data from.
    @param *buff The buffer to store the data
    @param size The max size to read from the pipe's buffer
    @returns The number of stored bytes
*/
int pipe_read(void* pipecb_t, char *buf, unsigned int size){
    Pipe_CB* pipe_CB = (Pipe_CB*)pipecb_t;
    // Checks
    if(pipe_CB==NULL || buf==NULL || size<1 || pipe_CB->reader == NULL )
        return -1;

    // Initialize buffer counter
    int buffer_counter=0;
    
    while(buffer_counter < size){
        // No data to Read
        while(pipe_CB->word_length==0){
            if(pipe_CB->writer == NULL)
                /* In case there is no more data stored and writer is closed,
                        return how much data has already been read
                    If writer was already closed when pipe_read() was called
                                then it will return 0.
                */
                return buffer_counter;
            kernel_wait(&pipe_CB->has_data, SCHED_PIPE);
        }

        // Store the data that read
        buf[buffer_counter] = pipe_CB->buffer[pipe_CB->r_position];
        // Next read position
        pipe_CB->r_position++;
        // Word length decrements
        pipe_CB->word_length--;

        // If buffer reached the end and need of read more data set r_position to 0 (Bounded Buffer)
        if(pipe_CB->r_position == ((int)PIPE_BUFFER_SIZE - 1)){
            pipe_CB->r_position = 0;
        }
        // If word_length is lesser than PIPE_BUFFER_SIZE then there is space to write new data.
        if(pipe_CB->word_length<PIPE_BUFFER_SIZE)
            kernel_broadcast(&pipe_CB->has_space);
        // Inc buffer_counter
        buffer_counter++;
    }
    
    return buffer_counter;
    
}
/**
    @brief Close the write end of the given pipe.
    
    If the given pipe exists and and the writer is activated 
    then the writer sets to NULL.
    
    If the reader is also NULL then free Pipe Control Block

    @param _pipecb: Pipe id.
    @returns 0 on success and -1 in case of error.
*/
int pipe_writer_close(void* _pipecb){
    Pipe_CB* pipe_CB = (Pipe_CB*)_pipecb;
    
    // Cases of failure 
    if(pipe_CB == NULL || pipe_CB->writer == NULL)
        return -1;
    
    // Set writer FCB to NULL
    pipe_CB->writer = NULL;

    // Wake reader to read the remaining data
    kernel_broadcast(&pipe_CB->has_data);

    // If reader fCB is NULL too free pipe control block
    if (pipe_CB->reader == NULL)
        free(pipe_CB);
    return 0;
}

/**
    @brief Close the read end of the given pipe.
    
    If the given pipe exists and and the reader is activated 
    then the reader sets to NULL.
    
    If the writer is also NULL then free Pipe Control Block

    @param _pipecb: Pipe id.
    @returns 0 on success and -1 in case of error.
  */
int pipe_reader_close(void* _pipecb){
    Pipe_CB* pipe_CB = (Pipe_CB*)_pipecb;

    // Cases of failure 
    if(pipe_CB == NULL || pipe_CB->reader == NULL)
        return -1;

    // Set reader FCB to null
    pipe_CB->reader = NULL;

    // Say to others that reader is closed      |
    // So wake the writers                      | false because reader == Null and pipe write will  
    // kernel_broadcast(&pipe_CB->has_space);   |

    // Unalocate the Pipe Control Bock if both reader-writer are closed
    if(pipe_CB->writer == NULL)
        free(pipe_CB);

    return 0;
}
