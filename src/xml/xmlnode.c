/************************************************************************
版权所有：杭州恒生电子股份有限公司
项目名称：公共项目
版    本：V1.0
操作系统：AIX4.2,SCO5
文件名称：xmlnode.c
文件描述：XML节点操作函数（XML结构内部使用）
项 目 组：中间业务产品研发组
程 序 员：中间业务产品研发组
发布日期：2001年09月11日
修改日期：2002年05月12日
************************************************************************/
/*
    修改记录：
    Tianhc :增加
    sl_NodeClear
    sl_AttrClear
                --用于清除指定节点的内容,但该节点本身不删除,对于同名节点需“占着这个位置,但内容不要”
    xn_ExportXMLString_Format
                --用于“格式化导出”成字符串
    修改
      xn_strtokdata
                --对于含TAB，空格等字符的XML 字符串导入时，将节点间的内容忽略  
    增加：
      xn_Setquot()函数,用于显示指定"导出为&quot;方式，缺省的话，导出为&quote;(错误的,但是旧的内部标准)                      
    修改:2006-11-15 22:10
      xn_importXmlString及xn_strtok函数,底层直接支持XML简写方式(<node/>)  
*/


#ifndef __XMLNODE_C__
#define __XMLNODE_C__

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#include	"xml.h"
#include  "trace_.h"

#ifdef	_WIN32
#include	<direct.h>
#define		FILE_SFLAG	'\\'
#include	<winsock2.h>
#include	<io.h>
#define		snprintf	_snprintf
#else
#define		FILE_SFLAG	'/'
#include	<dirent.h>
#include	<netinet/in.h>
/* #include	<inttypes.h> */
/* #pragma pack(1) SUN编译器对齐 */
#endif

/* #define	_TRACE_LOG_ */

#ifdef          _TRACE_LOG_
#include        "trace_.h"
#endif
#ifdef			_DEBUG_
#include        "debug.h"
#endif

extern int g_xmlErrno;
extern char g_szErrEx[128];
extern int g_xmlquot;

#define		XML_LOGCFGFILE	"XML_LOGCONFIG"

