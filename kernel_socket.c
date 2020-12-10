#include "kernel_cc.h"
#include "tinyos.h"
#include "kernel_socket.h"


// File Operations
static file_ops socketOperations = {
	.Open = NULL,
	.Read = NULL /*socket_read*/,
	.Write = NULL /*socket_write*/,
	.Close = NULL /*socket_close*/
};

// Allocation, initialize and return a new socket control block
SCB* new_socket(port_t p){
	// Allocation
	SCB* socket=(SCB*)xmalloc(sizeof(SCB));
	// Initialization
	socket->refcount = 0;
	socket->port = p;
	socket->type = SOCKET_UNBOUND;
	return socket;
}



// FCB* get_fcb(Fid_t fid)
// {
//   if(fid < 0 || fid >= MAX_FILEID) return NULL;

//   return CURPROC->FIDT[fid];
// }
/** Returns the SCB of the CURPROC
	With the use of get_fcb() 
*/ 
SCB* get_scb(Fid_t sock){
	FCB* socket_fcb = get_fcb(sock);

	if(socket_fcb == NULL)
		return NULL;
	else
		return (SCB*)socket_fcb->streamobj;
}

/**
	@brief Return a new socket bound on a port.

	This function returns a file descriptor for a new
	socket object.	If the @c port argument is NOPORT, then the 
	socket will not be bound to a port. Else, the socket
	will be bound to the specified port. 

	@param port The port the new socket will be bound to
	@returns A file id for the new socket, or NOFILE on error. Possible
		reasons for error:
		- the port is iilegal
		- the available file ids for the process are exhausted
*/
Fid_t sys_Socket(port_t port)
{	
	// In case of no valid Port_t
	if(port < 0 || port > MAX_PORT){
		return NOFILE;
	}

	Fid_t fid;
	FCB* fcb;

	// ?Check if valid fid and 
	if(!FCB_reserve(1, &fid, &fcb))
		return NOFILE;

	SCB* socket = new_socket(port);
	// Initialization
	socket->fcb = fcb;
	socket->fcb->streamobj = socket;
	socket->fcb->streamfunc = NULL;

	// Fid through FCB_reserve 
	return fid;
}

/**
	@brief Initialize a socket as a listening socket.

	A listening socket is one which can be passed as an argument to
	@c Accept. Once a socket becomes a listening socket, it is not
	possible to call any other functions on it except @c Accept, @Close
	and @c Dup2().

	The socket must be bound to a port, as a result of calling @c Socket.
	On each port there must be a unique listening socket (although any number
	of non-listening sockets are allowed).

	@param sock the socket to initialize as a listening socket
	@returns 0 on success, -1 on error. Possible reasons for error:
		- the file id is not legal
		- the socket is not bound to a port
		- the port bound to the socket is occupied by another listener
		- the socket has already been initialized
	@see Socket
 */
int sys_Listen(Fid_t sock)
{
	// Get the CURPROCS SCB
	SCB* socket = get_scb(sock);

	// Checks
	if(socket == NULL || socket->type != SOCKET_UNBOUND || socket->port == NOPORT || PORT_MAP[socket->port] != NULL)
		return -1;
	
	// Mark the socket as Listener
	socket->type = SOCKET_LISTENER;
	// Link PORT_MAP with SCB 
	PORT_MAP[socket->port] = socket;
	// Initialize the Cond_Var od socket
	socket->s_type.listen_s->req_available = COND_INIT;
	// Initialize the header of the listeners queue
	rlnode_init(&socket->s_type.listen_s->queue, NULL);

	return 0;
}



/**
	@brief Wait for a connection.

	With a listening socket as its sole argument, this call will block waiting
	for a single @c Connect() request on the socket's port. 
	one which can be passed as an argument to @c Accept. 

	It is possible (and desirable) to re-use the listening socket in multiple successive
	calls to Accept. This is a typical pattern: a thread blocks at Accept in a tight
	loop, where each iteration creates new a connection, 
	and then some thread takes over the connection for communication with the client.

	@param sock the socket to initialize as a listening socket
	@returns a new socket file id on success, @c NOFILE on error. Possible reasons 
	    for error:
		- the file id is not legal
		- the file id is not initialized by @c Listen()
		- the available file ids for the process are exhausted
		- while waiting, the listening socket @c lsock was closed

	@see Connect
	@see Listen
 */
