#include	"gas.inc"
Connection_t *Connection_Create(Server_t* server,int fd,int seq)
{
	Connection_t *c = (Connection_t*)calloc(1,sizeof(Connection_t));
	if(c == NULL)
		return NULL;
		
	memset(c->reqbuf,0x00,sizeof(c->reqbuf));
	memset(c->respbuf,0x00,sizeof(c->respbuf));
  c->reqlen = 0;
  c->resplen = 0;
  c->status = 0;
  c->server_p = server;
	c->timestamp = gettimestamp();
	
//	c->event.data.fd = fd;
//	c->event.events = 0;
	c->socketfd = fd;
	c->seqno = seq;
	
	if(pthread_mutex_init(&c->lock,NULL) != 0){
		perror("Init Connection_t,pthread_mutex_t  failed\n");  	
		return NULL;
	}
	return c;
}

void Connection_destroy(Connection_t *c)
{
	if(c != NULL)
	{
		pthread_mutex_destroy(&c->lock);
		free(c);
	}
}


//void close_session2(Connection_t* connection)
//{
//	if(connection == NULL)
//		return;
//	Server_t* svr = (Server_t*)connection->server_p;
//	Receiver_t *recv_p = &svr->Recv_task;	
//	Connection_t **tmp;
//	
//	int client_sockfd = connection->socketfd;
//	char key[20]={0};
//	sprintf(key,"%d",client_sockfd);
//	
//	tmp = (Connection_t**)map_get(&svr->mConnectionInfo,key);	
//	if(tmp == NULL)		return;
//	
//	map_remove(&svr->mConnectionInfo,key);
////	Connection_deinit(connection);
//	ReleaseConnection(connection);
//	
//	epoll_ctl(svr->Recv_task.epfd, EPOLL_CTL_DEL, client_sockfd,NULL);
//	close(client_sockfd);
//	
//	pthread_mutex_lock(&recv_p->lock);	
//	recv_p->poll_ev_num--;
//	pthread_mutex_unlock(&recv_p->lock); 
//	LOG(LL_NOTICE,"Socket[%d] closed",client_sockfd);
////	printf("socket[%d] closed\n",client_sockfd);
//}
long gettimestamp()
{
	struct timeval timestamp;
	gettimeofday(&timestamp,0);
	return timestamp.tv_sec;
}

Connection_t* GetConnectionByKey(Server_t* svr,int key)
{
	if(svr == NULL)
		return NULL;
	char skey[20]={0};
	sprintf(skey,"%d",key);
	Connection_t** hash_node = (Connection_t**)map_get(&svr->mConnectionInfo,skey);
	if(hash_node)
		return *hash_node;
	return NULL;
}

