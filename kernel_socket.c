
#include "tinyos.h"
#include "kernel_socket.h"

// File Operations
static file_ops socketOperations = {
	.Open = NULL,
	.Read = socket_read,
	.Write = socket_write,
	.Close = socket_close
};

// Allocation, initialize and return a new socket control block
SCB* new_socket(port_p p){
	// Allocation
	SCB* socket=xmalloc(sizeof(SCB));
	// Initialization
	socket->refcount = 0;
	socket->port = p;
	socket->type = SOCKET_UNBOUND;
	return socket;
}

/** Returns the SCB of the CURPROC
	With the use of get_fcb() 
*/ 
SCB* get_scb(Fid_t sock){
	FCB* socket_fcb = get_fcb(Fid_t sock);

	if(socket_fcb == NULL)
		return socket_fcb;
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
	// In caso of no valid Port_t
	if(port < 0 || port > MAX_PORT){
		return NOFILE;
	}

	Fid_t fid;
	FCB* fcb;

	// ?Check if valid fid
	if(!FCB_reserve(1, &fid, &fcb))
		return NOFILE;

	SCB* socket = new_socket(port);
	
	// Initialization
	socket->fcb = fcb;
	fcb->streamobj = socket;
	fcb->streamfunc = socketOperations;

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
	SCB* socket = get_scb(Fid_t sock);

	// Checks
	if(socket == NULL || socket->port == NOPORT || PORT_MAP[socket->port] != NULL || socket->type != SOCKET_UNBOUND)
		return -1;

	// Link PORT_MAP with SCB 
	PORT_MAP[socket->port] = socket;
	// Mark the socket as Listener
	socket->type = SOCKET_LISTENER;
	// Initialize the header of the listeners queue
	rlnode_init(&socket->s_type.socket_s->queue, NULL);
	// Initialize the Cond_Var od socket
	socket->s_type.socket_s->req_available = COND_INIT;

	return 0;
}


Fid_t sys_Accept(Fid_t lsock)
{
	return NOFILE;
}


int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{
	return -1;
}


int sys_ShutDown(Fid_t sock, shutdown_mode how)
{
	return -1;
}

