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

static void initializeTimestamp (resource *self, rstatus status, char* emessage) {
	printf("Initialize timestamp\n");
	status = OK;
}

static void readTimestamp(resource *self, double *message){

	struct timespec timestamp;
#if EPICS_REVISION >= 14
	/* Get timestamp is IOC is running on Epics 3.14.x */
	epicsTimeStamp timest;
	epicsTimeGetCurrent(&timest);
	epicsTimeToTimespec(&timestamp, &timest);
#else
	/* Get timestamp if IOC is running on Epics 3.13.x */
	clock_gettime(CLOCK_REALTIME, &timestamp);
#endif

	/* *message = timestamp.tv_sec*1000000000+timestamp.tv_nsec;*/
	*message = (double)timestamp.tv_sec+((double)timestamp.tv_nsec/1000000000.0);
}

static void initResourceTimestamp (resource *self) {
    self->initialize = (void *) &initializeTimestamp;
    self->read = (void *) &readTimestamp;
}


/**
 * Add timestamp resource to resource list
 * key	-	Key of the resource
 */
rstatus crlogicAddTimestampResource(char* key){
	resourceListItem* newNode;

	printf("Add Timestamp resource\n");

	/* Append at the beginning */
	newNode = calloc (1, sizeof(resourceListItem) );

	initResourceTimestamp( &(newNode->res));
	sprintf(newNode->res.key,"%.63s", key);

	newNode->next = resourceList;
	resourceList = newNode;

	return(OK);
}