#ifdef __cplusplus
extern "C" {
#endif /* end of __cplusplus */

#define		FAIL	    (-1)
#define     SUCC 	    (0)

#define		LIST_INCRE		8 /* 线形表缺省增加大小 */
#define		SL_ELEMTYPE		void*


struct xml_node;
typedef struct	wj_seq_list{ /* 顺序线性表结构 */
	SL_ELEMTYPE		*m_lpBase;	/* 存储空间基址 */
	unsigned int	m_nLen;     /* 当前长度 */
	unsigned int	m_nListSize;/* 前分配的存储容量(以sizeof(SL_ELEMTYPE)为单位 */
}WSEQLIST, *PWSEQLIST;

#define	sl_Length(pList)	((pList)->m_nLen)

typedef struct xml_node_attr{ /* 节点元素属性数据 */
	unsigned char	*m_pszName;	/* 属性名 */
	unsigned char	*m_pszData;	/* 属性值 */
}XMLNODEATTR, *PXMLNODEATTR;

typedef struct xml_node{ /* 节点元素结构 */
	WSEQLIST	m_DownList;	/* 子节点表 */
	WSEQLIST	m_AttrList;	/* 属性表 */

	unsigned int m_id;		/* 该节点的标识 */
	unsigned char	*m_pszName;		/* 该节点的名称 */ 
	unsigned char	*m_pszData;		/* 该节点的数据 */
	unsigned int m_iDataLen;/* 数据长度 */

}XMLNODE, *PXMLNODE;


#ifdef	__XML_TRACE__
#define		XL_INDSR		0x00000001 /* 引入DSR数据函数 */
#define		XL_TODSR		0x00000002 /* 引出DSR数据函数 */
#define		XL_INXML		0x00000004 /* 引入XML字串函数 */
#define		XL_TOXML		0x00000008 /* 引出XML字串函数 */
#define		XL_INFMT		0x00000100 /* 引入指定格式字串函数 */
#define		XL_TOFMT		0x00000200 /* 引出指定格式字串函数 */
#define		XL_ERROR		0x00000010 /* 详细错误 */
#define		XL_DATA			0x00000020 /* 详细数据 */
#define		XL_EXCHANGE		0x00000040 /* 交换数据函数 */
#define		XL_NODE			0x00010000 /* 节点操作函数 */
#define		XL_NODEATTR		0x00020000 /* 节点属性操作函数 */
#define		XL_COUNT		0x00030000 /* 统计操作 */
#define		XL_ALL			0xFFFFFFFF /* 所有操作 */
#define		XL_NORMAL		0x0000003F /* 常用操作 */
#endif

typedef struct xml_struct{ /* XML结构 */
	PXMLNODE	m_pRoot;	/* 根节点 */
#ifdef	__XML_TRACE__
	unsigned int m_dwTrcType;/* TRACE类型 */
	int			m_hTrace;	 /* TRACE */
#endif
}XMLSTRUCT,*PXMLSTRUCT;


static void xn_Destroy( PXMLNODE pNode );
static void xna_Destroy( PXMLNODEATTR pAttr );

/*********************************************************
 *	函数:	sl_NodeDestroy
 *	功能:	销毁顺序线性表
 *	参数一:	顺序线性表指针
 *	返回值:	成功,返回0；失败,返回-1
 *********************************************************/
static void sl_NodeDestroy( PWSEQLIST lpList )
{
SL_ELEMTYPE *pNodeP;
unsigned int i;

	if ( lpList->m_nLen )
	{
		pNodeP=lpList->m_lpBase;
		i = lpList->m_nLen;

		for ( ; i; pNodeP++,i-- )
			xn_Destroy(*pNodeP);
	}
  
	if ( lpList->m_lpBase )
		free((void*)lpList->m_lpBase );
}

static void sl_NodeClear( PWSEQLIST lpList )
{
SL_ELEMTYPE *pNodeP;
unsigned int i;

	if ( lpList->m_nLen )
	{
		pNodeP=lpList->m_lpBase;
		i = lpList->m_nLen;

		for ( ; i; pNodeP++,i-- )
			xn_Destroy(*pNodeP);
	}
	if ( lpList->m_lpBase )
		free((void*)lpList->m_lpBase );
  lpList->m_nLen = 0;   /*Tianhc 用于清除节点内容时(非删除)*/
}
/*********************************************************
 *	函数:	sl_AttrDestroy
 *	功能:	销毁顺序线性表
 *	参数一:	顺序线性表指针
 *	返回值:	成功,返回0；失败,返回-1
 *********************************************************/
static void sl_AttrDestroy( PWSEQLIST lpList )
{
SL_ELEMTYPE *pNodeP;
unsigned int i;

	pNodeP=lpList->m_lpBase;
	i = lpList->m_nLen;

	if ( lpList->m_nLen )
	{
		for ( ; i; pNodeP++,i-- )
			xna_Destroy(*pNodeP);
	}

	if ( lpList->m_lpBase )
		free((void*)lpList->m_lpBase );
}

/*********************************************************
 *	函数:	sl_AttrClear
 *	功能:	销毁顺序线性表,
 *        但不将基址Free,用于清除节点自身的所有属性
 *	参数一:	顺序线性表指针
 *	返回值:	成功,返回0；失败,返回-1
 *  Tianhc 2005-11-23 20:52
 *********************************************************/
static void sl_AttrClear( PWSEQLIST lpList )
{
SL_ELEMTYPE *pNodeP;
unsigned int i;

	pNodeP=lpList->m_lpBase;
	i = lpList->m_nLen;

	if ( lpList->m_nLen )
	{
		for ( ; i; pNodeP++,i-- )
			xna_Destroy(*pNodeP);
	}

	if ( lpList->m_lpBase )
		free((void*)lpList->m_lpBase );

	lpList->m_nLen = 0;
}

/*********************************************************
 *	函数:	sl_OwnAppeMem
 *	功能:	增加存储空间
 *	参数一:	顺序线性表指针
 *	返回值:	成功,返回0;失败,返回-1
 *********************************************************/
static int sl_OwnAppeMem( PWSEQLIST lpList, int iAppSize )
{
SL_ELEMTYPE *lpNewBase;

	if ( iAppSize<=0 )
		iAppSize =  LIST_INCRE;

	if ( lpNewBase=(SL_ELEMTYPE*)realloc(lpList->m_lpBase,
		(lpList->m_nListSize+iAppSize)*sizeof(SL_ELEMTYPE)) )
	{
		memset((void*)(lpNewBase+lpList->m_nLen),'\0',iAppSize*sizeof(SL_ELEMTYPE));
		lpList->m_lpBase=lpNewBase;
		lpList->m_nListSize+=iAppSize;
		return	0;
	}

	if ( !(lpNewBase=(SL_ELEMTYPE*)malloc((lpList->m_nListSize+iAppSize)*sizeof(SL_ELEMTYPE)) ) )
	{
		return	(-1);
	}
	memset((void*)(lpNewBase+lpList->m_nLen),'\0',iAppSize*sizeof(SL_ELEMTYPE));
	memcpy((void*)lpNewBase,(void*)lpList->m_lpBase,lpList->m_nLen*sizeof(SL_ELEMTYPE));
	free((void*)lpList->m_lpBase);
	lpList->m_lpBase=lpNewBase;
	lpList->m_nListSize+=iAppSize;
	return	0;
}


/***************************************************************
 *	函数:	sl_GetElem
 *	功能:	返回线性表指定位置的数据元素
 *	参数一:	顺序线性表句柄
 *	参数二:	指定位置
 *	参数三:	接受取出的元素指针
 *	返回值:	成功,返回0；失败,返回-1
 **************************************************************/
static int sl_GetElem( PWSEQLIST lpList, const int nGetPos,SL_ELEMTYPE* lpElem)
{

	if ( nGetPos< 1 || nGetPos> (int)(lpList->m_nLen) )
	{ /* nPos位置不合法 */
		return (-1);
	}

	memcpy((void*)lpElem,(void*)&(lpList->m_lpBase[nGetPos-1]),sizeof(SL_ELEMTYPE));
	return (0);
}


/***************************************************************
 *	函数:	sl_Append
 *	功能:	在顺序线性表尾添加一元素
 *	参数一:	顺序线性表指针
 *	参数二:	待添加的元素的指针
 *	返回值:	成功,返回0;失败,返回(-1)
 **************************************************************/
static int sl_Append( PWSEQLIST lpList, SL_ELEMTYPE pApeNode)
{
	if ( lpList->m_nLen >=lpList->m_nListSize )
	{ /* 当前存储空间已满，增加分配 */
		if ( sl_OwnAppeMem(lpList,0)==(-1) ) 
			return (-1);
	}

	lpList->m_lpBase[lpList->m_nLen]=pApeNode;
	++(lpList->m_nLen);
	return (0);
}

/***************************************************************
 *	函数:	sl_Delete
 *	功能:	删除指定位置元素
 *	参数一:	顺序线性表句柄
 *	参数二:	指定位置
 *	参数三:	接受被删除的元素指针(不需要置空)
 *	返回值:	成功,返回0；失败,返回-1
 **************************************************************/
static int sl_Delete( PWSEQLIST lpList,const int nDelPos,
                       SL_ELEMTYPE* lpElem )
{
SL_ELEMTYPE* lpBase;
SL_ELEMTYPE* p;
SL_ELEMTYPE* q;

	if ( nDelPos< 1 || nDelPos> (int)(lpList->m_nLen) ) /* nPos位置不合法 */
	{
		return (-1);
	}
	lpBase = lpList->m_lpBase;
	p=&(lpBase[nDelPos-1]);
	if ( lpElem )
		memcpy((void*)lpElem,(void*)p,sizeof(SL_ELEMTYPE));
	q=&(lpBase[lpList->m_nLen-1]);
	for ( ++p; p<=q; ++p )
		memcpy((void*)(p-1),(void*)p,sizeof(SL_ELEMTYPE));
	memset((void*)q,'\0',sizeof(SL_ELEMTYPE));
	--(lpList->m_nLen);
   return (0);
}



static unsigned int getstrid( const unsigned char *szStr, unsigned int len )
{
unsigned int data;
unsigned char *p;

	for ( data=0, p=(unsigned char*)szStr; *p&&len; p++,len-- )
		data = 17*data + *p;
/*		data = 17*data + (islower(*p)?*p-32:*p); */

	return	data;
}

static int _memicmp_( unsigned char *p1, unsigned char *p2, size_t n )
{

	for ( ; n; p1++, p2++,n-- ) {
		if ( *p1 == *p2 ||
			 islower(*p1) && ( *p1-*p2==32 ) ||
			 isupper(*p1) && ( *p2-*p1==32 ) )
			continue;
		else
			return	( ( *p1 > *p2 ) ? 1:-1 );
	}
	return	(0);
}

static int _memicmp( unsigned char *p1, unsigned char *p2, size_t n )
{
  if ( *p1 != *p2 )
        return  ( ( *p1 > *p2 ) ? 1:-1 );
  return memcmp(p1,p2,n );

}
/*
static int _memicmp_( const unsigned char *pData1, const unsigned char *pData2, size_t nCount )
{
unsigned char *pBuf1,*pBuf2;

	pBuf1 = (unsigned char*)pData1;
	pBuf2 = (unsigned char*)pData2;

	for ( ; nCount; pBuf1=pBuf1+1, pBuf2 =pBuf2+1,nCount-- ) {
		if ( islower(*pBuf1) && ( *pBuf1-*pBuf2==32 ) )
			continue;
		if ( isupper(*pBuf1) && ( *pBuf2-*pBuf1==32 ) )
			continue;

		if ( *pBuf1 == *pBuf2 )
			continue;
		else
			return	( ( *pBuf1 > *pBuf2 ) ? 1:-1 );
	}
	return	(0);
}
*/
#ifdef	_CAP_
#define	memicmp_	_memicmp_	/* XML节点不区分大小写 */
#else
#define	memicmp_	_memicmp	/* XML节点区分大小写 */
#endif

/***************************************************************
 *	函数:	sl_FindAttr
 *	功能:	在属性表中查找指定名称属性
 *	参数一:	属性顺序线性表句柄
 *	参数二:	待查找的属性名称;
 *	参数三:	接收位置的整数指针
 *	返回值:	成功,返回元素指针；失败,返回NULL
 **************************************************************/
static PXMLNODEATTR sl_FindAttr( PWSEQLIST lpList, unsigned char *szName, int *pnPos )
{
PXMLNODEATTR *lpElemP,*lpEndP;

	if ( !lpList->m_lpBase )
		return	NULL;

	lpEndP = (PXMLNODEATTR*)lpList->m_lpBase+lpList->m_nLen;
	lpElemP = (PXMLNODEATTR*)lpList->m_lpBase;

	for ( ; lpElemP<lpEndP; lpElemP++ )
	{
		if ( !memicmp_((*lpElemP)->m_pszName,szName,strlen((char*)szName)) )
		{
			if ( pnPos )
				*pnPos = ((char*)lpElemP-(char*)lpList->m_lpBase)/sizeof(SL_ELEMTYPE)+1;
			return *lpElemP;
		}
	}
	return	NULL;
}


/***************************************************************
 *	函数:	sl_FindNode
 *	功能:	在节点表中查找指定名称节点
 *	参数一:	节点顺序线性表句柄
 *	参数二:	待查找的节点名称;
 *	参数三:	名称长度字节;
 *	参数四:	索引,指明第几次符合条件的数据 0,为最后位置
 *	参数五:	接收位置的整数指针
 *	返回值:	成功,返回元素指针；失败,返回NULL
 **************************************************************/
static PXMLNODE sl_FindNode( PWSEQLIST lpList,
				unsigned char *pcName, unsigned int len, int nIdx, int *pnPos )
{
PXMLNODE *lpElemP,*lpEndP;
unsigned int	id;

	if ( !lpList->m_lpBase )
		return	NULL;

	id = getstrid( pcName, len );

	if ( nIdx )
	{
		lpEndP=(PXMLNODE*)lpList->m_lpBase+lpList->m_nLen;
		lpElemP = (PXMLNODE*)lpList->m_lpBase;
		for ( ; lpElemP<lpEndP; lpElemP++ )
		{
			if ( id==(*lpElemP)->m_id && !memicmp_((*lpElemP)->m_pszName,pcName,len) )
			{
				nIdx--;
				if ( !nIdx )
				{
					if ( pnPos )
						*pnPos = ((char*)lpElemP-(char*)lpList->m_lpBase)/sizeof(SL_ELEMTYPE)+1;
					return *lpElemP;
				}
			}
		}
	}
	else
	{
		lpElemP=(PXMLNODE*)lpList->m_lpBase+lpList->m_nLen-1;
		lpEndP = (PXMLNODE*)lpList->m_lpBase;
		for ( ; lpElemP>=lpEndP; lpElemP-- )
		{
			if ( id==(*lpElemP)->m_id && !memicmp_((*lpElemP)->m_pszName,pcName,len) )
			{
				if ( pnPos )
					*pnPos = ((char*)lpElemP-(char*)lpList->m_lpBase)/sizeof(SL_ELEMTYPE)+1;
				return *lpElemP;
			}
		}
	}

	return (NULL);
}


/***************************************************************
 *	函数:	sl_CountNode
 *	功能:	在节点表中统计指定名称节点的个数
 *	参数一:	节点顺序线性表句柄
 *	参数二:	开始统计的位置
 *	参数三:	待查找的节点名称;
 *	参数四:	节点名称长度
 *	返回值:	成功,返回元素指针；失败,返回NULL
 **************************************************************/
static int sl_CountNode( PWSEQLIST lpList, int nStPos, unsigned char *pcName, unsigned int len )
{
PXMLNODE *lpElemP,*lpEndP;
int		num;
unsigned id;

	id = getstrid( pcName, len );

	lpEndP=(PXMLNODE*)lpList->m_lpBase+lpList->m_nLen;
	lpElemP = (PXMLNODE*)lpList->m_lpBase+nStPos;
	for ( num=0; lpElemP<lpEndP; lpElemP++ )
	{
		if ( id==(*lpElemP)->m_id && !memicmp_((*lpElemP)->m_pszName,pcName,len) )
		{
			num++;
		}
	}

	return	num;
}


static int xn_explain( unsigned char *szBuf, unsigned char *pStr, unsigned int len );

/**********************************************************************
 *	函数：	xna_Build
 *	功能:	构造一个节点属性
 *	参数:
	szName
		属性的名称。
	len
		属性名称长度
	szData
		属性值。
 *	返回值:
	 成功	返回构造的节点属性指针
	 失败	返回NULL
 **********************************************************************/
static PXMLNODEATTR xna_Build( const unsigned char *szName, const int len, const unsigned char *szAttr )
{
PXMLNODEATTR	pAttr;
unsigned int	n;
	if ( !szName || !*szName || !len )
		return	NULL;

	if ( !(pAttr=(PXMLNODEATTR)calloc(1,sizeof(XMLNODEATTR))) )
	{
		return	NULL;
	}

	if ( szName )
	{
		if ( !(pAttr->m_pszName=(unsigned char*)malloc(len+1)) )
		{
			free(pAttr);
			return	NULL;
		}
		memcpy(pAttr->m_pszName,szName,len);
		pAttr->m_pszName[len]='\0';
	}

	if ( szAttr )
	{
		n = strlen((char*)szAttr);
		if ( !(pAttr->m_pszData=(unsigned char*)malloc(n+1)) )
		{
			free(pAttr->m_pszName);
			free(pAttr);
			return	NULL;
		}
/*		xn_explain( pAttr->m_pszData, (unsigned char*)szAttr, n );*/
		memcpy(pAttr->m_pszData,szAttr,n); 
		*(pAttr->m_pszData+n)='\0';
	}

	return	pAttr;
}

/**********************************************************************
 *	函数：	xna_Destroy
 *	功能:	销毁一个属性
 *	参数:
	pAttr
          节点属性结构指针。
 *	返回值:	 无
 **********************************************************************/
static void xna_Destroy( PXMLNODEATTR pAttr )
{
	if ( pAttr->m_pszData )
		free(pAttr->m_pszData);
	if ( pAttr->m_pszName )
		free(pAttr->m_pszName);

	free(pAttr);
}



/**********************************************************************
 *	函数：	xn_Build
 *	功能:	构造一个元素节点
 *	参数:
	 szName
		元素的名称。
	 uiNameLen
		元素名称的长度。
	 szData
		元素值。
	 uiDataLen
		元素值的长度。
 *	返回值:
	 成功	返回构造的节点指针
	 失败	返回NULL
	 修改记录：
	    不进行转义处理, Tianhc 2009-12-29 9:46:31
 **********************************************************************/
static PXMLNODE xn_Build( const unsigned char *szName, unsigned int uiNameLen,
						 const unsigned char *szData, unsigned int uiDataLen )
{
PXMLNODE	pNode;

	if ( !szName || !*szName || !uiNameLen )
		return	NULL;

	if ( !(pNode=(PXMLNODE)calloc(1,sizeof(XMLNODE))) )
	{
		return	NULL;
	}

	if ( !(pNode->m_pszName=(unsigned char*)malloc(uiNameLen+1)) )
	{
		free(pNode);
		return	NULL;
	}
	memcpy(pNode->m_pszName,szName,uiNameLen);
	pNode->m_pszName[uiNameLen]='\0';

	if ( szData && *szData && uiDataLen )
	{
		if ( !(pNode->m_pszData=(unsigned char*)malloc(uiDataLen+1)) )
		{
			free(pNode->m_pszName);
			free(pNode);
			return	NULL;
		}
/*		uiDataLen = xn_explain( pNode->m_pszData, (unsigned char*)szData, uiDataLen );*/
		memcpy(pNode->m_pszData,szData,uiDataLen); 
		pNode->m_pszData[uiDataLen]='\0';
		pNode->m_iDataLen = uiDataLen;
	}
	pNode->m_id = getstrid(szName,uiNameLen);

	return	pNode;
}

/**********************************************************************
 *	函数：	xn_Destroy
 *	功能:	销毁一个元素节点
 *	参数:
	pNode
          节点结构指针。
 *	返回值:	 无
 **********************************************************************/
static void xn_Destroy( PXMLNODE pNode )
{
	if ( pNode->m_pszData )
		free(pNode->m_pszData);
	if ( pNode->m_pszName )
		free(pNode->m_pszName);
	sl_AttrDestroy(&pNode->m_AttrList);
	sl_NodeDestroy(&pNode->m_DownList);
	free(pNode);
}

#ifdef	__XML_TRACE__
/***************************************************************
 *	函数:	getstr_
 *	功能:	获取*.ini配置文件配置选项
 *	参数:
	- psFile：文件名字串
	- psItem：指定项名字串
	- psName：指定名称
	- szDataBuf：接收字串内容缓冲区
	- iBufLen：缓冲区szDataBuf的长度
 *	返回值:
	- 成功	返回字串内容指针
	- 失败	返回NULL
 **************************************************************/
static char* getstr_( const char* psFile, const char* psItem, const char* psName,char* szDataBuf, int iBufLen)
{
FILE* pFileStream;
unsigned char	szBuf[256],*p,*q;
int		i;
char	*pBuf,bItemFlag='\0';
int		iLen=0;


	if ( !psFile || !psItem || !psName || !szDataBuf )
	{
		return	(NULL);
	}
	

	if ( ! (pFileStream=fopen(psFile,"rt") ) )
	{
		return	(NULL);
	}

	for(;;)	{
		/* 读取一行数据 */
		if ( !fgets((char*)szBuf,255,pFileStream) )
			break;
		p = szBuf;
		while ( *p==32 || *p==9 ) /* 滤除空格和制表符 */
			p++;

		/* 注释或回车符 */
		if ( !*p || *p=='#' || ( *p=='/' && *(p+1)=='/') || *p==13 || *p==10 )
			continue;

		if ( bItemFlag ) { /* 已经匹配Item */
			if ( *p == '[' ) /* 结束未找到指定Name */
				break;
			
			q=(unsigned char*)psName;

			while ( (*p==*q) && (*p!='=') && *q ) {
				p++;
				q++;
			}
			while ( *p==32 || *p==9 ) /* 滤除空格和制表符 */
				p++;

			if ( ( *p!='=' ) || *q  ) /* 不匹配Name */
				continue;

			p++;
			while ( *p==32 || *p==9 ) /* 滤除空格和制表符 */
				p++;
			
			pBuf = szDataBuf;
			i = 0;
			while ( !( !*p || *p=='#' || *p==9 || ( *p=='/' && *(p+1)=='/') || *p==13 || *p==10 )) {
				if ( *p==32 ) /* 计算字符间空格符 */
					i++;
				else {
					while ( i ) {
						i--;
						iLen++;
						if(iLen>iBufLen)
						{
							fclose( pFileStream );
							return (NULL);
						}
						*pBuf++ = 32;
					}
					iLen++;
					if(iLen>iBufLen)
					{
						fclose( pFileStream );
						return (NULL);
					}
					*pBuf++ = *p;
				}
				p++;
			}
			iLen++;
			if(iLen>iBufLen)
			{
				fclose( pFileStream );
				return (NULL);
			}
			*pBuf = '\0';

			fclose( pFileStream );
			return	szDataBuf;
		}

		/* 分析是否是指定Item项 */
		else {
			if ( *p != '[' )
				continue;
			p++;
			while ( *p==32 || *p==9 ) /* 滤除空格和制表符 */
				p++;

			q=(unsigned char*)psItem;

			while ( (*p==*q) && (*p!=']') && *q ) {
				p++;
				q++;
			}
			while ( *p==32 || *p==9 ) /* 滤除空格和制表符 */
				p++;

			if ( ( *p==']' ) && !*q  ) /* 匹配Item */
				bItemFlag='9';
		} /* End of else */
	}

	fclose( pFileStream );
	return	NULL;
}

static int xmlo_InitTrace(char *szCfgFile, PXMLSTRUCT pXML )
{
int		hTrace;
int		len;
char	szBuf[256],szDir[128];

	if ( access(szCfgFile,0) == -1 )
	{
		g_xmlErrno = XML_ELOGFILENOTEXIST;
		snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szCfgFile);
		return	FAIL;
	}
	if ( access(szCfgFile,4) == -1 )
	{
		g_xmlErrno = XML_ELOGFILENOTREAD;
		snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szCfgFile);
		return	FAIL;
	}

	if ( getstr_(szCfgFile,"COMMON","TRACEFILE",szBuf,sizeof(szBuf)) )
	{
		len = strlen(szBuf);
		snprintf(szBuf+len,sizeof(szBuf)-len,"_%s",pXML->m_pRoot->m_pszName);
	}
	else
	{
		getcwd(szBuf,sizeof(szBuf));
		len = strlen(szBuf);
		snprintf(szBuf+len,sizeof(szBuf)-len,"%cxml_log_%s",
			FILE_SFLAG,pXML->m_pRoot->m_pszName);
	}

	if ( !getstr_(szCfgFile,"COMMON","BAKDIR",szDir,sizeof(szDir)) )
	{
		getcwd(szDir,sizeof(szDir));
		len = strlen(szDir);
		snprintf(szDir+len,sizeof(szDir)-len,"%ctrace",FILE_SFLAG);
	}

	if ( (hTrace=trc_Build( szBuf, szDir))==FAIL )
	{
		g_xmlErrno = XML_EBUILDTRACE;
		return	FAIL;
	}

	if ( getstr_(szCfgFile,"COMMON","FLUSH",szBuf,sizeof(szBuf)) )
		trc_SetAttr( hTrace, TFS_FLUSH, atoi(szBuf));

	if ( getstr_(szCfgFile,"COMMON","ALWAYSOPEN",szBuf,sizeof(szBuf)) )
		trc_SetAttr( hTrace, TFS_ALWAYSOPEN, atoi(szBuf));

	if ( getstr_(szCfgFile,"COMMON","MAXSIZE",szBuf,sizeof(szBuf)) )
		trc_SetAttr( hTrace, TFS_MAXSIZE, atoi(szBuf));

	if ( getstr_(szCfgFile,"COMMON","MAXHOUR",szBuf,sizeof(szBuf)) )
		trc_SetAttr( hTrace, TFS_MAXHOUR, atoi(szBuf));

	if ( getstr_(szCfgFile,"COMMON","DAYTIME",szBuf,sizeof(szBuf)) )
		trc_SetAttr( hTrace, TFS_DAYTIME, atoi(szBuf));

	if ( getstr_(szCfgFile,"COMMON","WEEKDAY",szBuf,sizeof(szBuf)) )
		trc_SetAttr( hTrace, TFS_WEEKDAY, atoi(szBuf));

	if ( getstr_(szCfgFile,"COMMON","MONTHDAY",szBuf,sizeof(szBuf)) )
		trc_SetAttr( hTrace, TFS_MONTHDAY, atoi(szBuf));

	if ( getstr_(szCfgFile,"COMMON","DATA",szBuf,sizeof(szBuf)) )
	{
		pXML->m_dwTrcType = (unsigned int)atoi(szBuf);
	}
	else
	{
		if ( getstr_(szCfgFile,"COMMON","D_IMPORTDSR",szBuf,sizeof(szBuf)) )
		{
			if ( atoi(szBuf) )
				pXML->m_dwTrcType |= XL_INDSR;
		}
		if ( getstr_(szCfgFile,"COMMON","D_EXPORTDSR",szBuf,sizeof(szBuf)) )
		{
			if ( atoi(szBuf) )
				pXML->m_dwTrcType |= XL_TODSR;
		}
		if ( getstr_(szCfgFile,"COMMON","D_IMPORTXML",szBuf,sizeof(szBuf)) )
		{
			if ( atoi(szBuf) )
				pXML->m_dwTrcType |= XL_INXML;
		}
		if ( getstr_(szCfgFile,"COMMON","D_EXPORTXML",szBuf,sizeof(szBuf)) )
		{
			if ( atoi(szBuf) )
				pXML->m_dwTrcType |= XL_TOXML;
		}
		if ( getstr_(szCfgFile,"COMMON","D_IMPORTFMT",szBuf,sizeof(szBuf)) )
		{
			if ( atoi(szBuf) )
				pXML->m_dwTrcType |= XL_INFMT;
		}
		if ( getstr_(szCfgFile,"COMMON","D_EXPORTFMT",szBuf,sizeof(szBuf)) )
		{
			if ( atoi(szBuf) )
				pXML->m_dwTrcType |= XL_TOFMT;
		}
		if ( getstr_(szCfgFile,"COMMON","D_ERROR",szBuf,sizeof(szBuf)) )
		{
			if ( atoi(szBuf) )
				pXML->m_dwTrcType |= XL_ERROR;
		}
		if ( getstr_(szCfgFile,"COMMON","D_DATA",szBuf,sizeof(szBuf)) )
		{
			if ( atoi(szBuf) )
				pXML->m_dwTrcType |= XL_DATA;
		}
		if ( getstr_(szCfgFile,"COMMON","D_NODE",szBuf,sizeof(szBuf)) )
		{
			if ( atoi(szBuf) )
				pXML->m_dwTrcType |= XL_NODE;
		}
		if ( getstr_(szCfgFile,"COMMON","D_NODEATTR",szBuf,sizeof(szBuf)) )
		{
			if ( atoi(szBuf) )
				pXML->m_dwTrcType |= XL_NODEATTR;
		}
		if ( getstr_(szCfgFile,"COMMON","D_COUNT",szBuf,sizeof(szBuf)) )
		{
			if ( atoi(szBuf) )
				pXML->m_dwTrcType |= XL_COUNT;
		}
		if ( getstr_(szCfgFile,"COMMON","D_EXCHANGE",szBuf,sizeof(szBuf)) )
		{
			if ( atoi(szBuf) )
				pXML->m_dwTrcType |= XL_EXCHANGE;
		}
	}
	return	hTrace;
}
#endif