int ConnectionPool_create(ConnectionPool_t* sp,int size)
{
	if(sp == NULL) return FALSE;
	
	int ret,i;
	sp->session_arr = (Connection_t*)calloc(size,sizeof(Connection_t));
	ret = queue_create(&sp->s_unused,size);
	if(!ret)
	{
		free(sp->session_arr);
		return FALSE;
	}
//	char **sndbuf = (char**)calloc(size,SENDBUF*sizeof(char));
//	if(!sndbuf)
//		return FALSE;
//	char **rcvbuf = (char**)calloc(size,RECVBUF*sizeof(char));
//	if(!rcvbuf)
//	{
//		free(sndbuf);
//		return FALSE;
//	}
//	sp->commbuf = (char*)calloc(size,SENDBUF + RECVBUF);
//	if(!sp->commbuf)
//		return FALSE;
		
	for(i = 0 ;i < size ; i++)
	{
//		sp->session_arr[i].respbuf = &sp->commbuf[i*(SENDBUF + RECVBUF)];// sndbuf[i];
		sp->session_arr[i].resplen = SENDBUF;
//		sp->session_arr[i].reqbuf = &sp->commbuf[i*(SENDBUF + RECVBUF)+SENDBUF];//rcvbuf[i];
		sp->session_arr[i].reqlen = RECVBUF;
		queue_pushend(&sp->s_unused,&sp->session_arr[i]);
	}
	sp->count = size;
	
	if(pthread_mutex_init(&sp->lock,NULL) != 0){
		perror("Init Connection_t,pthread_mutex_t  failed\n");  	
		return FALSE;
	}
	return TRUE;
}
int ConnectionPool_destroy(ConnectionPool_t* sp)
{
	if(!sp)
		return FALSE;
	free(sp->session_arr);
//	free(sp->commbuf);
	queue_free(&sp->s_unused);
	pthread_mutex_destroy(&sp->lock);
//	queue_free(&sp->s_using);
	return TRUE;
}
Connection_t* GetConnection(Server_t* sp)
{
	if(!sp)		return NULL;

#ifdef	NOPOOL
	Connection_t* connection = (Connection_t*)calloc(1,sizeof(Connection_t));	
#endif
	
#ifdef	SESSPOOL
	pthread_mutex_lock(&sp->mConnectionPool.lock);
	Connection_t* connection = (Connection_t*)queue_popfront(&sp->mConnectionPool.s_unused);
	pthread_mutex_unlock(&sp->mConnectionPool.lock);
#endif	

#ifdef	MEMPOOL	
	Connection_t* connection = (Connection_t*)Mem_Alloc(&sp->mMempool);
	memset(connection,0x00,sizeof(Connection_t));
#endif	
	
	return connection;
}
int ReleaseConnection(Connection_t* connection)
{
//	if(!sp)		return FALSE;
	if(!connection) return FALSE;
	Server_t* svr = (Server_t*)connection->server_p;
	
#ifdef	SESSPOOL
	pthread_mutex_lock(&svr->mConnectionPool.lock);
	queue_pushend(&svr->mConnectionPool.s_unused,connection);
	pthread_mutex_unlock(&svr->mConnectionPool.lock);
	Connection_deinit(connection);
#endif

#ifdef	MEMPOOL	
	Mem_Free(&svr->mMempool,connection);
#endif

#ifdef	NOPOOL
	Connection_destroy(connection);
#endif

	return TRUE;
}
void CloseConnection(Connection_t* connection)
{
	if(connection == NULL)
		return;
	Server_t* svr = (Server_t*)connection->server_p;
	Receiver_t *recv_p = &svr->Recv_task;	
	Connection_t **tmp;
	
	int client_sockfd = connection->socketfd;
	char key[20]={0};
	sprintf(key,"%d",client_sockfd);
	
	tmp = (Connection_t**)map_get(&svr->mConnectionInfo,key);	
	if(tmp == NULL)		return;
	
	map_remove(&svr->mConnectionInfo,key);
//	Connection_destroy(connection);
	ReleaseConnection(connection);
	
//	epoll_ctl(svr->Recv_task.epfd, EPOLL_CTL_DEL, client_sockfd,NULL);
	close(client_sockfd);
	
	__sync_fetch_and_sub(&recv_p->poll_ev_num,1);
	
//	pthread_mutex_lock(&recv_p->lock);	
//	recv_p->poll_ev_num--;
//	pthread_mutex_unlock(&recv_p->lock); 
	LOG(LL_NOTICE,"Socket[%d] closed",client_sockfd);
}
int Connection_init(Connection_t *c,Server_t* server,int fd,int seq)
{
	if(c == NULL)
		return FALSE;

  c->reqlen = RECVBUF;
  c->resplen = SENDBUF;		
	memset(c->reqbuf,0x00,c->reqlen);
	memset(c->respbuf,0x00,c->resplen);
  c->status = 0;
  c->server_p = server;
	c->timestamp = gettimestamp();
	
//	c->event.data.fd = fd;
//	c->event.events = 0;
	c->socketfd = fd;
	c->seqno = seq;
	
	if(pthread_mutex_init(&c->lock,NULL) != 0){
		perror("Init Connection_t,pthread_mutex_t  failed\n");  	
		return FALSE;
	}
	return TRUE;
}

int Connection_deinit(Connection_t *c)
{
	if(c == NULL)
		return FALSE;
		
	memset(c->reqbuf,0x00,c->reqlen);
	memset(c->respbuf,0x00,c->resplen);
//  c->reqlen = 0;
//  c->resplen = 0;
  c->status = 0;
  c->server_p = NULL;
	c->timestamp = 0;
	
//	c->event.data.fd = 0;
//	c->event.events = 0;
	c->socketfd = 0;
	c->seqno = 0;
	
	pthread_mutex_destroy(&c->lock);

	return TRUE;
}	
