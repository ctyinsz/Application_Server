/************************************************************************
版权所有:杭州恒生电子股份有限公司
项目名称:公共项目
版    本:V1.0
操作系统:AIX4.2,SCO5
文件名称:trace.h
文件描述:简单XML操作函数库辅助跟踪函数
项 目 组:中间业务产品研发组
程 序 员:中间业务产品研发组
发布日期:2001年09月11日
修    订:中间业务产品研发组
修改日期:2002年05月12日
************************************************************************/
/*
修改记录
修改日期:
修改内容:
修改人:
*/
#ifndef __TRACE_H__
#define __TRACE_H__

#include  <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* end of __cplusplus */


typedef int  HTRACE;


/* 构造TRACE结构 */
HTRACE trc_Build( char* pszFile, char *pszBakDir );

/* 销毁TRACE结构 */
int trc_Destroy( HTRACE hTrace );

/* 设置获取属性 */
#define  TFS_ALWAYSOPEN  1 /* 文件是否一直打开 */
#define  TFS_FLUSH    2 /* 文件是否无缓冲（立即刷新）*/
#define  TFS_MAXSIZE    3 /* 文件记录最大长度 */
#define  TFS_MAXHOUR    4 /* 文件记录最大小时数 */
#define  TFS_DAYTIME    5 /* 记录文件每天转移时间 */
#define  TFS_WEEKDAY    6 /* 记录文件每周转移时间 */
#define  TFS_MONTHDAY  7 /* 记录文件每月转移时间 */
int trc_SetAttr( HTRACE hTrace, int iAttrType, unsigned int uiAttrData );
int trc_GetAttr( HTRACE hTrace, int iAttrType, unsigned int *puiAttrData );

/* 记录TRACE */
int trc_Write( HTRACE hTrace, char *pszFmt, ... );


/* 强制完成TRACE文件移动 */
int trc_MoveFile( HTRACE hTrace );

/* 获取版本号 */
unsigned int trc_GetVerion();

#ifdef __cplusplus
}
#endif /* end of __cplusplus */



#endif /* end of __TRACE_H__ */