/**********************************************************************
 *	函数：	xmlo_Build
 *	功能:	构造一个XML结构
 *	参数:
	 szRoot
		元素的根名称。
 *	返回值:
	 成功	返回构造的结构标识
	 失败	返回NULL
 **********************************************************************/
static PXMLSTRUCT xmlo_Build( char *szRootName )
{
PXMLSTRUCT	pXML;
char	*p;

	if ( !(pXML=(PXMLSTRUCT)calloc(1,sizeof(XMLSTRUCT))) )
	{
		g_xmlErrno = XML_ENOMEM;
		return	NULL;
	}

	if ( !(pXML->m_pRoot=xn_Build( (unsigned char*)szRootName, strlen(szRootName),NULL,0)) )
	{
		g_xmlErrno = XML_ENOMEM;
		free(pXML);
		return	NULL;
	}
#ifdef	__XML_TRACE__
{
/*char	szName[128],szDir[128];
	getcwd(szDir,sizeof(szDir));
	snprintf(szName,sizeof(szName),"%s%ctrace%cxml_log_%s",szDir,FILE_SFLAG,FILE_SFLAG,szRootName);
	snprintf(szDir+strlen(szDir),sizeof(szDir)-strlen(szDir),"%ctrace",FILE_SFLAG);
	if ( (pXML->m_hTrace=trc_Build( szName, szDir))==FAIL )
	{
		xn_Destroy(pXML->m_pRoot);
		free(pXML);
		return	NULL;
	}
	pXML->m_dwTrcType = XL_ALL;
	trc_SetAttr( pXML->m_hTrace, TFS_FLUSH, 1);
*/
	if ( (p=getenv(XML_LOGCFGFILE)) && *p )
	{
		if ( (pXML->m_hTrace=xmlo_InitTrace(p,pXML))==FAIL )
		{
			xn_Destroy(pXML->m_pRoot);
			free(pXML);
			return	NULL;
		}
		if ( pXML->m_dwTrcType & XL_NODE )
		{
			trc_Write(pXML->m_hTrace,"==================== 开始XML数据记录[%t] ====================\n\n",0);
			trc_Write(pXML->m_hTrace,">>> %t <<<  xml_Create('%s').\n\n",0,szRootName);
		}
	}
}
#endif
	return	pXML;
}

/**********************************************************************
 *	函数：	xmlo_Destroy
 *	功能:	销毁XML结构
 *	参数:
	 pXML
		XML结构指针。
 *	返回值:	无
 **********************************************************************/
static void xmlo_Destroy( PXMLSTRUCT pXML )
{
	if ( pXML->m_pRoot )
		xn_Destroy(pXML->m_pRoot);
#ifdef	__XML_TRACE__
	if ( pXML->m_dwTrcType & XL_NODE )
	{
		trc_Write(pXML->m_hTrace,">>> %t <<<  xml_Destroy().\n\n",0);
		trc_Write(pXML->m_hTrace,"==================== 结束XML数据记录[%t] ====================\n\n\n\n",0);
		trc_Destroy(pXML->m_hTrace);
	}
#endif
	free(pXML);
}


/**********************************************************************
 *	函数：	xn_SetData
 *	功能:	重新设置节点值
 *	参数:
	pNode
		节点结构指针。
	szData
		新的节点值
	len
		新的节点值长度
 *	返回值:
	成功	返回SUCC
	失败	返回FAIL
	修改记录：
	   不进行转义处理,Tianhc 2009-12-29 9:28:07
 **********************************************************************/
static int xn_SetData( PXMLNODE pNode, unsigned char *szData, unsigned int len )
{
unsigned char	*p;

	if ( szData && *szData && len>0 )
	{ /* 设置 */
		if ( !(p=realloc(pNode->m_pszData,len+1)) )
		{
			g_xmlErrno = XML_ENOMEM;
			return	FAIL;
		}
		pNode->m_pszData = p;
/*		len = xn_explain( pNode->m_pszData, szData, len );*/
		memcpy(pNode->m_pszData,szData,len); 
		*(pNode->m_pszData+len)='\0';
		pNode->m_iDataLen = len;
	}
	else
	{ /* 清除 */
		if ( pNode->m_pszData )
		{
			free(pNode->m_pszData);
			pNode->m_pszData = NULL;
		}
		pNode->m_iDataLen=0;
	}
	return	SUCC;
}


/**********************************************************************
 *	函数：	xn_SetDataWithConvert
 *	功能:	重新设置节点值(只提供给xm_ImportXMLString使用)
 *	参数:
	pNode
		节点结构指针。
	szData
		新的节点值
	len
		新的节点值长度
 *	返回值:
	成功	返回SUCC
	失败	返回FAIL
	说明：
	   与xn_SetData()区别是进行xn_explain处理
	   认为：只有Import时，才需要进行xn_explain转义处理，其余不应该进行处理
 **********************************************************************/
static int xn_SetDataWithConvert( PXMLNODE pNode, unsigned char *szData, unsigned int len )
{
unsigned char	*p;

	if ( szData && *szData && len>0 )
	{ /* 设置 */
		if ( !(p=realloc(pNode->m_pszData,len+1)) )
		{
			g_xmlErrno = XML_ENOMEM;
			return	FAIL;
		}
		pNode->m_pszData = p;
		len = xn_explain( pNode->m_pszData, szData, len );
/*		memcpy(pNode->m_pszData,szData,len); */
		*(pNode->m_pszData+len)='\0';
		pNode->m_iDataLen = len;
	}
	else
	{ /* 清除 */
		if ( pNode->m_pszData )
		{
			free(pNode->m_pszData);
			pNode->m_pszData = NULL;
		}
		pNode->m_iDataLen=0;
	}
	return	SUCC;
}

