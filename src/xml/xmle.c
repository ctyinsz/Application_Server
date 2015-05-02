/************************************************************************
版权所有:杭州恒生电子股份有限公司
项目名称:公共项目
版    本:V1.0
操作系统:AIX4.2,SCO5
文件名称:xmle.c
文件描述:简单XML操作函数库扩展函数
项 目 组:中间业务产品研发组
程 序 员:中间业务产品研发组
发布日期:2001年09月11日
修改日期:2002年05月12日
************************************************************************/
/*
修改记录
修改日期:
修改内容:增加直接XML文件导入导出函数。
修改人:陈鹏飞

修改日期:2002.07.18
修改内容:更新式导源xml中数据到目的xml中:函数 xml_TreeUpdata
		 具体调用接口参见函数说明
修改人:黄峰

修改日期:2005-08-09 14:51
修改内容:增加函数：
	   xml_XCopy			用于结点的复制(包括属性)
	   xml_node_copy	用于叶子节点的复制(包括属性)
	   xml_Conv2to1		用于将XML 2.0格式转为 XML 1.0格式
修改人:田欢春

*/
#include  "xmlnode.c"

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "xml.h"
#include "xmle.h"

PXMLNODE xml_GetNode( HXMLTREE hXML, const char *szElementName);


#define  MAX_NAMEPATH 1024
/******************************************************************************
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
                   const size_t uiElementBuffSize )
{
char *ptmp;
char stmp[MAX_NAMEPATH];
  memset(stmp,0,sizeof(stmp));
  memset(pucElementBuff,0,uiElementBuffSize);
  if((ptmp=strchr(szElementName,'@'))!=NULL)
  {
      memcpy(stmp,szElementName,ptmp-szElementName);
      return xml_GetElementAttr( hXML,stmp,ptmp+1, \
                        pucElementBuff,uiElementBuffSize);
  }
  else
    return xml_GetElement(hXML,szElementName,pucElementBuff,uiElementBuffSize);
}

/******************************************************************************
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
                   const char *szElementValue )
{
char *ptmp;
char stmp[MAX_NAMEPATH];
    memset(stmp,0,sizeof(stmp));
    if((ptmp=strchr(szElementName,'@'))!=NULL)
    {
        memcpy(stmp,szElementName,ptmp-szElementName);
        /*  目的节点不存在 */
        if(xml_ElementExist(hXML,stmp )==0)
        {
            if(xml_AddElement(hXML,stmp,"") == FAIL)
              return FAIL;
        }
        return xml_SetElementAttr( hXML,stmp,ptmp+1, \
                          szElementValue);
    }
    else return xml_SetElement(hXML,szElementName,szElementValue);
}

/***************************************************************************
功能:设置xml中项目值，转换实际ASCII内容数据以字符hex存储。
参数:  HXMLTREE hXML   xml的句柄值
    const char *szElementName 节点路径表示
    const *szElementValue,  原值缓冲
    int szElementLen    原值长度
返回值:
        成功    返回SUCC
        失败    返回FAIL
***************************************************************************/
int xml_SetBinElement(HXMLTREE hXML,
           const char *szElementName,
           char *szElementValue,
           int szElementLen)
{
  char * ptmp;
  if((ptmp=(char *)malloc(szElementLen*2+1))==NULL)
    return FAIL;
  memset(ptmp,0,szElementLen*2+1);
  if(xml_asctohex(ptmp,szElementLen*2,szElementValue,szElementLen)==0)
  {
    free(ptmp);
    return FAIL;
  }
  if(xml_SetElement(hXML,szElementName,ptmp)== FAIL)
  {
    free(ptmp);
    return FAIL;
  }
  free(ptmp);
  return SUCC;
}

