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

#include <time.h>
#include <timers.h>
#include <epicsVersion.h>

extern resourceListItem *resourceList;

/* VME58E memory structure */
typedef volatile struct {
	char dummy0[1024];
	struct Motors { /* A memory block for each motor */
		/*char dummy1[4];*/
		usint pos[2]; /* Position is stored in 2 16 bit words */
		char dummy2[124];
	} motors[8]; /* The controller can have up to 8 motors */
} vme58memory;


static void initializeVME58E (resource *self, rstatus status, char* emessage) {

	vme58memory *memory;
	rstatus retStatus;
	usint retValue;

	char * baseAddress;
	baseAddress = (char*) self->baseAddress;

	printf("Initialize VME58 memory access [base address] %p [channel number] %d\n", baseAddress, self->channel);

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

static void readVME58E(resource *self, double *message){

	int value;
	uint high = ((vme58memory *)self->pointer)->motors[self->channel].pos[0];
	uint low = ((vme58memory *)self->pointer)->motors[self->channel].pos[1];

	/* Combine the 2 16bit words of the motor */
	value = ((high << 16) | (low & 0x0000ffff));
	*message = (double) value;
}

static void initResourceVME58E (resource *self) {
    self->initialize = (void *) &initializeVME58E;
    self->read = (void *) &readVME58E;
}


/**
 * Add timestamp resource to resource list
 * key	-	Key of the resource
 */
rstatus crlogicAddVME58EMotorResource(char* key, int baseAddress, int channel){
	resourceListItem* newNode;

	printf("Add VME58E motor resource\n");

	/* Append at the beginning */
	newNode = calloc (1, sizeof(resourceListItem) );

	initResourceVME58E( &(newNode->res));
	sprintf(newNode->res.key,"%.63s", key);
	newNode->res.baseAddress = baseAddress;
	newNode->res.channel = channel;

	newNode->next = resourceList;
	resourceList = newNode;

	return(OK);
}

