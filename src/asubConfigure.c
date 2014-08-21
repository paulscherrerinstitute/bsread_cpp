#include <semLib.h> /* Semaphore */
#include <taskLib.h> /* Tasks */
#include <wdLib.h> /* Watchdog / Interrupt routine */

#include <stdio.h> /* Declaration of functions: 'printf', 'sprintf' */
#include <pipeDrv.h> /* Pipe */
#include <iosLib.h> /* Pipe related functions: 'open', 'write', 'read', 'close' */
#include <sysLib.h> /* Declaration of functions: 'sysClkRateGet' */

#include <string.h>

#include "bsread.h"

/* List of resources to be read out*/
extern resourceListItem *resourceList;

/* Get resource by name */
resourceListItem* bsreadGetResource(char* name){

	resourceListItem* currentNode;

	if(resourceList == NULL){
	}
	else{
		currentNode = resourceList;
		if(strcmp ( (currentNode->res.key) , name) == 0){
			return(currentNode);
		}

		while (currentNode->next != NULL ){
			currentNode=currentNode->next;
			if(strcmp (currentNode->res.key, name) == 0){
				return(currentNode);
			}
		}
	}
	return NULL;
}

/* Add resource at the beginning of the list */
resourceListItem* bsreadAddResource(char* key){
	resourceListItem* newNode;
	pvaddress channel_pvAddr;
	int rval;

	newNode = calloc (1, sizeof(resourceListItem) );

	/* Ensure that key does not cause an overflow */
	sprintf(newNode->res.key,"%.63s", key);

	/* Retrieve memory address of the channel and save address in list node */
	rval = dbNameToAddr(key, &channel_pvAddr);
	newNode->res.address = channel_pvAddr;

	newNode->next = resourceList;
	resourceList = newNode;
	items++;

	return(newNode);
}

/* Clear resources */
void bsreadClearResources() {
	resourceListItem* currentNode;
	resourceListItem* nextNode;

	if (resourceList == NULL) {
	} else {
		currentNode = resourceList;

		do {
			printf("Clear: %s\n", currentNode->res.key);
			nextNode = currentNode->next;
			free(currentNode);
			currentNode = nextNode;
		} while (currentNode != NULL);
	}
	items=0;
	resourceList=NULL;
}

void bsreadPrintResources(){
	resourceListItem* currentNode;

		if(resourceList == NULL){
		}
		else{
			currentNode = resourceList;
			printf("Resource: %s\n", (currentNode->res.key));

			while (currentNode->next != NULL ){
				currentNode=currentNode->next;
				printf("Resource: %s\n", (currentNode->res.key));
			}
		}
}
/* [END] Resource list (methods) */


static long bsreadConfigureInit(aSubRecord *prec) {
	resourceList = NULL;
	return 0;
}

/**
 * pvPrefix	-	Prefix of the PVs, e.g. MTEST-HW3:
 */
