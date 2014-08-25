#include <stdio.h>
#include <string.h>

#include <aSubRecord.h>
#include <registryFunction.h>
#include <epicsExport.h>
#include <recSup.h>

#include "bsread.h"
#include <stdlib.h>

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
	dbAddr channel_pvAddr;

	newNode = calloc (1, sizeof(resourceListItem) );

	/* Ensure that key does not cause an overflow */
	sprintf(newNode->res.key,"%.63s", key);

	/* Retrieve memory address of the channel and save address in list node */
	dbNameToAddr(key, &channel_pvAddr);
	newNode->res.address = channel_pvAddr;

	newNode->next = resourceList;
	resourceList = newNode;
	resourceListSize++;

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
	resourceList=NULL;
	resourceListSize=0;
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
	resourceListSize = 0;
	return 0;
}

static long bsreadConfigure(aSubRecord *prec) {
	
	int i;
	char* wfStr;
	char kname[64];
	
	bsreadClearResources();

	/* Reading from a string waveform */
	wfStr = (char*) prec->a;

	for (i=0; i<prec->noa; i++) {
	   	printf("Resource[%d] = %s\n", i, wfStr+40*i);
	   	sprintf(kname,"%.63s", wfStr+40*i);
	   	if(strcmp (kname, "") != 0){
	   		bsreadAddResource(kname);
	   	}
	}
	return 0;
}

epicsRegisterFunction(bsreadConfigureInit);
epicsRegisterFunction(bsreadConfigure);