/**********************************************************************
 *	函数：	xna_SetData
 *	功能:	重新设置属性值
 *	参数:
	pNode
		属性结构指针。
	szData
		新的属性值
	len
		新的属性值长度
 *	返回值:
	成功	返回SUCC
	失败	返回FAIL
	修改记录：
	  改为不进行转义处理,Tianhc 2009-12-29 9:04:56
 **********************************************************************/
static int xna_SetData( PXMLNODEATTR pAttr, unsigned char *szData, unsigned int len )
{
unsigned char	*p;

	if ( szData && *szData && len )
	{ /* 设置 */
		if ( !(p=realloc(pAttr->m_pszData,len+1)) )
		{
			g_xmlErrno = XML_ENOMEM;
			return	FAIL;
		}
		pAttr->m_pszData = p;
/*		len = xn_explain( pAttr->m_pszData, szData, len );*/
		memcpy(pAttr->m_pszData,szData,len); 
		*(pAttr->m_pszData+len)='\0';
	}
	else
	{ /* 清除 */
		if ( pAttr->m_pszData )
		{
			free(pAttr->m_pszData);
			pAttr->m_pszData = NULL;
		}
	}
	return	SUCC;
}

/**********************************************************************
 *	函数：	xna_SetDataWithConvert
 *	功能:	重新设置属性值(只提供给xn_ImportXMLString使用)
 *	参数:
	pNode
		属性结构指针。
	szData
		新的属性值
	len
		新的属性值长度
 *	返回值:
	成功	返回SUCC
	失败	返回FAIL
	修改记录：
	  进行转义处理,Tianhc 2009-12-29 9:04:56
 **********************************************************************/
static int xna_SetDataWithConvert( PXMLNODEATTR pAttr, unsigned char *szData, unsigned int len )
{
unsigned char	*p;

	if ( szData && *szData && len )
	{ /* 设置 */
		if ( !(p=realloc(pAttr->m_pszData,len+1)) )
		{
			g_xmlErrno = XML_ENOMEM;
			return	FAIL;
		}
		pAttr->m_pszData = p;
		len = xn_explain( pAttr->m_pszData, szData, len );
/*		memcpy(pAttr->m_pszData,szData,len); */
		*(pAttr->m_pszData+len)='\0';
	}
	else
	{ /* 清除 */
		if ( pAttr->m_pszData )
		{
			free(pAttr->m_pszData);
			pAttr->m_pszData = NULL;
		}
	}
	return	SUCC;
}

/**********************************************************************
 *	函数：	xn_Copy
 *	功能:	拷贝一个节点
		完成后，返回一个独立的节点
 *	参数:
	lpNode
		源节点
 *	返回值:
	成功	目标节点
	失败  NULL
  修改记录：
 **********************************************************************/
static PXMLNODE xn_Copy(PXMLNODE lpNode)
{
	PXMLNODE pNode = NULL;
	PXMLNODE *lpElemP,*lpEndP;
	PXMLNODEATTR *lpAttrP, *lpEndAP;
	PWSEQLIST lpList;
  if (lpNode == NULL)
  	return NULL;
  
  pNode = xn_Build((unsigned char*)lpNode->m_pszName, strlen(lpNode->m_pszName),
                    lpNode->m_pszData, lpNode->m_iDataLen);
  if (!pNode)
	{
		g_xmlErrno = XML_ENOMEM;
		return	NULL;
	}
	lpList = (PWSEQLIST) &(lpNode->m_DownList);
	lpEndP=(PXMLNODE*)lpList->m_lpBase+lpList->m_nLen;
	lpElemP = (PXMLNODE*)lpList->m_lpBase;
	for ( ; lpElemP<lpEndP; lpElemP++ )
	{
		PXMLNODE tpNode = xn_Copy(*lpElemP);
		if (!tpNode)
		{
			xn_Destroy(pNode);
			return NULL;
		}
		
		sl_Append(&(pNode->m_DownList), tpNode);
	}
	lpList = (PWSEQLIST) &(lpNode->m_AttrList);
	lpEndAP=(PXMLNODEATTR*)lpList->m_lpBase+lpList->m_nLen;
	lpAttrP = (PXMLNODEATTR*)lpList->m_lpBase;
	for ( ; lpAttrP<lpEndAP; lpAttrP++ )
	{
		PXMLNODEATTR pAttr = xna_Build((*lpAttrP)->m_pszName, strlen((*lpAttrP)->m_pszName), (*lpAttrP)->m_pszData);
		if (!pAttr)
		{
			g_xmlErrno = XML_ENOMEM;
			xn_Destroy(pNode);
			return NULL;
		}
		sl_Append(&(pNode->m_AttrList), pAttr);
	}
	
	return pNode;
}

/**********************************************************************
 *	函数：	xn_Move
 *	功能:	用指定节点数据替换
		完成后，接收数据节点的原有数据将删除，待移动数据节点将消失
 *	参数:
	pNode
		接收移入数据的节点
	pMoveNode
		待移动数据的节点
 *	返回值:
	成功	返回SUCC
	失败	返回FAIL
  修改记录：
 **********************************************************************/
static int xn_Move( PXMLNODE pNode, PXMLNODE pMoveNode)
{
	sl_NodeDestroy( &pNode->m_DownList );
	sl_AttrDestroy( &pNode->m_AttrList );
	if ( pNode->m_pszName )
		free( pNode->m_pszName );
	if ( pNode->m_pszData )
		free( pNode->m_pszData );

	memcpy(pNode, pMoveNode, sizeof(XMLNODE));

	free(pMoveNode);
	return	SUCC;
}


#define	XML_NAMEFLAG	'/'	/* 名称和名称之间的分隔符 */
#define	XML_POSFLAG		'|' /* 名称和位置之间的分隔符 */

/**********************************************************************
 *	函数：	xn_LocateNode
 *	功能:	获取XML树中中指定名称的节点
 *	参数:
	pRootNode
		根节点结构指针
	szName
		元素的名称，形如"/PRIVATE|2/电费单价|0"
		"/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
		NMAE	表示取最后一个元素，
		IDX		指定每一层元素在同名兄弟元素中的排序位置。
			如果iIndex为0，表示添加在最后
			如果为1，2，3……，表示取相应编号的元素。
			缺省为1
	pnUpPos
		上一层位置指针
	pXML
		XML结构指针
 *	返回值:
	成功	返回获取的节点指针
	失败	返回NULL
 **********************************************************************/
static PXMLNODE xn_LocateNode( PXMLNODE pRootNode, char *szName, int *pnUpPos, PXMLSTRUCT pXML )
{
unsigned char	*pName,*p;
PXMLNODE	lpNode,lpUp;
int iIndex,len;

	p=(unsigned char*)szName;
	lpNode = pRootNode;
	if ( *p=='/' )
		p++;

	while ( *p )
	{
		pName = p;
		while ( *p && !(*p==XML_NAMEFLAG||*p==XML_POSFLAG) )
			p++;

		if ( *p == XML_POSFLAG )
		{
			iIndex = 0;
			len = p++-pName;
			if ( !len || len==1 && !*pName )
			{ /* 字串名称错误 */
				g_xmlErrno = XML_EINVNODENAME;
				snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szName);
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: %s 字串分隔数据错误 - %s\n",szName, xml_StringError(xml_GetLastError()));
#endif
				return	NULL;
			}
			while ( *p && isdigit(*p) )
			{
				iIndex = iIndex*10+*p-'0';
				p++;
			}
			if ( !*p )
			{
				p--;
			}
			else if ( *p != XML_NAMEFLAG )
			{ /* 字串分隔数据错误 */
				g_xmlErrno = XML_EINVNODENAME;
				snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szName);
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: %s 字串分隔数据错误 - %s\n",szName, xml_StringError(xml_GetLastError()));
#endif
				return	NULL;
			}
		}
		else
		{
			iIndex = 1; /* 缺省为第一个 */
			len = p-pName;
			if ( !len || len==1 && !*pName )
			{ /* 字串名称错误 */
				g_xmlErrno = XML_EINVNODENAME;
				snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szName);
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: %s 字串分隔数据错误 - %s\n",szName, xml_StringError(xml_GetLastError()));
#endif
				return	NULL;
			}
			if ( !*p )
			{
				p--;
			}
		}

		lpUp = lpNode;
		if ( !(lpNode=sl_FindNode( &lpNode->m_DownList, pName, len, iIndex, pnUpPos )) )
		{ /* 不存在 */
			g_xmlErrno = XML_ENONODE;
			snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szName);
			return	NULL;
		}
		p++;
	}

	if ( pnUpPos )
		return	lpUp;
	return	lpNode;
}


/**********************************************************************
 *	函数：	xn_AddNode
 *	功能:	在XML树中添加一个节点
 *	参数:
	pRootNode
		根节点结构指针
	szName
		指定在XML结构要添加的元素的名称，形如"/PRIVATE|2/电费单价|0"
		"/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
		NMAE	表示取最后一个元素，
		IDX		指定每一层元素在同名兄弟元素中的排序位置。
			如果iIndex为0，表示添加在最后
			如果为1，2，3……，表示取相应编号的元素。
			缺省为0
	szData
		待添加的元素值指针
	pXML
		XML结构指针
 *	返回值:
	成功	返回填加的节点指针
	失败	返回NULL
 **********************************************************************/
static PXMLNODE xn_AddNode( PXMLNODE pRootNode, char *szName, char *szData, int iDataLen, PXMLSTRUCT pXML )
{
unsigned char	*pName,*p;
PXMLNODE	lpNode,pTpNode;
int iIndex,len;
int	bFlag=0;

	p=(unsigned char*)szName;
	lpNode = pRootNode;

	if ( *p=='/' )
		p++;

	while ( *p )
	{
		iIndex = 0;
		pName = p;
		while ( *p && !(*p==XML_NAMEFLAG||*p==XML_POSFLAG) )
			p++;

		if ( *p == XML_POSFLAG )
		{
			len = p++-pName;
			if ( !len || len==1 && !*pName )
			{ /* 字串名称错误 */
				g_xmlErrno = XML_EINVNODENAME;
				snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szName);
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: %s - %s\n",szName, xml_StringError(xml_GetLastError()));
#endif
				return	NULL;
			}

			while ( *p && isdigit(*p) )
			{
				iIndex = iIndex*10+*p-'0';
				p++;
			}
			if ( !*p )
			{
				p--;
			}
			else if ( *p!=XML_NAMEFLAG )
			{ /* 字串分隔数据错误 */
				g_xmlErrno = XML_EINVNODENAME;
				snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szName);
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: %s 字串分隔数据错误 - %s\n",szName, xml_StringError(xml_GetLastError()));
#endif
				return	NULL;
			}
		}
		else
		{
			len = p-pName;
			if ( !len || len==1 && !*pName )
			{ /* 字串名称错误 */
				g_xmlErrno = XML_EINVNODENAME;
				snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szName);
#ifdef __XML_TRACE__
					if ( pXML->m_dwTrcType&XL_ERROR )
						trc_Write(pXML->m_hTrace,"error: %s - %s\n",szName, xml_StringError(xml_GetLastError()));
#endif
				return	NULL;
			}

			if ( !*p )
			{
				p--;
			}
		}

		if ( !*(p+1) || bFlag || !(pTpNode=sl_FindNode( &lpNode->m_DownList, pName, len, iIndex, NULL )) || (pTpNode->m_pszData&&*pTpNode->m_pszData)  )
		{ /* 不存在或最后添加 */
			if ( !bFlag )
				bFlag = 1;
			if ( *(p+1) )
				pTpNode=xn_Build(pName,len,NULL,0 );
			else
				pTpNode=xn_Build(pName,len,(unsigned char*)szData,iDataLen);
			if ( !pTpNode )
			{
				g_xmlErrno = XML_ENOMEM;
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: 构造节点'%m'失败 - %s\n",pName,len,xml_StringError(xml_GetLastError()));
#endif
				return	NULL;
			}

			if ( sl_Append(&(lpNode->m_DownList),pTpNode)==-1 )
			{
				g_xmlErrno = XML_ENOMEM;
				xn_Destroy(pTpNode);
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: 添加节点失败 - %s\n",xml_StringError(xml_GetLastError()));
#endif
				return	NULL;
			}
		}
		lpNode = pTpNode;
		p++;
	}

	return	pTpNode;
}



