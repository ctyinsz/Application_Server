#include	"gas.inc"
int queue_create(queue_t* msg,int maxlen)
{
//	int res,i;
	if(msg == NULL) return FALSE;
	do
	{
	  msg->buf=(void**)calloc(maxlen,sizeof(void*));
	  if(msg->buf == NULL){
	  	perror("Malloc queue_t.buf  failed\n");   
	  	break;
	  }
//	  printf("in queue_create &msg=[%x],&msg->buf=[%x]\n",msg,msg->buf);
	  msg->maxlen = maxlen;
	  msg->ridx = 0;
	  msg->widx = 0;
	  msg->count = 0;
		if(pthread_mutex_init(&msg->queue_lock,NULL) != 0
			||pthread_cond_init(&msg->queue_not_full,NULL) != 0
			||pthread_cond_init(&msg->queue_not_empty,NULL) != 0){
			perror("Init queue_t.pthread_cond_t,pthread_mutex_t  failed\n");  	
			break;
		}
		return TRUE;
	}while(1);
	queue_destroy(msg);
	return FALSE;
}

int queue_destroy(queue_t* msg)
{
	if(msg == NULL)
		return FALSE;	
	pthread_mutex_destroy(&msg->queue_lock);
	pthread_cond_destroy(&msg->queue_not_full);
	pthread_cond_destroy(&msg->queue_not_empty);
	return queue_free(msg);
}
int queue_free(queue_t* msg)
{
	if(msg == NULL)
		return FALSE;	

	free(msg->buf);	
	return TRUE;
}
/**************************
队列操作函数
***************************/
void* queue_popfront(queue_t *msg)
{
	void* ret = NULL;
	if(msg == NULL) return NULL;
		
	if(msg->count<=0)
	{
		printf("Queue is empty\n");
		return NULL;
	}
	else
	{
		msg->count--;
		if(msg->ridx >= msg->maxlen)
			msg->ridx = 0;
	}
	ret = msg->buf[msg->ridx];
	msg->buf[msg->ridx] = NULL;
	msg->ridx++;
//	printf("pop:%d,idx:%d\n",*data,msg->ridx-1);
	return ret;
}
int queue_pushend(queue_t *msg ,void* data)
{
	if(msg == NULL) return FALSE;
		
	if(msg->count >= msg->maxlen)
	{
		printf("Queue is full\n");
		return FALSE;
	}
	else
	{
		msg->count++;
		if(msg->widx >= msg->maxlen)
			msg->widx = 0;
	}
	msg->buf[msg->widx] = data;
	msg->widx++;
//	printf("push:%d,idx:%d\n",data,msg->widx-1);
	return TRUE;	
}

int queue_isnull(queue_t *msg )
{
	if(msg == NULL) return -1;
		
	if(msg->count>0)
		return 0;
	else
		return 1;
}
int queue_isfull(queue_t *msg )
{
	if(msg == NULL) return -1;
		
	if(msg->count!=msg->maxlen)
		return 0;
	else
		return 1;
}

int list_create(list_t* msg,int maxlen)
{
	if(!msg)
		return FALSE;

	msg->tail=NULL;
	msg->head=NULL;
	msg->count=0;

	if(pthread_mutex_init(&msg->list_lock,NULL) != 0
		||pthread_cond_init(&msg->list_not_empty,NULL) != 0)
	{
		pthread_mutex_destroy(&msg->list_lock);
		pthread_cond_destroy(&msg->list_not_empty);
		return FALSE;
	}

	return TRUE;
}
int list_destroy(list_t* msg)
{
	if(!msg)
		return FALSE;
	
	listnode_t* curnode = msg->tail;

	while(msg->count)
	{
		msg->tail = curnode->prev;
		free (curnode->data);
		free (curnode);
		curnode = msg->tail;
		(msg->count)--;
	}
	msg->head = NULL;
	msg->tail = NULL;
	
	pthread_mutex_destroy(&msg->list_lock);
	pthread_cond_destroy(&msg->list_not_empty);
	
	return TRUE;
}

void* list_popend(list_t *msg)
{
	if(msg == NULL)
		return NULL;
	listnode_t* theLastNode = msg->tail;
	
	switch(msg->count)
	{
		case 0:
			return NULL;
		case 1:
			msg->head = NULL;
			msg->tail = NULL;
			break;
		default:
			theLastNode->prev->next = NULL;
			msg->tail = theLastNode->prev;		
	}
	(msg->count)--;
		
	return theLastNode;	
}
int list_pushfront(list_t *msg ,listnode_t* newnode)
{
	if(msg == NULL || newnode == NULL)
		return FALSE;
		
	newnode->next = NULL;
	newnode->prev = NULL;
	listnode_t* oldFirstNode = msg->head;
	
	switch(msg->count)
	{
		case 0:
			msg->head = newnode;
			msg->tail = newnode;
			break;
		default:
			oldFirstNode->prev = newnode;
			newnode->next = oldFirstNode;
			msg->head = newnode;
	}
	(msg->count)++;
	return TRUE;
}
int list_isnull(list_t *msg )
{
	if(msg == NULL) return -1;
		
	if(msg->count>0)
		return 0;
	else
		return 1;
}

listnode_t* listnode_create(void* data)
{
	if(data == NULL)
		return NULL;
		
	listnode_t* newnode = (listnode_t*)malloc(sizeof(listnode_t));
	return newnode;
}


