#include <stdio.h>
#include <string.h>

#include <aSubRecord.h>
#include <registryFunction.h>
#include <epicsExport.h>
#include <recSup.h>

#include "bsread.h"

/* List of resources to be read out*/
extern resourceListItem *resourceList;

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


static long bsreadConfigureInit(aSubRecord *prec) {
	resourceList = NULL;
	return 0;
}

static long bsreadConfigure(aSubRecord *prec) {

	int i;
	char* wfStr;
	struct buffer {
		char  value[maxNumberResources][dbValueSize(DBR_STRING)];
	} buffer;

	/* Reading from a string waveform */
	wfStr = (char*) prec->b;

	for (i=0; i<prec->nob; i++) {
	   	printf("Resource[%d] = %s\n", i, wfStr+40*i);
	   	bsreadAddResource(sprintf("%s", wfStr+40*i));
	}
	return 0;
}

epicsRegisterFunction(bsreadConfigureInit);
epicsRegisterFunction(bsreadConfigure);