/***************************************************************
 *	函数:	xn_explain
 *	功能:	解释带有转义字符的字串，将其还原为原来的字符串
 *	参数一:	接收解释后的字串缓冲指针(由调用函数保证空间大小)
 *	参数二:	待解释的带有转义字符的字串
 *	参数三:	字串长度
 *	返回值:	返回实际数据字节数
 *	备注：	仅供xn_ImportXMLString函数使用
 转义字符为下列字符集
	&lt;    &gt;        &apos;      &quote;     &amp;
	<       >           '           "           &
  ==>                             &quot;  --Tianhc 2006-6-15 17:54
 **************************************************************/
static int xn_explain( unsigned char *szBuf, unsigned char *pStr, unsigned int len )
{
unsigned char *p;
unsigned char *q;

	p = pStr;
	q = szBuf;

	while ( len--  )
	{
		if ( *p == '&' )
		{
			p++;
			if ( !_memicmp(p,"lt;",3) )
			{
				p+=3;
				len -= 3;
				*q++ = '<';
			}
			else if ( !_memicmp(p,"gt;",3) )
			{
				p+=3;
				len -= 3;
				*q++ = '>';
			}
			else if ( !_memicmp(p,"apos;",5) )
			{
				p+=5;
				len -= 5;
				*q++ = '\'';
			}
			else if (( !_memicmp(p,"quot;",5))  &&  (1==g_xmlquot))
			{
				p+=5;
				len -= 5;
				*q++ = '"';
			}
			else if (( !_memicmp(p,"quote;",6)) &&  (0==g_xmlquot))
			{
				p+=6;
				len -= 6;
				*q++ = '"';
			}
			else if ( !_memicmp(p,"amp;",4) )
			{
				p+=4;
				len -= 4;
				*q++ = '&';
			}
			else
			{ /* 不应出现的特殊字符不作为错误，仍返回原字符 */
				*q++='&';
			}
		}
		else
		{
			*q++ = *p++;
		}

	}

	return	q-szBuf;
}

/***************************************************************
 *	函数:	xn_explace
 *	功能:	将字符串中特殊字符用转义字符替换
 *	参数一:	接收转换后的字串缓冲指针
 *	参数二:	缓冲大小
 *	参数三:	待进行转换的带有特殊字符的字串
 *	参数四:	字串长度
 *	返回值:	经过转换, 返回写入缓冲区的字节数; 无需解释, 返回0
 *	备注：	仅供xn_ExportXMLString函数使用
 **************************************************************/
static int xn_explace( unsigned char *szBuf, unsigned int size, unsigned char *pStr, unsigned int len )
{
unsigned char *p,*q;
unsigned int num;

	p = pStr;
	q = szBuf;
	num = size;

	while ( len-- && num )
	{
		switch ( *p )
		{
		case	'<':
			if ( num<4 )
				return	FAIL;
			memcpy(q,"&lt;",4);
			q   += 4;
			num += 4;
			break;
		case	'>':
			if ( size<4 )
				return	FAIL;
			memcpy(q,"&gt;",4);
			q += 4;
			num += 4;
			break;
		case	'\'':
			if ( size<6 )
				return	FAIL;
			memcpy(q,"&apos;",6);
			q += 6;
			num += 6;
			break;
		case	'"':
      if (1==g_xmlquot)  /*" ==> &quot; Tianhc 2006-9-7 13:10*/
      {	 
				if ( size<6 )
					return	FAIL;
				memcpy(q,"&quot;",6);
				q += 6;
				num += 6;
				break;
		  } else
		  {
				if ( size<7 )    /*" ==> &quote; 为了兼容以前的错误!*/
					return	FAIL;
				memcpy(q,"&quote;",7);
				q += 7;
				num += 7;
				break;
		  }		
		case	'&':
			if ( size<5 )
				return	FAIL;
			memcpy(q,"&amp;",5);
			q += 5;
			num += 5;
			break;
		default:
			*q++ = *p;
			num--;
		}
		p++;
	}
	return	q-szBuf;
}

/***************************************************************
 *	函数:	xn_explacelen
 *	功能:	将字符串中特殊字符用转义字符替换,返回替换后长度
 *	参数一:	待进行转换的带有特殊字符的字串
 *	参数二:	字串长度
 *	返回值:	经过转换, 返回写入缓冲区的字节数; 
 *	备注：	仅供xn_ExportXMLStringLen函数使用
 **************************************************************/
static int xn_explacelen( unsigned char *pStr, unsigned int len )
{
unsigned char *p,*q;
unsigned int num;

	p = pStr;
	num = 0;

	while ( len-- )
	{
		switch ( *p )
		{
		case	'<':
			num += 4;
			break;
		case	'>':
			num += 4;
			break;
		case	'\'':
			num += 6;
			break;
		case	'"':
			num += 6;
			break;
		case	'&':
			num += 5;
			break;
		default:
			num++;
		}
		p++;
	}
	return	num;
}

/***************************************************************
 *	函数:	xn_ExportXMLStringKVL
 *	功能:	引出KVL格式字符串, 专用于KVL格式报文
 *	参数一:	待引出的节点指针
 *	参数二:	接收字串数据的缓冲区指针
 *	参数三:	缓冲区长度
 *	参数四:	是否引出该节点自身
 *	返回值:	成功,返回SUCC; 失败,返回FAIL;
 **************************************************************/
static int xn_ExportXMLStringKVL(PXMLNODE lpNode, char *szBuf, size_t size, int bNodeSelf )
{
PWSEQLIST	lpList;
PXMLNODEATTR *lpElemP;
PXMLNODE *lpNodeP;
int		iNameLen,iDataLen,len1,len2;
int		i,n,num;
unsigned char	*p;

	num = size;
	p = (unsigned char*)szBuf;


	if ( bNodeSelf )
	{
		iNameLen=strlen((char*)lpNode->m_pszName);
		if ( num <= iNameLen+1 )
			return	FAIL;
		*p++ = '<';
		memcpy(p,lpNode->m_pszName,iNameLen);
		p	+= iNameLen;
		num	-= iNameLen;

		/* 导出属性数据 */
		lpList = &lpNode->m_AttrList;
		if ( i=sl_Length(lpList) )
		{
			lpElemP=(PXMLNODEATTR*)lpList->m_lpBase;
			for ( ; i; i--, lpElemP++ )
			{
				len1 = strlen((char*)(*lpElemP)->m_pszName);
				if ( num < len1+4 )
					return	FAIL;

				*p++ = ' ';
				memcpy(p,(*lpElemP)->m_pszName,len1);
				p += len1;
				*p++ = '=';
				*p++ = '\"';
				num -= (len1+4);

				if ( (*lpElemP)->m_pszData )
				{
					if ( (len2=xn_explace(p,num,(*lpElemP)->m_pszData,
							strlen((char*)(*lpElemP)->m_pszData)))==FAIL )
						return	FAIL;
					num -= len2;
					p   += len2;
				}
				*p++ = '\"';
			}
		}
		*p++ = '>';
		if ( num <= 1 )
			return	FAIL;
		num--;
	}
/* 导出子节点数据 */
	lpList = &lpNode->m_DownList;
	if ( i=sl_Length(lpList) )
	{
		lpNodeP		= (PXMLNODE*)lpList->m_lpBase;
		for ( ; i; i--,lpNodeP++ )
		{
			if ( (n=xn_ExportXMLStringKVL((*lpNodeP),(char*)p,num,1))==FAIL )
				return	FAIL;
			num-=n;
			p+=n;
		}
	}

	if ( bNodeSelf )
	{
		/* 导出节点值数据 */
		if ( lpNode->m_pszData )
		{
			if ( (iDataLen=xn_explace(p,num,lpNode->m_pszData,lpNode->m_iDataLen))==FAIL )
				return	FAIL;
			num -= iDataLen;
			p += iDataLen;
		}

		if ( num < 4 )
			return	FAIL;

		*p++ = '<';
		*p++ = '/';
/*
		memcpy(p,lpNode->m_pszName,iNameLen);
		p+=iNameLen;
*/
		*p++ = '>';
		*p++ = '\0';
		num -= 4;
	}

	return	size-num;
}


/***************************************************************
 *	函数:	xn_ExportXMLString
 *	功能:	引出XML格式字符串
 *	参数一:	待引出的节点指针
 *	参数二:	接收字串数据的缓冲区指针
 *	参数三:	缓冲区长度
 *	参数四:	是否引出该节点自身
 *	返回值:	成功,返回SUCC; 失败,返回FAIL;
 **************************************************************/
static int xn_ExportXMLString(PXMLNODE lpNode, char *szBuf, size_t size, int bNodeSelf )
{
PWSEQLIST	lpList;
PXMLNODEATTR *lpElemP;
PXMLNODE *lpNodeP;
int		iNameLen,iDataLen,len1,len2;
int		i,n,num;
unsigned char	*p;

	num = size;
	p = (unsigned char*)szBuf;


	if ( bNodeSelf )
	{
		iNameLen=strlen((char*)lpNode->m_pszName);
		if ( num <= iNameLen+1 )
			return	FAIL;
		*p++ = '<';
		memcpy(p,lpNode->m_pszName,iNameLen);
		p	+= iNameLen;
		num	-= iNameLen;

		/* 导出属性数据 */
		lpList = &lpNode->m_AttrList;
		if ( i=sl_Length(lpList) )
		{
			lpElemP=(PXMLNODEATTR*)lpList->m_lpBase;
			for ( ; i; i--, lpElemP++ )
			{
				len1 = strlen((char*)(*lpElemP)->m_pszName);
				if ( num < len1+4 )
					return	FAIL;

				*p++ = ' ';
				memcpy(p,(*lpElemP)->m_pszName,len1);
				p += len1;
				*p++ = '=';
				*p++ = '\"';
				num -= (len1+4);

				if ( (*lpElemP)->m_pszData )
				{
					if ( (len2=xn_explace(p,num,(*lpElemP)->m_pszData,
							strlen((char*)(*lpElemP)->m_pszData)))==FAIL )
						return	FAIL;
					num -= len2;
					p   += len2;
				}
				*p++ = '\"';
			}
		}
		*p++ = '>';
		if ( num <= 1 )
			return	FAIL;
		num--;
	}

	/* 导出子节点数据 */
	lpList = &lpNode->m_DownList;
	if ( i=sl_Length(lpList) )
	{
		lpNodeP		= (PXMLNODE*)lpList->m_lpBase;
		for ( ; i; i--,lpNodeP++ )
		{
			if ( (n=xn_ExportXMLString((*lpNodeP),(char*)p,num,1))==FAIL )
				return	FAIL;
			num-=n;
			p+=n;
		}
	}

	if ( bNodeSelf )
	{
		/* 导出节点值数据 */
		if ( lpNode->m_pszData )
		{
			if ( (iDataLen=xn_explace(p,num,lpNode->m_pszData,lpNode->m_iDataLen))==FAIL )
				return	FAIL;
			num -= iDataLen;
			p += iDataLen;
		}

		if ( num < iNameLen+4 )
			return	FAIL;

		*p++ = '<';
		*p++ = '/';
		memcpy(p,lpNode->m_pszName,iNameLen);
		p+=iNameLen;
		*p++ = '>';
		*p++ = '\0';
		num -= (iNameLen+4);
	}

	return	size-num;
}


/***************************************************************
 *	函数:	xn_ExportXMLString
 *	功能:	引出XML格式字符串
 *	参数一:	待引出的节点指针
 *	参数二:	接收字串数据的缓冲区指针
 *	参数三:	缓冲区长度
 *	参数四:	是否引出该节点自身
 *	返回值:	成功,返回SUCC; 失败,返回FAIL;
 **************************************************************/
