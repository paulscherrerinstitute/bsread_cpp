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

/* Function provided by the Hytec 8001 driver*/
extern int Dim8001_write (unsigned short card, unsigned short signal, unsigned long mask, unsigned long val);


static void initializeHy8001Trigger (resource *self, rstatus status, char* emessage) {

	printf("Initialize Hy8001 trigger [card number] %d [channel number] %d\n", self->baseAddress, self->channel);
	status = OK;
}

static void readHy8001Trigger(resource *self, double *message){

	/* Fire the trigger (the time required for this need to be at the very minimum) */
	/* printf("Trigger\n"); */

	/*
	 * int Dim8001_write( unsigned short card, unsigned short signal, unsigned long mask, unsigned long val );
	 *
	 * signal: 0-63
	 * mask:   1
	 * val: 1 or 0
	 */
	/* Trigger pulse */
	Dim8001_write( self->baseAddress, self->channel, 1, 0);
	Dim8001_write( self->baseAddress, self->channel, 1, 1);
	Dim8001_write( self->baseAddress, self->channel, 1, 0);

	*message = 1;
}

static void initResourceHy8001Trigger (resource *self) {
    self->initialize = (void *) &initializeHy8001Trigger;
    self->read = (void *) &readHy8001Trigger;
}


/**
 * Add timestamp resource to resource list
 * key	-	Key of the resource
 */
rstatus crlogicAddHy8001TriggerResource(char* key, int card, int channel){
	resourceListItem* newNode;

	printf("Add Hy8001 trigger resource\n");

	/* Append at the beginning */
	newNode = calloc (1, sizeof(resourceListItem) );

	initResourceHy8001Trigger( &(newNode->res));
	sprintf(newNode->res.key,"%.63s", key);
	newNode->res.baseAddress = card;
	newNode->res.channel = channel;

	newNode->next = resourceList;
	resourceList = newNode;

	return(OK);
}
