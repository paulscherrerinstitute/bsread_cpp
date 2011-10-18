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


static void initializeChannel (resource *self, rstatus status, char* emessage) {

	pvaddress channel_pvAddr;

	printf("Initialize DataWriter\n");

	dbNameToAddr(self->key, &channel_pvAddr);

	self->pointer = (void *) &channel_pvAddr;
	status = OK;
}

static void readChannel(resource *self, double *message){
	dbGetField (((pvaddress *)self->pointer), DBR_DOUBLE, message, NULL, NULL, NULL);
}

static void initResourceChannel (resource *self) {
    self->initialize = (void *) &initializeChannel;
    self->read = (void *) &readChannel;
}


/**
 * Add timestamp resource to resource list
 * key	-	Key of the resource
 */
rstatus crlogicAddChannelResource(char* key){
	resourceListItem* newNode;

	printf("Add Channel resource\n");

	/* Append at the beginning */
	newNode = calloc (1, sizeof(resourceListItem) );

	initResourceChannel( &(newNode->res));
	sprintf(newNode->res.key,"%.63s", key);

	newNode->next = resourceList;
	resourceList = newNode;

	return(OK);
}

