#ifndef _GAS_
#define _GAS_

#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iconv.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>  
#include <pthread.h>
#include <sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <sys/time.h>
#include <dlfcn.h> 
//#include <jemalloc/jemalloc.h>
#include <time.h>
#include <sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#define MEMPOOL
//#define SESSPOOL
//#define NOPOOL

#define MEMPOOLINITSIZE  1024
#define MEMPOOLGROWSIZE	 512

#define LEN_COMMBUF 20000
#define RCVTIMEOUT 10
#define SNDTIMEOUT 60
#define CONTIMEOUT 2
#define CLOSEWAIT  5
#define MAXEPOLLEVENTS  1024
#define MAXEPOLLSIZE		100000
#define MAX_WRITE_TRIES 1024
#define MAX_READ_TRIES  1024
#define MAPSIZE  10000
#define LEN_COMMHEAD  10

#define set_val(p,v)  while(!(__sync_bool_compare_and_swap(p,*p,v)))

#include	"xmle.h"
#include "config.h"
#include "mxml.h"

#include "pub.h"
#include "base.h"
#include "mxml.h"
#include "ocilib.h"
#include "threadpool.h"
#include "MemoryPool.h"
#include "log.h"
#include "macro_define.h"
#include "map.h"
#include "datastruct.h"
#include "server_core.h"
#include "connection.h"

#ifndef TRUE
  #define TRUE                   1
  #define FALSE                  0
#endif


#endif

