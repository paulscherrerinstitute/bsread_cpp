#include <stdio.h>
#include <string.h>

#include <aSubRecord.h>
#include <registryFunction.h>
#include <epicsExport.h>
#include <recSup.h>

#include "bsread.h"

/*static void *zmqCtx;*/
static void *zmqSock;

static long bsreadReadInit(aSubRecord *prec) {
	int hwm = 100;
	char *addr = "inproc://bsread";

	printf("Open internal queue\n");
	/*zmqCtx = zmq_ctx_new();*/
	zmqSock = zmq_socket(zmqCtx, ZMQ_PUSH);
	zmq_setsockopt(zmqSock, ZMQ_SNDHWM, &hwm, sizeof(hwm));
	zmq_bind(zmqSock, addr);
	return 0;
}

/**
 * TODO The 'do' block has a time constraint of 1ms. We need to check the whether this was met. otherwise we
 * have to have some kind of drop count that is also served as a channel access channel.
 */
static long bsreadRead(aSubRecord *prec) {
	if (resourceList == NULL) {
		printf("Nothing to read out");
	} else {
		int items = 0;
		double m[resourceListSize];
		resourceListItem* currentNode = resourceList;

		do {
			printf("Read %s > %d\n", currentNode->res.key, items);
			dbGetField(&currentNode->res.address, DBR_DOUBLE, &m[items], NULL, NULL, NULL);
			currentNode = currentNode->next;
			printf("Value > %f\n", m[items]);
			items++;
		} while (currentNode != NULL);

		printf("Send...\n");
		zmq_send(zmqSock, (char*) &m, sizeof(m), 0);
	}

	return 0;
}

epicsRegisterFunction(bsreadReadInit);
epicsRegisterFunction(bsreadRead);
