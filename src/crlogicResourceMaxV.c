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

/* MaxV memory structure */
typedef volatile struct {
	int motEnc[16]; /* combined array for 8 motor and 8 encoder positions*/
	int dummy[4]; /* not used */
	int posReqMailBox; /* position request mail box */
	int smot[8]; /* snapshot 8 motor positions */
	int senc[8]; /* snapshot 8 encoder positions */
} maxVmemory;


static void initializeMaxV (resource *self, rstatus status, char* emessage) {

	maxVmemory *memory;
	rstatus retStatus;
	usint retValue;

	char * baseAddress;
	baseAddress = (char*) self->baseAddress;

	printf("Initialize MaxV memory access [base address] %p [channel number] %d\n", baseAddress, self->channel);

	/* Retrieve pointer to memory */
	retStatus = sysBusToLocalAdrs (VME_AM_USR_SHORT_IO, baseAddress, (char **) &memory);
	if(retStatus != OK) {
		printf ("Cannot retrieve pointer to motor card memory structure\n");
		printf ("Check that module base address = %p is correct\n", baseAddress);
		sprintf (emessage, "Cannot access motor memory");
		status = ERROR;
		return;
	}

	/* Check if memory is readable */
	retStatus = vxMemProbe ((char *) memory, VX_READ, 2, (char *) &retValue); /* TODO check if 2 can be replaced by sizeof(usint)*/
	if(retStatus != OK){
		printf ("Cannot read memory\n");
		sprintf (emessage,"Cannot read memory");
		status = ERROR;
		return;
	}

	self->pointer = (void *) memory;
	status = OK;
}

static void readMaxV(resource *self, double *message){
	int value = ((maxVmemory *)self->pointer)->motEnc[self->channel];
	*message = (double) value;
}

static void initResourceMaxV (resource *self) {
    self->initialize = (void *) &initializeMaxV;
    self->read = (void *) &readMaxV;
}


/**
 * Add timestamp resource to resource list
 * key	-	Key of the resource
 */
rstatus crlogicAddMaxVMotorResource(char* key, int baseAddress, int channel){
	resourceListItem* newNode;

	printf("Add MaxV resource\n");

	/* Append at the beginning */
	newNode = calloc (1, sizeof(resourceListItem) );

	initResourceMaxV( &(newNode->res));
	sprintf(newNode->res.key,"%.63s", key);
	newNode->res.baseAddress = baseAddress;
	newNode->res.channel = channel;

	newNode->next = resourceList;
	resourceList = newNode;

	return(OK);
}

rstatus crlogicAddMaxVEncoderResource(char* key, int baseAddress, int channel){
	resourceListItem* newNode;

	printf("Add MaxV encoder resource\n");

	/* Append at the beginning */
	newNode = calloc (1, sizeof(resourceListItem) );

	initResourceMaxV( &(newNode->res));
	sprintf(newNode->res.key,"%.63s", key);
	newNode->res.baseAddress = baseAddress;
	newNode->res.channel = (channel+8); /* Encoder starts 8 elements above the motors */

	newNode->next = resourceList;
	resourceList = newNode;

	return(OK);
}
