#include <stdio.h>
#include <stdlib.h>

#include <iosLib.h>

#include <vxLib.h>
#include <sysLib.h>
#include <vme.h>

#include <dbAccess.h> /* EPICS database access header */

/* Define data types */
#define uchar unsigned char
#define usint unsigned short int
#define uint unsigned int
#define sint short int
#define vusint volatile unsigned short int
#define vuint volatile unsigned int
#define vdbl volatile double
#define vunion volatile union
#define pvaddress struct dbAddr
#define rstatus STATUS

#define maxNumberResources 40

typedef struct _message {
	double values[maxNumberResources];
	int length;
} message;


typedef struct _resource {
    int (*initialize) (struct _resource *self, rstatus status, char* emessage);
    int (*read) (struct _resource *self, double *message);

    char key[64];
    int baseAddress;
    int channel;

    pvaddress pointer;

} resource;

typedef struct _resourceListItem {
	resource res;
	struct _resourceListItem* next;
} resourceListItem;

/* Declaration DataWriter functions */
rstatus crlogicDataWriterOpen(char* pvPrefix, int resourceCount, resource* resourceArray, char* emessage);
void crlogicDataWriterWrite(message* message);
rstatus crlogicDataWriterClose(char* emessage);
