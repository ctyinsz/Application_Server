#include	"gas.inc"
extern char *optarg;
extern int opterr;

int main(int argc, char **argv)
{
	int portnumber;
	int res,stat;
	void *shm = NULL;
	int shmid;
	opterr = 0;
	
	while((res = getopt(argc,argv,"hkp:")) != -1)
	{
		switch(res)
		{
			case 'p':
				portnumber = atoi(optarg);
				stat = 0;
				break;
			case 'k':
				stat = 1;
				break;
			default:
				printf("Usage : %s -p port|-k|-h\n\
				 -p port: start server at port\n\
				 -k :shut server \n\
				 -h :help\n",argv[0]);
				 return 1;
		}
	}
	shmid = shmget((key_t)1234, sizeof(char), 0666|IPC_CREAT);
	if(shmid == -1)
	{
		fprintf(stderr, "shmget failed\n");
		exit(EXIT_FAILURE);
	}
	//将共享内存连接到当前进程的地址空间
	shm = shmat(shmid, (void*)0, 0);
	if(shm == (void*)-1)
	{
		fprintf(stderr, "shmat failed\n");
		exit(EXIT_FAILURE);
	}
//	printf("Memory attached at %X\n", (int)shm);	
//	LOG(LL_DEBUG,"Memory attached at %X",(int)shm);
	char *keepalive = (char*)shm;
//	LOG(LL_DEBUG,"shut down flag[%c]",*keepalive);
//	printf("shut down flag[%c]\n",*keepalive);
	if(stat == 1)
	{
		*keepalive = 'k';
//		puts("write shut down flag");
		exit(1);
	}
	else
	{
		if(*keepalive == 'k')
			*keepalive = 'r';
	}

	char logpath[256]={0};
  time_t now;
	now = time(&now);
	struct tm vtm; 
  localtime_r(&now, &vtm);
	sprintf(logpath,"%s/log/%d%02d%02d",getenv("HOME"),1900+vtm.tm_year,vtm.tm_mon + 1, vtm.tm_mday);
	log_init(LL_DEBUG, "MPP_SDS", logpath);
//	LOG(LL_DEBUG,"start server LL_DEBUG");
//	LOG(LL_WARNING,"start server LL_WARNING");
//	LOG(LL_NOTICE,"start server LL_NOTICE");
//	LOG(LL_ERROR,"start server LL_ERROR");
//	LOG_TRACE();
  Server_t server;
  res = Server_init(&server,portnumber,5,1000);
  if(res == FALSE)
  {
  	LOG(LL_ERROR,"Server_init failed.");
  	exit(1);
  }
	res = Server_start(&server);
  if(res == FALSE)
  {
  	LOG(LL_ERROR,"Server_start failed.");
  	Server_destroy(&server);
  	exit(1);
  }
  LOG(LL_NOTICE,"Server started at port[%d]!",portnumber);
  while(1)
  {
  	if(*keepalive == 'k')
  	{
  		LOG(LL_NOTICE,"Server shutting down!");
	  	Server_destroy(&server);
	  	*keepalive = 0;
	  	shmdt(shm);
	  	shmctl(shmid, IPC_RMID, 0);
	  	pthread_exit(NULL);
	  	return 0;
	  }
	  sleep(5);
  }
  return 0;
}

