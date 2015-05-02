#include	"gas.inc"
Log_Writer WARN_W;
Log_Writer INFO_W;
static __thread  char tls_buffer[_LOG_BUFFSIZE];

int log_init(LogLevel l, const char* p_modulename, const char* p_logdir)
{
	//如果路径存在文件夹，则判断是否存在
	if (access (p_logdir, 0) == -1)
	{
		if (mkdir (p_logdir, S_IRUSR | S_IWUSR |S_IRGRP |S_IWGRP |S_IXUSR|S_IXGRP) < 0)
			fprintf(stderr, "create folder failed\n");
	}
	char _location_str[_LOG_PATH_LEN];
	snprintf(_location_str, _LOG_PATH_LEN, "%s/%s.access", p_logdir, p_modulename);	
	loginit(&INFO_W,l, _location_str,1,0);
	snprintf(_location_str, _LOG_PATH_LEN, "%s/%s.error", p_logdir, p_modulename);
	//warning级别以上日志去WARN_W  去向由宏决定的 请见macro_define.h
	if(l > LL_WARNING)
		loginit(&WARN_W,l, _location_str,1,0);
	else
		loginit(&WARN_W,LL_WARNING, _location_str,1,0);
	return TRUE;
}

const char* logLevelToString(LogLevel l) {
        switch ( l ) {
			case LL_DEBUG:
				return "DEBUG";
			case LL_TRACE:
				return "TRACE";
			case LL_NOTICE:
				return "NOTICE";
			case LL_WARNING:
				return "WARN" ;
			case LL_ERROR:
				return "ERROR";
			default:
				return "UNKNOWN";
        }
}
	
int checklevel(Log_Writer * lw,LogLevel l)
{
	if(l >= lw->m_system_level)
		return TRUE;
	else
		return FALSE;
}

int loginit(Log_Writer *lw,LogLevel l, const  char *filelocation, int append, int issync)
{
	MACRO_RET(NULL == lw, FALSE);
	MACRO_RET(NULL != lw->fp, FALSE);
    lw->m_system_level = l;
    lw->m_isappend = append; 
    lw->m_issync = issync; 
	if(strlen(filelocation) >= (sizeof(lw->m_filelocation) -1))
	{
		fprintf(stderr, "the path of log file is too long:%d limit:%d\n", strlen(filelocation), sizeof(lw->m_filelocation) -1);
		exit(0);
	}
	//本地存储filelocation  以防止在栈上的非法调用调用
	strncpy(lw->m_filelocation, filelocation, sizeof(lw->m_filelocation));
	lw->m_filelocation[sizeof(lw->m_filelocation) -1] = '\0';
	
	if('\0' == lw->m_filelocation[0])
	{
		lw->fp = stdout;
		fprintf(stderr, "now all the running-information are going to put to stderr\n");
		return TRUE;
	}
	
	lw->fp = fopen(lw->m_filelocation, append ? "a":"w");
	if(lw->fp == NULL)
	{
		fprintf(stderr, "cannot open log file,file location is %s\n", lw->m_filelocation);
		exit(0);
	}
	//setvbuf (fp, io_cached_buf, _IOLBF, sizeof(io_cached_buf)); //buf set _IONBF  _IOLBF  _IOFBF
	setvbuf (lw->fp,  (char *)NULL, _IOLBF, 0);
	fprintf(stderr, "now all the running-information are going to the file %s\n", lw->m_filelocation);
	return TRUE;
}

int premakestr(Log_Writer *lw,char* m_buffer, LogLevel l)
{
  time_t now;
	now = time(&now);;
	struct tm vtm; 
  localtime_r(&now, &vtm);
  return snprintf(m_buffer, _LOG_BUFFSIZE, "%-6s: %d%02d%02d|%02d:%02d:%02d|", logLevelToString(l),
            1900+vtm.tm_year,vtm.tm_mon + 1, vtm.tm_mday, vtm.tm_hour, vtm.tm_min, vtm.tm_sec);
}

int logWrite(Log_Writer *lw,LogLevel l, char* logformat,...)
{
	MACRO_RET(NULL == lw, FALSE);
	MACRO_RET(!checklevel(lw,l), FALSE);
	int _size;
	int prestrlen = 0;
	
	char * star = tls_buffer;
	prestrlen = premakestr(lw,star, l);
	star += prestrlen;
	
	va_list args;
	va_start(args, logformat);
	_size = vsnprintf(star, _LOG_BUFFSIZE - prestrlen, logformat, args);
	va_end(args);
	
	if(NULL == lw->fp)
		fprintf(stderr, "%s", tls_buffer);
	else
		_write(lw,tls_buffer, prestrlen + _size);
	return TRUE;
}

int logWrite2(LogLevel l, char* logformat,...)
{
	Log_Writer *lw;
	if(l>=LL_WARNING)
		lw = &WARN_W;
	else
		lw = &INFO_W;
		
	MACRO_RET(NULL == lw, FALSE);
	MACRO_RET(!checklevel(lw,l), FALSE);
	int _size;
	int prestrlen = 0;
	
	char * star = tls_buffer;
	prestrlen = premakestr(lw,star, l);
	star += prestrlen;
	
	va_list args;
	va_start(args, logformat);
	_size = vsnprintf(star, _LOG_BUFFSIZE - prestrlen, logformat, args);
	va_end(args);
	
	if(NULL == lw->fp)
		fprintf(stderr, "%s", tls_buffer);
	else
		_write(lw,tls_buffer, prestrlen + _size);
	return TRUE;
}

int _write(Log_Writer *lw,char *_pbuffer, int len)
{
	if(0 != access(lw->m_filelocation, W_OK))
	{	
		pthread_mutex_lock(&lw->m_mutex);
		//锁内校验 access 看是否在等待锁过程中被其他线程loginit了  避免多线程多次close 和init
		if(0 != access(lw->m_filelocation, W_OK))
		{
			logclose(lw);
			loginit(lw,lw->m_system_level, lw->m_filelocation, lw->m_isappend, lw->m_issync);
		}
		pthread_mutex_unlock(&lw->m_mutex);
	}

	if(1 == fwrite(_pbuffer, len, 1, lw->fp)) //only write 1 item
	{
		if(lw->m_issync)
          	fflush(lw->fp);
		*_pbuffer='\0';
    }
    else 
	{
        int x = errno;
	    fprintf(stderr, "Failed to write to logfile. errno:%s    message:%s", strerror(x), _pbuffer);
	    return FALSE;
	}
	return TRUE;
}

LogLevel get_level(Log_Writer *lw)
{
	return lw->m_system_level; 
}

int logclose(Log_Writer *lw)
{
	if(lw->fp == NULL)
		return FALSE;
	fflush(lw->fp);
	fclose(lw->fp);
	lw->fp = NULL;
	return TRUE;
}



