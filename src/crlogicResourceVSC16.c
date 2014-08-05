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

/* VSC16 memory structure */
typedef volatile struct {
	usint reset[2]; /* Reset (write only) */
	usint control[2]; /* Control */
	usint dirn[2]; /* Direction */
	uchar dummy1[4]; /* Filler */
	usint status_id[2]; /* Status ID */
	usint irq_lvl[2]; /* IRQ level */
	usint irq_mask[2]; /* IRQ mask */
	usint irq_reset[2]; /* IRQ reset (write only) */
	usint ser_num[2]; /* Serial number */
	usint type[2]; /* Module type */
	usint manuf[2]; /* Manufacturer */
	uchar dummy2[0x54]; /* Filler */
	uint channel[16]; /* Scaler channels  1 to 16 */
} vsc16memory;


static void initializeVSC16 (resource *self, rstatus status, char* emessage) {

	vsc16memory *memory;
	rstatus retStatus;
	usint retValue;

	char * baseAddress;
	baseAddress = (char*) self->baseAddress;

	printf("Initialize VSC16 memory access [base address] %p [channel number] %d\n", baseAddress, self->channel);

	/* Retrieve pointer to memory */
	retStatus = sysBusToLocalAdrs( VME_AM_EXT_USR_DATA, baseAddress, (char **) &memory);
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

static void readVSC16(resource *self, double *message){
	uint value = ((vsc16memory *)self->pointer)->channel[self->channel];
	*message = (double) value;
}

static void initResourceVSC16 (resource *self) {
    self->initialize = (void *) &initializeVSC16;
    self->read = (void *) &readVSC16;
}


/**
 * Add timestamp resource to resource list
 * key	-	Key of the resource
 */
rstatus crlogicAddVSC16Resource(char* key, int baseAddress, int channel){
	resourceListItem* newNode;

	printf("Add VSC16 resource\n");

	/* Append at the beginning */
	newNode = calloc (1, sizeof(resourceListItem) );

	initResourceVSC16( &(newNode->res));
	sprintf(newNode->res.key,"%.63s", key);
	newNode->res.baseAddress = baseAddress;
	newNode->res.channel = channel;

	newNode->next = resourceList;
	resourceList = newNode;

	return(OK);
}

