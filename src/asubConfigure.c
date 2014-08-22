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


	int i;
	char* wfStr;
	struct buffer {
		char  value[maxNumberResources][dbValueSize(DBR_STRING)];
	} buffer;

	/** test reading from a string waveform **/
	wfStr = (char*) prec->b;

	for (i=0; i<prec->nob; i++) {
	   	printf("inpb[%d] = %s\n", i, wfStr+40*i);
	   	bsreadAddResource(sprintf("%s", wfStr+40*i));
	}
	return 0;
}

epicsRegisterFunction(bsreadConfigureInit);
epicsRegisterFunction(bsreadConfigure);