/****************************************************************************
功能:把xml中以字符hex存储的内容转换为实际的ASCII内容数据
参数:  HXMLTREE hXML   XML句柄
    const *szElementName  节点路径表示
    char *pucElementBuff, 取值缓冲
    const size_t uiElementBuffSize  缓冲长度
返回值:
        >=0 取到值,并放入缓冲的长度
        失败    返回FAIL
*****************************************************************************/
int xml_GetBinElement(HXMLTREE hXML,
           const char *szElementName,
           char *pucElementBuff,
                     const size_t uiElementBuffSize )
{
  char * ptmp;
  int ilen;
  memset( pucElementBuff,0,uiElementBuffSize);
  if((ptmp=(char *)malloc(uiElementBuffSize*2+1))==NULL)
    return FAIL;
  memset(ptmp,0,uiElementBuffSize*2+1);
  ilen=uiElementBuffSize*2;
  if(xml_GetElement(hXML,szElementName,ptmp,ilen)== FAIL)
  {
    free(ptmp);
    return FAIL;
  }
  ilen=strlen(ptmp);
  if((ilen=xml_hextoasc(pucElementBuff,uiElementBuffSize,ptmp,ilen)) == 0)
  {
    free(ptmp);
    return FAIL;
  }
  free(ptmp);
  return ilen;
}
/******************************************************************************
功能:倒入子xml树内容数据到指定节点，如果该节点不存在则增加
参数:  HXMLTREE hXML   XML句柄
      const char *szImportXML,
      const char *szImportName,
      int bReplace 返回值:>=0  取到值,并放入缓冲的长度
功能:
        成功    返回SUCC
        失败    返回FAIL
*******************************************************************************/
int xml_ImportXMLStringE( HXMLTREE hXML,
      const char *szImportXML,
      const char *szImportName)
{
char sPath[MAX_NAMEPATH];
int icount;
    if((icount=xml_ElementCount(hXML,szImportName)) == FAIL)
      return FAIL;
    sprintf(sPath,"%s|%d",szImportName,icount+1);
    /*  不存在目的节点  */
    if(xml_AddElement(hXML,sPath,"") == FAIL)
      return FAIL;
    if(xml_ImportXMLString(hXML,szImportXML,sPath,0) == FAIL)
      return FAIL;
    return SUCC;
}
/******************************************************************************
功能:处理输入的串中的XML项目，用确定值进行替换，
参数:  HXMLTREE hXML   XML句柄
      const char *pstrbuf, 原输入串缓冲
      int isize;缓冲大小。
    如果'[' 间所指节点不存在则返回‘’.符号'\'为'[',']'的转意符。
功能:
        成功    返回SUCC
        失败    返回FAIL
*******************************************************************************/
int xml_ParseXMLString(HXMLTREE hXML,char * sexp,int isize)
{
  int istatu=0;
  int ilenth,i,j=0,k=0,imax;
  char *stmp,*sresult,clast=0;

  imax=isize;
  ilenth=strlen(sexp);
  if((stmp=(char *)malloc(isize))==NULL)
    return FAIL;
  memset(stmp,0,isize);
  if((sresult=(char *)malloc(isize))==NULL)
  {
    free(stmp);
    return FAIL;
  }
  memset(sresult,0,isize);

  for(i=0;i<ilenth;i++)
  {
    if((sexp[i]=='[')&&( clast!='\\'))
    {
      if(istatu==0)
      {
        memset(stmp,0,isize);
        j=0;
      }
      else
        stmp[j++]=sexp[i];
      istatu++;
    }
    else if((sexp[i]==']' )&&(clast!='\\'))
    {
      istatu--;
      if(istatu==0)
      {
        if((xml_ItemToString(hXML,stmp,isize)) == FAIL)
        {
          free(sresult);
          free(stmp);
          return FAIL;
        }
        k+=strlen(stmp);
        if(k>isize){
          if( realloc(sresult,k+isize)==NULL)
          {
            free(sresult);
            free(stmp);
            return FAIL;
          }
          if( realloc(stmp,k+isize)==NULL)
          {
            free(sresult);
            free(stmp);
            return FAIL;
          }
          isize=k+isize;
        }
        strcat(sresult,stmp);
      }
      else stmp[j++]=sexp[i];
    }
    else
    {
      if(((sexp[i]=='[')&&(clast=='\\'))||((sexp[i]==']')&&(clast=='\\')))
      {
        if(istatu!=0)
          stmp[j-1]=sexp[i];
        else sresult[k-1]=sexp[i];
      }
      else{
        if(istatu!=0)
          stmp[j++]=sexp[i];
        else sresult[k++]=sexp[i];
      }
      if(k>=isize){
        if( realloc(sresult,k+isize)==NULL)
        {
          free(sresult);
          free(stmp);
          return FAIL;
        }
        if( realloc(stmp,k+isize)==NULL)
        {
          free(sresult);
          free(stmp);
          return FAIL;
        }
        isize=k+isize;
      }
    }
    sresult[k]=0;
    clast=sexp[i];
    if(istatu<0)
    {
      free(sresult);
      free(stmp);
      return FAIL;
    }
  }
  if(istatu!=0)
  {
    free(sresult);
    free(stmp);
    return FAIL;
  }
  strncpy(sexp,sresult,imax-1);
  free(sresult);
  free(stmp);
  return SUCC;
}

