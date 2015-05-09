#include	"gas.inc"
/******************************************************************************
函数名称:  server_init
函数功能:  初始化服务器
输入参数:  port--服务器监听端口
输出参数:  
返 回 值:  无
******************************************************************************/
int tcpserver_init(int port)
{
	int rcd = 0,optv = 1;
	int server_sockfd;
//	int client_sockfd;	
	struct sockaddr_in server_sockaddr;
//	struct sockaddr_in client_sockaddr;	
	
  socklen_t server_len;
  server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sockfd == -1) {
		printf("create server_socket error!\n");
		return -1;
	}
	//设为非阻塞
//	if (fcntl(server_sockfd, F_SETFL, O_NONBLOCK) == -1) {
//		printf("Set server socket nonblock failed\n");
//		return -1;
//	}	
//	rcd = setnonblocking(server_sockfd);
//	if (rcd == -1) {
//		printf("Set server socket nonblock failed\n");
//		close(server_sockfd);
//		return -1;
//	}	

	memset(&server_sockaddr, 0, sizeof(server_sockaddr));
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//设置监听端口
	server_sockaddr.sin_port = htons(port);
	server_len = sizeof(server_sockaddr);	

	rcd = setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &optv, sizeof(optv));
	if (rcd < 0) {
		fprintf(stdout, "SO_REUSEADDR failed: %s\n", strerror(errno));
		return rcd;
	}
	//绑定
	rcd = bind(server_sockfd, (struct sockaddr *) &server_sockaddr, server_len);
	if (rcd == -1) {
		printf("bind port %d error!\n", ntohs(server_sockaddr.sin_port));
		return -1;
	}
	return server_sockfd;	
}
/******************************************************************************
函数名称:  server_start
函数功能:  启动服务器
输入参数:  server_sockfd--服务器文件描述符
输出参数:  
返 回 值:  无
******************************************************************************/
int tcpserver_start(int server_sockfd)
{
	int rcd;
	rcd = listen(server_sockfd, BACKLOG);
	if (rcd == -1) {
		printf("listen error!\n");
		return -1;
	}	
	printf("Server is  waiting on socket=%d \n", server_sockfd);	
	return rcd;	
}
/******************************************************************************
函数名称:  server_accept
函数功能:  接收连接请求
输入参数:  server_sockfd--服务器文件描述符  timeout-- 超时时间
输出参数:  
返 回 值:  无
******************************************************************************/
//int tcpserver_accept(int server_sockfd,int timeout)
//{
//	int rcd;
//	int client_sockfd;	
//	struct sockaddr_in client_sockaddr;	
//	socklen_t clisocklen;
//	
//	fd_set watchset;
//	struct timeval tv;	
//	FD_ZERO(&watchset);
//	FD_SET(server_sockfd, &watchset);	
//	
//	while(1)
//	{
//		tv.tv_sec = timeout;
//		tv.tv_usec = 0;
//		rcd = select(server_sockfd + 1, &watchset, NULL, NULL, &tv);
//		switch (rcd)
//		{
//			case -1:
//				printf("Select error\n");
//				return -1;
//			case 0:
//				printf("Select time_out\n");
//				FD_ZERO(&watchset);
//				FD_CLR(server_sockfd, &watchset);
//				FD_SET(server_sockfd, &watchset);
//				break;
//			default:
//				if (FD_ISSET(server_sockfd, &watchset))
//				{
//					clisocklen = sizeof(client_sockaddr);
//					client_sockfd = accept(server_sockfd,(struct sockaddr *) &client_sockaddr, &clisocklen);
//					if (client_sockfd < 0)
//					{
//						printf("Accept error\n");
//						return -1;
//					}
//					printf("\nopen communication with  Client %s on socket %d\n",inet_ntoa(client_sockaddr.sin_addr), client_sockfd);
//
//					return client_sockfd;
//				}
//				FD_ZERO(&watchset);
//				FD_CLR(server_sockfd, &watchset);
//				FD_SET(server_sockfd, &watchset);				
//		}	
//	}
//}

