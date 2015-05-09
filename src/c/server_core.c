#include	"gas.inc"
int Server_init(Server_t *svr,int port,int th_pool_size,int s_pool_zsie)
{
	if(svr == NULL) return FALSE;
	int ret;
	svr->lsnport = port;
	svr->keepalive = 1;
	svr->seq_no = 0;

	map_init(&svr->mConnectionInfo,MAPSIZE);
#ifdef	SESSPOOL	
	ret = ConnectionPool_create(&svr->mConnectionPool,s_pool_zsie);
	if(!ret)
		return FALSE;
#endif	

#ifdef	MEMPOOL	
	ret = MemoryPool_create(&svr->mMempool,sizeof(Connection_t),MEMPOOLINITSIZE,MEMPOOLGROWSIZE);
	if(!ret)
		return FALSE;
#endif	
		
	if(thpool_init(&svr->thpool_p ,th_pool_size) == FALSE)
	{
		#ifdef	SESSPOOL	
		ConnectionPool_destroy(&svr->mConnectionPool);
		#endif	
	 	return FALSE;
	}
	
	if(recv_task_create(&svr->Recv_task) == FALSE) 
	{
		#ifdef	SESSPOOL	
		ConnectionPool_destroy(&svr->mConnectionPool);
		#endif	
		thpool_destroy(&svr->thpool_p);
		return FALSE;
	}

	return TRUE;
}
void Server_destroy(Server_t* svr)
{	
	if(svr == NULL) return;
	svr->keepalive = 0;	
	
	pthread_join(svr->ConnectionMgr,NULL);
	pthread_join(svr->Recv_task.Recv_task,NULL);
	pthread_join(svr->Accept_task,NULL);
	
	recv_task_destroy(&svr->Recv_task);
	thpool_destroy(&svr->thpool_p);
	map_deinit(&svr->mConnectionInfo);
	
#ifdef	SESSPOOL
	ConnectionPool_destroy(&svr->mConnectionPool);
#endif
#ifdef	MEMPOOL	
	MemoryPool_destroy(&svr->mMempool);
#endif
}

void Server_free(Server_t* svr)
{
}
int Server_start(Server_t* svr)
{
	if(svr == NULL) return FALSE;
	int ret;
			
	ret = pthread_create(&svr->Recv_task.Recv_task,NULL,(void *)Server_thread_recv,(void*)svr);
	if(ret == 0)
		LOG(LL_NOTICE,"Start Recv_task thread 0x%x...",svr->Recv_task.Recv_task);
	else
	{
		LOG(LL_ERROR,"Start Recv_task thread faild[%d]", ret);
		return FALSE;
	}
			
	ret = pthread_create(&svr->ConnectionMgr,NULL,(void *)Server_thread_ConnectionMgr,(void*)svr);
	if(ret == 0)
		LOG(LL_NOTICE,"Start ConnectionMgr thread 0x%x...",svr->ConnectionMgr);
	else
	{
		LOG(LL_ERROR,"Start ConnectionMgr thread faild[%d]", ret);
		return FALSE;
	}
	
	ret = pthread_create(&svr->Accept_task,NULL,(void *)Server_thread_accept,(void*)svr);
	if(ret == 0)
		LOG(LL_NOTICE,"Start Accept_task thread 0x%x...",svr->Accept_task);
	else
	{
		LOG(LL_ERROR,"Start Accept_task thread faild[%d]", ret);
		return FALSE;
	}
	return TRUE;;			
}

