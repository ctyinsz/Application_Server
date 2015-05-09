#ifndef _SERVERCORE_H_
#define _SERVERCORE_H_

#include	"gas.inc"

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
	int keepalive;		/*0 ¹Ø±Õ 1 ¿ªÆô*/
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

void* thpool_task(void* arg);

long gettimestamp();

int recv_task_create(Receiver_t*);
void recv_task_destroy(Receiver_t* recv_t);

double  timeval_subtract(struct   timeval*   result,   struct   timeval*   x,   struct   timeval*   y);
#endif
