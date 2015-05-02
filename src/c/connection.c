#include	"gas.inc"
Connection_t *Connection_Create(Server_t* server,int fd,int seq)
{
	Connection_t *c = (Connection_t*)calloc(1,sizeof(Connection_t));
	if(c == NULL)
		return NULL;
	
	memset(c->commbuf,0x00,sizeof(c->commbuf));
	c->buflen = 0;
  c->status = 0;
  c->server_p = server;
	c->timestamp = gettimestamp();
	
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
		
	for(i = 0 ;i < size ; i++)
	{
		sp->session_arr[i].buflen = 0;
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
	queue_free(&sp->s_unused);
	pthread_mutex_destroy(&sp->lock);
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

  c->buflen	= 0;
	memset(c->commbuf,0x00,sizeof(c->commbuf));
  c->status = 0;
  c->server_p = server;
	c->timestamp = gettimestamp();
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
		
	memset(c->commbuf,0x00,sizeof(c->commbuf));
	c->buflen = 0;
  c->status = 0;
  c->server_p = NULL;
	c->timestamp = 0;
	c->socketfd = 0;
	c->seqno = 0;
	
	pthread_mutex_destroy(&c->lock);

	return TRUE;
}	
int RecvAllData(Connection_t * conn)
{
	if(conn == NULL)
		return -1;
	
	int n,rc = 0,cnt = 0;
	do
	{
		n = recv(conn->socketfd, rc + conn->commbuf, sizeof(conn->commbuf)-rc,0);
		LOG(LL_DEBUG,"Socket[%d] seq[%d] recv ret[%d],error[%s]buf[%s]",conn->socketfd,conn->seqno,n,strerror(errno),conn->commbuf);
		if(n > 0)
		{
			rc += n;
		}
		else if(n == 0)
		{
			return 0;
		}
		else
		{
			if(errno == EAGAIN)
				break;
			else
			{
				if(errno == EINTR)
				{
					cnt++;
					if (cnt > MAX_READ_TRIES)
					{
						LOG(LL_NOTICE,"recv tries [%d],fd:[%d]",cnt,conn->socketfd);
						break;
					}
				}
				else
					return -1;
			}
		}
	}while(rc < sizeof(conn->commbuf));
	
	pthread_mutex_lock(&conn->lock);
	conn->buflen = rc;
	conn->timestamp = gettimestamp();
	conn->status = 2;
	pthread_mutex_unlock(&conn->lock);
					
	return rc;
}
int SendAllData(Connection_t * conn)
{
	if(conn == NULL)
		return -1;

	int n,rc = 0,cnt = 0;
	do
	{
		n = send(conn->socketfd, rc + conn->commbuf, conn->buflen-rc,0);
		LOG(LL_DEBUG,"Socket[%d] seq[%d] send ret[%d],error[%s]buf[%s]",conn->socketfd,conn->seqno,n,strerror(errno),conn->commbuf);
		if(n == -1)
		{
			if (cnt > MAX_WRITE_TRIES)
			{
				LOG(LL_DEBUG,"send timeout,fd:[%d]",conn->socketfd);
				return -1;
			}
			if (errno == EINTR)
			{
				LOG(LL_DEBUG,"fd:[%d] EINTR",conn->socketfd);
				cnt++;
			}
			else if (errno == EAGAIN)
			{
				LOG(LL_DEBUG,"fd:[%d] EAGAIN",conn->socketfd);
				cnt++;
			}
			else
			{
				LOG(LL_DEBUG,"fd:[%d] [%s]",conn->socketfd,strerror(errno));
				return -1;
			}
		}
		else
		{
			rc += n;
		}
	}while(rc < conn->buflen);
	
	conn->commbuf[0] = 0;
	conn->timestamp = gettimestamp();
	conn->status = 3;
	shutdown(conn->socketfd,1);	
					
	return rc;
}
