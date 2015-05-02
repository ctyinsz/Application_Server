#ifndef _SESSION_
#define _SESSION_

#include	"gas.inc"

#define MEMPOOL
//#define SESSPOOL
//#define NOPOOL

#define MEMPOOLINITSIZE  1024*5
#define MEMPOOLGROWSIZE	 1024

#define RECVBUF 2048
#define SENDBUF 2048
#define RCVTIMEOUT 10
#define SNDTIMEOUT 10
#define CONTIMEOUT 2
#define CLOSEWAIT  5
#define MAXEPOLLEVENTS  1024
#define MAXEPOLLSIZE		100000

#define set_val(p,v)  while(!(__sync_bool_compare_and_swap(p,*p,v)))

typedef struct _Connection
{
	char reqbuf[RECVBUF];
	char respbuf[SENDBUF];
	int reqlen;
	int resplen;
	long timestamp;
	int status;								/*0 队列等待 1 事件等待 2 待处理 3 待发送*/
	int socketfd;
	int seqno;
//	struct epoll_event event;/*for epoll_ctl()*/
	struct _Server*  server_p;
	pthread_mutex_t	lock;
}Connection_t;

//typedef struct _ConnectionPoolNode
//{
//	Connection_t *session_arr;
//	char *commbuf;		//count * (RECVBUF,SENDBUF)
//	int count;	
//}ConnectionPoolNode_t;

typedef struct _ConnectionPool
{
	Connection_t *session_arr;
	queue_t s_unused;
//	char *commbuf;		//count * (RECVBUF,SENDBUF)
	pthread_mutex_t	lock;
//	queue_t s_using;
	int count;	
	
//	list_t session_arr;
//	list_t s_unused;
//	int sum;
//	int left;
}ConnectionPool_t;

typedef struct _Receiver
{
	int epfd;
	int poll_ev_num;
	pthread_t Recv_task;
	pthread_mutex_t	lock;
}Receiver_t;

typedef struct _Server
{
	int lsnport;
	int keepalive;		/*0 关闭 1 开启*/
	int seq_no;
	thpool_t		 		thpool_p;
	/*hash_map  */
	map_void_t	 		mConnectionInfo;
#ifdef	SESSPOOL
	ConnectionPool_t 	mConnectionPool;
#endif	
#ifdef MEMPOOL
	MemoryPool_t    mMempool;
#endif
	pthread_t		 		Accept_task;
	Receiver_t	 		Recv_task;
	pthread_t		 		ConnectionMgr;
}Server_t;

void* recv_task(void* arg);
int Server_init(Server_t* svr,int port,int pool_size,int s_pool_size);
void Server_destroy(Server_t* svr);
void Server_free(Server_t* svr);
int Server_start(Server_t* svr);

void Server_thread_accept(void* );
void Server_thread_send(void* );
void Server_thread_recv(void* );
void Server_thread_ConnectionMgr(void* );
int DoRequest(Connection_t* );
void* thpool_task(void* arg);

Connection_t* Connection_Create(Server_t* server_p,int fd,int seq);
int Connection_init(Connection_t *s,Server_t* server,int fd,int seq);
void Connection_destroy(Connection_t *);
int Connection_deinit(Connection_t *);
void CloseConnection(Connection_t*);
//void close_session2(Connection_t*);
Connection_t* GetConnectionByKey(Server_t* svr,int key);
long gettimestamp();

int recv_task_create(Receiver_t*);
void recv_task_destroy(Receiver_t* recv_t);

int ConnectionPool_create(ConnectionPool_t* ,int size);
int ConnectionPool_destroy(ConnectionPool_t*);
Connection_t* GetConnection(Server_t*);
int ReleaseConnection(Connection_t*);
double  timeval_subtract(struct   timeval*   result,   struct   timeval*   x,   struct   timeval*   y);
#endif
