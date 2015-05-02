#ifndef _THREADPOOL_
#define _THREADPOOL_
#include	"gas.inc"

//	定义一个任务节点
typedef void* (*FUNC)(void* arg);
typedef struct _thpool_job_t
{    
	FUNC		function;								//函数指针
	void*		arg;     								//函数参数。
	struct _thpool_job_t* prev;			//指向上一个节点
	struct _thpool_job_t* next;			//指向下一个节点
}thpool_job_t;

//	定义一个工作队列
typedef struct _thpool_job_queue
{
	thpool_job_t*   head;          //队列头指针 
	thpool_job_t*   tail;			   	//队列末尾指针
	int             jobN;					//任务数
	pthread_cond_t	queue_not_empty;
	pthread_mutex_t	queue_lock;
}thpool_jobqueue; 
 
//线程池
typedef struct _thpool_t
{
	int							keepalive;			//线程池状态
	pthread_t*      threads;    		//线程指针数
	int							threadsN;    		//线程数
	thpool_jobqueue jobqueue;   		//指向队列指针
}thpool_t;
 
//typedef struct thread_data
//{                            
//	pthread_mutex_t *mutex_p;
//	thpool_t        *tp_p;
//}thread_data;

 //初始化线程池内部的线程数
int				thpool_init(thpool_t*,int threadN);
//销毁线程池
void 			thpool_destroy(thpool_t* tp_p);
//添加任务
int 			thpool_add_work(thpool_t* tp_p, void *(*function_p)(void*), void* arg_p);


//工作线程
void thpool_thread_do(void* tp_p);

//初始化任务队列
int 	thpool_jobqueue_init(thpool_t* tp_p);
//清空任务队列
void	thpool_jobqueue_empty(thpool_t* tp_p);
//增加任务
void	thpool_jobqueue_add(thpool_t* tp_p, thpool_job_t* newjob_p);
//删除队尾节点
int		thpool_jobqueue_removelast(thpool_t* tp_p);
//获取并删除队尾节点
thpool_job_t* thpool_jobqueue_pop(thpool_t* tp_p);
//获取队尾节点
thpool_job_t* thpool_jobqueue_pick(thpool_t* tp_p);

#endif
