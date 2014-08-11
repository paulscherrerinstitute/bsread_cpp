/*
 *
 * Copyright 2010 Paul Scherrer Institute. All rights reserved.
 *
 * This code is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This code is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this code. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "bsread.h"

#include <string.h>
#include <zmq.h>

static void *zmqCtx;
static void *zmqSock;

STATUS bsreadWriterOpen(char* emessage) {

	int hwm = 100;
	char *addr = "tcp://*:8080";

	printf("open writer");
	zmqCtx = zmq_ctx_new();
	zmqSock = zmq_socket(zmqCtx, ZMQ_PUSH);
	zmq_setsockopt(zmqSock, ZMQ_SNDHWM, &hwm, sizeof(hwm));
	zmq_bind(zmqSock, addr);

	return (OK);
}

void bsreadWriterWrite(message* message) {
	char jsonFmt[] = "{\"htype\":\"crlogic-1.0\",\"elements\":\"%d\"}";
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

STATUS bsreadWriterClose(char* emessage) {
	zmq_close(zmqSock);
	zmq_ctx_destroy(zmqCtx);
	return (OK);
}

