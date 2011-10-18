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

/* DCR508 memory structure */
typedef volatile struct {
	uint discLo0; /* Lo discriminator for Cntr 0 */
	uint discHi0; /* Hi discriminator for Cntr 0 */
	uint discLo1; /* Lo discriminator for Cntr 1 */
	uint discHi1; /* Hi discriminator for Cntr 1 */
	uint discLo2; /* Lo discriminator for Cntr 2 */
	uint discHi2; /* Hi discriminator for Cntr 2 */
	uint discLo3; /* Lo discriminator for Cntr 3 */
	uint discHi3; /* Hi discriminator for Cntr 3 */
	uint discLo4; /* Lo discriminator for Cntr 4 */
	uint discHi4; /* Hi discriminator for Cntr 4 */
	uint fill0[6]; /* Filler */
	uint channel[6]; /* Counter 1-6 (6=opto) */
	uint fill1[10]; /* Filler */
	uint csr; /* Control and Status Register */
} dcr508memory;


static void initializeDCR508 (resource *self, rstatus status, char* emessage) {

	dcr508memory *memory;
	rstatus retStatus;
	usint retValue;

	char * baseAddress;
	baseAddress = (char*) self->baseAddress;

	printf("Initialize DCR508 memory access [base address] %p [channel number] %d\n", baseAddress, self->channel);

	/* Retrieve pointer to memory */
	retStatus = sysBusToLocalAdrs( VME_AM_USR_SHORT_IO, baseAddress, (char **) &memory);
	if(retStatus != OK) {
		printf ("Cannot retrieve pointer to motor card memory structure\n");
		printf ("Check that module base address = %p is correct\n", baseAddress);
		sprintf (emessage, "Cannot access motor memory");
		status = ERROR;
		return;
	}

	/* Check if memory is readable */
	retStatus = vxMemProbe ((char *) &memory->channel, VX_READ, 4, (char *) &retValue); /* TODO check if 4 can be replaced by sizeof(uint)*/
	if(retStatus != OK){
		printf ("Cannot read memory\n");
		sprintf (emessage,"Cannot read memory");
		status = ERROR;
		return;
	}

	self->pointer = (void *) memory;
	status = OK;
}

static void readDCR508(resource *self, double *message){
	uint value = ((dcr508memory *)self->pointer)->channel[self->channel];
	*message = (double) value;
}

static void initResourceDCR508 (resource *self) {
    self->initialize = (void *) &initializeDCR508;
    self->read = (void *) &readDCR508;
}


/**
 * Add timestamp resource to resource list
 * key	-	Key of the resource
 */
rstatus crlogicAddDCR508Resource(char* key, int baseAddress, int channel){
	resourceListItem* newNode;

	printf("Add DCR508 resource\n");

	/* Append at the beginning */
	newNode = calloc (1, sizeof(resourceListItem) );

	initResourceDCR508( &(newNode->res));
	sprintf(newNode->res.key,"%.63s", key);
	newNode->res.baseAddress = baseAddress;
	newNode->res.channel = channel;

	newNode->next = resourceList;
	resourceList = newNode;

	return(OK);
}

