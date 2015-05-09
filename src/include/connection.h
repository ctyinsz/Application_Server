#ifndef _CONN_H_
#define _CONN_H_


#include	"gas.inc"

typedef struct _Connection
{
	char commbuf[LEN_COMMBUF];
	int buflen;
	long timestamp;
	int status;								/*0 队列等待 1 事件等待 2 待处理 3 待发送*/
	int socketfd;
	int seqno;
	struct _Server*  server_p;
	pthread_mutex_t	lock;
}Connection_t;

typedef struct _ConnectionPool
{
	Connection_t *session_arr;
	queue_t s_unused;
	pthread_mutex_t	lock;
	int count;	
}ConnectionPool_t;

Connection_t* Connection_Create(Server_t* server_p,int fd,int seq);
int Connection_init(Connection_t *s,Server_t* server,int fd,int seq);
void Connection_destroy(Connection_t *);
int Connection_deinit(Connection_t *);
void CloseConnection(Connection_t*);
Connection_t* GetConnectionByKey(Server_t* svr,int key);
int DoRequest(Connection_t* );
int RecvAllData(Connection_t * conn);
int SendAllData(Connection_t * conn);

int ConnectionPool_create(ConnectionPool_t* ,int size);
int ConnectionPool_destroy(ConnectionPool_t*);
Connection_t* GetConnection(Server_t*);
int ReleaseConnection(Connection_t*);
#endif
