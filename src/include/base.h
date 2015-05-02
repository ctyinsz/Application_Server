#ifndef _BASE_
#define _BASE_
#include	"gas.inc"
#define BACKLOG 500

#define ERROR(errmsg) { \
  printf("[%s][%d]MSG:[%s]\n",__FILE__,__LINE__,errmsg); \
  return -1; \
}
#define set_nonblocking(s)  fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK)
#define set_blocking(s)     fcntl(s, F_SETFL, fcntl(s, F_GETFL) & ~O_NONBLOCK)

struct struSOCKLIST                  
{                                    
  int sockfd;                      
  int status; /*R_OK 4 W_OK 02 X_OK 01 */
  int opstat; /*R_OK 4 W_OK 02 X_OK 01 0 待查证*/
};                            
typedef struct struSOCKLIST SOCKLIST;

void daemon_init( int ign_sigcld );

int tcpserver_init(int port);
int tcpserver_start(int sockfd);
/*nbflag 0 超时返回 1 超时不返回*/
int tcpserver_accept(int sockfd,int timeout,int nbflag);

int getsockinfo(int commfd,int *localport, char *localaddr,int *remoteport,char *remoteaddr);
int setnonblocking(int sock);

int tcpserver_config(SOCKLIST *socketlist,size_t count,int op,int fd,int status);
int tcpserver_waiting(SOCKLIST *socketlist,size_t count,size_t *maxtime);

int epoll_config(int epfd, int op, int fd,uint32_t events);
//int epoll_waiting(int epfd, struct epoll_event * events,int maxevents, int timeout);

int recvallbytes(int s, void **buf, size_t len,int flags);


#endif
