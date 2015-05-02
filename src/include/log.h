#ifndef   _MACRO_LogModule
#define   _MACRO_LogModule
#include	"gas.inc"
/* 每个线程的buffer size*/
#define   _LOG_BUFFSIZE  1024*1024*4
/* 当前进程的 Stream IO buffer size*/
#define   _SYS_BUFFSIZE  1024*1024*8
/* log 文件字符串路径最大长度*/
#define	  _LOG_PATH_LEN  250
/* 日志对应的模块名*/
#define   _LOG_MODULE_LEN 32


typedef  enum _LogLevel
{  
	LL_DEBUG = 1,
	LL_TRACE = 2,
	LL_NOTICE = 3, 
	LL_WARNING = 4, 
	LL_ERROR = 5,
}LogLevel;

/**
*	Log_Writer  日志类
*/

/**
 * LogLevel 日志级别
 * p_modulename 模块名 如mysql
 * p_logdir  日志输出目录
 * */
int log_init(LogLevel l, const char* p_modulename, const char* p_logdir);


typedef  struct _Log_Writer
{  
		LogLevel m_system_level;
		FILE* fp;
		int m_issync;
		int m_isappend;
		char m_filelocation[_LOG_PATH_LEN];
		pthread_mutex_t m_mutex;
//		__thread  static char m_buffer[_LOG_BUFFSIZE];
}Log_Writer;


	
int loginit(Log_Writer *,LogLevel l, const  char *filelocation, int append, int issync);
int logWrite(Log_Writer *,LogLevel l,char *logformat,...);
int logWrite2(LogLevel l,char *logformat,...);
LogLevel get_level(Log_Writer *);
int logclose(Log_Writer *);
		
const char* logLevelToString(LogLevel l);
int checklevel(Log_Writer *,LogLevel l);
int premakestr(Log_Writer *,char* m_buffer, LogLevel l);
int _write(Log_Writer *,char *_pbuffer, int len);

extern Log_Writer WARN_W;
extern Log_Writer INFO_W;


#endif
