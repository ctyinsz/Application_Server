/************************************************************************
版权所有：杭州恒生电子股份有限公司
项目名称：公共项目
版    本：V1.0
操作系统：AIX4.2,SCO5
文件名称：trace.c
文件描述：简单XML操作函数库辅助跟踪函数
项 目 组：中间业务产品研发组
程 序 员：中间业务产品研发组
发布日期：2001年09月11日
修改日期：2002年05月12日
************************************************************************/
/**********************************************************************
			可根据指定文件长度、每天特定时间、每周特定日期、
			每月特定日期、文件记录总时长等要求完成自动备份

 **********************************************************************/

#include	"trace_.h"

#include	<ctype.h>
#include	<string.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<time.h>

#ifdef  SUN
#ifndef __STDC__
#define __STDC__    /* SunOs5.8 stdarg.h*/
#endif
#endif
#include    <stdarg.h>

#include	<sys/types.h>
#include	<sys/stat.h>


#ifdef _WIN32
#include	<direct.h>
#include	<io.h>
#define		FILE_SFLAG	'\\'
#define mkdir_(a,b)	mkdir(a)
#define	snprintf	_snprintf
#else
#include	<dirent.h>
#define		FILE_SFLAG	'/'
#define mkdir_(a,b)	mkdir(a,b)
#endif