static int xn_ExportXMLString_Format(PXMLNODE lpNode, char *szBuf, size_t size, int bNodeSelf,int ilevel )
{
PWSEQLIST	lpList;
PXMLNODEATTR *lpElemP;
PXMLNODE *lpNodeP;
int		iNameLen,iDataLen,len1,len2;
int		i,n,num;
unsigned char	*p;

	num = size;
	p = (unsigned char*)szBuf;


	if ( bNodeSelf )
	{
		iNameLen=strlen((char*)lpNode->m_pszName);
		/*增加TAB键*/
		for (i=1;i<=ilevel;i++)   
		{
		  /**p++ = 9;*/
		  *p++ = ' ';
      num--;
		}
		if ( num <= iNameLen+1 )
			return	FAIL;
		*p++ = '<';
		memcpy(p,lpNode->m_pszName,iNameLen);
		p	+= iNameLen;
		num	-= iNameLen;

		/* 导出属性数据 */
		lpList = &lpNode->m_AttrList;
		if ( i=sl_Length(lpList) )
		{
			lpElemP=(PXMLNODEATTR*)lpList->m_lpBase;
			for ( ; i; i--, lpElemP++ )
			{
				len1 = strlen((char*)(*lpElemP)->m_pszName);
				if ( num < len1+4 )
					return	FAIL;

				*p++ = ' ';
				memcpy(p,(*lpElemP)->m_pszName,len1);
				p += len1;
				*p++ = '=';
				*p++ = '\"';
				num -= (len1+4);

				if ( (*lpElemP)->m_pszData )
				{
					if ( (len2=xn_explace(p,num,(*lpElemP)->m_pszData,
							strlen((char*)(*lpElemP)->m_pszData)))==FAIL )
						return	FAIL;
					num -= len2;
					p   += len2;
				}
				*p++ = '\"';
			}
		}
		*p++ = '>';
		if ( num <= 1 )
			return	FAIL;
		num--;
	}

	/* 导出子节点数据 */
	lpList = &lpNode->m_DownList;
	if ( i=sl_Length(lpList) )
	{
		lpNodeP		= (PXMLNODE*)lpList->m_lpBase;
		ilevel++;
		 /*有子节点*/
		*p++ = '\n';
		num--;
		for ( ; i; i--,lpNodeP++ )
		{
			if ( (n=xn_ExportXMLString_Format((*lpNodeP),(char*)p,num,1,ilevel))==FAIL )
				return	FAIL;
			num-=n;
			p+=n;
		}
		/*增加TAB键*/
		for (i=1;i<ilevel;i++)   
		{
		  /**p++ = 9;*/
		  *p++ = ' ';
      num--;
		}
		
	}

	if ( bNodeSelf )
	{
		/* 导出节点值数据 */
		if ( lpNode->m_pszData )
		{
			if ( (iDataLen=xn_explace(p,num,lpNode->m_pszData,lpNode->m_iDataLen))==FAIL )
				return	FAIL;
			num -= iDataLen;
			p += iDataLen;
		}
    
		if ( num < iNameLen+5 )
			return	FAIL;

		*p++ = '<';
		*p++ = '/';
		memcpy(p,lpNode->m_pszName,iNameLen);
		p+=iNameLen;
		*p++ = '>';
		*p++ = '\n';  /*本节点结束增加回车*/
		*p++ = '\0';
		num -= (iNameLen+5);
	}

	return	size-num;
}

/***************************************************************
 *	函数:	xn_strtok
 *	功能:	拆分XML格式字符串
 *	参数一:	XML字串指针
 *	参数二:	接收字串数据开始地址指针的指针
 *	参数三:	接收字串长度的指针
 *	参数四:	接收字串分解结束位置地址指针的指针
 *	返回值:	返回分解类型见#define
 *	备注：	仅供xn_ImportXMLString函数使用
 修改记录：
    Tianhc 2006-11-15 19:07
    对于XML 2.0的处理(简写支持),/>
    1.定义/> 为XT_SHORT
    2.两种情况会遇到XT_SHORT如下：
      <node1/>
      <node1 name="xxxx"/>
      一种是在XT_HEADLEFT时遇到
      另一种在属性取值是遇到
 **************************************************************/
#define		XT_ERROR		(-1)/* 错误				*/
#define		XT_END			0	/* 结束				*/
#define		XT_HEADLEFT		1	/* 开始左标识 <		*/
#define		XT_TAILLEFT		2	/* 结束左标识 </	*/
#define		XT_RIGHT		3	/* 右标识 >			*/
#define		XT_EQUALE		4	/* 等于标识 =		*/
#define		XT_STRING		5	/* 字串数据			*/
#define   XT_SHORT    9 /*  简写型 />   */
static int xn_strtok( unsigned char *szXML, unsigned char**pStP, int *n, unsigned char**pEndP )
{
unsigned char	*p = szXML;

	while ( *p && isspace(*p) )
		p++;

	if ( !*p )
		return	XT_END;
/*mod by src */
/* if ( *p=='<'&& (*(p+1)=='?' || *(p+1)=='!') )*/
   if ( *p=='<'&& (*(p+1)=='?') )
	{char	ch;
		ch = *(p+1);
		p += 2;
		while ( *p && !(*p==ch&&*(p+1)=='>') )
		{
			p++;
		}
		if ( !*p )
			return	XT_ERROR;
		else
			p+=2;

		while ( *p && isspace(*p) )
			p++;
		if ( !*p )
			return	XT_END;
	}
    /*For omit the xml comment <!-- xxx --> add by src*/
    if (*p == '<' && (*(p + 1) == '!' && *(p + 2) == '-' && *(p + 3) == '-'))
    {
        char   ch;
        ch = *(p + 2);
        p += 4;
        while ( *p && !(*p == ch && *(p + 1) == '-' && *(p + 2) == '>'))
        {
            p++;
        }
        if ( !*p )
        {
            return  XT_ERROR;
        }
        else
        {
            p += 3;
        }

        while ( *p && isspace(*p) )
        {
            p++;
        }
        if ( !*p )
        {
            return  XT_END;
        }
    }
	if ( *p=='<' )
	{
		if ( *(p+1)=='/' )
		{
			*pEndP = p+2;
			return	XT_TAILLEFT;
		}
		else
		{
			*pEndP = p+1;
			return	XT_HEADLEFT;
		}
	}
	else if ( *p=='>' ) 
	{
		*pEndP = p+1;
		return	XT_RIGHT;
	}
	else if (( *p=='/' ) && ( *(p+1)=='>' ))
	{
		*pEndP = p+2;
		return	XT_SHORT;
	}	
	else if ( *p=='=' )
	{
		*pEndP = p+1;
		return	XT_EQUALE;
	}
	else
	{
		*pStP = p;
		while ( *p && !isspace(*p) && *p!='<' && *p!='>' && *p!='=' )
			p++;
		*pEndP = p;
		*n = p - *pStP;
		if (*p == '>' && *(p-1) == '/')   /*在取结点名称时遇到XT_SHORT*/
		{
			*n = *n - 1;
			*pEndP= p+1;
			return XT_SHORT;
		}	
		return	XT_STRING;
	}
}

/***************************************************************
 *	函数:	xn_strtokdata
 *	功能:	专用于拆分XML格式字符串中的某个节点的数据值
 *	参数一:	XML字串指针,在字符'<'后开始
 *	参数二:	接收字串数据开始地址指针的指针
 *	参数三:	接收字串长度的指针
 *	参数四:	接收字串分解结束位置地址指针的指针,以'>'结束
 *	返回值:	返回分解类型见#define
 *	备注：	仅供xn_ImportXMLString函数使用
 **************************************************************/
static int xn_strtokdata( unsigned char *szXML, unsigned char**pStP, int *n, unsigned char**pEndP )
{
unsigned char	*p = szXML;
unsigned char	*p2;

  	while ( *p && *p=='\n')
	  	p++;

		p2=p;
	 	while ( *p2 && (*p2=='\n' || *p2==' ' || *p2==9 || *p2==13) ) 
	 	    p2++;
	  	
  
	if ( !*p || *p=='<' || *p=='>' )
		return	XT_ERROR;

	*pStP = p;
	while ( *p && *p!='<' && *p!='>' )
		p++;

	if ( !*p )
		return	XT_ERROR;
  
  /*节点与节点间的内容需跳过,非节点内容 Tianhc 2005-12-01 13:45*/
  if ( *p=='<' && *(p+1)!='/' )
  {
  	 *pStP = p2;
  } 			
   
	*pEndP = p;
	*n = p - *pStP;
	return	XT_STRING;
}


/***************************************************************
 *	函数:	xn_strtokattr
 *	功能:	专用于拆分XML格式字符串中的某个节点的属性值
 *	参数一:	XML字串指针,在字符'='后开始
 *	参数二:	接收字串数据开始地址指针的指针
 *	参数三:	接收字串长度的指针
 *	参数四:	接收字串分解结束位置地址指针的指针,以'"'结束
 *	返回值:	返回分解类型见#define
 *	备注：	仅供xn_ImportXMLString函数使用
 **************************************************************/
static int xn_strtokattr( unsigned char *szXML, unsigned char**pStP, int *n, unsigned char**pEndP )
{
unsigned char	*p = szXML;

	if ( *p != '"' )
		return	XT_ERROR;

	*pStP = p++;

	if ( !*p )
		return	XT_END;

	while ( *p && *p!='<' && *p!='>' && *p!='"' )
		p++;

	if ( *p != '"' )
		return	XT_ERROR;

	*pEndP = ++p;
	*n = p - *pStP;
	return	XT_STRING;
}


/***************************************************************
 *	函数:	sl_Move
 *	功能:	将一个数据表中的所有元素移动到另一表中
 *	参数一:	接收数据顺序线性表结构指针
 *	参数二:	移出数据的顺序线性表结构指针
 *	返回值:	成功,返回0；失败,返回-1
 *	备注：	仅供xn_ImportXMLString函数使用
 **************************************************************/
static int sl_Move( PWSEQLIST lpList, PWSEQLIST lpMoveList )
{
	if ( !sl_Length(lpMoveList) )
		return	SUCC;
	if ( lpList->m_nLen+lpMoveList->m_nLen >=lpList->m_nListSize )
	{ /* 当前存储空间已满，增加分配 */
		if ( sl_OwnAppeMem(lpList,lpMoveList->m_nLen)==(-1) ) 
			return FAIL;
	}
	memcpy( lpList->m_lpBase+lpList->m_nLen,
		lpMoveList->m_lpBase,
		lpMoveList->m_nLen*sizeof(SL_ELEMTYPE));

	lpList->m_nLen+=lpMoveList->m_nLen;
	lpMoveList->m_nLen = 0;
	return SUCC;
}

/***************************************************************
 *	函数:	xn_ImportXMLString
 *	功能:	将XML格式字符串导入到XML结构指定节点下
 *	参数一:	接收引入数据的节点指针
 *	参数二:	引入的XML字串数据指针
 *	参数三:	是否替换该节点
  *	参数四:	XML指针
 *	返回值:	成功,返回SUCC；失败,返回FAIL
 **************************************************************/
