#include <string.h>
#include <epicsThread.h>

#include <registryFunction.h>
#include <epicsExport.h>

#include "bsread.h"

/*static void *zmqCtx;*/
static void *zmqSock;

static void *zmqCtxExtern;
static void *zmqSockExtern;

int bsreadSend() {
	int hwm = 100;
	char *addr = "tcp://*:8080";
	/*int length = 1;
	char arr[sizeof(double) * length];
	int i;
	double value =0.0;*/
	
	printf("Open writer\n");
	/*zmqCtx = zmq_ctx_new();*/
	zmqSock = zmq_socket(zmqCtx, ZMQ_PULL);
	zmq_setsockopt(zmqSock, ZMQ_RCVHWM, &hwm, sizeof(hwm));
	zmq_connect (zmqSock, "inproc://bsread");
	/*zmq_setsockopt (zmqSock, ZMQ_SUBSCRIBE, "", 0);*/

	

	printf("Open sender");
	zmqCtxExtern = zmq_ctx_new();
	zmqSockExtern = zmq_socket(zmqCtxExtern, ZMQ_PUSH);
	zmq_setsockopt(zmqSockExtern, ZMQ_SNDHWM, &hwm, sizeof(hwm));
	zmq_bind(zmqSockExtern, addr);

	while(1){
		zmq_msg_t msg;
		zmq_msg_init (&msg);

		/* Block until a message is available to be received from socket */
		zmq_msg_recv (&msg, zmqSock, 0);
		printf("Message received\n");
		
		/*
		for (i = 0; i < length; i++) {
			memcpy(arr + i * sizeof(double), &value, sizeof(double));
			value=value+1;
		}
		*/
		/*zmq_send(zmqSockExtern, arr, length * sizeof(double), 0);*/
		zmq_msg_send(zmqSockExtern, &msg,0);


		/* Release message */
		zmq_msg_close (&msg);
	}

	zmq_close(zmqSock);
	
	zmq_close(zmqSockExtern);
	zmq_ctx_destroy(zmqCtxExtern);
	
	/*zmq_ctx_destroy(zmqCtx);*/

	return (0);
}

static void bsreadSendInit(){
	zmqCtx = zmq_ctx_new();
	printf("Create send thread\n");
	epicsThreadCreate(
			"bsreadSend",
			epicsThreadPriorityMedium,
			epicsThreadGetStackSize(epicsThreadStackSmall),
			(EPICSTHREADFUNC) bsreadSend,
			NULL);
}

epicsExportRegistrar(bsreadSendInit);

/*
void bsreadWriterWrite(message* message) {
	char jsonFmt[] = "{\"htype\":\"bsread-1.0\",\"elements\":\"%d\"}";
	char buf[256];
	int len, i;
	double* val = message->values;

	char arr[sizeof(double) * message->length];

	len = sprintf(buf, jsonFmt, message->length);
	zmq_send(zmqSock, buf, len, ZMQ_SNDMORE);

	for (i = 0; i < message->length; i++) {
		memcpy(arr + i * sizeof(double), val + i, sizeof(double));
	}
	zmq_send(zmqSock, arr, message->length * sizeof(double), 0);
}
*/