Fid_t sys_Accept(Fid_t lsock)
{

	SCB* listener = get_scb(lsock);

	// Checks
	if(listener == NULL || listener->type != SOCKET_LISTENER)
		return NOFILE;

	listener->refcount++;

	// While queue of request is empty and listener port is NOPORT then wait 
	while (rlist_len(&listener->s_type.listen_s->queue) == 0 && listener->port != NOPORT){ 
		kernel_wait(&listener->s_type.listen_s->req_available, SCHED_PIPE);
	}

	// If listener port is NOPORT the fail
	if (listener->port == NOPORT)
		return NOFILE;

	// Take the 1st waiting connection request
	rlnode* request = rlist_pop_front(&listener->s_type.listen_s->queue);
	// The connection(&socket) that waited to connect
	c_req* c_req = request->c_req;

	// The fid that point to the newly created unbound server socket
	Fid_t serv_fid = sys_Socket(listener->port);
	
	// Server SCB
	SCB* server_scb = get_scb(serv_fid);
	// &&&&&&&&&&&&&&&&&&&&&&
	// In case of bad constraction of socket
	if (server_scb == NULL)
		return NOFILE;

	// Set the type of server
	server_scb->type = SOCKET_PEER;
	// Requester Socket
	SCB* client_scb = c_req->peer;
	// Set the type of client
	client_scb->type = SOCKET_PEER;

	// TODO: Check the union tyoe for client. Can we rechange it?
    // connect peers 
	server_scb->s_type.peer_s->peer = client_scb;
	client_scb->s_type.peer_s->peer = server_scb;

	// Initiallise 
	Pipe_CB* p1 = pipe_init();
	Pipe_CB* p2 = pipe_init();

	// Connect pipe1 
	p1->writer = server_scb->fcb;
	p1->reader = client_scb->fcb;
	
	// Connect pipe2
	p2->writer = client_scb->fcb;
	p2->reader = server_scb->fcb;

	// Set teh functions of Server<->Client sockets
	server_scb->fcb->streamfunc = &socketOperations;
	client_scb->fcb->streamfunc = &socketOperations;

	// Connect server socket with pipes
	server_scb->s_type.peer_s->write_pipe = p1;
	server_scb->s_type.peer_s->read_pipe = p2;
	// Connect client socket with pipes
	client_scb->s_type.peer_s->read_pipe = p1;
	client_scb->s_type.peer_s->write_pipe = p2;

	// Requester admitted
	c_req->admitted=1;
	// One request passed
	listener->refcount--;

	// TODO: where to set
	// if (listener->refcount == 0) //REVISIT THIS
	// 	free(listener);

	kernel_signal(&c_req->connected_cv);
	return serv_fid;
}

