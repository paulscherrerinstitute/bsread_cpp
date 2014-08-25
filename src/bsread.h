#include <dbAccess.h>

typedef struct _resource {
    char key[64];
    dbAddr address;
} resource;

typedef struct _resourceListItem {
	resource res;
	struct _resourceListItem* next;
} resourceListItem;


#ifndef BSREAD_CONTEXT
#define BSREAD_CONTEXT

/* One context for interprocess communication */
#include <zmq.h>
void *zmqCtx;

/* List of resources to be read out*/
resourceListItem *resourceList;
int resourceListSize;
#endif