int xml_ItemToString(HXMLTREE hXML,char * sexp,int isize)
{
  int istatu=0;
  int ilenth,i,j=0,k=0,imax;
  char *sresult,*stmp,*stmpres, clast=0;

  imax=isize;
  ilenth=strlen(sexp);
  if((stmp=malloc(isize))==NULL)
    return FAIL;
  memset(stmp,0,isize);
  if((sresult=malloc(isize))==NULL)
  {
    free(stmp);
    return FAIL;
  }
  memset(sresult,0,isize);
  if((stmpres=malloc(isize))==NULL)
  {
    free(sresult);
    free(stmp);
    return FAIL;
  }
  memset(stmpres,0,isize);

  for(i=0;i<ilenth;i++)
  {
    if((sexp[i]=='[')&&(clast!='\\'))
    {
      if(istatu==0)
      {
        memset(stmp,0,sizeof(stmp));
        j=0;
      }
      else
        stmp[j++]=sexp[i];
      istatu++;
    }
    else if((sexp[i]==']')&&(clast!='\\'))
    {
      istatu--;
      if(istatu==0)
      {
        if((xml_ItemToString(hXML,stmp,isize))== FAIL)
        {
          free(stmpres);
          free(sresult);
          free(stmp);
          return FAIL;
        }
        k+=strlen(stmp);
        if(k>=isize)
        {
          if( realloc(sresult,k+isize)==NULL)
          {
            free(stmpres);
            free(sresult);
            free(stmp);
            return FAIL;
          }
          if( realloc(stmp,k+isize)==NULL)
          {
            free(stmpres);
            free(sresult);
            free(stmp);
            return FAIL;
          }
          isize=k+isize;
        }
        strcat(sresult,stmp);
      }
      else stmp[j++]=sexp[i];
    }
    else
    {
      if(((sexp[i]=='[')&&(clast=='\\'))||((sexp[i]==']')&&(clast=='\\')))
      {
        if(istatu!=0)
          stmp[j-1]=sexp[i];
        else sresult[k-1]=sexp[i];
      }
      else
      {
        if(istatu!=0)
          stmp[j++]=sexp[i];
        else sresult[k++]=sexp[i];
      }
    }
    if(k>=isize)
    {
      if( realloc(sresult,k+isize)==NULL)
      {
        free(stmpres);
        free(sresult);
        free(stmp);
        return FAIL;
      }
      if( realloc(stmp,k+isize)==NULL)
      {
        free(stmpres);
        free(sresult);
        free(stmp);
        return FAIL;
      }
      isize=k+isize;
    }
    sresult[k]=0;
    clast=sexp[i];
    if(istatu<0)
    {
      free(stmpres);
      free(sresult);
      free(stmp);
      return FAIL;
    }
  }
  if(istatu!=0)
  {
    free(stmpres);
    free(sresult);
    free(stmp);
    return FAIL;
  }
  if((xml_GetElementE(hXML,sresult,stmpres,isize)== FAIL)
     &&(xml_GetLastError()!=XML_ENONODE))
  {
    free(stmpres);
    free(sresult);
    free(stmp);
    return FAIL;
  }
  strncpy(sexp,stmpres,imax-1);
  free(stmpres);
  free(sresult);
  free(stmp);
  return SUCC;
}



