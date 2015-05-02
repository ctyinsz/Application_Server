/************************************************************************
版权所有:杭州恒生电子股份有限公司
项目名称:公共项目
版    本:V1.0
操作系统:AIX4.2,SCO5
文件名称:xmle.h
文件描述:简单XML操作函数库扩展函数
项 目 组:中间业务产品研发组
程 序 员:中间业务产品研发组
发布日期:2001年09月11日
修    订:中间业务产品研发组
修改日期:2002年05月12日
************************************************************************/

/*
修改记录
修改日期:
修改内容:增加直接XML文件导入导出函数。
修改人:陈鹏飞
*/

#ifndef __XMLE_H__
#define __XMLE_H__

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "xml.h"


#define  MAX_NAMEPATH 1024
/*******************************************************************************
函数:xml_GetElemntE
功能:对XML数据访问封装,根据路径访问节点或属性
参数:
    hXML
                指定XML设备句柄
        szElementName
              
              节点路径表示:
              
                指定在XML结构要获取的元素的名称，形如"/PRIVATE|2/电费单价|0"
                "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
                NMAE    指定每一层元素名,
                IDX             指定每一层元素在同名兄弟元素中的排序位置。
                        如果iIndex为0，表示取最后一个元素，
                        如果为1，2，3……，表示取相应编号的元素。
                        缺省为1
              属性路径表示:
                先由节点指定规则指定在XML结构中的节点。
                在通过@符号指定节点的属性。
                形如"/PRIVATE|2/电费单价@单位
                "/PRIVATE|2/电费单价"指定节点路径，"@单位"指定节点属性。
        pucElementBuff
                接收获取元素值的缓冲区指针
        iElementBuffSize
                缓冲区大小
返回值:
        成功    返回SUCC
        失败    返回FAIL    
******************************************************************************/
int xml_GetElementE(HXMLTREE hXML,
                   const char *szElementName,
                   char *pucElementBuff,
                   const size_t uiElementBuffSize );


/**************************************************************************************
函数:xml_SetElemntE
功能:对XML数据访问封装,根据路径设置节点或属性值
参数:
*      功能:    在XML树中设置一个叶子节点或某一节点上的属性
                叶子节点(数据节点)可以重复存在
                如果该节点不存在, 添加该节点;
                        如果路径节点不存在, 则连同这些节点一起添加
                如果该节点已存在, 修改该节点数据;
 *      参数:
        hXML
                指定XML设备句柄
        szElementName
              节点路径表示:
              
                指定在XML结构要获取的元素的名称，形如"/PRIVATE|2/电费单价|0"
                "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
                NMAE    指定每一层元素名,
                IDX             指定每一层元素在同名兄弟元素中的排序位置。
                        如果iIndex为0，表示取最后一个元素，
                        如果为1，2，3……，表示取相应编号的元素。
                        缺省为1
              属性路径表示:
                先由节点指定规则指定在XML结构中的节点。
                在通过@符号指定节点的属性。
                形如"/PRIVATE|2/电费单价@单位
                "/PRIVATE|2/电费单价"指定节点路径，"@单位"指定节点属性。
        szElementValue
                待设置的元素值指针
                节点分为数据节点和属性节点

返回值:
    成功    返回SUCC
        失败    返回FAIL    
*****************************************************************************/
int xml_SetElementE( HXMLTREE hXML,
                   const char *szElementName,
                   const char *szElementValue );

/***************************************************************************
功能:设置xml中项目值，转换实际ASCII内容数据以字符hex存储。
参数:  HXMLTREE hXML   xml的句柄值
    const char *szElementName  节点路径表示
    const *szElementValue,  原值缓冲
    int szElementLen    原值长度
返回值:=0成功 
    <0失败
***************************************************************************/
int xml_SetBinElement(HXMLTREE hXML,
           const char *szElementName,     
           char *szElementValue,
           int szElementLen);

/****************************************************************************
功能:把xml中以字符hex存储的内容转换为实际的ASCII内容数据
参数:  HXMLTREE hXML    XML句柄
    const *szElementName  节点路径表示
    char *pucElementBuff,  取值缓冲
    const size_t uiElementBuffSize  缓冲长度
返回值:>=0  取到值,并放入缓冲的长度
    <0 失败
*****************************************************************************/
int xml_GetBinElement(HXMLTREE hXML,
           const char *szElementName,
           char *pucElementBuff,
                      const size_t uiElementBuffSize );