int tcpserver_accept(int server_sockfd,int timeout,int nbflag)
{
	int rcd;
	int client_sockfd;	
	struct sockaddr_in client_sockaddr;	
	socklen_t clisocklen;
	
	fd_set watchset;
	struct timeval tv;	
	FD_ZERO(&watchset);
	FD_SET(server_sockfd, &watchset);	
	
	do
	{
		tv.tv_sec = timeout;
		tv.tv_usec = 0;
		rcd = select(server_sockfd + 1, &watchset, NULL, NULL, &tv);
		switch (rcd)
		{
			case -1:
				printf("Select error\n");
				return -1;
			case 0:
//				printf("Select time_out\n");
				FD_ZERO(&watchset);
				FD_CLR(server_sockfd, &watchset);
				FD_SET(server_sockfd, &watchset);
				break;
			default:
				if (FD_ISSET(server_sockfd, &watchset))
				{
					clisocklen = sizeof(client_sockaddr);
					client_sockfd = accept(server_sockfd,(struct sockaddr *) &client_sockaddr, &clisocklen);
					if (client_sockfd < 0)
					{
						printf("Accept error\n");
						return -1;
					}
					printf("\nopen communication with  Client %s on socket %d\n",inet_ntoa(client_sockaddr.sin_addr), client_sockfd);

					return client_sockfd;
				}
				FD_ZERO(&watchset);
				FD_CLR(server_sockfd, &watchset);
				FD_SET(server_sockfd, &watchset);				
		}	
	}while(nbflag);
	return rcd;
}
/******************************************************************************
函数名称:  getsockinfo
函数功能:  根据连接套接字获取本地信息
输入参数:  commfd -连接套接字
输出参数:  
返 回 值:  无
******************************************************************************/
int getsockinfo(int commfd,int *localport, char *localaddr,int *remoteport,char *remoteaddr)
{
  struct sockaddr_in saddr;
  int len;

  len = sizeof(struct sockaddr_in);
  if ( getsockname(commfd,(struct sockaddr *)&saddr,&len) == 0 )
  {
    if (localaddr!=NULL) 
    	strcpy(localaddr,inet_ntoa(saddr.sin_addr));
    if (localport!=NULL) 
    	*localport = ntohs(saddr.sin_port); 
  }
  else
  {
    return -1;
  }
  memset(&saddr, 0x00, sizeof(struct sockaddr_in));
  if ( getpeername(commfd,(struct sockaddr *)&saddr, &len) == 0 )
  {
    if (remoteaddr!=NULL) 
    	strcpy(remoteaddr,inet_ntoa(saddr.sin_addr));
    if (remoteport!=NULL) 
    	*remoteport = ntohs(saddr.sin_port);
  }
  else
  {
    return -1;
  }
  return 0;	
}

/***************************************************
 * 函数名称: daemon_init
 * 函数功能: 创建一个精灵进程
 * 输入参数: ign_sigcld是否忽略SIGCLD信号 0不忽略
 * 输出参数: 无
 * 函数返回: 无
 ****************************************************/
void daemon_init( int ign_sigcld )
{
  register int childpid;

  if ( getppid() == 1 )
    return;
    
  if ( ( childpid = fork() ) < 0 )
  {
      fprintf(stderr,"Can not fork child process!\n");
      exit(1);
  }
  else if ( childpid > 0 ) /*parent*/
  {
      exit(0);
  }

  /*用setsid以创建一个新对话期 使调用进程：
          a) 成为新对话期的首进程，
          b)成为一个新进程组的首进程，
          c)没有控制终端。
  */
  if ( setsid() == -1 )
  {
    fprintf(stderr,"Fail to Call setsid!\n");
    exit(1);
  }

  umask(0);

  chdir("/");

  signal(SIGCLD,SIG_IGN);
  signal(SIGINT,SIG_IGN);

  return ;
}
/***************************************************
 * 函数名称: setnonblocking
 * 函数功能: 设置套接字为非阻塞
 * 输入参数: sock 套接字
 * 输出参数: 无
 * 函数返回: 无
 ****************************************************/
int setnonblocking(int sock)
{
    int opts;
    opts=fcntl(sock,F_GETFL);
    if(opts<0)
    {
        perror("fcntl(sock,GETFL)");
        return -1;
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts)<0)
    {
        perror("fcntl(sock,SETFL,opts)");
        return -1;
    }
    return 0;
}

/***************************************************
 * 函数名称: epoll_config
 * 函数功能: 设置epoll
 * 输入参数: epfd,op,fd,events
 * 输出参数: 无
 * 函数返回: 无
 ****************************************************/
int epoll_config(int epfd, int op, int fd,uint32_t events)
{
	struct epoll_event ev;
	ev.data.fd = fd;
	ev.events = events;
	return epoll_ctl(epfd,op,fd,&ev);
}

/***************************************************
 * 函数名称: server_config
 * 函数功能: 设置SOCKLIST
 * 输入参数: socketlist,
 op,0 增加 1删除 2修改
 fd,
 status R_OK 4 W_OK 02 X_OK 01
 * 输出参数: 无
 * 函数返回: 无
 ****************************************************/
