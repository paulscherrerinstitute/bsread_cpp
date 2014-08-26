#include <string.h>
#include <epicsThread.h>

#include <registryFunction.h>
#include <epicsExport.h>

#include "bsread.h"

static void *zmqSock;

static void *zmqCtxExtern;
static void *zmqSockExtern;

int bsreadSend() {
	int hwm = 100;
	char *addr = "tcp://*:8080";

#ifdef DEBUG
	printf("[Sender] Connect to internal queue\n");
#endif
	
	zmqSock = zmq_socket(zmqCtx, ZMQ_PULL);
	zmq_setsockopt(zmqSock, ZMQ_RCVHWM, &hwm, sizeof(hwm));
	zmq_connect (zmqSock, "inproc://bsread");

#ifdef DEBUG
	printf("[Sender] Open sender queue");
#endif
	
	zmqCtxExtern = zmq_ctx_new();
	zmqSockExtern = zmq_socket(zmqCtxExtern, ZMQ_PUSH);
	zmq_setsockopt(zmqSockExtern, ZMQ_SNDHWM, &hwm, sizeof(hwm));
	zmq_bind(zmqSockExtern, addr);

	while(1){
		zmq_msg_t msg;
		zmq_msg_init (&msg);

		/* Block until a message is available to be received from socket */
		zmq_msg_recv (&msg, zmqSock, 0);
		
		zmq_msg_send(&msg, zmqSockExtern, 0);

		zmq_msg_close (&msg);
	}

	zmq_close(zmqSock);
	
	zmq_close(zmqSockExtern);
	zmq_ctx_destroy(zmqCtxExtern);

	return (0);
}

static void bsreadSendInit(){
	zmqCtx = zmq_ctx_new();
#ifdef DEBUG
	printf("[Sender] Create send thread\n");
#endif
	epicsThreadCreate(
			"bsreadSend",
			epicsThreadPriorityMedium,
			epicsThreadGetStackSize(epicsThreadStackSmall),
			(EPICSTHREADFUNC) bsreadSend,
			NULL);
}

epicsExportRegistrar(bsreadSendInit);
