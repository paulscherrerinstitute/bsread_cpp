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

/* Function provided by the Hytec 8001 driver*/
extern int Hy8401_read_ai (int card, int channel, void* , unsigned long* val);


static void initializeHy8401 (resource *self, rstatus status, char* emessage) {
	
	printf("Initialize Hy8401 [card number] %d [channel number] %d\n", self->baseAddress, self->channel);
	status = OK;
}

static void readHy8401(resource *self, double *message){
	
	unsigned long val;
	Hy8401_read_ai(self->baseAddress, self->channel, NULL, &val);
	
	/*Return trigger count and increment counter */
	*message = (double) val;
}

static void initResourceHy8401 (resource *self) {
    self->initialize = (void *) &initializeHy8401;
    self->read = (void *) &readHy8401;
}


/**
 * Add timestamp resource to resource list
 * key	-	Key of the resource
 */
rstatus crlogicAddHy8401Resource(char* key, int card, int channel){
	resourceListItem* newNode;
	
	printf("Add Hy8401 resource\n");
	
	/* Append at the beginning */
	newNode = calloc (1, sizeof(resourceListItem) );
	
	initResourceHy8401( &(newNode->res));
	sprintf(newNode->res.key,"%.63s", key);
	newNode->res.baseAddress = card;
	newNode->res.channel = channel;
	
	newNode->next = resourceList;
	resourceList = newNode;
	
	return(OK);
}