/************************************************************************
函数声明:HXMLTREE xml_CreateXMLFromFile(char *sFileName)
函数功能:由文件内容生成XML结构
输入参数:XML格式文件名称
输出参数:
          >0--成功 XML 句柄
          <0--失败
          -2--文件为空
项 目 组:中间业务产品研发组
程 序 员:陈鹏飞
发布日期:2000年01月01日
修改日期:2002年04月01日
************************************************************************/
HXMLTREE xml_CreateXMLFromFile(char *sFileName)
{
  /*函数调用返回*/
  int iResult;

  /*XML结构句柄*/
  HXMLTREE iHandleXML;

  /*文件指针*/
  FILE *pfFile;

  caddr_t sFileHead;
  struct stat struStat;

  /******************************函数开始*******************************/
  /*打开XML格式的文件*/
  pfFile = fopen(sFileName, "r");
  if (pfFile == NULL)
    return FAIL;

  /*将文件映射到内存*/
  fstat(fileno(pfFile), &struStat);
  if ( struStat.st_size == 0 )
    return FAIL;

  if ((sFileHead = mmap(NULL, (size_t)struStat.st_size, PROT_READ, MAP_SHARED,
      fileno(pfFile), (off_t)0)) == (caddr_t) - 1)
  {
    munmap(sFileHead, (size_t)struStat.st_size);
    fclose(pfFile);
    return FAIL;
  }

  /*初始化XML结构*/
  iHandleXML = xml_Create("root");
  if (iHandleXML == -1)
  {
    munmap(sFileHead, (size_t)struStat.st_size);
    fclose(pfFile);
    return FAIL;
  }

  /*将文件内容导入XML结构中*/
  iResult = xml_ImportXMLString(iHandleXML, sFileHead, NULL, 1);
  if (iResult == FAIL)
  {
    munmap(sFileHead, (size_t)struStat.st_size);
    fclose(pfFile);
    return FAIL;
  }

  munmap(sFileHead, (size_t)struStat.st_size);

  /*关闭文件*/
  fclose(pfFile);

  /*函数返回*/
  return iHandleXML;
}