#ifdef __cplusplus
extern "C" {
#endif /* end of __cplusplus */

#ifdef SUCC
#undef SUCC
#endif
#ifdef FAIL
#undef FAIL
#endif

#define	SUCC	(0)
#define	FAIL	(-1)


typedef struct wj_file_trace_struct
{ /* 文件TRACE结构 */
	char	*m_pszFile;			/* 记TRACE的文件 */
	char	*m_pszBakDir;		/* 转移的备份目录 */
	unsigned int m_dwType;		/* 类型 */
	unsigned int m_unMaxByte;	/* 连续记载的最大字节数 */
	unsigned int m_unMaxHour;	/* 连续记载的小时数 */
	time_t	m_tmDayTime;		/* TRACE文件每天转移时间 */
	time_t	m_tmWeekDay;		/* TRACE文件每周转移时间 */
	time_t	m_tmMonthDay;		/* TRACE文件每月转移时间 */

	FILE	*m_fpOpen;			/* 打开的文件流设备 */
	unsigned char m_unFileIdx;	/* 当前备份文件顺序号 */
	time_t	m_tmLastMove;		/* 最后一次移动的时间 */
}FILETRACE,*PFILETRACE;


/* 结构内部属性 */
#define	TFA_ALWAYSOPEN	0x00000001 /* 文件是否一直打开 */
#define	TFA_FLUSH		0x00000002 /* 文件是否无缓冲（立即刷新）*/
#define	TFA_MAXSIZE		0x00000010 /* 文件记录最大长度 */
#define	TFA_MAXHOUR		0x00000020 /* 文件记录最大小时数 */
#define	TFA_DAYTIME		0x00000040 /* 记录文件每天转移时间 */
#define	TFA_WEEKDAY		0x00000080 /* 记录文件每周转移时间 */
#define	TFA_MONTHDAY	0x00000100 /* 记录文件每月转移时间 */



/***************************************************************
 *	函数:	trc_getidxnum
 *	功能:	找出指定目录下指定时间特定名称TRACE的索引最大值
 *	参数:
	- pszDir		TRACE所在的备份目录
	- pszBakDir		TRACE名
	- ptm			指定时间指针
 *	返回值:
	- 成功	返回最大序号值
 **************************************************************/
#ifdef _WIN32
static int trc_getidxnum( char *pszDir, char *pszFile, struct tm *ptm )
{
struct _finddata_t  fdata;
long	fd;
char	szDir[128],szFile[128],*p;
int		len;
int		idx,n;

	getcwd(szDir,sizeof(szDir));
	chdir(pszDir);


	if ( p=strrchr(pszFile,'.') )
	{
		len = p-pszFile;
		memcpy(szFile,pszFile,len);
		len+=snprintf(szFile+len,sizeof(szFile)-len,"_%04d%02d%02d_*",ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday)-1;
	}
	else
	{
		len=snprintf(szFile,sizeof(szFile),"%s_%04d%02d%02d_*",pszFile,ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday)-1;
	}
	if ( (fd=_findfirst(szFile,&fdata))==-1 )
	{
		chdir(szDir);
		return	0;
	}

	if ( p=strrchr( szFile, FILE_SFLAG ) )
	{
		len = strlen(p+1);
	}

	idx=atoi(&fdata.name[len]);

	while ( !_findnext(fd,&fdata) )
	{
		if ( (n=atoi(&fdata.name[len])) > idx )
			idx=n;
	}
	_findclose(fd);
	chdir(szDir);
	return	idx;
}
#else
static int trc_getidxnum( char *pszDir, char *pszFile, struct tm *ptm )
{
DIR		*dp;
struct dirent *dirp;
int		len,idx,n;
char	szFile[128],*p,*pName;

	idx = 0;

	if ( p=strrchr(pszFile,'.') )
	{
		len = p-pszFile;
		memcpy(szFile,pszFile,len);
		len+=snprintf(szFile+len,sizeof(szFile)-len,"_%04d%02d%02d_",ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday);
	}
	else
	{
		len=snprintf(szFile,sizeof(szFile),"%s_%04d%02d%02d_",pszFile,ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday);
	}

	if ( p=strrchr( szFile, FILE_SFLAG ) )
		len = strlen(++p);
	else
		p = szFile;

	if ( !(dp=opendir(pszDir)) )
		return	0;

	while ( dirp=readdir(dp) )
	{
		pName = dirp->d_name;
#ifdef  SUN
		pName = dirp->d_name-2;
#endif
/*		printf("readdir file name=[%s]\n",pName); */
		if ( !memcmp(pName,p,len) )
		{
/*			printf("file name=[%s]\n",pName); */
			if ( (n=atoi(&pName[len]))>idx )
				idx = n;
		}
	}
	closedir(dp);
	return	idx;
}
#endif

/***************************************************************
 *	函数:	trc_Build
 *	功能:	构造TRACE结构
 *	参数:
	- pszFile		记载日志的文件名
	- pszBakDir	备份目录
 *	返回值:
	- 成功	返回设备句柄
	- 失败	返回FAIL
 *	备注:	缺省属性为文件常打开,其它均不设置
 **************************************************************/
int trc_Build( char* pszFile, char *pszBakDir )
{
PFILETRACE	pTrace;
int		n;
char	*p;
char	szDir[128];

	if ( !pszFile || !*pszFile )
	{
		return	FAIL;
	}

	if ( !(pTrace=(PFILETRACE)calloc(1,sizeof(FILETRACE))) )
	{
		return	FAIL;
	}

	if ( !(pTrace->m_pszFile=strdup(pszFile) ) )
	{
		free(pTrace);
		return	FAIL;
	}

	if ( pszBakDir && *pszBakDir)
	{
		p = pszBakDir;
	}
	else
	{
		getcwd(szDir,sizeof(szDir));
		p = szDir;
	}
	if ( !(pTrace->m_pszBakDir=strdup(p) ) )
	{
		free(pTrace->m_pszFile);
		free(pTrace);
	}
	n = strlen(pTrace->m_pszBakDir);
	if ( pTrace->m_pszBakDir[n]==FILE_SFLAG )
		pTrace->m_pszBakDir[n]='\0';

	/* 检查目录是否存在，不存在建立 */
	if ( access(pTrace->m_pszBakDir,0)==-1 )
	{
		if ( mkdir_(pTrace->m_pszBakDir,0700|0050|0005)==-1 )
		{
			free(pTrace->m_pszBakDir);
			free(pTrace->m_pszFile);
			free(pTrace);
			return	FAIL;
		}
	}

	pTrace->m_dwType = TFA_ALWAYSOPEN;
	if ( ! (pTrace->m_fpOpen=fopen(pszFile,"ab")) )
	{
		if ( pTrace->m_pszBakDir )
			free(pTrace->m_pszBakDir);
		free(pTrace->m_pszFile);
		free(pTrace);
		return FAIL;
	}

	pTrace->m_tmLastMove = time(NULL);
	pTrace->m_unFileIdx = trc_getidxnum(pTrace->m_pszBakDir,pTrace->m_pszFile,localtime(&pTrace->m_tmLastMove))+1;
	return	(HTRACE)pTrace;
}


/***************************************************************
 *	函数:	trc_Destroy
 *	功能:	销毁TRACE结构
 *	参数:
	- hTrace		TRACE结构句柄
 *	返回值:	
	- 成功	返回SUCC
	- 失败	返回FAIL
 **************************************************************/
int trc_Destroy( HTRACE hTrace )
{
PFILETRACE	pTrace;

	if ( !hTrace )
		return	FAIL;

	pTrace = (PFILETRACE)hTrace;

	if ( pTrace->m_pszFile )
	{
		free( pTrace->m_pszFile );
	}

	if ( pTrace->m_pszBakDir )
	{
		free( pTrace->m_pszBakDir );
	}

	if ( pTrace->m_fpOpen )
		fclose( pTrace->m_fpOpen );

	free(pTrace);
	return	SUCC;
}


/***************************************************************
 *	函数:	trc_SetAttr
 *	功能:	设置TRACE结构属性
 *	参数:
	- hTrace		TRACE结构句柄
	- iAttrType		属性类别
		TFS_ALWAYSOPEN	文件是否一直打开
			非0, 打开;  0,不打开
		TFS_FLUSH		文件是否无缓冲（立即刷新）
			非0, 是;	0,否
		TFS_MAXSIZE		文件记录最大长度(以字节计算)
			范围 1-4294967295(unsigned int的最大值)
		TFS_MAXDAY		文件记录最大小时数
			范围 1-4294967295(unsigned int的最大值)
		TFS_DAYTIME		记录文件每天转移时间
			格式为数字hhmmss, 如: 130440表示下午1点零4分40秒
			范围 000001-235959
		TFS_WEEKDAY		记录文件每周转移时间
			范围 1-7, 7表示星期日
		TFS_MONTHDAY	记录文件每月转移时间
			范围 1-31
	- uiAttrData	属性数据
		如果该参数不为0, 设置相应类别属性
		如果该参数为0, 则清除相应类别属性
 *	返回值:	
	- 成功	返回SUCC
	- 失败	返回FAIL
 **************************************************************/
int trc_SetAttr( HTRACE hTrace, int iAttrType, unsigned int uiAttrData )
{
PFILETRACE	pTrace;
struct tm *ptmWhen;
time_t		tmNow;
   
	if ( !hTrace )
		return	FAIL;

	pTrace = (PFILETRACE)hTrace;

	switch ( iAttrType )
	{
	case	TFS_ALWAYSOPEN: /* 文件是否一直打开 */
		if ( pTrace->m_dwType&TFA_ALWAYSOPEN )
		{
			if ( uiAttrData )
				break;
			/* 关闭长打开的属性 */
			fclose(pTrace->m_fpOpen);
			pTrace->m_fpOpen=0;
			pTrace->m_dwType&=(~TFA_ALWAYSOPEN);
		}
		else
		{
			if ( !uiAttrData )
				break;
			if ( ! (pTrace->m_fpOpen=fopen(pTrace->m_pszFile,"ab")) )
			{
				return FAIL;
			}
			pTrace->m_dwType|=TFA_ALWAYSOPEN;
		}
		break;

	case	TFS_FLUSH:		/* 文件是否无缓冲（立即刷新）*/
		if ( pTrace->m_dwType&TFA_FLUSH )
		{
			if ( uiAttrData )
				break;
			pTrace->m_dwType&=(~TFA_FLUSH);
		}
		else
		{
			if ( !uiAttrData )
				break;
			pTrace->m_dwType|=TFA_FLUSH;
		}
		break;

	case	TFS_MAXSIZE:	/* 文件记录最大长度 */
		if ( pTrace->m_dwType&TFA_MAXSIZE )
		{
			if ( uiAttrData )
			{ /* 修改 */
				pTrace->m_unMaxByte=uiAttrData;
			}
			else
			{ /* 清除 */
				pTrace->m_dwType&=(~TFA_MAXSIZE);
				pTrace->m_unMaxByte=0;
			}
		}
		else
		{
			if ( !uiAttrData )
				break;
			pTrace->m_unMaxByte = uiAttrData;
			pTrace->m_dwType|=TFA_MAXSIZE;
		}
		break;

	case	TFS_MAXHOUR:		/* 文件记录最大小时数 */
		if ( pTrace->m_dwType&TFA_MAXHOUR )
		{
			if ( uiAttrData )
			{ /* 修改 */
				pTrace->m_unMaxHour = uiAttrData;
			}
			else
			{ /* 清除 */
				pTrace->m_dwType&=(~TFA_MAXHOUR);
				pTrace->m_unMaxHour = 0;
			}
		}
		else
		{
			if ( !uiAttrData )
				break;
			pTrace->m_unMaxHour = uiAttrData;
			pTrace->m_dwType|=TFA_MAXHOUR;
		}
		break;

	case	TFS_DAYTIME:	/* 记录文件每天转移时间 */
		if ( uiAttrData>235959 )
			return	FAIL;
		if ( pTrace->m_dwType&TFA_DAYTIME )
		{
			if ( !uiAttrData )
			{ /* 清除 */
				pTrace->m_dwType&=(~TFA_DAYTIME);
				pTrace->m_tmDayTime = 0;
				return	SUCC;
			}
		}
		else
		{
			if ( !uiAttrData )
				return	SUCC;
			pTrace->m_dwType|=TFA_DAYTIME;
		}

		/* 设置每天转移时间 */
		tmNow = time(NULL);
		ptmWhen = localtime(&tmNow);
		ptmWhen->tm_hour=uiAttrData/10000;
		ptmWhen->tm_min = uiAttrData%10000/100;
		ptmWhen->tm_sec = uiAttrData%100;
		pTrace->m_tmDayTime = mktime(ptmWhen);
		if ( tmNow > pTrace->m_tmDayTime )
			pTrace->m_tmDayTime+=86400;
		break;

	case	TFS_WEEKDAY:	/* 记录文件每周转移时间 */
		if ( uiAttrData>7 )
			return	FAIL;
		if ( pTrace->m_dwType&TFA_WEEKDAY )
		{
			if ( !uiAttrData )
			{ /* 清除 */
				pTrace->m_dwType&=(~TFA_WEEKDAY);
				pTrace->m_tmWeekDay = 0;
			}
		}
		else
		{
			if ( !uiAttrData )
				return	SUCC;
			pTrace->m_dwType|=TFA_WEEKDAY;
		}

		/* 设置每周转移日期 */
		if ( uiAttrData==7 )
			uiAttrData=0;
		tmNow = time(NULL);
		ptmWhen = localtime(&tmNow);
		if ( ptmWhen->tm_wday >= (int)uiAttrData )
			ptmWhen->tm_mday+=7;
		ptmWhen->tm_mday += uiAttrData-ptmWhen->tm_wday;
		ptmWhen->tm_hour = 0;
		ptmWhen->tm_min  = 0;
		ptmWhen->tm_sec  = 0;
		pTrace->m_tmWeekDay = mktime(ptmWhen);
		break;

	case	TFS_MONTHDAY:	/* 记录文件每月转移时间 */
		if ( uiAttrData>31 )
			return	FAIL;
		if ( pTrace->m_dwType&TFA_MONTHDAY )
		{
			if ( !uiAttrData )
			{ /* 清除 */
				pTrace->m_dwType&=(~TFA_MONTHDAY);
				pTrace->m_tmMonthDay = 0;
			}
		}
		else
		{
			if ( !uiAttrData )
				return	SUCC;
			pTrace->m_dwType|=TFA_MONTHDAY;
		}
		/* 设置每月转移日期 */
		tmNow = time(NULL);
		ptmWhen = localtime(&tmNow);
		if ( ptmWhen->tm_mday >= (int)uiAttrData )
			ptmWhen->tm_mon++;
		ptmWhen->tm_mday = uiAttrData;
		ptmWhen->tm_hour = 0;
		ptmWhen->tm_min  = 0;
		ptmWhen->tm_sec  = 0;
		pTrace->m_tmMonthDay = mktime(ptmWhen);
		break;

	default:
		return	FAIL;
	}
	return	SUCC;
}


/***************************************************************
 *	函数:	trc_GetAttr
 *	功能:	获取TRACE结构属性
 *	参数:
	- hTrace		TRACE结构句柄
	- iAttrType		属性类别
		TFS_ALWAYSOPEN 文件是否一直打开
		TFS_FLUSH		文件是否无缓冲（立即刷新）
		TFS_MAXSIZE		文件记录最大长度
		TFS_MAXDAY		文件记录最大小时数
		TFS_DAYTIME		记录文件每天转移时间
		TFS_WEEKDAY		记录文件每周转移时间
		TFS_MONTHDAY	记录文件每月转移时间
	- uiAttrData	接收属性数据指针
		如果该参数返回为0, 则说明没有设置相应类别属性
 *	返回值:	
	- 成功	返回SUCC
	- 失败	返回FAIL
 **************************************************************/
int trc_GetAttr( HTRACE hTrace, int iAttrType, unsigned int *puiAttrData )
{
PFILETRACE	pTrace;
   
	if ( !hTrace && !puiAttrData )
		return	FAIL;

	pTrace = (PFILETRACE)hTrace;

	switch ( iAttrType )
	{
	case	TFS_ALWAYSOPEN: /* 文件是否一直打开 */
		*puiAttrData = pTrace->m_dwType&TFA_ALWAYSOPEN?1:0;
		break;

	case	TFS_FLUSH:		/* 文件是否无缓冲（立即刷新）*/
		*puiAttrData = pTrace->m_dwType&TFA_FLUSH?1:0;
		break;

	case	TFS_MAXSIZE:	/* 文件记录最大长度 */
		*puiAttrData = pTrace->m_unMaxByte;
		break;

	case	TFS_MAXHOUR:		/* 文件记录最大小时数 */
		*puiAttrData = pTrace->m_unMaxHour;
		break;

	case	TFS_DAYTIME:	/* 记录文件每天转移时间 */
		{struct tm *ptm;
		ptm = localtime(&pTrace->m_tmMonthDay);
		*puiAttrData = ptm->tm_hour*10000+ptm->tm_min*100+ptm->tm_sec;
		}
		break;

	case	TFS_WEEKDAY:	/* 记录文件每周转移时间 */
		if ( !(*puiAttrData=localtime(&pTrace->m_tmWeekDay)->tm_wday) )
			*puiAttrData=7;
		break;

	case	TFS_MONTHDAY:	/* 记录文件每月转移时间 */
		*puiAttrData = localtime(&pTrace->m_tmMonthDay)->tm_mday;
		break;

	default:
		return	FAIL;
	}
	return	SUCC;
}

/***************************************************************
 *	函数:	trc_MoveFile
 *	功能:	强制完成TRACE文件移动
 *	参数:
	- hTrace		TRACE结构句柄
 *	返回值:	
	- 成功	返回记录的字节数
	- 失败	返回FAIL
 **************************************************************/
int trc_MoveFile( HTRACE hTrace )
{
PFILETRACE	pTrace;
char	szNewFile[256];
char	*p;
int		n;
time_t	tmNow;
struct tm *ptm;

	if ( !hTrace )
		return	FAIL;
	pTrace = (PFILETRACE)hTrace;

	tmNow=time(NULL);
	if ( tmNow/86400 != pTrace->m_tmLastMove/86400 )
	{
		pTrace->m_tmLastMove = tmNow;
		pTrace->m_unFileIdx = 1;
	}
	ptm = localtime(&tmNow);

	if ( pTrace->m_fpOpen )
		fclose(pTrace->m_fpOpen);

	if (p=strrchr(pTrace->m_pszFile,'.') )
	{
		n = snprintf(szNewFile,sizeof(szNewFile),"%s%c",pTrace->m_pszBakDir,FILE_SFLAG);
		memcpy(szNewFile+n,pTrace->m_pszFile,p-pTrace->m_pszFile);
		n += (p-pTrace->m_pszFile);
		n += snprintf(szNewFile+n,sizeof(szNewFile)-n,"_%04d%02d%02d_%03d",ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday,pTrace->m_unFileIdx);
		strcpy(szNewFile+n,p);
	}
	else
	{
		p = strrchr( pTrace->m_pszFile, FILE_SFLAG );
		sprintf(szNewFile,"%s%s_%04d%02d%02d_%03d",pTrace->m_pszBakDir,p,ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday,pTrace->m_unFileIdx);
	}
	
	rename(pTrace->m_pszFile,szNewFile);

	if ( pTrace->m_dwType&TFA_ALWAYSOPEN )
	{ /* 打开文件 */
		if ( ! (pTrace->m_fpOpen=fopen(pTrace->m_pszFile,"ab")) )
		{
			pTrace->m_fpOpen=0;
			return	FAIL;
		}
	}

	pTrace->m_unFileIdx++;

	return	SUCC;
}

static int vfprintf_( FILE *stream, char *szFormat, va_list pArg );

/***************************************************************
 *	函数:	trc_Write
 *	功能:	记录TRACE
 *	参数:
	- hTrace		TRACE结构句柄
	- pszFmt		记录格式
	- ...			格式对应内容
 *	返回值:	
	- 成功	返回记录的字节数
	- 失败	返回FAIL
 **************************************************************/
int trc_Write( HTRACE hTrace, char *pszFmt, ... )
{
PFILETRACE	pTrace;
FILE	*fp;
struct stat fs;

va_list	pArg;
int		num;
struct tm *ptmWhen;

	if ( !hTrace )
		return	FAIL;

	pTrace = (PFILETRACE)hTrace;

	if ( stat( pTrace->m_pszFile, &fs )==(-1) )
	{
		return	FAIL;
	}

	if ( fs.st_size )
	{
		if ( pTrace->m_dwType&TFA_MAXHOUR
			&& (unsigned int)(time(NULL)-(unsigned int)fs.st_ctime)/3600 > pTrace->m_unMaxHour )
		{ /* 超过最大小时数,移动TRACE文件 */
			trc_MoveFile(hTrace);
		}
		else if ( pTrace->m_dwType&TFA_DAYTIME
			&& time(NULL) > pTrace->m_tmDayTime )
		{ /* 达到每天转移时间*/
			pTrace->m_tmDayTime+=86400;
			trc_MoveFile(hTrace);
		}
		else if ( pTrace->m_dwType&TFA_WEEKDAY
			&& time(NULL) > pTrace->m_tmWeekDay )
		{ /* 达到每周转移时间*/
			ptmWhen = localtime(&pTrace->m_tmWeekDay);
			ptmWhen->tm_mday+=7;
			pTrace->m_tmWeekDay = mktime(ptmWhen);
			trc_MoveFile(hTrace);
		}
		else if ( pTrace->m_dwType&TFA_MONTHDAY
			&& time(NULL) > pTrace->m_tmMonthDay )
		{ /* 达到每月转移时间*/
			ptmWhen = localtime(&pTrace->m_tmMonthDay);
			ptmWhen->tm_mon++;
			pTrace->m_tmMonthDay = mktime(ptmWhen);
			trc_MoveFile(hTrace);
		}

	}

	if ( pTrace->m_fpOpen )
		fp = pTrace->m_fpOpen;
	else if ( !(fp=fopen(pTrace->m_pszFile,"ab")) )
	{
		return	FAIL;
	}

	va_start( pArg, pszFmt );
		num = vfprintf_( fp, pszFmt, pArg );
	va_end( pArg );


	if ( pTrace->m_dwType & TFA_FLUSH )
		fflush(fp);

	if ( !pTrace->m_fpOpen )
		fclose(fp);

	/* 检查大小 */
	if ( pTrace->m_dwType&TFA_MAXSIZE &&
		(unsigned int)fs.st_size+num > pTrace->m_unMaxByte )
	{ /* 超过最大字节数,移动TRACE文件 */
		trc_MoveFile(hTrace);
	}

	return	num;
}









/*************************************************************************************
 *	函数：	trc_GetVerion
 *	功能:	获取版本号
 *	参数:	无
 *	返回值: 版本号 1.0
		高字节为大版本号
		低字节为顺序号
 *************************************************************************************/
#define MAKELONG_(a, b)      ((unsigned int)(((unsigned short)(a)) | ((unsigned int)((unsigned short)(b))) << 16))
unsigned int trc_GetVerion()
{
	return	MAKELONG_(1,0);
}

/*************************************************************************************
 *	函数：	trc_GetVerion
 *	功能:	获取最后版本日期
 *	参数:	无
 *	返回值: 版本日期串, 格式为 "yyyy-mm-dd"
  *************************************************************************************/
char *trc_GetLastVerData()
{
	return	"2001-09-17";
}


typedef struct _HeadFmt_{ /* 格式头结构 */
	char	m_szFmt[64]; /* 头格式字串,用于sprintf格式字串参数 */
	int		m_nStartRow; /* 起始行,用于sprintf第一个可变参数 */
	int		m_nHeadBytes;/* 格式化子串后的头字节数 */
}_HEADFMT_,*_PHEADFMT_;

/**********************************************************************
 *	函数:	_mkhead_(内部使用)
 *	功能:	分解转换头地址格式字串
 *	参数一:	接收转化后格式的头部结构指针
 *	参数五:	源地址格式字串 [直接显示字符][地址位数][.起始行数][直接显示字符]
		NULL	不设地址
		如:		"[8.0]" 8位地址格式从头开始
 *	返回值:	返回0
 **********************************************************************/
static int _mkhead_( _PHEADFMT_ pHead, char* psInpData )
{
int	num=0;
int	size=64;
char	*p;

	if ( !pHead )
		return	(-1);

	p = pHead->m_szFmt;
	pHead->m_nStartRow = 0;

	while ( size && *psInpData && ( *psInpData<'0'||*psInpData>'9') ) {
		*p++ = *psInpData++;
		size--;	num++;
	}

	if ( !*psInpData || size<2 ) {
		*p='\0';
		return	num;
	}

	*p++ = '%';
	*p++ = '0';
	size -= 2;
	num += atoi(psInpData);

	while ( size && *psInpData>='0' && *psInpData<='9' ){
		*p++ = *psInpData++;
		size--;
	}
	if ( size ) {
		*p++ ='X';
		size--;
	}

	if ( *psInpData == '.' ) { /* 指定开始地址值 */
		psInpData++;
		pHead->m_nStartRow = atoi(psInpData);
		while ( *psInpData>='0' && *psInpData<='9' )
			psInpData++;
	}

	while( size && *psInpData ) {
		*p++ = *psInpData++;
		size--;num++;
	}
	if ( !size ) {
		p--;
		num--;
	}
	*p = '\0';
	pHead->m_nHeadBytes = num;
	return	0;
}

/**********************************************************************
 *	函数:	_filedout_(内部使用)
 *	功能:	用 16 进制和字符两种合并形式格式化一块缓冲区内容并输出到文件流
 *	参数一:	打开的文件流指针
 *	参数二:	数据指针
 *	参数三:	数据内容大小
 *	参数五:	头部结构指针
 *	返回值:	成功,返回输出的字节数; 失败,返回-1
 **********************************************************************/
static int _filedout_( FILE *stream, const void *pData, size_t nLen, _PHEADFMT_ pHead )
{
unsigned char *pt,*p;
int	nStep;
int	num;
int	nAddrSt;

	num = 0;
	if ( pHead && pHead->m_nHeadBytes ) {
		nAddrSt = pHead->m_nStartRow<< 4;
		nAddrSt -= 16;
	}

	p = pt = (unsigned char*)pData;
	while ( nLen ) {

		/* 前地址 */
		nStep=16;
		if ( pHead && pHead->m_nHeadBytes )
			num += fprintf( stream, pHead->m_szFmt, nAddrSt+=16 );

		/* 前16个字符 */
		for ( ; nLen && nStep>8; nLen--,nStep--,p++ )
			num += fprintf( stream, "%02X ", *p );

		for ( ; nStep > 8; nStep-- )
			num += fprintf( stream, "   " );

		num += fprintf( stream, " " );

		/* 后16个字符 */
		for ( ; nLen && nStep; nLen--,nStep--,p++ )
			num += fprintf( stream, "%02X ", *p );

		for ( ; nStep; nStep-- )
			num += fprintf( stream, "   " );

		num += fprintf( stream, "  " );

		/* 最后文本字符 */
/*		nStep = p-pt; */
		for ( ; pt < p; pt++ ) {
			if( isprint( *pt ) || ( *pt&0x80 ) && p-pt!=1 )
				num += fprintf( stream, "%c", *pt );
			else
				num += fprintf( stream, "." );
		}

/*		if ( nStep==16 ) */
			num += fprintf( stream, "\n" );
			fflush(stream);
	}
/*	fflush( stream ); */
	return	num;
}


/**********************************************************************
 *	函数:	_bufoutfile_(内部使用)
 *	功能:	缓冲输出数据到文件流
 *	参数一:	打开的输出文件流指针
 *	参数二:	缓冲指针
 *	参数三:	缓冲数据长度指针
 *	参数四:	数据内容
 *	参数五:	数据长度
 *	参数六:	头部结构指针(行数自动增加)
 *	返回值:	返回实际输出的字节数
 **********************************************************************/
static int _bufoutfile_( FILE *stream, char *szOut, int *pnOut, char *pData, int nData, _PHEADFMT_ pHead )
{
int	n=0;

	if ( *pnOut ) { /* 有剩余字符 */
		if ( nData+*pnOut <16 ){ /* 不足拷入缓冲区 */
			memcpy( szOut+*pnOut, pData, nData );
			*pnOut += nData;
			return	0;
		}
		else {
			int	num = 16-*pnOut;
			memcpy( szOut+*pnOut, pData, num );
			n = _filedout_(stream,pData,16,pHead);
			pHead->m_nStartRow++;
			pData += num;
			nData -= num;
		}
	}
	*pnOut = nData%16;
	if ( nData >= 16 )
		n += _filedout_(stream,pData,nData-*pnOut,pHead);

	memcpy( szOut, pData+(nData-*pnOut), *pnOut );
	pHead->m_nStartRow += (nData/16);

	return	n;
}

/**********************************************************************
 *	函数:	vfprintf_
 *	功能:	使用格式参数表输出格式化数据到文件流
 *	参数一:	打开的输出文件流指针
 *	参数二:	数据格式字串
 *	参数三:	格式参数表指针
 *	返回值:	成功,返回输出的字节数; 失败,返回0
 **********************************************************************/
static int vfprintf_( FILE *stream, char *szFormat, va_list pArg )
{
_HEADFMT_ head;
char	szBuf[128];
char	*p,*q;
size_t	n;
char	szOut[16]; /* 缓冲输出池 */
int		nOut=0;    /* 缓冲池数据个数 */
int		num=0;     /* 输出数据字节数 */
char	ucHexFlag=0;/* 是否十六进制输出标记 */

	p = szFormat;

	while ( *szFormat ) {
		if ( *szFormat != '%' ){
			szFormat++;
			continue;
		}

		if ( szFormat != p ) {
			if ( ucHexFlag ) /* 十六进制形式 */
				num += _bufoutfile_( stream, szOut, &nOut, p, szFormat-p, &head );
			else {
				num += (szFormat-p);
				while( p<szFormat )
					fprintf( stream, "%c",*p++ );
			}
		}

		szFormat++;
		switch ( *szFormat ) {
			case	't': /* 加上时间 */
			case	'T': {
				struct	tm *ptm;
				time_t	tm;

				tm = va_arg( pArg, time_t );
				if ( !tm )
					time(&tm);
				ptm = localtime( &tm );

				if ( *(szFormat+1)=='\'' ) { /* 开始格式 */
					char szTmp[128];
					szFormat += 2;
					q = szTmp; n=sizeof(szTmp)-1;
					while ( *szFormat && *szFormat!='\'' && n ) {
						*q++ = *szFormat++;
						n-- ;
					}
					*q = '\0';
					n = strftime( szBuf,sizeof(szBuf),szTmp,ptm );
				}
				else
					n=snprintf( szBuf,sizeof(szBuf),"%04d-%02d-%02d %02d:%02d:%02d",ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);

				if ( ucHexFlag ) /* 十六进制形式 */
					num += _bufoutfile_( stream, szOut, &nOut, szBuf, n, &head );
				else
					num += fprintf( stream,szBuf );
				}
				szFormat++;
				break;
		
			case	'm': /* 内存数据 */
			case	'M':
				if ( !(p = va_arg( pArg, char* )) )
					break;

				if ( !(n = va_arg( pArg, int )) )
					break;

				if ( ucHexFlag ) /* 十六进制形式 */
					num += _bufoutfile_( stream, szOut, &nOut, p, n, &head );
				else
					num += fprintf( stream,p );
				szFormat++;
				break;

			case	's': /* 字符串 */
			case	'S':
				if ( !(p = va_arg( pArg, char* )) )
					break;

				if ( ucHexFlag ) /* 十六进制形式 */
					num += _bufoutfile_( stream, szOut, &nOut, p, strlen(p), &head );
				else
					num += fprintf( stream,p );
				szFormat++;
				break;

			case	'h': /* 开始转为十六进制调试格式 */
				if ( *(szFormat+1)=='\'' ) { /* 开始格式 */
					szFormat += 2;
					q = szBuf; n=sizeof(szBuf)-1;
					while ( *szFormat && *szFormat!='\'' && n ) {
						*q++ = *szFormat++;
						n-- ;
					}
					*q = '\0';
					_mkhead_( &head,szBuf );
				}
				else {
					strcpy(head.m_szFmt,"[%08X]  " );
					head.m_nStartRow  = 0;
					head.m_nHeadBytes = 12;
				}
				ucHexFlag = 1;
				szFormat++;
				break;

			case	'H': /* 结束十六进制调试格式 */
				if ( !ucHexFlag ) /* 非十六进制形式 */
					break;
				if ( nOut ) { /* 有剩余字符 */
					num += _filedout_(stream,szOut,nOut,&head);
					nOut = 0;
				}
				ucHexFlag = 0;
				szFormat++;
				break;

			default:{
				char szFTmp[64];
				p = szFTmp;
				*p++ = '%';
				n=sizeof(szFTmp)-1;
				while ( *szFormat && *szFormat != '%' && *szFormat != '\\' && n-- )
					*p++ = *szFormat++;
				*p='\0';
				n = snprintf( szBuf,sizeof(szBuf),szFTmp, va_arg(pArg,int ) );

				if ( ucHexFlag ) /* 十六进制形式 */
					num += _bufoutfile_( stream, szOut, &nOut, szBuf, n, &head );
				else
					num += fprintf( stream,szBuf );
				}
				break;
		}
		p = szFormat;
	}

	if ( szFormat != p ) {
		n = szFormat - p;
		if ( ucHexFlag ) /* 十六进制形式 */
			num += _bufoutfile_( stream, szOut, &nOut, p, n, &head );
		else
			num += fprintf( stream,p );
	}

	if ( nOut ) /* 有剩余字符 */
		num += _filedout_(stream,szOut,nOut,&head);

	return	num;
}


#ifdef __cplusplus
}
#endif /* end of __cplusplus */

