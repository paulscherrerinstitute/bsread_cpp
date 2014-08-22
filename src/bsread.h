#include <dbAccess.h>

#define pvaddress struct dbAddr
#define maxNumberResources 40


typedef struct _message {
	double values[maxNumberResources];
	int length;
} message;

typedef struct _resource {
    char key[64];
    pvaddress address;
} resource;

typedef struct _resourceListItem {
	resource res;
	struct _resourceListItem* next;
} resourceListItem;

/* Declaration DataWriter functions */
/* emessage		-	Error message if initialization fails (max 128 characters) */
int bsreadWriterOpen(char* emessage);
/* message		-	Message to be send/written */
void bsreadWriterWrite(message* message);
/* emessage		-	Error message if initialization fails (max 128 characters) */
int bsreadWriterClose(char* emessage);

/*
#ifndef RLISTBLA
#define RLISTBLA
extern resourceListItem *resourceList = NULL;
#endif
*/
