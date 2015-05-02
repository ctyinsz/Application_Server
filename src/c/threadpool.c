#include	"gas.inc"
int  thpool_init(thpool_t* thpool,int threadN)
{
//	thpool_t* thpool;
	int ret;
	if(!threadN || threadN < 1)
		threadN = 1;

	//分配线程池内存
//	thpool = (thpool_t*) calloc(1,sizeof(thpool_t));
	if(thpool ==NULL)
	{
		printf("malloc thpool_t error");
		return FALSE;
	}
	
	//分配线程数
	thpool->threadsN = threadN;

	if(thpool_jobqueue_init(thpool) == FALSE)
		return FALSE;	
	
	thpool->threads =(pthread_t*) calloc(threadN,sizeof(pthread_t));
	if(thpool->threads == NULL)
	{
		printf("malloc thpool->threads error");
		return FALSE;
	}

	thpool->keepalive = 1;

	int t;
	for(t = 0;t< threadN ;t++)
	{
		ret = pthread_create(&(thpool->threads[t]),NULL,(void *)thpool_thread_do,(void*)thpool);
		if(ret == 0)
			printf("start thread 0x%x...\n", thpool->threads[t]);
		else
			printf("start thread faild[%d]\n", ret);
	}
	return TRUE;
}

void thpool_destroy(thpool_t* tp_p)
{
	int i ;
	tp_p->keepalive = 0;

	pthread_cond_broadcast(&tp_p->jobqueue.queue_not_empty);

	for(i = 0;i < (tp_p->threadsN); i++)
	{
		pthread_join(tp_p->threads[i],NULL);
	}

	thpool_jobqueue_empty(tp_p);

	free(tp_p->threads);

///	free(tp_p->jobqueue);
//	free (tp_p);
}
//将消息加入线程池
int thpool_add_work(thpool_t* tp_p, void* (*function_p)(void*), void* arg_p)
{
	thpool_job_t* newjob;
	newjob = (thpool_job_t*) malloc(sizeof(thpool_job_t));
	
	if(newjob ==NULL)
	{
		fprintf(stderr, "thpool_add_work(): Could not allocate memory for new job\n");
		return -1;
	}
	newjob ->function = function_p;
	newjob ->arg      = arg_p;

	pthread_mutex_lock(&tp_p->jobqueue.queue_lock);
	thpool_jobqueue_add(tp_p,newjob);
	pthread_cond_signal(&tp_p->jobqueue.queue_not_empty);
	pthread_mutex_unlock(&tp_p->jobqueue.queue_lock);     
	return 0;
}
//工作线程
void thpool_thread_do(void* thpool)
{
	thpool_t* tp_p = (thpool_t*)thpool;
	thpool_jobqueue* tpjq_p = &(tp_p->jobqueue);
	thpool_job_t*  job_p;
	FUNC function;
	void* arg_buff;
	while(1)
	{
		pthread_mutex_lock(&tp_p->jobqueue.queue_lock);
		while ((tpjq_p->jobN == 0) && (tp_p->keepalive))
		{
//			printf("thread 0x%x is waiting\n", pthread_self());
			pthread_cond_wait(&tpjq_p->queue_not_empty, &tpjq_p->queue_lock);
		}
		if(!(tp_p->keepalive))
		{
			pthread_mutex_unlock(&tpjq_p->queue_lock);
			printf("thread 0x%x is exiting\n", pthread_self());
			pthread_exit(NULL);
		}

		job_p = thpool_jobqueue_pop(tp_p);
		function = job_p->function;
		arg_buff = job_p->arg;
//		printf("thread 0x%x get task[%d] ",pthread_self(),(int)arg_buff);
		pthread_mutex_unlock(&tpjq_p->queue_lock);
		function(arg_buff);   //运行 你的方法。
		free(job_p);         ////释放掉。
	}
	pthread_exit(NULL);	 ;
}
//对双向队列初始化
int thpool_jobqueue_init(thpool_t* tp_p){
	if(!tp_p)
		return FALSE;

//	tp_p->jobqueue=(thpool_jobqueue*)malloc(sizeof(thpool_jobqueue));
//	if (tp_p->jobqueue==NULL) return -1;
	tp_p->jobqueue.tail=NULL;
	tp_p->jobqueue.head=NULL;
	tp_p->jobqueue.jobN=0;

//	tp_p->jobqueue->queue_lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
//	tp_p->jobqueue->queue_not_empty = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
	if(pthread_mutex_init(&tp_p->jobqueue.queue_lock,NULL) != 0
		||pthread_cond_init(&tp_p->jobqueue.queue_not_empty,NULL) != 0)
		return FALSE;
	
	return TRUE;
}
//清空队列
void thpool_jobqueue_empty(thpool_t* tp_p)
{
	thpool_job_t* curjob;
	curjob = tp_p->jobqueue.tail;
	
	while(tp_p->jobqueue.jobN)
	{
		tp_p->jobqueue.tail = curjob->prev;
		free (curjob);
		curjob = tp_p->jobqueue.tail;
		(tp_p->jobqueue.jobN)--;
	}
	tp_p->jobqueue.head = NULL;
	tp_p->jobqueue.tail = NULL;
	
	pthread_mutex_destroy(&tp_p->jobqueue.queue_lock);
	pthread_cond_destroy(&tp_p->jobqueue.queue_not_empty);
//	free(tp_p->jobqueue->queue_lock);
//	free(tp_p->jobqueue->queue_not_empty);
}

void thpool_jobqueue_add(thpool_t* tp_p, thpool_job_t* newjob_p)
{
	newjob_p->next = NULL;
	newjob_p->prev = NULL;
	thpool_job_t* oldFirstJob;
	
	oldFirstJob = tp_p->jobqueue.head;
	
	switch(tp_p->jobqueue.jobN)
	{
		case 0:
			tp_p->jobqueue.head = newjob_p;
			tp_p->jobqueue.tail = newjob_p;
			break;
		default:
			oldFirstJob->prev = newjob_p;
			newjob_p->next = oldFirstJob;
			tp_p->jobqueue.head = newjob_p;
	}
	(tp_p->jobqueue.jobN)++;

	return;
}

//返回并移除队列的一个节点
thpool_job_t* thpool_jobqueue_pop(thpool_t* tp_p)
{
	if(tp_p ==NULL)
		return NULL;
	thpool_job_t* theLastJob = tp_p->jobqueue.tail;
	if(thpool_jobqueue_removelast(tp_p) >= 0 )
		return theLastJob;
	else
		return NULL;
}
//删除队列的最后一个节点
int thpool_jobqueue_removelast(thpool_t* tp_p)
{
	if(tp_p ==NULL)
		return -1;
	thpool_job_t* theLastJob;
	theLastJob = tp_p->jobqueue.tail;
	switch(tp_p->jobqueue.jobN)
	{
		case 0:
			return -1;
		case 1:
			tp_p->jobqueue.head =NULL;
			tp_p->jobqueue.tail =NULL;
			break;
		default:
			theLastJob->prev->next = NULL;
			tp_p->jobqueue.tail = theLastJob->prev;		
	}
	(tp_p->jobqueue.jobN)--;

	return 0;	
}
//获取第一个队列的一个节点
thpool_job_t* thpool_jobqueue_pick(thpool_t* tp_p)
{
	if(tp_p == NULL)
		return NULL;
	thpool_job_t* theLastJob = tp_p->jobqueue.tail;
	return theLastJob;
}