static long bsreadConfigure(aSubRecord *prec) {
	short int status;
	STATUS retStatus;
	int ticksToInterrupt;
	int ticksPerSecond;

	int numMessagesRemaining;

	int pipeId;

	/* Character array to store array message */
	char errormessage[dbValueSize(DBR_STRING)];

	/* Variable to hold message read from the pipe */
	message writeMessage;

	/* Initialize base PV variables */
	printf("Initialize PVs\n");
	bsreadInitializePVs(pvPrefix);

	/* Set status to SETUP */
	status = LOGIC_SETUP;
	dbPutField (&bsreadStatus_pvAddr, DBR_ENUM, &status, 1);

	/* Update ticks per second record with IOC clock rate*/
	ticksPerSecond = sysClkRateGet();
	dbPutField (&bsreadTicksPerSecond_pvAddr, DBR_LONG, &ticksPerSecond, 1);

	/* Create semaphore to synchronize WatchDog ISR and Read Task */
	bsreadWdTSyncSemaphore = semBCreate (SEM_Q_FIFO, SEM_EMPTY);

	/* Create data pipe */
	retStatus = pipeDevCreate (bsreadPipeName, 2048, sizeof(message));
	if (retStatus != OK) {
		printf ("Cannot create data pipe - Abort\n");
		status = LOGIC_ERROR;
		dbPutField (&bsreadStatus_pvAddr, DBR_ENUM, &status, 1);
		return;
	}

	/* Spawn Read task */
	printf("Start read task\n");
	taskSpawn ("bsreadR", readTaskPriority, VX_FP_TASK, 20000, (FUNCPTR) bsreadReadTask, 0,0,0,0,0,0,0,0,0,0);

	/* Create a WatchDog ID */
	bsreadWdId = wdCreate ();

	/* BEGIN main loop*/
	while(1){

		int r;
		long numr;
		struct buffer {
		    char  value[maxNumberResources][dbValueSize(DBR_STRING)];
		} buffer;

		/* Update status */
		dbGetField (&bsreadStatus_pvAddr, DBR_ENUM, &status, NULL, NULL, NULL);

		/* Wait until fault cause is fixed */
		while(status==LOGIC_FAULT){
			taskDelay(100);

			/* Update status */
			dbGetField (&bsreadStatus_pvAddr, DBR_ENUM, &status, NULL, NULL, NULL);

			/* Only exit this loop if status INACTIVE is set*/
			if(status==LOGIC_INACTIVE){
				break;
			}
			else{
				/* Restore fault status */
				status=LOGIC_FAULT;
				dbPutField (&bsreadStatus_pvAddr, DBR_ENUM, &status, 1);
			}
		}

		/* Status now is INACTIVE */
		status = LOGIC_INACTIVE;
		dbPutField (&bsreadStatus_pvAddr, DBR_ENUM, &status, 1);

		/* Clear message record */
		sprintf(errormessage," ");
		dbPutField (&bsreadMessage_pvAddr, DBR_STRING, errormessage, 1);


		/* Wait until status changes to ACTIVE */
		printf("Waiting ...\n");
		while(status==LOGIC_INACTIVE){
			taskDelay(100);

			/* Update status */
			dbGetField (&bsreadStatus_pvAddr, DBR_ENUM, &status, NULL, NULL, NULL);

			/* Only exit this loop if status INITIALIZE or FAULT is set*/
			if(status==LOGIC_INITIALIZE || status==LOGIC_FAULT){
				break;
			}
			else{
				/* Restore inactive status */
				status=LOGIC_INACTIVE;
				dbPutField (&bsreadStatus_pvAddr, DBR_ENUM, &status, 1);
			}
		}

		/* If status is fault continue to the beginning of the main loop */
		if(status==LOGIC_FAULT){
			continue;
		}

		/* Status now is INITIALIZE */
		status = LOGIC_INITIALIZE;
		dbPutField (&bsreadStatus_pvAddr, DBR_ENUM, &status, 1);


		/* Get resource keys */
		numr = maxNumberResources;
		dbGetField (&bsreadReadoutResources_pvAddr, DBR_STRING, &buffer, NULL, &numr, NULL);

		for(r=0;r<maxNumberResources; r++){
			if(strcmp (buffer.value[r], "") != 0){
				bsreadAddResource(buffer.value[r]);
			}
			else{
				break;
			}
		}

		/* If status is fault continue to the beginning of the main loop */
		if(status==LOGIC_FAULT){
			continue;
		}


		/* Open data pipe for reading */
		pipeId = open(bsreadPipeName, O_RDONLY, 0);
		if(pipeId == ERROR){
			printf("Open pipe (read) failed, error code: %d\n", errno);
			/* Set error message */
			sprintf(errormessage,"Cannot open pipe");
			dbPutField (&bsreadMessage_pvAddr, DBR_STRING, errormessage, 1);
			/* Set status to FAULT */
			status = LOGIC_FAULT;
			dbPutField (&bsreadStatus_pvAddr, DBR_ENUM, &status, 1);
			continue;
		}

		/* Clear all (old) messages that are in the pipe*/
		ioctl (pipeId, FIOFLUSH, 0);

		/* Open writer*/
		printf("Open writer\n");
		retStatus = bsreadWriterOpen(errormessage);
		if (retStatus != OK) {
			printf ("Cannot open writer\n");
			/* Set error message */
			dbPutField (&bsreadMessage_pvAddr, DBR_STRING, errormessage, 1);
			/* Set status to FAULT */
			status = LOGIC_FAULT;
			dbPutField (&bsreadStatus_pvAddr, DBR_ENUM, &status, 1);
			continue;
		}


		/* Get number of ticks between interrupts */
		dbGetField (&bsreadTicksBetweenInterrupts_pvAddr, DBR_LONG, &ticksToInterrupt, NULL, NULL, NULL);

		/* Start WatchDog (i.e. start data acquisition) */
		wdStart(bsreadWdId, 1, (FUNCPTR) bsreadWatchDogISR, ticksToInterrupt);

		/* Set status to ACTIVE */
		status = LOGIC_ACTIVE;
		dbPutField (&bsreadStatus_pvAddr, DBR_ENUM, &status, 1);

		printf("Active ...\n");

		while(status == LOGIC_ACTIVE){

			/* Wait until first messages are arriving */
			taskDelay(100);

			/* Read number of messages in data pipe */
			ioctl (pipeId, FIONMSGS, (int) &numMessagesRemaining);

			while (status == LOGIC_ACTIVE && numMessagesRemaining>0){
				/* Read message from data pipe*/
				read(pipeId, (char*) &writeMessage, sizeof(message));

				/* Write message */
				bsreadWriterWrite(&writeMessage);

				/* Read number of remaining messages in data pipe */
				ioctl (pipeId, FIONMSGS, (int) &numMessagesRemaining);

				/* Update status */
				dbGetField (&bsreadStatus_pvAddr, DBR_ENUM, &status, NULL, NULL, NULL);
			}

			/* Update status */
			dbGetField (&bsreadStatus_pvAddr, DBR_ENUM, &status, NULL, NULL, NULL);

			/* Only exit this loop if status INITIALIZE or FAULT is set*/
			if(status==LOGIC_STOP || status==LOGIC_FAULT){
				break;
			}
			else{
				/* Restore fault status */
				status=LOGIC_ACTIVE;
				dbPutField (&bsreadStatus_pvAddr, DBR_ENUM, &status, 1);
			}
		}

		/* Status is STOP or FAULT */

		/* Stop */
		printf("Stopping ...\n");

		/* Stop WatchDog */
		wdStart(bsreadWdId, 1, (FUNCPTR) bsreadWatchDogISR, 0);

		/* Read remaining messages in data pipe */
		ioctl (pipeId, FIONMSGS, (int) &numMessagesRemaining);
		while(numMessagesRemaining>0){
			/* Read message from data pipe*/
			read(pipeId, (char*) &writeMessage, sizeof(message));

			/* Serialize/write message */
			bsreadWriterWrite(&writeMessage);

			/* Read number of remaining messages in data pipe */
			ioctl (pipeId, FIONMSGS, (int) &numMessagesRemaining);
		}

		printf("Close writer\n");
		/* Close writer */
		retStatus = bsreadWriterClose(errormessage);
		if (retStatus != OK) {
			printf("Cannot close writer\n");
			/* Set error message */
			dbPutField (&bsreadMessage_pvAddr, DBR_STRING, errormessage, 1);
			/* Set status to FAULT */
			status = LOGIC_FAULT;
			dbPutField (&bsreadStatus_pvAddr, DBR_ENUM, &status, 1);
		}

		/* Close data pipe */
		close(pipeId);

		printf("Clear resource list\n");
		/* Clear resource list */
		bsreadClearResources();
		printf("Resources cleared\n");
	}
	/* END main loop */
}