/**
	@brief Create a connection to a listener at a specific port.

	Given a socket @c sock and @c port, this call will attempt to establish
	a connection to a listening socket on that port. If sucessful, the
	@c sock stream is connected to the new stream created by the listener.

	The two connected sockets communicate by virtue of two pipes of opposite directions, 
	but with one file descriptor servicing both pipes at each end.

	The connect call will block for approximately the specified amount of time.
	The resolution of this timeout is implementation specific, but should be
	in the order of 100's of msec. Therefore, a timeout of at least 500 msec is
	reasonable. If a negative timeout is given, it means, "infinite timeout".

	@params sock the socket to connect to the other end
	@params port the port on which to seek a listening socket
	@params timeout the approximate amount of time to wait for a
	        connection.
	@returns 0 on success and -1 on error. Possible reasons for error:
	   - the file id @c sock is not legal (i.e., an unconnected, non-listening socket)
	   - the given port is illegal.
	   - the port does not have a listening socket bound to it by @c Listen.
	   - the timeout has expired without a successful connection.
*/
int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{	
	// Get the pointer to our SCB struct from the Fid_t argument
	SCB* socket = get_scb(sock);  

	if(socket == NULL || socket->type != SOCKET_LISTENER || port > MAX_PORT || port < 1 || PORT_MAP[port] != NULL){
		return -1;
	}
	// Increase refcount of SCB
	socket->refcount++;
	
	//Build the connection request c_req
	c_req* request = xmalloc(sizeof(c_req));
	request->admitted = 0;
	// Point to the parent peer 
	request->peer = socket; 
	request->connected_cv = COND_INIT;

	//Get the listener SCB from the specified(arg) port 
	SCB* listener_scb = PORT_MAP[port];	
	//initialise the rlnode of the request to point to itself(intrusive lists u know)
	rlnode_init(&request->queue_node, &request);
	// Add the request to the listener's list
	rlist_push_back(&listener_scb->s_type.listen_s->queue, &request->queue_node);
	// Signal the listener that there is a request to handle!
	kernel_signal(&listener_scb->s_type.listen_s->req_available); 

	// How it works
	// The result of the kernel_timedwait() will be stored here
	timeout_t timeout_result;  

	while(request->admitted == 0){
		//IMPORTANT: the connect() function waits in the condvar of the request(struct)
		//When the accept() accepts the request, it has to signal that condition variable
		//to begin the data exchange.
		timeout_result = kernel_timedwait(&request->connected_cv, SCHED_PIPE, timeout);
		if (timeout_result==0){
			// the above condition satisfied means (not sure) that the kernel wait was timed out
			// so we remove the request from the listener scb list and free its space
			rlist_remove(&request->queue_node);
			free(request);
			return -1;
		}
	}

	//decrease socket refcount
	socket->refcount--;
	// TODO: Need to check if refcount == 0
	return 0;
}



/**
   @brief Shut down one direction of socket communication.

   With a socket which is connected to another socket, this call will 
   shut down one or the other direction of communication. The shut down
   of a direction has implications similar to those of a pipe's end shutdown.
   More specifically, assume that this end is socket A, connected to socket
   B at the other end. Then,

   - if `ShutDown(A, SHUTDOWN_READ)` is called, any attempt to call `Write(B,...)`
     will fail with a code of -1.
   - if ShutDown(A, SHUTDOWN_WRITE)` is called, any attempt to call `Read(B,...)`
     will first exhaust the buffered data and then will return 0.
   - if ShutDown(A, SHUTDOWN_BOTH)` is called, it is equivalent to shutting down
     both read and write.

   After shutdown of socket A, the corresponding operation `Read(A,...)` or `Write(A,...)`
   will return -1.

   Shutting down multiple times is not an error.
   
   @param sock the file ID of the socket to shut down.
   @param how the type of shutdown requested
   @returns 0 on success and -1 on error. Possible reasons for error:
       - the file id @c sock is not legal (a connected socket stream).
*/

// typedef enum {
//   SHUTDOWN_READ=1,    /**< Shut down the read direction. */
//   SHUTDOWN_WRITE=2,   *< Shut down the write direction. 
//   SHUTDOWN_BOTH=3     /**< Shut down both directions. */
// } shutdown_mode;

int sys_ShutDown(Fid_t sock, shutdown_mode how)
{

	SCB* socket_cb = get_scb(sock);

	if( socket_cb == NULL || socket_cb->type != SOCKET_PEER)
		return -1;

	switch(how){
		case SHUTDOWN_READ:
			return pipe_reader_close(socket_cb->s_type.peer_s->read_pipe);
		case SHUTDOWN_WRITE:
			return pipe_writer_close(socket_cb->s_type.peer_s->write_pipe);
		case SHUTDOWN_BOTH:
			if(pipe_reader_close(socket_cb->s_type.peer_s->read_pipe) == 0 && pipe_writer_close(socket_cb->s_type.peer_s->write_pipe) == 0)
				return 0;
			return -1;
		default:
			return -1;
	}
	return -1;
}
