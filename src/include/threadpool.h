#ifndef _THREADPOOL_
#define _THREADPOOL_
#include	"gas.inc"

//	����һ������ڵ�
typedef void* (*FUNC)(void* arg);
typedef struct _thpool_job_t
{    
	FUNC		function;								//����ָ��
	void*		arg;     								//����������
	struct _thpool_job_t* prev;			//ָ����һ���ڵ�
	struct _thpool_job_t* next;			//ָ����һ���ڵ�
}thpool_job_t;

//	����һ����������
typedef struct _thpool_job_queue
{
	thpool_job_t*   head;          //����ͷָ�� 
	thpool_job_t*   tail;			   	//����ĩβָ��
	int             jobN;					//������
	pthread_cond_t	queue_not_empty;
	pthread_mutex_t	queue_lock;
}thpool_jobqueue; 
 
//�̳߳�
typedef struct _thpool_t
{
	int							keepalive;			//�̳߳�״̬
	pthread_t*      threads;    		//�߳�ָ����
	int							threadsN;    		//�߳���
	thpool_jobqueue jobqueue;   		//ָ�����ָ��
}thpool_t;
 
//typedef struct thread_data
//{                            
//	pthread_mutex_t *mutex_p;
//	thpool_t        *tp_p;
//}thread_data;

 //��ʼ���̳߳��ڲ����߳���
int				thpool_init(thpool_t*,int threadN);
//�����̳߳�
void 			thpool_destroy(thpool_t* tp_p);
//�������
int 			thpool_add_work(thpool_t* tp_p, void *(*function_p)(void*), void* arg_p);


//�����߳�
void thpool_thread_do(void* tp_p);

//��ʼ���������
int 	thpool_jobqueue_init(thpool_t* tp_p);
//����������
void	thpool_jobqueue_empty(thpool_t* tp_p);
//��������
void	thpool_jobqueue_add(thpool_t* tp_p, thpool_job_t* newjob_p);
//ɾ����β�ڵ�
int		thpool_jobqueue_removelast(thpool_t* tp_p);
//��ȡ��ɾ����β�ڵ�
thpool_job_t* thpool_jobqueue_pop(thpool_t* tp_p);
//��ȡ��β�ڵ�
thpool_job_t* thpool_jobqueue_pick(thpool_t* tp_p);

#endif
