#include "bsread.h"

#include <string.h>
#include <zmq.h>

static void *zmqCtx;
static void *zmqSock;

int bsreadWriterOpen(char* emessage) {

	int hwm = 100;
	char *addr = "tcp://*:8080";

	printf("open writer");
	zmqCtx = zmq_ctx_new();
	zmqSock = zmq_socket(zmqCtx, ZMQ_PUSH);
	zmq_setsockopt(zmqSock, ZMQ_SNDHWM, &hwm, sizeof(hwm));
	zmq_bind(zmqSock, addr);

	return (0);
}

void bsreadWriterWrite(message* message) {
	char jsonFmt[] = "{\"htype\":\"bsread-1.0\",\"elements\":\"%d\"}";
	char buf[256];
	int len, res, i;
	double* val = message->values;

	char arr[sizeof(double) * message->length];

	/* Send header */
	len = sprintf(buf, jsonFmt, message->length);
	res = zmq_send(zmqSock, buf, len, ZMQ_SNDMORE);

	/* Send data*/
	for (i = 0; i < message->length; i++) {
		memcpy(arr + i * sizeof(double), val + i, sizeof(double));
	}
	res = zmq_send(zmqSock, arr, message->length * sizeof(double), 0);
	/*printf("%s\n",buf);*/ /*just for extreme debugging*/
}

int bsreadWriterClose(char* emessage) {
	zmq_close(zmqSock);
	zmq_ctx_destroy(zmqCtx);
	return (0);
}

