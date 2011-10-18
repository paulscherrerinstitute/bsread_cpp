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

#include <string.h>
#include <nfsDrv.h>

static pvaddress otfDataFileName_pvAddr;
static pvaddress otfNFSServerName_pvAddr;
static pvaddress otfNFSShareName_pvAddr;
static pvaddress otfFileAppend_pvAddr;

static char nfsServerName[128];
static char nfsShareName[128];

static FILE *datafileDescriptor;
static int firstRunFlag = 1;
static char lockfile[128];

/**
 * Initialize data writer
 * pvPrefix		-	Prefix of the OTF records
 * emessage		-	Error message if initialization fails (max 128 characters)
 */
rstatus crlogicDataWriterOpen(char* pvPrefix, int resourceCount, resource* resourceArray, char* emessage){
	rstatus retStatus;

	char newNfsServerName[128];
	char newNfsShareName[128];

	char dataFileName[128];
	char filename[128];
	char *dataDirMountPoint = "/crlogicnfsdata";

	int fileDescriptor;
	int append = 0;
	int writeHeader = 1;

	printf("Initialize DataWriter\n");

	/* Execute code that only need to (/must) be executed once*/
	if(firstRunFlag){
		char pvname[128];

		printf("[first call to init function]\n");

		sprintf(pvname, "%s:NFSSE", pvPrefix);
		dbNameToAddr(pvname, &otfNFSServerName_pvAddr);

		sprintf(pvname, "%s:NFSSH", pvPrefix);
		dbNameToAddr(pvname, &otfNFSShareName_pvAddr);

		sprintf(pvname, "%s:DFNAM", pvPrefix);
		dbNameToAddr(pvname, &otfDataFileName_pvAddr);

		sprintf(pvname, "%s:FAPPE", pvPrefix);
		dbNameToAddr(pvname, &otfFileAppend_pvAddr);

		/* Get nfs server and share name */
		dbGetField (&otfNFSServerName_pvAddr, DBR_STRING, &newNfsServerName, NULL, NULL, NULL);
		dbGetField (&otfNFSShareName_pvAddr, DBR_STRING, &newNfsShareName, NULL, NULL, NULL);

		/* Mount file system */
		retStatus = nfsMount(newNfsServerName, newNfsShareName, dataDirMountPoint);
		if(retStatus != OK) {
			printf ("Cannot mount specified file system [nfs server] %s [share] %s\n", newNfsServerName, newNfsShareName);
			sprintf (emessage, "Cannot mount FS");
			return(ERROR);
		}

		strcpy (nfsServerName, newNfsServerName);
		strcpy (nfsShareName, newNfsShareName);

		firstRunFlag = 0;
	}

	/* Get nfs server and share name */
	dbGetField (&otfNFSServerName_pvAddr, DBR_STRING, &newNfsServerName, NULL, NULL, NULL);
	dbGetField (&otfNFSShareName_pvAddr, DBR_STRING, &newNfsShareName, NULL, NULL, NULL);

	/* If nfs server name or nfs share name has changed unmount old and mount new file system*/
	if(strcmp (nfsServerName, newNfsServerName) != 0 || strcmp (nfsShareName, newNfsShareName) != 0){
		printf("Mount new file system [%s:%s]\n", newNfsServerName, newNfsShareName);

		nfsUnmount(dataDirMountPoint);
		retStatus = nfsMount(newNfsServerName, newNfsShareName, dataDirMountPoint);
		if(retStatus != OK) {
			printf ("Cannot mount specified file system [nfs server] %s [share] %s\n", newNfsServerName, newNfsShareName);
			sprintf (emessage, "Cannot mount FS");

			/* Ensure that next time the mount command is issued */
			strcpy (nfsServerName, "");
			strcpy (nfsShareName, "");
			return(ERROR);
		}

		strcpy (nfsServerName, newNfsServerName);
		strcpy (nfsShareName, newNfsShareName);
	}

	dbGetField (&otfDataFileName_pvAddr, DBR_STRING, &dataFileName, NULL, NULL, NULL);
	sprintf(filename, "%s/%s", dataDirMountPoint, dataFileName);
	sprintf(lockfile, "%s/%s.lock", dataDirMountPoint, dataFileName);

	/* Check if data should be appended */
	dbGetField (&otfFileAppend_pvAddr, DBR_LONG, &append, NULL, NULL, NULL);

	/* If append, check if file already exists, if yes, write no header*/
	if(append){
		FILE *fp = fopen(filename,"r");
		if( fp ) {
		/* File exists */
		fclose(fp);
		writeHeader = 0;
		}
	}

	printf("Create lockfile: %s\n", lockfile);
	/* Create lock file */
	fileDescriptor = open(lockfile, O_WRONLY | O_CREAT, 0777);
	/* Check if creation OK */
	if (fileDescriptor == ERROR) {
		printf("Cannot create file [%s]\n", lockfile);
		sprintf(emessage, "Cannot create lockfile");
		return ERROR;
	}
	close (fileDescriptor);


	printf("Open/Create data file [%s]\n",filename);

	/* Create buffer file */
	fileDescriptor = open(filename, O_WRONLY | O_CREAT, 0777);
	/* Check if creation OK */
	if (fileDescriptor == ERROR) {
		printf("Cannot create file [%s]\n", filename);
		sprintf(emessage, "Cannot create file");
		return ERROR;
	}
	close (fileDescriptor);

	/* Check if data should be appended if file already exists */
	dbGetField (&otfFileAppend_pvAddr, DBR_LONG, &append, NULL, NULL, NULL);
	if(append){
		datafileDescriptor = fopen (filename, "a");
	}
	else {
		datafileDescriptor = fopen (filename, "w");
	}

	if (datafileDescriptor == NULL) {
		printf ("Cannot open file [%s] for writing\n", filename);
		sprintf (emessage, "Cannot open file");
		return ERROR;
	}

	/* If data is appended to a file write no header */
	if(writeHeader){
		int count = 0;
		/* Write header */
		/* TODO write real header */
		fprintf(datafileDescriptor, "# ");
		for(count=0; count<resourceCount; count++){
			fprintf (datafileDescriptor, "%s\t", resourceArray[count].key);
		}
		fprintf (datafileDescriptor, "\n");
		fflush(datafileDescriptor);
	}


	return(OK);
}

/**
 * Write data to file
 * message	-	Data message to be written to file
 */
void crlogicDataWriterWrite(message* message){
	int i=0;

	for(i=0;i < message->length; i++){
		fprintf (datafileDescriptor, "%.12f\t", message->values[i]);
	}

	fprintf (datafileDescriptor, "\n");
	fflush(datafileDescriptor);
}

/**
 * Close data file
 */
rstatus crlogicDataWriterClose(char* emessage){
	/* Close file */
	fclose(datafileDescriptor);

	printf("Remove lockfile: %s\n", lockfile);
	remove(lockfile);

	return(OK);
}
