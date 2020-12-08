#include "tinyos.h"
#include "kernel_streams.h"
#include "kernel_pipe.h"


static SCB* PORT_MAP[MAX_PORT+1];

enum socket_type{
  SOCKET_UNBOUND,
  SOCKET_PEER, 
  SOCKET_LISTENER
};

typedef struct listener_socket{
  rlnode queue;
  CondVar req_available;
}listen_st;

typedef struct unbound_socket{
  rlnode unbound_socket;
}unbound_st;

typedef struct socket_control_block SCB;

typedef struct peer_socket{
  SCB* peer;
  Pipe_CB* write_pipe;
  Pipe_CB* read_pipe;
}peer_st;

typedef struct socket_control_block{
  uint refcount;  // like the one we used in previous control blocks
  FCB* fcb;  //points back to the parent FCB
  enum socket_type type;
  port_t port;
  
  union scb_action{
    listen_st* socket_s;  // might not be pointers
    unbound_st* unbound_s;
    peer_st* peer_s;
  } s_type;
    
}SCB;

struct connection_request{
  int admited; // a flag that shows if the connection request is already accepted or not
  SCB* peer;  //points to the socket that made the rekuest
  
  CondVar connected_cv;  // shows that its connected?maybe?
  rlnode queue_node;  // intrusive list node. Used in the queue rlnode in the listen SCB.
};