/************************************************************************
函数声明:int xml_CreateXMLFileFromTree(HXMLTREE iXmlhandle,char *sFileName)
函数功能:由XML结构生成文件内容
输入参数:XML句柄、XML格式文件名称
输出参数:
          >0--成功
          -1--文件打开失败失败
          -2--参数错误
项 目 组:中间业务产品研发组
程 序 员:张勇钢
发布日期:2000年01月01日
修改日期:2002年04月01日
************************************************************************/
int xml_CreateXMLFileFromTree(HXMLTREE iXmlhandle,char *sFileName)
{
  int iResult;
  FILE *pfFile;
  int iFileSize;
  if ( !*sFileName || !sFileName || !iXmlhandle)
    return FAIL;
  pfFile = fopen(sFileName, "w");
  if (pfFile == NULL)
    return FAIL;

  fclose(pfFile);

  return SUCC;
}


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
int xml_GetBinElementLen( HXMLTREE hXML,const char *szElementName)
{
  return xml_GetElementLen( hXML,szElementName)/2;
}
/**************************************************************
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
        成功    返回SUCC
        失败    返回FAIL
项 目 组:中间业务产品研发组
程 序 员:huangfeng
发布日期:2002年07月19日
修改日期:2002年07月19日
例:
**************************************************************/
int xml_TreeUpdata(HXMLTREE sxml,HXMLTREE dxml,char *dpathname)
{
 char dcurDIR[101];
 char scurDIR[101];
 int icount,i;
 char sname[101];

 memset(dcurDIR,0,sizeof(dcurDIR));
 memset(scurDIR,0,sizeof(scurDIR));

 icount=xml_ChildElementCount(sxml,"/");
 if(icount== FAIL) return FAIL;
 for(i=1;i<=icount;i++)
 {
 	 memset(sname,0,sizeof(sname));
 	 if(xml_GetChildElementName(sxml,"/",i,sname,sizeof(sname))==FAIL)
 	   return FAIL;
 	 snprintf(scurDIR,sizeof(scurDIR),"/%s",sname);
 	 snprintf(dcurDIR,sizeof(dcurDIR),"%s/%s",dpathname,sname);
 	 if(xn_SubExchang(sxml,dxml,scurDIR,dcurDIR)== FAIL)
 	   return FAIL;
  }
  return SUCC;
}
/********************************************************
* 函数：xn_subExchang
* 功能：仅提供给xml_TreeUpdata调用,树中节点的更新转换
* 简单复制，对于同名节点,属性均无处理，不建议使用 By Tianhc
**********************************************************/
int xn_SubExchang( HXMLTREE sxml, HXMLTREE dxml,char *spathname,char *dpathname)
{
 int chlcount,i;
 char  scurDIR[101],dcurDIR[101];
 char databuf[2048];

 memset(databuf,0,sizeof(databuf));
 if(xml_IsLeafNode(sxml,spathname))
 {
 	if(xml_GetElement(sxml,spathname,databuf,sizeof(databuf)) == FAIL)
 		return FAIL;
 	if(xml_SetElement(dxml,dpathname,databuf) == FAIL)
 		return FAIL;
 }
 else
 {
   if((chlcount=xml_ChildElementCount(sxml,spathname))== FAIL)
   	  return FAIL;
   for(i=1;i<=chlcount;i++)
   {
 	  if(xml_GetChildElementName(sxml,spathname,i,databuf,sizeof(databuf))== FAIL)
 		return FAIL;
 	  snprintf(scurDIR,sizeof(scurDIR),"%s/%s",spathname,databuf);
 	  snprintf(dcurDIR,sizeof(dcurDIR),"%s/%s",dpathname,databuf);
 	  if(xn_SubExchang(sxml,dxml,scurDIR,dcurDIR)== FAIL)
 	  	return FAIL;
   }
 }
 return SUCC;
}
/************************************************************************
函数声明:
int xml_CopyElement(HXMLTREE hXML,
                    const char *szSrcElementName,
                    const char *szDecElementName)
函数功能:XML元素拷贝
拷贝两个XML元素内容
输入参数:
         HXMLTREE hXML                  XML句柄
         const char *szSrcElementName   源元素路径
         const char *szDecElementName   目标元素路径
返    回:
         0  拷贝成功
         FAIL 拷贝失败
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
                    const char *szDecElementName)
{
char sTmp[4096];
 	if(xml_GetElement(hXML,szSrcElementName,sTmp,sizeof(sTmp))== FAIL)
 		return FAIL;
 	if(xml_SetElement(hXML,szDecElementName,sTmp) == FAIL)
 		return FAIL;
  return strlen(sTmp);

}


/*
     递归调用,树的遍历
     复制方式:  1-覆盖方式, 0-不覆盖方式(跳过), 2-添加
     增加属性的支持  Tianhc 2005-06-02 14:28
     Tianhc 2005-11-20 22:16
     对于儿子有重名的BUG处理
     处理大的XML时(>2M),效率较低,用底层的函数能加快速度?---定位更快,且内容长度>5000时不会出错
*/
int xml_XCopy(HXMLTREE lXmlhandle , char *source, char *dest, int mode)
{
  int num,i,num1,num2,ret=0,j,k;
  char sname[255],sname1[255],sname2[255];
  char sname_add[255],sname_broth[255];
  int ibroth;
  int iret=0;

  num=xml_ChildElementCount( lXmlhandle, source);
  if (num<0) return -1;

  /*判断是否为叶子节点*/
  if (num == 0)
  {
     /*目标节点不存在或者目标节点存在且为添加方式*/
     if (xml_GetNode( lXmlhandle, dest) == NULL || mode == 2)
     {
        /*增加空节点*/
        if (xml_AddElement(lXmlhandle, dest, "") < 0)
            return -7;
        /*属性+值一起复制*/
        snprintf(sname_add, sizeof(sname_add), "%s|0", dest);
        if (xml_node_copy(lXmlhandle,source,sname_add) < 0)
            return -8;
     }
     else if (mode == 1) /*目标存在且为覆盖方式*/
     {
         if (xml_node_copy(lXmlhandle, source, dest) < 0)
             return -11;
     }
     return 1;
  }

  for (i=1;i<=num;i++)
  {
    if ( xml_GetChildElementName( lXmlhandle, source , i, sname, sizeof(sname) )==-1 )
      return -2;
    /*儿子有可能重名!Tianhc 2005-11-20 22:17*/
    ibroth=1;
    for (k=1;k<i;k++)
    {
         if (xml_GetChildElementName( lXmlhandle, source , k, sname_broth, sizeof(sname) )==-1)
              return -3;
         if (strcmp(sname,sname_broth)==0)
              ibroth++;
    }
    if (ibroth > 1)
    {
      snprintf(sname1,sizeof(sname1),"%s/%s|%d",source,sname,ibroth);
      if (strcmp(dest,"/")==0) /*tianhc 2006-6-1 20:48*/
         snprintf(sname2,sizeof(sname2),"/%s|%d",sname,ibroth);
      else
         snprintf(sname2,sizeof(sname2),"%s/%s|%d",dest,sname,ibroth);
    }
    else
    {
      snprintf(sname1,sizeof(sname1),"%s/%s",source,sname);
      if (strcmp(dest,"/")==0)  /*tianhc 2006-6-1 20:48*/
          snprintf(sname2,sizeof(sname2),"/%s",sname);
        else
          snprintf(sname2,sizeof(sname2),"%s/%s",dest,sname);
    }
    /*判断是否为叶子节点*/
    if (xml_IsLeafNode(lXmlhandle,sname1)==1)
    {
         iret++;
         /*节点是否存在*/
         if (xml_GetNode( lXmlhandle, sname2)!=NULL)  /*目标已存在*/
         {
            if (mode==0) /*目标存在,但不覆盖*/
               continue;
            if (mode==2) /*目标存在,添加方式*/
            {
               /*增加节点,不赋值*/
               snprintf(sname_add,sizeof(sname_add),"%s/%s", dest,sname);
               if (xml_AddElement( lXmlhandle, sname_add, "") == -1 )
                 return -7;
               /*属性+值一起复制-->针对叶子节点 Tianhc 2005-06-02 14:38*/
               snprintf(sname_add,sizeof(sname_add),"%s/%s|0", dest,sname);
               ret = xml_node_copy(lXmlhandle,sname1,sname_add);
               if ( ret < 0 )
                  return -8;
            } else
            {
                  /*覆盖*/
               ret = xml_node_copy(lXmlhandle,sname1,sname2);
               if ( ret < 0 )
                  return ret;
            }
         } else
         {
                  /*目标不存在*/
               ret = xml_node_copy(lXmlhandle,sname1,sname2);
               if ( ret < 0 )
                  return ret;
         }
    } else
    {
         ret=xml_XCopy( lXmlhandle, sname1, sname2, mode);
         if (ret<0)
            return ret;
         iret+=ret;
    }
  }
  return iret;
}

