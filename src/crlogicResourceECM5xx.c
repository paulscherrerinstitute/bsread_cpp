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

#include "crlogic.h"

#include <time.h>
#include <timers.h>
#include <epicsVersion.h>

extern resourceListItem *resourceList;

/* ECM5xx memory structure */
typedef volatile struct {
	int counter[4];
} ecm5xxmemory;


static void initializeECM5xx (resource *self, rstatus status, char* emessage) {

	ecm5xxmemory *memory;
	rstatus retStatus;
	usint retValue;

	char * baseAddress;
	baseAddress = (char*) self->baseAddress;

	printf("Initialize ECM5xx memory access [base address] %p [channel number] %d\n", baseAddress, self->channel);

	/* Retrieve pointer to memory */
	retStatus = sysBusToLocalAdrs (VME_AM_STD_USR_DATA, baseAddress, (char **) &memory);
	if(retStatus != OK) {
		printf ("Cannot retrieve pointer to motor card memory structure\n");
		printf ("Check that module base address = %p is correct\n", baseAddress);
		sprintf (emessage, "Cannot access motor memory");
		status = ERROR;
		return;
	}

	/* Check if memory is readable */
	retStatus = vxMemProbe ((char *) &memory, VX_READ, 4, (char *) &retValue);
	if(retStatus != OK){
		printf ("Cannot read encoder card memory\n");
		sprintf (emessage, "Cannot read encoder memory");
		status = ERROR;
		return;
	}

	self->pointer = (void *) memory;
	status = OK;
}

static void readECM5xx(resource *self, double *message){
	
	/* Read memory */
	int data = ((ecm5xxmemory *)self->pointer)->counter[self->channel];
	*message = (double) data;
}

static void initResourceECM5xx (resource *self) {
    self->initialize = (void *) &initializeECM5xx;
    self->read = (void *) &readECM5xx;
}


/**
 * Add timestamp resource to resource list
 * key	-	Key of the resource
 */
rstatus crlogicAddECM5xxEncoderResource(char* key, int baseAddress, int channel){
	resourceListItem* newNode;

	printf("Add ECM5xx encoder resource\n");

	/* Append at the beginning */
	newNode = calloc (1, sizeof(resourceListItem) );

	initResourceECM5xx( &(newNode->res));
	sprintf(newNode->res.key,"%.63s", key);
	newNode->res.baseAddress = baseAddress;
	newNode->res.channel = channel;

	newNode->next = resourceList;
	resourceList = newNode;

	return(OK);
}
