#include <stdio.h>
#include <string.h>

#include <aSubRecord.h>
#include <registryFunction.h>
#include <epicsExport.h>
#include <recSup.h>

#include "bsread.h"

static void *zmqSock;

static long bsreadReadInit(aSubRecord *prec) {
	int hwm = 100;
	char *addr = "inproc://bsread";

#ifdef DEBUG
	printf("[Read] Open internal queue\n");
#endif

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

#ifdef DEBUG
		printf("[Read] Nothing to read out");
#endif

	} else {
		int items = 0;
		double m[resourceListSize];
		resourceListItem* currentNode = resourceList;

		do {
			dbGetField(&currentNode->res.address, DBR_DOUBLE, &m[items], NULL, NULL, NULL);
			currentNode = currentNode->next;

#ifdef DEBUG
			printf("[Read] Read %s > Value > %f\n", currentNode->res.key, m[items]);
#endif

			items++;
		} while (currentNode != NULL);

		zmq_send(zmqSock, (char*) &m, sizeof(m), 0);
	}

	return 0;
}

epicsRegisterFunction(bsreadReadInit);
epicsRegisterFunction(bsreadRead);