/*
   XML叶子节点复制。
   只针对叶子节点,复制内容：属性+值
*/
int xml_node_copy(HXMLTREE lXmlhandle,char *sname_src,char *sname_dest)
{
	 char sbuf[5000],sattr[255];
	 int inum,i,iret;
	 PXMLNODE lpNode;

	 inum=xml_AttributeCount( lXmlhandle, sname_src);
	 if (inum < 0)
	   return -21;
   for (i=1; i<=inum ; i++)
	 {
	 	  memset(sattr,0,sizeof(sattr));
	 	  iret=xml_GetAttributeName(lXmlhandle, sname_src, i, sattr, sizeof(sattr));
	 	  if (iret<0)
	 	    return -22;
	    iret=xml_GetElementAttr( lXmlhandle,sname_src,sattr,sbuf, sizeof(sbuf));
	    if (iret < 0)
	      return -23;
	    /*节点不存在,则增加 Tianhc 2005-06-03 12:06*/
      if (xml_ElementExist ( lXmlhandle, sname_dest) == 0)
      {
        if (xml_SetElement( lXmlhandle, sname_dest, "") == -1)
          return -24;
	    }
	    iret=xml_SetElementAttr( lXmlhandle, sname_dest, sattr, sbuf);
	    if (iret < 0)
	      return -25;
	 }
	 /*属性加完后,加节点的值,判断是否为叶节点*/
	 if (xml_IsLeafNode( lXmlhandle, sname_src ) == 1 )
   {
	   /*if (xml_GetElement( lXmlhandle, sname_src, sbuf, sizeof(sbuf)) == -1)*/
	   lpNode=xml_GetNode(lXmlhandle,sname_src);
	   if ( lpNode== NULL )
	        return -25;
	   /*if (sbuf[0]!=0) */
	   if  ( lpNode->m_iDataLen != 0 )
	   {
	       if (xml_SetElement( lXmlhandle, sname_dest, lpNode->m_pszData) == -1)
	           return -26;
		 } else
		 {
	       if (xml_SetElement( lXmlhandle, sname_dest, "") == -1)
	           return -27;
		 }
	 }
	 return 0;
}