void Server_thread_accept(void* para_p)
{
	Server_t* svr = (Server_t*)para_p;
	Connection_t* conn;
	Receiver_t* recv_p = &svr->Recv_task;
	
	struct   timeval   start,diff; 

	int server_sock,client_sock;
//	int timeout=0;
	int ret;

	server_sock = tcpserver_init(svr->lsnport);

	if(server_sock > 0)
		tcpserver_start(server_sock);
	else
		exit(1);

	struct sockaddr_in client_sockaddr;	
	socklen_t clisocklen = sizeof(struct sockaddr_in);
	struct epoll_event event;
	struct  timeval stTimeVal={CONTIMEOUT,0};
	if ( setsockopt(server_sock,SOL_SOCKET,SO_RCVTIMEO,&stTimeVal,sizeof(stTimeVal))==-1)
	{
		close(server_sock);
		exit(1);
	}

//	set_nonblocking(server_sock);
	while(svr->keepalive)
	{
		client_sock = accept(server_sock,(struct sockaddr *) &client_sockaddr, &clisocklen);
//		client_sock = tcpserver_accept(server_sock,2,0);
//		client_sock = tcpserver_accept(server_sock,timeout,0);
		if(client_sock == -1)
		{
			if(errno == EAGAIN)
			{
				continue;
			}
		}
		if(client_sock > 0)
		{
//			LOG(LL_NOTICE,"Open communication with  Client %s on socket %d\n",inet_ntoa(client_sockaddr.sin_addr), client_sock);

			set_nonblocking(client_sock);
			memset(&event,0x00,sizeof(event));
			event.events = EPOLLIN;
			event.data.fd = client_sock;
			
			ret = epoll_ctl(recv_p->epfd, EPOLL_CTL_ADD, client_sock, &event);
			if(ret < 0)
			{
				LOG(LL_NOTICE,"epoll_ctl ret[%d][%s]",errno,strerror(errno));
				continue;
			}
			
			//保存socket和序列号到队列
			conn = GetConnection(svr);
			if(conn == NULL)
			{
				close(client_sock);
				continue;
			}
			Connection_init(conn,svr,client_sock,svr->seq_no++);  //  __sync_fetch_and_add(&svr->seq_no,1)
			conn->status = 1;

			__sync_fetch_and_add(&recv_p->poll_ev_num,1);

//			pthread_mutex_lock(&recv_p->lock);
//			recv_p->poll_ev_num++;
//			pthread_mutex_unlock(&recv_p->lock);
			
			//将该任务的时间戳记录到hash表
			map_set(&svr->mConnectionInfo,client_sock, (void*)conn);
			LOG(LL_NOTICE,"Socket[%d] Connected,seq[%d]",client_sock,conn->seqno);
		}
	}
	close(server_sock);
	LOG(LL_NOTICE,"Server_thread_accept thread 0x%x is exiting!",pthread_self());
//	printf("Server_thread_accept thread 0x%x is exiting\n", pthread_self());
	pthread_exit(NULL);	
}

void Server_thread_recv(void* para_p)
{
	Server_t* svr = (Server_t*)para_p;
	Receiver_t *recv_p = &svr->Recv_task;
	
	Connection_t* conn = NULL;

	int nfds,i,n; 

	int client_sockfd;
	int ret = 0;
	struct epoll_event ev,events[MAXEPOLLEVENTS];
	memset(&ev,0x00,sizeof(ev));
	memset(events,0x00,sizeof(events));	

//	struct timespec ts;
	while(svr->keepalive)
	{
		nfds = epoll_wait(recv_p->epfd,events,MAXEPOLLEVENTS,1000);
		for(i=0;i<nfds;++i)
		{
			client_sockfd = events[i].data.fd;
			if(client_sockfd < 0)
				continue;
			conn = GetConnectionByKey(svr,client_sockfd);
			if(conn == NULL)
			{
				close(client_sockfd);
				continue;
			}
			
			LOG(LL_NOTICE,"Socket[%d] seq[%d] has event[%x].",client_sockfd,conn->seqno,events[i].events);
			
			if((events[i].events&EPOLLERR)||(events[i].events&EPOLLHUP))
			{
				CloseConnection(conn);
				conn = NULL;
			}
			else if(events[i].events&EPOLLIN)
			{
				n = RecvAllData(conn);
//				LOG(LL_DEBUG,"Socket[%d] seq[%d] recv ret[%d],error[%s]buf[%s]",client_sockfd,conn->seqno,n,strerror(errno),conn->commbuf);
				if(n <= 0)
				{
					LOG(LL_DEBUG,"Socket[%d] seq[%d] recv ret[%d],error[%s]",client_sockfd,conn->seqno,n,strerror(errno));
					CloseConnection(conn);
					conn = NULL;
				}
				else
				{
					LOG(LL_DEBUG,"Socket[%d] seq[%d] recv ret[%d],buf[%s]",client_sockfd,conn->seqno,n,conn->commbuf);
					ret = thpool_add_work(&svr->thpool_p,(void*)thpool_task,(void*)conn);
				}
			}
			else{}			
		}
	}
	LOG(LL_NOTICE,"Server_thread_recv thread 0x%x is exiting!",pthread_self());
//	printf("Server_thread_recv thread 0x%x is exiting\n", pthread_self());
	pthread_exit(NULL);	
}

