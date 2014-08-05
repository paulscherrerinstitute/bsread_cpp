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

#include "../crlogic.h"

#include <string.h>
#include <zmq.h>

static void *zmqCtx;
static void *zmqSock;


/**
 * Initialize data writer
 * pvPrefix		-	Prefix of the OTF records
 * emessage		-	Error message if initialization fails (max 128 characters)
 */
rstatus crlogicDataWriterOpen(char* pvPrefix, int resourceCount, resource* resourceArray, char* emessage){


	int hwm = 100;
	char *addr = "tcp://*:8080";

        printf("open writer");
	zmqCtx = zmq_ctx_new();
	zmqSock= zmq_socket (zmqCtx, ZMQ_PUSH);
	zmq_setsockopt(zmqSock,ZMQ_SNDHWM,&hwm,sizeof(hwm));
	zmq_bind(zmqSock, addr);

	return(OK);
}

/**
 * Write data to file
 * message	-	Data message to be written to file
 */
void crlogicDataWriterWrite(message* message) {

	char jsonFmt[] = "{\"htype\":\"chunk-1.0\",\"frame\":%d,\"shape\":[%d,%d],\"type\":\"%s\"}";
	char buf[256];
	int len, res;

	printf(".");
	len = sprintf(buf, jsonFmt, 1, 2, 3, "cool");

	res = zmq_send(zmqSock, buf, len, ZMQ_SNDMORE);

	len = sprintf(buf, "%.12f", message->values[0]);
	res = zmq_send(zmqSock, buf, len, 0);

}

/**
 * Close data file
 */
rstatus crlogicDataWriterClose(char* emessage){
        printf("close writer");

	zmq_close (zmqSock);
	zmq_ctx_destroy (zmqCtx);
	return(OK);
}




