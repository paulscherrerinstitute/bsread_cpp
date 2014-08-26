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
	newNode->next = NULL;

	/* Ensure that key does not cause an overflow */
	sprintf(newNode->res.key,"%.63s", key);

	/* Retrieve memory address of the channel and save address in list node */
	dbNameToAddr(key, &channel_pvAddr);
	newNode->res.address = channel_pvAddr;

	if(resourceList == NULL){
		resourceList = newNode;
		resourceListLast = newNode;	
	}
	else{
		resourceListLast->next = newNode;
		resourceListLast = newNode;	
	}
	
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

#ifdef DEBUG
			printf("[Configure] Clear: %s\n", currentNode->res.key);
#endif

			nextNode = currentNode->next;
			free(currentNode);
			currentNode = nextNode;
		} while (currentNode != NULL);
	}
	resourceList=NULL;
	resourceListLast=NULL;
	resourceListSize=0;
}

#ifdef DEBUG
void bsreadPrintResources(){
	resourceListItem* currentNode;

		if(resourceList == NULL){
		}
		else{
			currentNode = resourceList;
			printf("[Configure] Resource: %s\n", (currentNode->res.key));

			while (currentNode->next != NULL ){
				currentNode=currentNode->next;
				printf("[Configure] Resource: %s\n", (currentNode->res.key));
			}
		}
}
#endif


static long bsreadConfigureInit(aSubRecord *prec) {
	resourceList = NULL;
	resourceListLast = NULL;
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

#ifdef DEBUG
		printf("[Configure] Resource[%d] = %s\n", i, wfStr+40*i);
#endif

		sprintf(kname,"%.63s", wfStr+40*i);
	   	if(strcmp (kname, "") != 0){
	   		bsreadAddResource(kname);
	   	}
	   	else{
	   		break;
	   	}
	}
	return 0;
}

epicsRegisterFunction(bsreadConfigureInit);
epicsRegisterFunction(bsreadConfigure);