int tcpserver_config(SOCKLIST *socketlist,size_t count,int op,int fd,int status)
{
	int index=0;
	for(index = 0;index<count;index++)
	{
		if(op == 0)
		{
			if(socketlist[index].sockfd <= 0)
			{
				socketlist[index].sockfd = fd;
				socketlist[index].status = status;
				socketlist[index].opstat = 0;
				return 0;
			}
		}
		else if(op == 1)
		{
			if(socketlist[index].sockfd == fd)
			{
				socketlist[index].sockfd = 0;
				socketlist[index].status = 0;
				socketlist[index].opstat = 0;
				return 0;
			}
		}
		else if(op == 2)
		{
			if(socketlist[index].sockfd == fd)
			{
				socketlist[index].status = status;
				socketlist[index].opstat = 0;
				return 0;
			}
		}
		else
			return -1;
	}
	if((index == count)&&(socketlist[index].sockfd <= 0))
		return -1;
}

/***************************************************
 * 函数名称: server_waiting
 * 函数功能: 等待socket状态
 * 输入参数: socketlist,
 * 输出参数: 无
 * 函数返回: 无
 ****************************************************/
int tcpserver_waiting(SOCKLIST *socketlist,size_t count,size_t *maxtime)
{
  struct  timeval stTimeVal;
  fd_set  rset,wset,xset;
  int   maxfd = -1;
//  time_t  tm;
//  time_t  dm;
  size_t  index;
  int   iRet;
  int socknum = 0;
  if ( socketlist == NULL )
  {
    printf("[%s][%d]套接字非法!\n",__FILE__,__LINE__);
    errno = EBADF;
    return -1;
  }  
  FD_ZERO(&rset);
  FD_ZERO(&wset);
  FD_ZERO(&xset);  
  if ( maxtime != NULL )
  {
//    tm = time(NULL);
    stTimeVal.tv_sec = *maxtime;
    stTimeVal.tv_usec = 0;
  }
  
  for ( index =0 ; index < count; index++)
  {
    if ( socketlist[index].sockfd > 0 )
    {
    	socknum++;
      if ( socketlist[index].sockfd> maxfd)
      {
        maxfd = socketlist[index].sockfd;
      }
      if ( socketlist[index].status & R_OK)
      {
        FD_SET(socketlist[index].sockfd,&rset);
      }
      if ( socketlist[index].status & W_OK)
      {
        FD_SET(socketlist[index].sockfd,&wset);
      }
      if ( socketlist[index].status& X_OK)
      {
        FD_SET(socketlist[index].sockfd, &xset);
      }
//      socketlist[index].status = 0;
    }
  }
  if(socknum == 0)
  	return 1;

  for ( ;; )
  {
    iRet = select(maxfd + 1,&rset,&wset,&xset,maxtime==NULL?NULL:&stTimeVal);
    
    if ( iRet == 0 )
    {
      /*printf("[%s][%d]轮询套接字超时!\n",__FILE__,__LINE__);*/
      
      return 1;
    }
    if ( iRet < 0 )
    {
      if ( errno == EINTR ) /*重启*/
      {
        continue;
      }
      else 
      {
        perror("select ret==0");
        return -1;
      }
    }
    else
    {
      break;
    }
  }

  for ( index = 0 ;index <count; index++)
  {
    if ( socketlist[index].sockfd > 0 )
    {
      if ( FD_ISSET(socketlist[index].sockfd,&rset))
      {
        socketlist[index].opstat |= R_OK;
      }
      if ( FD_ISSET(socketlist[index].sockfd,&wset))
      {
        socketlist[index].opstat |= W_OK;
      }
      if ( FD_ISSET( socketlist[index].sockfd,&xset))
      {
        socketlist[index].opstat |= X_OK;
      }
    }
  }
	return 0;
}

/***************************************************
 * 函数名称: recvallbytes
 * 函数功能: 读取所有可读数据
 * 输入参数: ,
 * 输出参数: 无
 * 函数返回: 无
 ****************************************************/
int recvallbytes(int s, void **buf, size_t len,int flags)
{
	void *tmp;
	int n=0,i=0,j=0;
	int buflen = len;
	int bytescounter=0;
	setnonblocking(s);
	do
	{
		n = recv(s,*buf+bytescounter,len,flags);
		if( n < len && n >= 0)
		{
			bytescounter = bytescounter + n;
			return bytescounter;
		}
		else if(n == len)
		{
			tmp = realloc(*buf,2*len+bytescounter);
			memset(tmp+bytescounter+len,0x00,len);
			if(!tmp)
			{
				errno = ENOMEM;
				return -1;
			}
			else
			{
				*buf = tmp;
				bytescounter = bytescounter + n;
			}
		}
		else
			return bytescounter;
	}
	while(1);
}