#define	XML_LEV	32	/* 引入的XML字串最多层数 */
static int xn_ImportXMLString(PXMLNODE lpOwnNode, char *szImportXML, int bReplace, PXMLSTRUCT pXML )
{
unsigned char	*p,*pStr,*pTp;
int		len,nType,bSetAttr;
PXMLNODE	lpRootNode,lpTpNode;
PXMLNODEATTR lpAttr;

PXMLNODE	lpNode[XML_LEV];
int		lev,lev_save;

	lev=0;
	lev_save = 0;
	p = (unsigned char*)szImportXML;

	/* 构造临时节点，所有引入的数据节点先暂时放在该节点下 */
	if ( !(lpRootNode=xn_Build((unsigned char*)"__tp__",4,NULL,0)) )
	{
		g_xmlErrno = XML_ENOMEM;
#ifdef __XML_TRACE__
		if ( pXML->m_dwTrcType&XL_ERROR )
			trc_Write(pXML->m_hTrace,"error: 构造临时节点失败 - %s\n",xml_StringError(xml_GetLastError()));
#endif
		return	FAIL;
	}

	lpNode[lev]=lpRootNode;
	while (1) {
		nType = xn_strtok(p,&pStr,&len,&p);
		switch ( nType )
		{
		case	XT_END: /* 结束	*/
			if ( lev )
				goto	error_proc;

			if ( bReplace )
			{
				len = sl_Length(&lpRootNode->m_DownList);
				if ( bReplace<0 )
					bReplace = len+1+bReplace;
				if ( !len || len<bReplace )
				{
					g_xmlErrno = XML_EXMLNONODE;
					snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",lpOwnNode->m_pszName);
#ifdef __XML_TRACE__
					if ( pXML->m_dwTrcType&XL_ERROR )
						trc_Write(pXML->m_hTrace,"error: 替换节点[%s]失败 - %s\n",lpOwnNode->m_pszName,xml_StringError(xml_GetLastError()));
#endif
					goto	error_proc;
				}
				sl_Delete(&lpRootNode->m_DownList,bReplace,(void**)&lpTpNode);
				/* 替换节点 */
				xn_Move(lpOwnNode,lpTpNode);
			}
			else
			{
				/* 引入数据全部完成后，最后将临时节点下的所有节点数据转移到正式节点下 */
				if ( sl_Move(&lpOwnNode->m_DownList,&lpRootNode->m_DownList)==FAIL )
				{
					g_xmlErrno = XML_ENOMEM;
#ifdef __XML_TRACE__
					if ( pXML->m_dwTrcType&XL_ERROR )
						trc_Write(pXML->m_hTrace,"error: 临时节点数据转移失败 - %s\n",xml_StringError(xml_GetLastError()));
#endif
					goto	error_proc;
				}
			}
			xn_Destroy(lpRootNode);
			return	SUCC;

		case	XT_HEADLEFT: /* 开始左标识 < */
			bSetAttr = 0;
			nType=xn_strtok(p,&pStr,&len,&p);
			if (( nType!= XT_STRING ) && ( nType!= XT_SHORT ))
			{
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: 第%d字节前数据应为开始标识字串 - %s\n",(char*)p-szImportXML,xml_StringError(XML_EXMLSYNTAX));
#endif
				goto	error_proc;
			}

			if ( !(lpTpNode=xn_Build(pStr,len,NULL,0)) )
			{
				g_xmlErrno = XML_ENOMEM;
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: 构造节点'%m'失败 - %s\n",pStr,len,(char*)p-szImportXML,xml_StringError(XML_ENOMEM));
#endif
				goto	error_proc;
			}

			if ( sl_Append(&lpNode[lev]->m_DownList,(void*)lpTpNode)==-1 )
			{
				g_xmlErrno = XML_ENOMEM;
				xn_Destroy(lpTpNode);
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: 添加节点'%s'失败 - %s\n",lpTpNode->m_pszName,(char*)p-szImportXML,xml_StringError(XML_ENOMEM));
#endif
				goto	error_proc;
			}
  
			if ( ++lev>XML_LEV )
			{/* 超过最大层数 */
				g_xmlErrno = XML_EXMLMAXLEV;
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: 第%d字节开始数据 - %s\n",(char*)p-szImportXML,xml_StringError(XML_EXMLMAXLEV));
#endif
				goto	error_proc;
			}
			lpNode[lev]=lpTpNode;

      if (nType == XT_SHORT)   /*此种情况下只需加结点即可*/
      {
      	lev--;
      	break;
      }		

			while (1)
			{  /* 分解属性 */
				nType = xn_strtok(p,&pStr,&len,&p);
				if ( nType==XT_RIGHT )
				{ /* 结束右括号 */
					break;
				}
				else if ( nType==XT_SHORT )  /*遇到/>--XML2.0*/
				{
					 break;
				}	
				else if ( nType==XT_STRING )
				{ /* 属性名 */
					if ( !bSetAttr )
						bSetAttr = 1;

					if ( !(lpAttr=xna_Build(pStr,len,NULL)) )
					{
						g_xmlErrno = XML_ENOMEM;
#ifdef __XML_TRACE__
						if ( pXML->m_dwTrcType&XL_ERROR )
							trc_Write(pXML->m_hTrace,"error: 构造属性'%m'失败 - %s\n",pStr,len,(char*)p-szImportXML,xml_StringError(XML_ENOMEM));
#endif
						goto	error_proc;
					}
					if ( sl_Append(&lpNode[lev]->m_AttrList,(void*)lpAttr)==-1 )
					{
						g_xmlErrno = XML_ENOMEM;
						xna_Destroy(lpAttr);
#ifdef __XML_TRACE__
						if ( pXML->m_dwTrcType&XL_ERROR )
							trc_Write(pXML->m_hTrace,"error: 添加属性'%s'失败 - %s\n",lpAttr->m_pszName,(char*)p-szImportXML,xml_StringError(XML_ENOMEM));
#endif
						goto	error_proc;
					}

					if ( xn_strtok(p,&pStr,&len,&p)!=XT_EQUALE )
					{ /* 赋值号'=' */
#ifdef __XML_TRACE__
					if ( pXML->m_dwTrcType&XL_ERROR )
						trc_Write(pXML->m_hTrace,"error: 第%d字节前数据应为属性赋值符号'=' - %s\n",(char*)p-szImportXML,xml_StringError(XML_EXMLSYNTAX));
#endif
						goto	error_proc;
					}
					if ( xn_strtokattr(p,&pStr,&len,&p)!=XT_STRING || *pStr!='\"' || *(pStr+len-1)!='\"' )
					{ /* 属性值 */
#ifdef __XML_TRACE__
						if ( pXML->m_dwTrcType&XL_ERROR )
							trc_Write(pXML->m_hTrace,"error: 第%d字节 属性值数据'%m'缺少双引号 - %s\n",(char*)pStr-szImportXML,pStr,len,xml_StringError(XML_EXMLSYNTAX));
#endif
						goto	error_proc;
					}

					if ( xna_SetDataWithConvert(lpAttr,pStr+1,len-2)==FAIL )
					{ /* 设置属性值 */
						g_xmlErrno = XML_ENOMEM;
#ifdef __XML_TRACE__
						if ( pXML->m_dwTrcType&XL_ERROR )
							trc_Write(pXML->m_hTrace,"error: 设置属性'%s' 值 '%m' 失败 - %s\n",lpAttr->m_pszName,pStr+1,len-2,(char*)p-szImportXML,xml_StringError(XML_ENOMEM));
#endif
						goto	error_proc;
					}
				}
				else
				{
#ifdef __XML_TRACE__
					if ( pXML->m_dwTrcType&XL_ERROR )
						trc_Write(pXML->m_hTrace,"error1: 第%d字节前数据不能解析 - %s\n",(char*)p-szImportXML,xml_StringError(XML_EXMLSYNTAX));
#endif
					goto	error_proc;
				}
			}
      if ( nType==XT_SHORT )  /*无需多余处理*/
      {
      	 lev--;
      	 break;
      }		

			/* 拆分数据值 */
			if ( xn_strtokdata(p,&pStr,&len,&pTp) == XT_STRING )
			{ /* 设置数据值 */
				p = pTp;
#ifdef __ContrlAttr__ /* 不控制 */
				if ( bSetAttr )
				{ /* 该节点已经设置属性，不能作为数据节点 */
#ifdef __XML_TRACE__
					if ( pXML->m_dwTrcType&XL_ERROR )
						trc_Write(pXML->m_hTrace,"error: 第%d字节 数据'%m' 已经设置属性数据 - %s\n",(char*)pStr-szImportXML,pStr,len,xml_StringError(XML_EXMLSYNTAX));
#endif
					goto	error_proc;
				}
#endif

				/* 设置转义后的数据 */
				if ( xn_SetDataWithConvert(lpNode[lev],pStr,len)==FAIL )
				{
					g_xmlErrno = XML_ENOMEM;
#ifdef __XML_TRACE__
					if ( pXML->m_dwTrcType&XL_ERROR )
						trc_Write(pXML->m_hTrace,"error: 设置节点'%s' 值 '%m' 失败 - %s\n",lpNode[lev]->m_pszName,pStr,len,(char*)p-szImportXML,xml_StringError(XML_ENOMEM));
#endif
					goto	error_proc;
				}
				    lev_save = lev;

			}
			break;

		case	XT_TAILLEFT: /* 结束左标识 </ */
			if ( xn_strtok(p,&pStr,&len,&p)!= XT_STRING )
			{
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: 第%d字节前数据应为结束标识字串 - %s\n",(char*)p-szImportXML,xml_StringError(XML_EXMLSYNTAX));
#endif
				goto	error_proc;
			}

			if ( memicmp_(lpNode[lev]->m_pszName,pStr,len) )
			{
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: 第%d字节数据'%m'层数匹配错误,应为'%s' - %s\n",(char*)pStr-szImportXML,pStr,len,lpNode[lev]->m_pszName,xml_StringError(XML_EXMLSYNTAX));
#endif
				goto	error_proc;
			}

			if ( xn_strtok(p,&pStr,&len,&p)!= XT_RIGHT )
			{
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: 第%d字节前数据应为结束标识'>' - %s\n",(char*)p-szImportXML,xml_StringError(XML_EXMLSYNTAX));
#endif
				goto	error_proc;
			}
			lev--;
			break;

		default:
			g_xmlErrno = XML_EXMLSYNTAX;
#ifdef __XML_TRACE__
			if ( pXML->m_dwTrcType&XL_ERROR )
				trc_Write(pXML->m_hTrace,"error2: [%d]第%d字节前数据不能解析 - %s\n",nType,(char*)p-szImportXML,xml_StringError(XML_EXMLSYNTAX));
#endif
error_proc:
#ifdef __XML_TRACE__
			if ( pXML->m_dwTrcType&XL_ERROR )
				trc_Write(pXML->m_hTrace,"错误所在位置为下列已经扫描的字符前后：\n%h%m%H",szImportXML,(char*)p-szImportXML);
#endif
			xn_Destroy(lpRootNode);
			return	FAIL;	
		}
	}

/*	return	SUCC; */
}


/***************************************************************
 *	函数:	xn_ImportXMLStringKVL
 *	功能:	将KVL格式字符串导入到XML结构指定节点下
 *	参数一:	接收引入数据的节点指针
 *	参数二:	引入的XML字串数据指针
 *	参数三:	是否替换该节点
  *	参数四:	XML指针
 *	返回值:	成功,返回SUCC；失败,返回FAIL
 **************************************************************/