/******************************************************************************
功能:倒入子xml树内容数据到指定节点，如果该节点不存在则增加
参数:  HXMLTREE hXML    XML句柄
      const char *szImportXML,
      const char *szImportName,
      int bReplace 返回值:>=0  取到值,并放入缓冲的长度      
功能:  <0 失败  0  成功
*******************************************************************************/
int xml_ImportXMLStringE( HXMLTREE hXML,
      const char *szImportXML,
      const char *szImportName
      );

/******************************************************************************
功能:处理输入的串中的XML项目，用确定值进行替换，
参数:  HXMLTREE hXML    XML句柄
      const char *pstrbuf, 原输入串缓冲
      int isize;缓冲大小。      
    如果'[' 间所指节点不存在则返回‘’.符号'\'为'[',']'的转意符。
功能:  <0 失败  0  成功
*******************************************************************************/
int xml_ParseXMLString(HXMLTREE hXML,char * sexp,int isize);

int xml_ItemToString(HXMLTREE hXML,char * sexp,int isize);

/************************************************************************
函数声明:HXMLTREE xml_CreateXMLFromFile(char *sFileName)
函数功能:由文件内容生成XML结构
输入参数:XML格式文件名称
输出参数:
          >0--成功 XML 句柄
          -1--失败
项 目 组:中间业务产品研发组
程 序 员:陈鹏飞
发布日期:2000年01月01日
修改日期:2002年04月01日
************************************************************************/
HXMLTREE xml_CreateXMLFromFile(char *sFileName);


/**********************************************************************
 *  函数：  xml_GetBinElementLen
 *  功能: 获取XML树中指定二进制节点数据长度
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要获取的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
 *  返回值:
  成功  返回实际字节数
  失败  返回FAIL
项 目 组:中间业务产品研发组
程 序 员:
发布日期:2000年01月01日
修改日期:2002年04月01日
例:
 **********************************************************************/
int xml_GetBinElementLen( HXMLTREE hXML,const char *szElementName);

/**********************************************************************
 *  函数：  xml_TreeUpdata
 *  功能: 用源xml树中数据更新目标树xml中节点数据
 *  参数:
  sxml  
    源xml树句柄
  dxml
	目地树句柄
  dpathname
    指定目的XML树中要更新根节点的名称，形如"/PRIVATE"
    "/NAME1|IDX1/NAME|2
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
使用说明：数据节点更新过程
		  如果子节点同名则替换目的树中节点的内容
		  如果目的树中不存在则新增节点
		  不影响目的树中的其他节点
		限制：对于同目录下的同名节点作为一个节点处理。
 *  返回值:
  成功  0
  失败  -1
项 目 组:中间业务产品研发组
程 序 员:huangfeng
发布日期:2002年07月19日
修改日期:2002年07月19日
例:
***********************************************************************/
int xml_TreeUpdata(HXMLTREE sxml,HXMLTREE dxml,char *dpathname);

/************************************************************************
函数声明:
int xml_CopyElement(HXMLTREE hXML,
                    const char *szSrcElementName,
                    const char *szDecElementName)
函数功能:拷贝两个XML元素内容
输入参数: 
         HXMLTREE hXML                  XML句柄
         const char *szSrcElementName   源元素路径
         const char *szDecElementName   目标元素路径
返    回: 
         0  拷贝成功
         -1 拷贝失败
限    制:
只能拷贝两个叶子节点
内容最大长度不超过4096字节

项 目 组:中间业务产品研发组
程 序 员:张勇钢
发布日期:2000年01月01日
修改日期:2002年04月01日
例:
************************************************************************/
int xml_CopyElement(HXMLTREE hXML,
                    const char *szSrcElementName,
                    const char *szDecElementName);
/************************************************************************
 * 函数声明:HXMLTREE xml_CreateXMLFromFile_OPTROOT(char *sFileName, int ioptroot)
 * 函数功能:由文件内容生成XML结构
 * 输入参数:sFileName      XML格式文件名称
 *          ioptroot       是否替换根节点
 *              >0 替换 用XML字串中第一层上从左到右顺序指定位置节点替换
 *              <0 替换 用XML字串中第一层上从右到左顺序指定位置节点替换
 *              0  不替换,所有数据引入到该节点下
 *                             
 * 输出参数:
 *           >0--成功 XML 句柄
 *           <0--失败
 *           -2--文件为空
 * 项 目 组:中间业务产品研发组
 * 程 序 员:SRC
 * ************************************************************************/
HXMLTREE xml_CreateXMLFromFile_OPTROOT(char *sFileName, int ioptroot);

#endif /*End of __XMLE_H__ */
