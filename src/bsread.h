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

#ifndef Z_CONTEXT
#define Z_CONTEXT

#include <zmq.h>
void *zmqCtx;
#endif