/****************************************************************
函数名：ConvXmlTwo
函数功能：将XML2.0格式的内容转换为XML1.0格式（<xxxx/>格式转换为<xxxx></xxxx>格式）
输入参数:
	 char *str :XML字符串
	 int ilen  :长度,一般为sizeof(str),返回的内容比输入的内容长.
注：
   已修改底层函数,目前底层xn_ImportXMLString函数直接支持<xxx/>的XML 2.0简写方式,
   本函数在XML较大时(>1M)效率较低，弃用!
   Tianhc 2006-11-15 22:03
*****************************************************************/
int xml_Conv2to1(char *str,int ilen)
{
    char *buffer;
    int i=0,j=0,k=0,itmp=0,jtmp=0;
    int flag=0;
    if (  str == NULL )
      return -1;
    return 0;

    if((buffer=malloc(ilen*2))==NULL)
       return -2;
    for (i = 0; i < strlen(str); i++)
    {
        if (str[i] =='>')
           flag=1;
        else if (str[i] =='<')
        {
           j=k;
           flag=0;
        }
/*
        if (flag==1 && str[i] ==' ' && str[i] != '\0')
            continue;
*/
        if (str[i]=='/'&&str[i+1]=='>') /*将<xxxx/>表示空节点的情况转换为<xxxx></xxxx>格式*/
        {
            jtmp = k;
            buffer[k++] = '>';
            buffer[k++] = '<';
            buffer[k++] = '/';
            for(itmp = j+1; itmp<= jtmp;itmp++)
            {
            	/*
            	  Tianhc 2005-06-03 14:52
            	  针对<ATTRIBUTE NAME="1" />的情况,认为有空格则是带属性的结点
            	*/
            	if (buffer[itmp] == ' ')
            	{
            	   buffer[k++]='>';
            		 break;
            	}
            	buffer[k++] = buffer[itmp];
            }
	          i++;
            continue;
        }
        buffer[k++] = str[i];
    }
    buffer[k] = '\0';
    if (k>ilen)   /*缓冲区不够长*/
    {
        free(buffer);
    	  return -3;
    }
    strcpy(str , buffer);
    return 0;
}

/************************************************************************
函数声明:HXMLTREE xml_CreateXMLFromFile_OPTROOT(char *sFileName, int ioptroot)
函数功能:由文件内容生成XML结构
输入参数:sFileName      XML格式文件名称
         ioptroot       是否替换根节点
            >0 替换 用XML字串中第一层上从左到右顺序指定位置节点替换
            <0 替换 用XML字串中第一层上从右到左顺序指定位置节点替换
            0  不替换,所有数据引入到该节点下

输出参数:
          >0--成功 XML 句柄
          <0--失败
          -2--文件为空
项 目 组:中间业务产品研发组
程 序 员:SRC
************************************************************************/
HXMLTREE xml_CreateXMLFromFile_OPTROOT(char *sFileName, int ioptroot)
{
  /*函数调用返回*/
  int iResult;

  /*XML结构句柄*/
  HXMLTREE iHandleXML;

  /*文件指针*/
  FILE *pfFile;

  caddr_t sFileHead;
  struct stat struStat;

  /******************************函数开始*******************************/
  /*打开XML格式的文件*/
  pfFile = fopen(sFileName, "r");
  if (pfFile == NULL)
    return FAIL;

  /*将文件映射到内存*/
  fstat(fileno(pfFile), &struStat);
  if ( struStat.st_size == 0 )
    return FAIL;

  if ((sFileHead = mmap(NULL, (size_t)struStat.st_size, PROT_READ, MAP_SHARED,
      fileno(pfFile), (off_t)0)) == (caddr_t) - 1)
  {
    munmap(sFileHead, (size_t)struStat.st_size);
    fclose(pfFile);
    return FAIL;
  }

  /*初始化XML结构*/
  iHandleXML = xml_Create("root");
  if (iHandleXML == -1)
  {
    munmap(sFileHead, (size_t)struStat.st_size);
    fclose(pfFile);
    return FAIL;
  }

  /*将文件内容导入XML结构中*/
  iResult = xml_ImportXMLString(iHandleXML, sFileHead, NULL, ioptroot);
  if (iResult == FAIL)
  {
    munmap(sFileHead, (size_t)struStat.st_size);
    fclose(pfFile);
    return FAIL;
  }

  munmap(sFileHead, (size_t)struStat.st_size);

  /*关闭文件*/
  fclose(pfFile);

  /*函数返回*/
  return iHandleXML;
}
