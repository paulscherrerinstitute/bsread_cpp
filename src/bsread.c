#include <string.h>
#include <zmq.h>
#include <epicsThread.h>
#include <epicsExport.h>

#include "bsread.h"

static void *zmqCtx;
static void *zmqSock;

int bsreadSend() {
	int hwm = 100;
	printf("open writer");
	zmqCtx = zmq_ctx_new();
	zmqSock = zmq_socket(zmqCtx, ZMQ_SUB);
	zmq_setsockopt(zmqSock, ZMQ_SNDHWM, &hwm, sizeof(hwm));
	zmq_connect (zmqSock, "inproc://bsread");
	zmq_setsockopt (zmqSock, ZMQ_SUBSCRIBE, "", 0);

	while(1){
		zmq_msg_t msg;
		zmq_msg_init (&msg);

		/* Block until a message is available to be received from socket */
		zmq_msg_recv (&msg, zmqSock, 0);

		/* Release message */
		zmq_msg_close (&msg);
	}

	zmq_close(zmqSock);
	zmq_ctx_destroy(zmqCtx);

	return (0);
}

int bsreadSendInit(){
	epicsThreadCreate(
			"bsreadSend",
			epicsThreadStackSizeClass.epicsThreadPriorityMedium ,
			epicsThreadStackMedium,
			(EPICSTHREADFUNC) bsreadSend,
			NULL);
}

epicsRegisterFunction(bsreadSendInit);

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