static int xn_ImportXMLStringKVL(PXMLNODE lpOwnNode, char *szImportXML, int bReplace, PXMLSTRUCT pXML )
{
unsigned char	*p,*pStr,*pTp;
int		len,nType,bSetAttr;
PXMLNODE	lpRootNode,lpTpNode;
PXMLNODEATTR lpAttr;

PXMLNODE	lpNode[XML_LEV];
int		lev,lev_save;

	lev=0;
	lev_save = 0;
	p = (unsigned char*)szImportXML;

	/* 构造临时节点，所有引入的数据节点先暂时放在该节点下 */
	if ( !(lpRootNode=xn_Build((unsigned char*)"__tp__",4,NULL,0)) )
	{
		g_xmlErrno = XML_ENOMEM;
#ifdef __XML_TRACE__
		if ( pXML->m_dwTrcType&XL_ERROR )
			trc_Write(pXML->m_hTrace,"error: 构造临时节点失败 - %s\n",xml_StringError(xml_GetLastError()));
#endif
		return	FAIL;
	}

	lpNode[lev]=lpRootNode;
	while (1) {
		nType = xn_strtok(p,&pStr,&len,&p);
		switch ( nType )
		{
		case	XT_END: /* 结束	*/
			if ( lev )
				goto	error_proc;

			if ( bReplace )
			{
				len = sl_Length(&lpRootNode->m_DownList);
				if ( bReplace<0 )
					bReplace = len+1+bReplace;
				if ( !len || len<bReplace )
				{
					g_xmlErrno = XML_EXMLNONODE;
					snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",lpOwnNode->m_pszName);
#ifdef __XML_TRACE__
					if ( pXML->m_dwTrcType&XL_ERROR )
						trc_Write(pXML->m_hTrace,"error: 替换节点[%s]失败 - %s\n",lpOwnNode->m_pszName,xml_StringError(xml_GetLastError()));
#endif
					goto	error_proc;
				}
				sl_Delete(&lpRootNode->m_DownList,bReplace,(void**)&lpTpNode);
				/* 替换节点 */
				xn_Move(lpOwnNode,lpTpNode);
			}
			else
			{
				/* 引入数据全部完成后，最后将临时节点下的所有节点数据转移到正式节点下 */
				if ( sl_Move(&lpOwnNode->m_DownList,&lpRootNode->m_DownList)==FAIL )
				{
					g_xmlErrno = XML_ENOMEM;
#ifdef __XML_TRACE__
					if ( pXML->m_dwTrcType&XL_ERROR )
						trc_Write(pXML->m_hTrace,"error: 临时节点数据转移失败 - %s\n",xml_StringError(xml_GetLastError()));
#endif
					goto	error_proc;
				}
			}
			xn_Destroy(lpRootNode);
			return	SUCC;

		case	XT_HEADLEFT: /* 开始左标识 < */
			bSetAttr = 0;
			nType=xn_strtok(p,&pStr,&len,&p);
			if (( nType!= XT_STRING ) && ( nType!= XT_SHORT ))
			{
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: 第%d字节前数据应为开始标识字串 - %s\n",(char*)p-szImportXML,xml_StringError(XML_EXMLSYNTAX));
#endif
				goto	error_proc;
			}

			if ( !(lpTpNode=xn_Build(pStr,len,NULL,0)) )
			{
				g_xmlErrno = XML_ENOMEM;
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: 构造节点'%m'失败 - %s\n",pStr,len,(char*)p-szImportXML,xml_StringError(XML_ENOMEM));
#endif
				goto	error_proc;
			}

			if ( sl_Append(&lpNode[lev]->m_DownList,(void*)lpTpNode)==-1 )
			{
				g_xmlErrno = XML_ENOMEM;
				xn_Destroy(lpTpNode);
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: 添加节点'%s'失败 - %s\n",lpTpNode->m_pszName,(char*)p-szImportXML,xml_StringError(XML_ENOMEM));
#endif
				goto	error_proc;
			}
  
			if ( ++lev>XML_LEV )
			{/* 超过最大层数 */
				g_xmlErrno = XML_EXMLMAXLEV;
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: 第%d字节开始数据 - %s\n",(char*)p-szImportXML,xml_StringError(XML_EXMLMAXLEV));
#endif
				goto	error_proc;
			}
			lpNode[lev]=lpTpNode;

      if (nType == XT_SHORT)   /*此种情况下只需加结点即可*/
      {
      	lev--;
      	break;
      }		

			while (1)
			{  /* 分解属性 */
				nType = xn_strtok(p,&pStr,&len,&p);
				if ( nType==XT_RIGHT )
				{ /* 结束右括号 */
					break;
				}
				else if ( nType==XT_SHORT )  /*遇到/>--XML2.0*/
				{
					 break;
				}	
				else if ( nType==XT_STRING )
				{ /* 属性名 */
					if ( !bSetAttr )
						bSetAttr = 1;

					if ( !(lpAttr=xna_Build(pStr,len,NULL)) )
					{
						g_xmlErrno = XML_ENOMEM;
#ifdef __XML_TRACE__
						if ( pXML->m_dwTrcType&XL_ERROR )
							trc_Write(pXML->m_hTrace,"error: 构造属性'%m'失败 - %s\n",pStr,len,(char*)p-szImportXML,xml_StringError(XML_ENOMEM));
#endif
						goto	error_proc;
					}
					if ( sl_Append(&lpNode[lev]->m_AttrList,(void*)lpAttr)==-1 )
					{
						g_xmlErrno = XML_ENOMEM;
						xna_Destroy(lpAttr);
#ifdef __XML_TRACE__
						if ( pXML->m_dwTrcType&XL_ERROR )
							trc_Write(pXML->m_hTrace,"error: 添加属性'%s'失败 - %s\n",lpAttr->m_pszName,(char*)p-szImportXML,xml_StringError(XML_ENOMEM));
#endif
						goto	error_proc;
					}

					if ( xn_strtok(p,&pStr,&len,&p)!=XT_EQUALE )
					{ /* 赋值号'=' */
#ifdef __XML_TRACE__
					if ( pXML->m_dwTrcType&XL_ERROR )
						trc_Write(pXML->m_hTrace,"error: 第%d字节前数据应为属性赋值符号'=' - %s\n",(char*)p-szImportXML,xml_StringError(XML_EXMLSYNTAX));
#endif
						goto	error_proc;
					}
					if ( xn_strtokattr(p,&pStr,&len,&p)!=XT_STRING || *pStr!='\"' || *(pStr+len-1)!='\"' )
					{ /* 属性值 */
#ifdef __XML_TRACE__
						if ( pXML->m_dwTrcType&XL_ERROR )
							trc_Write(pXML->m_hTrace,"error: 第%d字节 属性值数据'%m'缺少双引号 - %s\n",(char*)pStr-szImportXML,pStr,len,xml_StringError(XML_EXMLSYNTAX));
#endif
						goto	error_proc;
					}

					if ( xna_SetDataWithConvert(lpAttr,pStr+1,len-2)==FAIL )
					{ /* 设置属性值 */
						g_xmlErrno = XML_ENOMEM;
#ifdef __XML_TRACE__
						if ( pXML->m_dwTrcType&XL_ERROR )
							trc_Write(pXML->m_hTrace,"error: 设置属性'%s' 值 '%m' 失败 - %s\n",lpAttr->m_pszName,pStr+1,len-2,(char*)p-szImportXML,xml_StringError(XML_ENOMEM));
#endif
						goto	error_proc;
					}
				}
				else
				{
#ifdef __XML_TRACE__
					if ( pXML->m_dwTrcType&XL_ERROR )
						trc_Write(pXML->m_hTrace,"error1: 第%d字节前数据不能解析 - %s\n",(char*)p-szImportXML,xml_StringError(XML_EXMLSYNTAX));
#endif
					goto	error_proc;
				}
			}
      if ( nType==XT_SHORT )  /*无需多余处理*/
      {
      	 lev--;
      	 break;
      }		

			/* 拆分数据值 */
			if ( xn_strtokdata(p,&pStr,&len,&pTp) == XT_STRING )
			{ /* 设置数据值 */
				p = pTp;
#ifdef __ContrlAttr__ /* 不控制 */
				if ( bSetAttr )
				{ /* 该节点已经设置属性，不能作为数据节点 */
#ifdef __XML_TRACE__
					if ( pXML->m_dwTrcType&XL_ERROR )
						trc_Write(pXML->m_hTrace,"error: 第%d字节 数据'%m' 已经设置属性数据 - %s\n",(char*)pStr-szImportXML,pStr,len,xml_StringError(XML_EXMLSYNTAX));
#endif
					goto	error_proc;
				}
#endif

				/* 设置转义后的数据 */
				if ( xn_SetDataWithConvert(lpNode[lev],pStr,len)==FAIL )
				{
					g_xmlErrno = XML_ENOMEM;
#ifdef __XML_TRACE__
					if ( pXML->m_dwTrcType&XL_ERROR )
						trc_Write(pXML->m_hTrace,"error: 设置节点'%s' 值 '%m' 失败 - %s\n",lpNode[lev]->m_pszName,pStr,len,(char*)p-szImportXML,xml_StringError(XML_ENOMEM));
#endif
					goto	error_proc;
				}
				    lev_save = lev;

			}
			break;

		case	XT_TAILLEFT: /* 结束左标识 </ */
			if ( xn_strtok(p,&pStr,&len,&p)!= XT_RIGHT )
			{
#ifdef __XML_TRACE__
				if ( pXML->m_dwTrcType&XL_ERROR )
					trc_Write(pXML->m_hTrace,"error: 第%d字节前数据应为结束标识'>' - %s\n",(char*)p-szImportXML,xml_StringError(XML_EXMLSYNTAX));
#endif
				goto	error_proc;
			}
			lev--;
			break;

		default:
			g_xmlErrno = XML_EXMLSYNTAX;
#ifdef __XML_TRACE__
			if ( pXML->m_dwTrcType&XL_ERROR )
				trc_Write(pXML->m_hTrace,"error2: [%d]第%d字节前数据不能解析 - %s\n",nType,(char*)p-szImportXML,xml_StringError(XML_EXMLSYNTAX));
#endif
error_proc:
#ifdef __XML_TRACE__
			if ( pXML->m_dwTrcType&XL_ERROR )
				trc_Write(pXML->m_hTrace,"错误所在位置为下列已经扫描的字符前后：\n%h%m%H",szImportXML,(char*)p-szImportXML);
#endif
			xn_Destroy(lpRootNode);
			return	FAIL;	
		}
	}

/*	return	SUCC; */
}


/**********************************************************************
 *	函数:	splitstr
 *	功能:	从一个字符串中依据空隔分离出所有域
 *	参数一:	字符串命令指针
 *	参数三:	存放域的字符串的指针数组
 *	参数四:	指针数组大小
 *	返回值:	成功,返回实际分离出的域个数;失败返回0
 *	备注：	仅供xn_ExportDSRString,xn_ImportDSRString函数使用
 **********************************************************************/
static int splitstr( unsigned char *szData, unsigned char *szField[], size_t num )
{
size_t		n=(int)num;
unsigned char *pszStr = (unsigned char*)szData;

	if( !pszStr || !pszStr[0] || !szField || !n )
		return	0;

	*szField++ = pszStr;
	while ( *pszStr && n ) {
		if ( *pszStr<127 && isspace(*pszStr) )
		{
			*pszStr++ = '\0';
			while ( isspace(*pszStr) )
				pszStr++;
			if ( --n )
				*szField++ = pszStr;
		}
		else
			pszStr++;
	}
	*pszStr='\0';

	return	num-n;
}


/**********************************************************************
 *	函数:	strchg_
 *	功能:	整理字符串，解释所有的转义字符
 *	参数一:	字符串指针
 *	参数二:	接收整理后字符串的缓冲区指针
 *	参数二:	缓冲区大小
 *	返回值:	成功,返回字符个数;失败返回-1
 **********************************************************************/
static int strchg_( char *pStr, char *pBuf, size_t size )
{
char    *p,*q;
size_t	n;
int		i,data;

    p = pStr;
	q = pBuf;
	n = size;
    
/*	while ( *p && n > 1 ) */
	while ( *p && n )
	{
		if ( *p == '\\' )
		{
			if ( p[1]=='\r' && p[2]=='n' )
			{	/* 回车换行 */
				p+=3;
			}
			else if ( p[1]=='\n' )
			{ /* 换行 */
				p+=2;
			}
			else
			{
				if ( p[1]=='n' || p[1]=='N' )
				{
					p+=2; *q++ = '\n';
				}
				else if ( p[1]=='t' || p[1]=='T' )
				{
					p+=2; *q++ = '\t';
				}
				else if ( p[1]=='v' || p[1]=='V' )
				{
					p+=2; *q++ = '\v';
				}
				else if ( p[1]=='r' || p[1]=='R' )
				{
					p+=2; *q++ = '\r';
				}
				else if ( p[1]=='0' && (p[2]=='x'||p[2]=='X') )
				{	/* 16进制数处理 */
					p+=2;data=0;
					for ( p++,data=i=0; i<2; i++,p++ )
					{
						if ( isdigit(*p) )
						{
							data = data*16 + *p-'0';
						}
						else if ( *p>='A' && *p<='F' )
						{
							data = data*16 + *p - 55;
						}
						else if ( *p>='a' && *p<='f' )
						{
							data = data*16 + *p - 87;
						}
						else
						{
							break;
						}
					}
					if ( i )
					{
						*q++=data;
					}
				}
				else
				{/* 10进制数处理 */
					for ( p++,data=i=0; i<3; i++,p++ )
					{
						if ( isdigit(*p) )
						{
							data = data*10+*p-'0';
						}
						else
						{
							break;
						}
					}
					if ( !i )
					{
						*q++ = *p++;
					}
					else if (data>256)
					{
						*q++ = data/10;
						p--;
					}
					else
					{
						*q++ = data;
					}
				}
				n--;
			}
		}
		else
		{
			*q++ = *p++;
			n--;
		}
	}
	*q = '\0';
	return	size-n;
}

/**********************************************************************
 *	函数:	xn_Setquot
 *	功能:	设置内部对"字符的转换标准,1-导出为&quot;(XML标准),0-导出为&quote;旧系统错误的标准
 *	参数一:	模式
 *	备注：	只为兼容旧模式的一个补丁
 *       如果改为新标准，则旧系统需合部重新编译,包括ASPK+IFI所有应用,有较大代价且易出问题
 **********************************************************************/
static void xn_Setquot( int imode )
{
  g_xmlquot = imode;
}

#ifdef __cplusplus
}
#endif /* end of __cplusplus */


#endif /* end of  __XMLNODE_C__ */


/**********************************************************************
 *	函数：	xn_Rename
 *	功能:	节点改名
 *	参数:
	pNode
		节点结构指针。
	sname_new
		新的节点名称
 *	返回值:
	成功	返回SUCC
	失败	返回FAIL
 **********************************************************************/
static int xn_Rename( PXMLNODE pNode, char *sname_new)
{
	if ((pNode == NULL ) || (sname_new[0] == 0))
		 return -1;
	free( pNode->m_pszName );
	if ( !(pNode->m_pszName=(unsigned char*)malloc(strlen(sname_new)+1)) )
     return -2;
	strcpy(pNode->m_pszName,sname_new);
	pNode->m_pszName[strlen(sname_new)]='\0';
  return 0;   	
}	