void* thpool_task(void* arg)
{
	if(arg == NULL)
		return NULL;

	Connection_t* conn = (Connection_t*)arg;
//	Server_t* svr = (Server_t*)conn->server_p;
	
	int client_sockfd = conn->socketfd;
	
	int ret;

	LOG(LL_NOTICE,"in thpool,task sock[%d],seq=[%d]!",client_sockfd,conn->seqno);

	pthread_mutex_lock(&conn->lock);

	ret = DoRequest(conn);
	if(ret == TRUE)
	{
		ret = SendAllData(conn);
		if(ret < 0)
		{
			LOG(LL_DEBUG,"Socket[%d] seq[%d] send ret[%d],error[%s]",conn->socketfd,conn->seqno,ret,strerror(errno));
		}
		else
		{
			LOG(LL_DEBUG,"Socket[%d] seq[%d] send ret[%d]",conn->socketfd,conn->seqno,ret);
		}
	}
	pthread_mutex_unlock(&conn->lock);
	return NULL;
}


void Server_thread_ConnectionMgr(void* para_p)
{
	Server_t* svr = (Server_t*)para_p;
	Connection_t* conn,**tmp;
	int key;
	long curtime,difftime;
	int ret;
	
	while(svr->keepalive)
	{
		sleep(5);
		map_iter_t iter = map_iter(&svr->mConnectionInfo);
//		while(0)
		while ((key = map_next(&svr->mConnectionInfo, &iter)) > 0)
		{
			curtime =	gettimestamp();
			tmp = (Connection_t**)map_get(&svr->mConnectionInfo,key);
			if(tmp == NULL)		continue;
			conn = *tmp;
			
			ret = pthread_mutex_trylock(&conn->lock);	
			if(ret != 0)
			{
				if(errno == EBUSY)
					continue;
				else
				{
					LOG(LL_NOTICE,"pthread_mutex_trylock on connection ret errno=[%s]",strerror(errno));
					CloseConnection(conn);
					continue;
				}
			}
			difftime = curtime - conn->timestamp;
			LOG(LL_NOTICE,"in Server_thread_ConnectionMgr socket[%d][%d],seq[%d],idletime[%ld]",key,conn->status,conn->seqno,difftime);
//			printf("in Server_thread_ConnectionMgr socket[%s][%d],idletime[%ld]\n",key,conn->status,curtime - conn->timestamp);
			if((conn->status == 1 && (curtime - conn->timestamp)>RCVTIMEOUT)
				|| (conn->status == 2 && (curtime - conn->timestamp)>SNDTIMEOUT)
				|| (conn->status == 3 && (curtime - conn->timestamp)>CLOSEWAIT))
			{
				shutdown(conn->socketfd,2);
				pthread_mutex_unlock(&conn->lock);
				CloseConnection(conn);
				conn = NULL;
			}
			else
				pthread_mutex_unlock(&conn->lock);
		}
	}
	LOG(LL_NOTICE,"Server_thread_ConnectionMgr thread 0x%x is exiting",pthread_self());
//	printf("Server_thread_ConnectionMgr thread 0x%x is exiting\n", pthread_self());
	pthread_exit(NULL);	
}

int recv_task_create(Receiver_t* recv_t)
{
	if(recv_t == NULL) return FALSE;

	if(pthread_mutex_init(&recv_t->lock,NULL)!=0)
	{
		recv_task_destroy(recv_t);
		return FALSE;
	}
	recv_t->epfd = epoll_create(MAXEPOLLSIZE);
	recv_t->poll_ev_num = 0;
	return TRUE;
}
void recv_task_destroy(Receiver_t* recv_t)
{
	pthread_mutex_destroy(&recv_t->lock);
	close(recv_t->epfd);
}

int DoRequest(Connection_t * conn)
{
	if(conn == NULL)
		return FALSE;
//	printf("in DoRequest\n");
	conn->timestamp = gettimestamp();
	conn->status = 3;
	return TRUE;
}

double   timeval_subtract(struct   timeval*   result,   struct   timeval*   x,   struct   timeval*   y)   
{   
 //   int   nsec;   
    
    struct   timeval   end;
    if(y==0)
    {
    	gettimeofday(&end,0);
    	y=&end;
    }  

    if   (   x->tv_sec>y->tv_sec   )   
              return   -1;   

    if   (   (x->tv_sec==y->tv_sec)   &&   (x->tv_usec>y->tv_usec)   )   
              return   -1;   

    result->tv_sec   =   (   y->tv_sec-x->tv_sec   );   
    result->tv_usec   =   (   y->tv_usec-x->tv_usec   );   

    if   (result->tv_usec<0)   
    {   
              result->tv_sec--;   
              result->tv_usec+=1000000;   
    }
    
    gettimeofday(x,0);

    return   result->tv_sec + result->tv_usec/1000000.0;   
} 
