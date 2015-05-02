#ifndef _DATASTRUCT_
#define _DATASTRUCT_
#include	"gas.inc"

typedef struct _listnode
{
	void *data;
	struct _listnode *prev;	
	struct _listnode *next;	
}listnode_t;

typedef struct _list
{
	listnode_t *head;
	listnode_t *tail;
	int count;
	pthread_cond_t		list_not_empty;
	pthread_mutex_t		list_lock;
}list_t;

typedef struct _queue
{
	void** buf;
	int maxlen;
	int ridx;
	int widx;
	int count;
	pthread_cond_t		queue_not_empty;
	pthread_cond_t		queue_not_full;
	pthread_mutex_t		queue_lock;
}queue_t;

int queue_create(queue_t*,int maxlen);
int queue_destroy(queue_t* msg);
int queue_free(queue_t* msg);
void* queue_popfront(queue_t *msg);
int queue_pushend(queue_t *msg ,void* data);
int queue_isnull(queue_t *msg );
int queue_isfull(queue_t *msg );

int list_create(list_t*,int maxlen);
int list_destroy(list_t* msg);
void* list_popend(list_t *msg);
int list_pushfront(list_t *msg ,listnode_t* data);
int list_isnull(list_t *msg );

listnode_t* listnode_create(void* data);

#endif
