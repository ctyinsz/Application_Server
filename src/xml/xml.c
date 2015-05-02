/************************************************************************
版权所有：杭州恒生电子股份有限公司
项目名称：公共项目
版    本：V1.0
操作系统：AIX4.2,SCO5
文件名称：xmle.c
文件描述：简单XML操作函数库
项 目 组：中间业务产品研发组
程 序 员：中间业务产品研发组
发布日期：2001年09月11日
修改日期：2002年05月12日
************************************************************************/
/**********************************************************************
  转义字符&;
  &lt;    &gt;        &apos;      &quote;     &amp;
  <       >           '           "           &
 **********************************************************************/

/*
   修改记录：
   Tianhc 2005-11-21 21:17
   增加：
    xml_ExportXMLStringHEAD()  指定XML头部信息
    xml_node_strlen            计算导出数据的长度，用于Export前分配内存
    xml_NodeMove               结点内容移动
*/


int g_xmlErrno=0; /* 错误代码 */
char g_szErrEx[128];/* 错误扩展信息 */
int g_xmlquot=1;

#include  "xmlnode.c"

#ifdef __cplusplus
extern "C" {
#endif /* end of __cplusplus */

PXMLNODE xml_GetNode( HXMLTREE hXML, const char *szElementName);


/**********************************************************************
 *  函数：  xml_Create
 *  功能: 初始化XML结构
 *  参数:
  szRootName
          指定XML结构根元素的名称。
 *  返回值:
  成功  返回XML结构句柄
  失败  返回FAIL
 **********************************************************************/
HXMLTREE xml_Create( const char *szRootName)
{
PXMLSTRUCT pXML;

  if ( !szRootName || !*szRootName )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  if ( !(pXML=xmlo_Build((char*)szRootName)) )
  {
    return  FAIL;
  }

  g_xmlErrno = XML_NOERROR;

  return  (HXMLTREE)pXML;
}

/**********************************************************************
 *  函数：  xml_Destroy
 *  功能: 删除XML结构
 *  参数:
  hXML
          待删除的XML设备句柄。
 *  返回值:
  成功  返回字节数
  失败  返回FAIL
 **********************************************************************/
int xml_Destroy( HXMLTREE hXML )
{
  if ( !hXML )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  xmlo_Destroy( (PXMLSTRUCT)hXML );
  g_xmlErrno = XML_NOERROR;
  return  SUCC;
}

/**********************************************************************
 *  函数：  xml_Clear
 *  功能: 清除XML结构中所有数据，只保留根节点
 *  参数:
  hXML
          待清除的XML设备句柄。
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_Clear( HXMLTREE hXML )
{
PXMLNODE *lpElemP,*lpEndP;
PXMLNODEATTR *lpElemAttrP,*lpEndAttrP;
PXMLSTRUCT  pXML;

  if ( !hXML )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }
  pXML = (PXMLSTRUCT)hXML;

#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
    trc_Write(pXML->m_hTrace,">>> %t <<<  xml_Clear().\n",0);
#endif

  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }


  if ( sl_Length(&pXML->m_pRoot->m_DownList) )
  {
    lpEndP=(PXMLNODE*)pXML->m_pRoot->m_DownList.m_lpBase+pXML->m_pRoot->m_DownList.m_nLen;
    lpElemP = (PXMLNODE*)pXML->m_pRoot->m_DownList.m_lpBase;
    for ( ; lpElemP<lpEndP; lpElemP++ )
    {
      xn_Destroy(*lpElemP);
    }
    pXML->m_pRoot->m_DownList.m_nLen=0;
  }

  if ( sl_Length(&pXML->m_pRoot->m_AttrList) )
  {
    lpEndAttrP=(PXMLNODEATTR*)pXML->m_pRoot->m_AttrList.m_lpBase+pXML->m_pRoot->m_DownList.m_nLen;
    lpElemAttrP = (PXMLNODEATTR*)pXML->m_pRoot->m_AttrList.m_lpBase;
    for ( ; lpElemAttrP<lpEndAttrP; lpElemAttrP++ )
    {
      xna_Destroy(*lpElemAttrP);
    }
    pXML->m_pRoot->m_AttrList.m_nLen=0;
  }

  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  SUCC;
}

/**********************************************************************
 *  函数：  xml_AddElement
 *  功能: 在XML树中添加一个叶子节点
    叶子节点(数据节点)可以重复存在
    如果该节点不存在, 添加该节点;
    如果该节点已存在, 添加同名节点;
    填加时如果路径节点不存在, 则连同这些节点一起添加
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要添加的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  在该层上的节点元素名，
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示添加在最后
      如果为1，2，3……，表示取相应编号的元素。
      缺省为0
  szElementValue
    待添加的元素值指针
    节点分为数据节点和属性节点
    如果该参数有字串值，则该节点为数据节点，
    如果该参数值为""或NULL，则该节点可以为属性节点，以后该节点可以添加属性。
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_AddElement( HXMLTREE hXML,
       const char *szElementName,
       const char *szElementValue )
{
PXMLSTRUCT pXML;
int   len;

  if ( !hXML || !szElementName || !*szElementName )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
  {
    if ( szElementValue )
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_AddElement(...,'%s','%s').\n",0,szElementName,szElementValue);
    else
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_AddElement(...,'%s',NULL).\n",0,szElementName);
  }
#endif

  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( szElementValue )
    len = strlen(szElementValue);
  else
    len = 0;

  if ( !xn_AddNode( pXML->m_pRoot, (char*)szElementName, (char*)szElementValue, len, pXML ) )
  {
    return  FAIL;
  }

  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  SUCC;
}

/**********************************************************************
 *  函数：  xml_SetElement
 *  功能: 在XML树中设置一个叶子节点
    叶子节点(数据节点)可以重复存在
    如果该节点不存在, 添加该节点;
      如果路径节点不存在, 则连同这些节点一起添加
    如果该节点已存在, 修改该节点数据;
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要添加的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  在该层上的节点元素名，
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示添加在最后
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
  szElementValue
    待设置的元素值指针
    节点分为数据节点和属性节点
    如果该参数有字串值，则该节点为数据节点，
    如果该参数值为""或NULL，则该节点可以为属性节点，以后该节点可以添加属性。
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_SetElement( HXMLTREE hXML,
       const char *szElementName,
       const char *szElementValue )
{
PXMLSTRUCT pXML;
PXMLNODE  lpNode;
int   len;

  if ( !hXML || !szElementName || !*szElementName )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
  {
    if ( szElementValue )
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_SetElement(...,'%s','%s').\n",0,szElementName,szElementValue);
    else
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_SetElement(...,'%s',NULL).\n",0,szElementName);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( szElementValue )
    len = strlen(szElementValue);
  else
    len = 0;

  if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML )) )
  {
    if ( g_xmlErrno!=XML_ENONODE )
      return  FAIL;

    /* 不存在该节点添加 */
    if ( !xn_AddNode( pXML->m_pRoot, (char*)szElementName, (char*)szElementValue, len, pXML ) )
    {
      return  FAIL;
    }
  }
  else
  { /* 存在该节点 */
    if ( sl_Length(&(lpNode->m_DownList)) )
    { /* 该节点不为数据节点 */
      g_xmlErrno = XML_ENOTDATANODE;
      snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szElementName);
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(XML_ENOTDATANODE),szElementName);
#endif
      return  FAIL;
    }

    if ( xn_SetData(lpNode,(unsigned char*)szElementValue,len)==FAIL )
    {
#ifdef __XML_TRACE__
      if ( pXML->m_dwTrcType&XL_ERROR )
        trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(xml_GetLastError()),szElementName);
#endif
      return  FAIL;
    }
  }

  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  SUCC;
}

/**********************************************************************
 *  函数：  xml_GetElement
 *  功能: 获取XML树中指定节点数据
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
  pucElementBuff
    接收获取元素值的缓冲区指针
  iElementBuffSize
    缓冲区大小
 *  返回值:
  成功  返回实际字节数
  失败  返回FAIL
 **********************************************************************/
int xml_GetElement( HXMLTREE hXML,
       const char *szElementName,
       char *pucElementBuff,
       const size_t uiElementBuffSize )
{
PXMLSTRUCT pXML;
PXMLNODE  lpNode;

  if ( !hXML || !szElementName || !*szElementName || !pucElementBuff || !uiElementBuffSize )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }
  memset( pucElementBuff,0,uiElementBuffSize);
  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
    trc_Write(pXML->m_hTrace,">>> %t <<<  xml_GetElement(...,'%s',0x%p,%u).\n",0,szElementName,pucElementBuff,uiElementBuffSize);
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML )) )
  {
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
      trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(xml_GetLastError()),szElementName);
#endif
    return  FAIL;
  }

  if ( lpNode->m_pszData && *lpNode->m_pszData )
  {
    if( uiElementBuffSize<= strlen((char*)lpNode->m_pszData) )
    {
      g_xmlErrno = XML_DFENOSPC;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_DFENOSPC));
#endif
      return  FAIL;
    }
    else
      memcpy(pucElementBuff,lpNode->m_pszData,lpNode->m_iDataLen);
  }
  else
    *pucElementBuff='\0';

  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  lpNode->m_iDataLen;
}



/**********************************************************************
 *  函数：  xml_ModifyElement
 *  功能: 修改XML树中指定节点数据
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要修改的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
  szElementValue
    元素新的值指针
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_ModifyElement( HXMLTREE hXML,
       const char *szElementName,
       const char *szElementValue )
{
PXMLSTRUCT pXML;
PXMLNODE  lpNode;
int   len;

  if ( !hXML || !szElementName || !*szElementName )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
  {
    if ( szElementValue )
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ModifyElement(...,'%s','%s').\n",0,szElementName,szElementValue);
    else
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ModifyElement(...,'%s',NULL).\n",0,szElementName);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML )) )
  {
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
      trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(xml_GetLastError()),szElementName);
#endif
    return  FAIL;
  }

  if ( sl_Length(&(lpNode->m_DownList)) )
  { /* 该节点不为数据节点 */
    g_xmlErrno = XML_ENOTDATANODE;
    snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szElementName);
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(XML_ENOTDATANODE),szElementName);
#endif
    return  FAIL;
  }

  if ( szElementValue )
    len = strlen(szElementValue);
  else
    len = 0;

  if ( xn_SetData(lpNode,(unsigned char*)szElementValue,len)==FAIL )
  {
    return  FAIL;
  }
  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  SUCC;
}



/**********************************************************************
 *  函数：  xml_DelElement
 *  功能: 删除XML树中指定节点数据
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要删除的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_DelElement( HXMLTREE hXML, const char *szElementName )
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;
int   nPos;

  if ( !hXML || !szElementName || !*szElementName )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
  {
    trc_Write(pXML->m_hTrace,">>> %t <<<  xml_DelElement(...,'%s').\n",0,szElementName);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,&nPos,pXML )) )
  {
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
      trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(xml_GetLastError()),szElementName);
#endif
    return  FAIL;
  }

  if ( sl_Delete(&lpNode->m_DownList,nPos,(void*)&lpNode)==-1 )
  {
    g_xmlErrno = XML_EINVDATAPOS;
    return  FAIL;
  }

  xn_Destroy(lpNode);

  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  SUCC;
}


/**********************************************************************
 *  函数：  xml_ElementExist
 *  功能: 判断XML树中指定节点是否存在
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要删除的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
 *  返回值:
  成功  存在,返回1; 不存在,返回0
  失败  返回FAIL
 **********************************************************************/
int xml_ElementExist( HXMLTREE hXML, const char *szElementName )
{
PXMLSTRUCT  pXML;
int   ret;

  if ( !hXML || !szElementName || !*szElementName )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
  {
    trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ElementExist(...,'%s').\n",0,szElementName);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML ) )
  {
    g_xmlErrno = XML_NOERROR;
    ret = 1;
  }
  else if ( g_xmlErrno==XML_ENONODE )
  {
    g_xmlErrno = XML_NOERROR;
    ret = 0;
  }
  else
    ret = FAIL;

#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  ret;
}

/**********************************************************************
 *  函数：  xml_IsLeafNode
 *  功能: 判断XML树中指定节点是否为叶子节点(数据节点)
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要判断的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
 *  返回值:
  成功  是叶子节点,返回1; 不是,返回0
  失败  返回FAIL
 **********************************************************************/
int xml_IsLeafNode( HXMLTREE hXML, const char *szElementName )
{
PXMLSTRUCT pXML;
PXMLNODE  lpNode;
int     ret;

  if ( !hXML || !szElementName || !*szElementName )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_COUNT )
  {
    trc_Write(pXML->m_hTrace,">>> %t <<<  xml_IsLeafNode(...,'%s').\n",0,szElementName);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML )) )
  {
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
      trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(xml_GetLastError()),szElementName);
#endif
    return  FAIL;
  }

  g_xmlErrno = XML_NOERROR;

  ret = !sl_Length(&(lpNode->m_DownList));
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_COUNT )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  ret;
}



/**********************************************************************
 *  函数：  xml_ChildElementCount
 *  功能: 获取XML树中指定节点的子节点个数
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构中的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
 *  返回值:
  成功  返回子节点个数
  失败  返回FAIL
 **********************************************************************/
int xml_ChildElementCount( HXMLTREE hXML,
       const char *szElementName )
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;


  if ( !hXML || !szElementName || !*szElementName )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_COUNT )
  {
    trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ChildElementCount(...,'%s').\n",0,szElementName);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML )) )
  {
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
      trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(xml_GetLastError()),szElementName);
#endif
    return  FAIL;
  }

  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_COUNT )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  sl_Length(&lpNode->m_DownList);
}



/**********************************************************************
 *  函数：  xml_GetChildElementName
 *  功能: 获取XML树节点中指定位置子节点名
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
  iPos
    指定位置
  pucNameBuff
    接收获取元素名的缓冲区指针
  uiNameBuffSize
    缓冲区大小
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_GetChildElementName( HXMLTREE hXML,
       const char *szElementName,
       const int iPos,
       char *pucNameBuff,
       const size_t uiNameBuffSize )
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;

  if ( !hXML || !szElementName || !*szElementName || !iPos || !pucNameBuff || !uiNameBuffSize )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
  {
    trc_Write(pXML->m_hTrace,">>> %t <<<  xml_GetChildElementName(...,'%s',%d,0x%p,%u).\n",
      0,szElementName,iPos,pucNameBuff,uiNameBuffSize);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML )) )
  {
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
      trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(xml_GetLastError()),szElementName);
#endif
    return  FAIL;
  }

  if ( sl_GetElem(&lpNode->m_DownList,iPos,(void*)&lpNode)==(-1) )
  {
    g_xmlErrno = XML_EINVDATAPOS;
    return  FAIL;
  }

  if ( lpNode->m_pszName && *lpNode->m_pszName )
  {
    if( uiNameBuffSize<= strlen((char*)lpNode->m_pszName) )
    {
      g_xmlErrno = XML_DFENOSPC;
      return  FAIL;
    }
    else
      strcpy(pucNameBuff,(char*)lpNode->m_pszName);
  }
  else
    *pucNameBuff='\0';

  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  SUCC;
}



/**********************************************************************
 *  函数：  xml_AddElementAttr
 *  功能: 在XML树中添加一个节点属性
    属性节点不可以重复存在
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要添加属性的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  表示取最后一个元素，
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示添加在最后
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
  szAttributeName
    待添加的属性名指针
  szAttributeValue
    待添加的属性值指针
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_AddElementAttr( HXMLTREE hXML,
       const char *szElementName,
       const char *szAttributeName,
       const char *szAttributeValue )
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;
PXMLNODEATTR pAttr;

  if ( !hXML || !szAttributeName ||!*szAttributeName || !szAttributeValue )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODEATTR )
  {
    if ( szAttributeValue )
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_AddElementAttr(...,'%s','%s','%s').\n",
        0,szElementName,szAttributeName,szAttributeValue);
    else
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_AddElementAttr(...,'%s','%s',NULL).\n",
        0,szElementName,szAttributeName);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( !szElementName || !*szElementName )
  {
    lpNode=pXML->m_pRoot;
  }
  else
  {
    if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML )) )
    {
#ifdef __XML_TRACE__
      if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
        trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(xml_GetLastError()),szElementName);
#endif
      return  FAIL;
    }
  }


  if ( lpNode->m_pszData && *(lpNode->m_pszData) && sl_Length(&lpNode->m_DownList) )
  {
    g_xmlErrno = XML_EDATANODE;
    snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szElementName);
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(g_xmlErrno),szElementName);
#endif
    return  FAIL;
  }


  if ( sl_FindAttr(&(lpNode->m_AttrList),(unsigned char*)szAttributeName,NULL) )
  {
    g_xmlErrno = XML_EATTREXIST;
    snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szAttributeName);
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s - [%s: %s]\n",xml_StringError(g_xmlErrno),szElementName,szAttributeName);
#endif
    return  FAIL;
  }

  if( !(pAttr=xna_Build( (unsigned char*)szAttributeName, strlen(szAttributeName),(unsigned char*)szAttributeValue )) )
  {
    g_xmlErrno = XML_ENOMEM;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(g_xmlErrno));
#endif
    return  FAIL;
  }

  if ( sl_Append(&(lpNode->m_AttrList),pAttr)==-1 )
  {
    xna_Destroy(pAttr);
    g_xmlErrno = XML_ENOMEM;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(g_xmlErrno));
#endif
    return  FAIL;
  }

  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODEATTR )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  SUCC;
}



/**********************************************************************
 *  函数：  xml_SetElementAttr
 *  功能: 在XML树中设置一个节点属性
    属性节点不可以重复存在
    如果节点属性不存在，则填加一个节点属性；
    如果节点属性存在，则修改其值为新值
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要设置属性的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  表示取最后一个元素，
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示添加在最后
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
  szAttributeName
    待设置的属性名指针
  szAttributeValue
    待设置的属性值指针
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_SetElementAttr( HXMLTREE hXML,
       const char *szElementName,
       const char *szAttributeName,
       const char *szAttributeValue )
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;
PXMLNODEATTR lpAttr;

  if ( !hXML || !szAttributeName ||!*szAttributeName || !szAttributeValue )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODEATTR )
  {
    if ( szAttributeValue )
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_SetElementAttr(...,'%s','%s','%s').\n",
        0,szElementName,szAttributeName,szAttributeValue);
    else
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_SetElementAttr(...,'%s','%s',NULL).\n",
        0,szElementName,szAttributeName);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( !szElementName || !*szElementName )
  {
    lpNode=pXML->m_pRoot;
  }
  else
  {
    if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML )) )
    {
#ifdef __XML_TRACE__
      if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
        trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(xml_GetLastError()),szElementName);
#endif
      return  FAIL;
    }
  }


  if ( lpNode->m_pszData && *(lpNode->m_pszData) && sl_Length(&lpNode->m_DownList) )
  {
    g_xmlErrno = XML_EDATANODE;
    snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szElementName);
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(XML_EDATANODE),szElementName);
#endif
    return  FAIL;
  }


  if ( lpAttr=sl_FindAttr(&(lpNode->m_AttrList),(unsigned char*)szAttributeName,NULL) )
  { /* 存在, 修改其值为新属性值 */
    if ( xna_SetData(lpAttr,(unsigned char*)szAttributeValue,strlen(szAttributeValue))==FAIL )
    {
      g_xmlErrno = XML_ENOMEM;
      return  FAIL;
    }
  }
  else
  { /* 不存在, 填加属性 */
    if( !(lpAttr=xna_Build( (unsigned char*)szAttributeName, strlen(szAttributeName),(unsigned char*)szAttributeValue )) )
    {
      g_xmlErrno = XML_ENOMEM;
      return  FAIL;
    }

    if ( sl_Append(&(lpNode->m_AttrList),lpAttr)==-1 )
    {
      xna_Destroy(lpAttr);
      g_xmlErrno = XML_ENOMEM;
      return  FAIL;
    }
  }

  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODEATTR )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  SUCC;
}



/**********************************************************************
 *  函数：  xml_GetElementAttr
 *  功能: 获取XML树中指定节点数据
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要获取属性的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
  szAttributeName
    属性名指针
  pucAttributeBuff
    接收获取元素属性值的缓冲区指针
  iElementBuffSize
    缓冲区大小
 *  返回值:
  成功  返回实际字节数
  失败  返回FAIL
 **********************************************************************/
int xml_GetElementAttr( HXMLTREE hXML,
       const char *szElementName,
       const char *szAttributeName,
       char *pucAttributeBuff,
       const size_t uiAttrBuffSize )
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;
PXMLNODEATTR lpAttr;

  if ( !hXML || !szAttributeName || !*szAttributeName || !pucAttributeBuff || !uiAttrBuffSize )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }
  memset( pucAttributeBuff,0,uiAttrBuffSize);

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODEATTR )
  {
    trc_Write(pXML->m_hTrace,">>> %t <<<  xml_GetElementAttr(...,'%s','%s',0x%p,%u).\n",
      0,szElementName,szAttributeName,pucAttributeBuff,uiAttrBuffSize);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }


  if ( !szElementName || !*szElementName )
  {
    lpNode=pXML->m_pRoot;
  }
  else
  {
    if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName, NULL,pXML )) )
    {
#ifdef __XML_TRACE__
      if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
        trc_Write(pXML->m_hTrace,"error: %s- [%s]\n",xml_StringError(xml_GetLastError()),szElementName);
#endif
      return  FAIL;
    }
  }

  if ( !(lpAttr=sl_FindAttr(&(lpNode->m_AttrList),(unsigned char*)szAttributeName,NULL)) )
  {
    g_xmlErrno = XML_ENOATTR;
    snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szAttributeName);
    return  FAIL;
  }

  if ( lpAttr->m_pszData && *lpAttr->m_pszData )
  {
    if( uiAttrBuffSize<= strlen((char*)lpAttr->m_pszData) )
    {
      g_xmlErrno = XML_DFENOSPC;
      return  FAIL;
    }
    else
      strcpy(pucAttributeBuff,(char*)lpAttr->m_pszData);
  }
  else
    *pucAttributeBuff = '\0';

  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODEATTR )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  strlen(pucAttributeBuff);
}



/**********************************************************************
 *  函数：  xml_DelElementAttr
 *  功能: 删除XML树中指定节点属性
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要删除属性的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
  szAttributeName
    属性名指针
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_DelElementAttr( HXMLTREE hXML,
       const char *szElementName,
       const char *szAttributeName)
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;
PXMLNODEATTR lpAttr;
int   nPos;

  if ( !hXML || !szAttributeName || !*szAttributeName )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODEATTR )
  {
    trc_Write(pXML->m_hTrace,">>> %t <<<  xml_DelElementAttr(...,'%s','%s').\n",
      0,szElementName,szAttributeName);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( !szElementName || !*szElementName )
  {
    lpNode=pXML->m_pRoot;
  }
  else
  {
    if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML )) )
    {
#ifdef __XML_TRACE__
      if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
        trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(xml_GetLastError()),szElementName);
#endif
      return  FAIL;
    }
  }

  if ( !(lpAttr=sl_FindAttr(&(lpNode->m_AttrList),(unsigned char*)szAttributeName,&nPos)) )
  {
    g_xmlErrno = XML_ENOATTR;
    snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szAttributeName);
    return  FAIL;
  }

  if ( sl_Delete(&lpNode->m_AttrList,nPos,(void*)&lpAttr)==-1 )
  {
    g_xmlErrno = XML_EINVDATAPOS;
    return  FAIL;
  }

  xna_Destroy(lpAttr);

  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODEATTR )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  SUCC;
}


/**********************************************************************
 *  函数：  xml_ElementAttrExist
 *  功能: 判断XML树中指定节点属性是否存在
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要判断属性的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
  szAttributeName
    属性名指针
 *  返回值:
  成功  存在,返回1; 不存在,返回0
  失败  返回FAIL
 **********************************************************************/
int xml_ElementAttrExist( HXMLTREE hXML,
       const char *szElementName,
       const char *szAttributeName)
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;
int     ret;

  if ( !hXML || !szAttributeName || !*szAttributeName )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODEATTR )
  {
    trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ElementAttrExist(...,'%s','%s').\n",
      0,szElementName,szAttributeName);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( !szElementName || !*szElementName )
  {
    lpNode=pXML->m_pRoot;
  }
  else
  {
    if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML )) )
    {
#ifdef __XML_TRACE__
      if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
        trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(xml_GetLastError()),szElementName);
#endif
      return  FAIL;
    }
  }

  if ( sl_FindAttr(&(lpNode->m_AttrList),(unsigned char*)szAttributeName,NULL) )
    ret = 1;
  else
    ret = 0;

  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODEATTR )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  ret;
}



/**********************************************************************
 *  函数：  xml_ModifyElementAttr
 *  功能: 修改XML树中指定节点属性数据
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要获取属性的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
  szAttributeName
    属性名指针
  szAttributeValue
    待添加的属性值指针
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_ModifyElementAttr( HXMLTREE hXML,
       const char *szElementName,
       const char *szAttributeName,
       const char *szAttributeValue )
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;
PXMLNODEATTR lpAttr;

  if ( !hXML || !szAttributeName || !*szAttributeName || !*szAttributeName )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODEATTR )
  {
    if ( szAttributeValue )
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ModifyElementAttr(...,'%s','%s','%s').\n",
        0,szElementName,szAttributeName,szAttributeValue);
    else
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ModifyElementAttr(...,'%s','%s',NULL).\n",
        0,szElementName,szAttributeName);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( !szElementName || !*szElementName )
  {
    lpNode=pXML->m_pRoot;
  }
  else
  {
    if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName, NULL,pXML )) )
    {
#ifdef __XML_TRACE__
      if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
        trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(xml_GetLastError()),szElementName);
#endif
      return  FAIL;
    }
  }

  if ( !(lpAttr=sl_FindAttr(&(lpNode->m_AttrList),(unsigned char*)szAttributeName,NULL)) )
  {
    g_xmlErrno = XML_ENOATTR;
    snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szAttributeName);
    return  FAIL;
  }

  if ( xna_SetData(lpAttr,(unsigned char*)szAttributeValue,strlen(szAttributeValue))==FAIL )
  {
    g_xmlErrno = XML_ENOMEM;
    return  FAIL;
  }
  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODEATTR )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  SUCC;
}



/**********************************************************************
 *  函数：  xml_AttributeCount
 *  功能: 获取XML树中指定节点的属性个数
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构中的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
 *  返回值:
  成功  返回该节点子元素个数
  失败  返回FAIL
 **********************************************************************/
int xml_AttributeCount( HXMLTREE hXML,
       const char *szElementName )
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;

  if ( !hXML )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_COUNT )
  {
    trc_Write(pXML->m_hTrace,">>> %t <<<  xml_AttributeCount(...,'%s').\n",
      0,szElementName);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( !szElementName || !*szElementName )
  {
    lpNode=pXML->m_pRoot;
  }
  else
  {
    if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML )) )
    {
#ifdef __XML_TRACE__
      if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
        trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(xml_GetLastError()),szElementName);
#endif
      return  FAIL;
    }
  }

  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_COUNT )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  sl_Length(&lpNode->m_AttrList);
}




/**********************************************************************
 *  函数：  xml_GetAttributeName
 *  功能: 获取XML树节点中指定位置属性名
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
  iPos
    指定位置
  pucNameBuff
    接收获取元素属性名的缓冲区指针
  uiNameBuffSize
    缓冲区大小
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_GetAttributeName( HXMLTREE hXML,
       const char *szElementName,
       const int iPos,
       char *pucAttrNameBuff,
       const size_t uiAttrNameBuffSize )
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;
PXMLNODEATTR  lpAttr;

  if ( !hXML || !iPos || !pucAttrNameBuff || !uiAttrNameBuffSize )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODEATTR )
  {
    trc_Write(pXML->m_hTrace,">>> %t <<<  xml_GetAttributeName(...,'%s',%d,0x%p,%u).\n",
      0,szElementName,iPos,pucAttrNameBuff,uiAttrNameBuffSize);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( !szElementName || !*szElementName )
  {
    lpNode=pXML->m_pRoot;
  }
  else
  {
    if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML )) )
    {
#ifdef __XML_TRACE__
      if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
        trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(xml_GetLastError()),szElementName);
#endif
      return  FAIL;
    }
  }

  if ( sl_GetElem(&lpNode->m_AttrList,iPos,(void*)&lpAttr)==(-1) )
  {
    g_xmlErrno = XML_EINVDATAPOS;
    return  FAIL;
  }

  if ( lpAttr->m_pszName && *lpAttr->m_pszName )
  {
    if( uiAttrNameBuffSize<= strlen((char*)lpAttr->m_pszName) )
    {
      g_xmlErrno = XML_DFENOSPC;
      return  FAIL;
    }
    else
      strcpy(pucAttrNameBuff,(char*)lpAttr->m_pszName);
  }
  else
    *pucAttrNameBuff='\0';

  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODEATTR )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  SUCC;
}




/**********************************************************************
 *  函数：  xml_ElementCount
 *  功能: 统计XML树中指定节点元素同一层上的个数
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要统计的元素的名称，形如"/PRIVATE|2/电费单价"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn/FINDNAME"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
 *  返回值:
  成功  返回个数
  失败  返回FAIL
 **********************************************************************/
int xml_ElementCount( HXMLTREE hXML,
    const char *szElementName )
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;
int   n;
char  *pName;

  if ( !hXML || !szElementName || !*szElementName )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_COUNT )
  {
    trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ElementCount(...,'%s').\n",
      0,szElementName);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,&n,pXML )) )
    return  0;

  pName = strrchr(szElementName,'/');
  if ( !pName )
    pName = (char*)szElementName;
  else if ( *pName=='/' )
    pName++;

  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_COUNT )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  sl_CountNode( &lpNode->m_DownList, n, (unsigned char*)pName, strlen(pName) )+1;
}


/**********************************************************************
 *  函数：  xml_ExportXMLStringKVL
 *  功能: 导出KVL字串数据
 *  参数:
  hXML
    指定XML设备句柄
  pucXMLBuff
    接收导出取字串的缓冲区指针
  uiXMLBuffLen
    缓冲区大小
  szExportName
    指定XML结构导出位置的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
    如果为NULL或""，则导出所有数据
  bNodeSelf
    是否包含节点自身数据
    非0   导出时包含该节点元素。
    0   导出时导出该节点所有子节点数据，不包含该节点本身。
 *  返回值:
  成功  返回导出的字节数
  失败  返回FAIL
 **********************************************************************/
int xml_ExportXMLStringKVL( HXMLTREE hXML,
      char *pucXMLBuff, size_t uiXMLBuffLen,
      const char *szExportName, int bNodeSelf )
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;
int num,n;

  if ( !hXML || !pucXMLBuff || !uiXMLBuffLen )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_TOXML )
  {
    if ( szExportName )
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ExportXMLString(...,0x%p,%u,%s).\n",0,pucXMLBuff,uiXMLBuffLen,szExportName);
    else
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ExportXMLString(...,0x%p,%u,NULL).\n",0,pucXMLBuff,uiXMLBuffLen);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( szExportName && *szExportName )
  {
    if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szExportName,NULL,pXML )) )
    {
#ifdef __XML_TRACE__
      if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
        trc_Write(pXML->m_hTrace,"error: %s - [%s] \n",xml_StringError(xml_GetLastError()),szExportName);
#endif
      return  FAIL;
    }
  }
  else
  {
    lpNode = pXML->m_pRoot;
  }

  num = 0;
  if ( uiXMLBuffLen<=0 )
  {
    g_xmlErrno = XML_DFENOSPC;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }
  /*
  memcpy(pucXMLBuff,"<?xml version='1.0' encoding=\"GB2312\"?>\n",40);
  */

  if ( (n=xn_ExportXMLStringKVL(lpNode,(char*)pucXMLBuff+num,uiXMLBuffLen-num,bNodeSelf))==FAIL)
  {
    g_xmlErrno = XML_DFENOSPC;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }
  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType&XL_TOXML && pXML->m_dwTrcType&XL_DATA )
    trc_Write(pXML->m_hTrace,"export xml string length = [%d]\n%h%m%H\n",num+n,pucXMLBuff,num+n);
#endif
  return  num+n;
}

/**********************************************************************
 *  函数：  xml_ExportXMLString
 *  功能: 导出XML字串数据
 *  参数:
  hXML
    指定XML设备句柄
  pucXMLBuff
    接收导出取字串的缓冲区指针
  uiXMLBuffLen
    缓冲区大小
  szExportName
    指定XML结构导出位置的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
    如果为NULL或""，则导出所有数据
  bNodeSelf
    是否包含节点自身数据
    非0   导出时包含该节点元素。
    0   导出时导出该节点所有子节点数据，不包含该节点本身。
 *  返回值:
  成功  返回导出的字节数
  失败  返回FAIL
 **********************************************************************/
int xml_ExportXMLString( HXMLTREE hXML,
      char *pucXMLBuff, size_t uiXMLBuffLen,
      const char *szExportName, int bNodeSelf )
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;
int num,n;

  if ( !hXML || !pucXMLBuff || !uiXMLBuffLen )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_TOXML )
  {
    if ( szExportName )
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ExportXMLString(...,0x%p,%u,%s).\n",0,pucXMLBuff,uiXMLBuffLen,szExportName);
    else
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ExportXMLString(...,0x%p,%u,NULL).\n",0,pucXMLBuff,uiXMLBuffLen);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( szExportName && *szExportName )
  {
    if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szExportName,NULL,pXML )) )
    {
#ifdef __XML_TRACE__
      if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
        trc_Write(pXML->m_hTrace,"error: %s - [%s] \n",xml_StringError(xml_GetLastError()),szExportName);
#endif
      return  FAIL;
    }
  }
  else
  {
    lpNode = pXML->m_pRoot;
  }

  num = 40;
  if ( uiXMLBuffLen<=40 )
  {
    g_xmlErrno = XML_DFENOSPC;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }
  memcpy(pucXMLBuff,"<?xml version='1.0' encoding=\"GB2312\"?>\n",40);


  if ( (n=xn_ExportXMLString(lpNode,(char*)pucXMLBuff+num,uiXMLBuffLen-num,bNodeSelf))==FAIL)
  {
    g_xmlErrno = XML_DFENOSPC;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }
  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType&XL_TOXML && pXML->m_dwTrcType&XL_DATA )
    trc_Write(pXML->m_hTrace,"export xml string length = [%d]\n%h%m%H\n",num+n,pucXMLBuff,num+n);
#endif
  return  num+n;
}


/**********************************************************************
 *  函数：  xml_ExportXMLStringEh
 *  功能: 导出XML字串数据(增强函数)
 *  参数:
  hXML
    指定XML设备句柄
  pucXMLBuff
    接收导出取字串的缓冲区指针
  uiXMLBuffLen
    缓冲区大小
  szExportName
    指定XML结构导出位置的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
    如果为NULL或""，则导出所有数据
  bNodeSelf
    是否包含节点自身数据
    非0   导出时包含该节点元素。
    0   导出时导出该节点所有子节点数据，不包含该节点本身。
  int iLineMode             XML头部换行模式,0 不换行 1 换行
  int iParaCount           XML处理指令个数
  ...                      XML处理指令字符串列表
限    制:XML处理指令字符串列表中只允许使用字符串，否则会产生溢出

 *  返回值:
  成功  返回导出的字节数
  失败  返回FAIL
 **********************************************************************/
int xml_ExportXMLStringEh( HXMLTREE hXML,
      char *pucXMLBuff, size_t uiXMLBuffLen,
      const char *szExportName, int bNodeSelf ,
      int iLineMode,int iParaCount,...
      )
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;
int num,n,i;
va_list vaParas;
char *p;

  if ( !hXML || !pucXMLBuff || !uiXMLBuffLen )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_TOXML )
  {
    if ( szExportName )
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ExportXMLString(...,0x%p,%u,%s).\n",0,pucXMLBuff,uiXMLBuffLen,szExportName);
    else
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ExportXMLString(...,0x%p,%u,NULL).\n",0,pucXMLBuff,uiXMLBuffLen);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( szExportName && *szExportName )
  {
    if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szExportName,NULL,pXML )) )
    {
#ifdef __XML_TRACE__
      if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
        trc_Write(pXML->m_hTrace,"error: %s - [%s] \n",xml_StringError(xml_GetLastError()),szExportName);
#endif
      return  FAIL;
    }
  }
  else
  {
    lpNode = pXML->m_pRoot;
  }

  num = 40;
  if ( uiXMLBuffLen<=40 )
  {
    g_xmlErrno = XML_DFENOSPC;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }
  strcpy(pucXMLBuff,"<?xml ");

  va_start(vaParas,iParaCount);
  for(i=0;i<iParaCount;++i)
  {
    p=va_arg(vaParas,char *);
    if ( (long )p <= 0 )
      break;
    strcat(pucXMLBuff,p);
    strcat(pucXMLBuff," ");

  }
  if ( iLineMode )
    strcat(pucXMLBuff,">\n");
  else
    strcat(pucXMLBuff,">");

  va_end(vaParas);


  if ( (n=xn_ExportXMLString(lpNode,(char*)pucXMLBuff+num,uiXMLBuffLen-num,bNodeSelf))==FAIL)
  {
    g_xmlErrno = XML_DFENOSPC;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }
  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType&XL_TOXML && pXML->m_dwTrcType&XL_DATA )
    trc_Write(pXML->m_hTrace,"export xml string length = [%d]\n%h%m%H\n",num+n,pucXMLBuff,num+n);
#endif
  return  num+n;
}


/*
  xml_ExportXMLStringEh有点看不懂，不好用，改为人为加一个头
  Tianhc 2005-10-20 14:58
*/
int xml_ExportXMLStringHEAD( HXMLTREE hXML,
      char *pucXMLBuff, size_t uiXMLBuffLen,
      const char *szExportName, int bNodeSelf ,
      int iLineMode,char *shead)
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;
int num,n,i;
va_list vaParas;
char *p;

  if ( !hXML || !pucXMLBuff || !uiXMLBuffLen )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_TOXML )
  {
    if ( szExportName )
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ExportXMLString(...,0x%p,%u,%s).\n",0,pucXMLBuff,uiXMLBuffLen,szExportName);
    else
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ExportXMLString(...,0x%p,%u,NULL).\n",0,pucXMLBuff,uiXMLBuffLen);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( szExportName && *szExportName )
  {
    if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szExportName,NULL,pXML )) )
    {
#ifdef __XML_TRACE__
      if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
        trc_Write(pXML->m_hTrace,"error: %s - [%s] \n",xml_StringError(xml_GetLastError()),szExportName);
#endif
      return  FAIL;
    }
  }
  else
  {
    lpNode = pXML->m_pRoot;
  }

/*
  Tianhc 2009-7-15 17:05:21
  对于<40字节的XML内容,且不需要头部的导出会有问题
  if ( uiXMLBuffLen<=40 )
*/
  if (uiXMLBuffLen < strlen(shead))
  {
    g_xmlErrno = XML_DFENOSPC;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  /*Tianhc 2005-10-20 14:58*/
  num = strlen(shead);
  strcpy(pucXMLBuff,shead);
  if ( (iLineMode == 1) && (num > 0) )
  {
  	  strcat(pucXMLBuff,"\n");
  	  num++;
  }
  if ( (n=xn_ExportXMLString(lpNode,(char*)pucXMLBuff+num,uiXMLBuffLen-num,bNodeSelf))==FAIL)
  {
    g_xmlErrno = XML_DFENOSPC;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }
  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType&XL_TOXML && pXML->m_dwTrcType&XL_DATA )
    trc_Write(pXML->m_hTrace,"export xml string length = [%d]\n%h%m%H\n",num+n,pucXMLBuff,num+n);
#endif
  return  num+n;
}


int xml_ExportXMLStringFMT( HXMLTREE hXML,
      char *pucXMLBuff, size_t uiXMLBuffLen,
      const char *szExportName, int bNodeSelf ,
      int iLineMode,char *shead)
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;
int num,n,i;
va_list vaParas;
char *p;

  if ( !hXML || !pucXMLBuff || !uiXMLBuffLen )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_TOXML )
  {
    if ( szExportName )
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ExportXMLString(...,0x%p,%u,%s).\n",0,pucXMLBuff,uiXMLBuffLen,szExportName);
    else
      trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ExportXMLString(...,0x%p,%u,NULL).\n",0,pucXMLBuff,uiXMLBuffLen);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( szExportName && *szExportName )
  {
    if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szExportName,NULL,pXML )) )
    {
#ifdef __XML_TRACE__
      if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
        trc_Write(pXML->m_hTrace,"error: %s - [%s] \n",xml_StringError(xml_GetLastError()),szExportName);
#endif
      return  FAIL;
    }
  }
  else
  {
    lpNode = pXML->m_pRoot;
  }

  if ( uiXMLBuffLen<=40 )
  {
    g_xmlErrno = XML_DFENOSPC;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  /*Tianhc 2005-10-20 14:58*/
  num = strlen(shead);
  strcpy(pucXMLBuff,shead);
  if ( (iLineMode == 1) && (num > 0) )
  {
  	  strcat(pucXMLBuff,"\n");
  	  num++;
  }
  if ( (n=xn_ExportXMLString_Format(lpNode,(char*)pucXMLBuff+num,uiXMLBuffLen-num,bNodeSelf,0))==FAIL)
  {
    g_xmlErrno = XML_DFENOSPC;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }
  g_xmlErrno = XML_NOERROR;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType&XL_TOXML && pXML->m_dwTrcType&XL_DATA )
    trc_Write(pXML->m_hTrace,"export xml string length = [%d]\n%h%m%H\n",num+n,pucXMLBuff,num+n);
#endif
  return  num+n;
}

/**********************************************************************
 *  函数：  xml_ImportXMLString
 *  功能: 将XML格式字符串导入到XML结构指定节点下
      XML字串最多32层
 *  参数:
  hXML
    指定接收导入XML设备句柄
  szImportXMLString
    XML格式字串指针
  szImportName
    指定在XML结构接收导入的位置的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
    如果为NULL或""，则导入在根节点下
  bReplace
    是否替换szImportName节点
    >0 替换 用XML字串中第一层上从左到右顺序指定位置节点替换
    <0 替换 用XML字串中第一层上从右到左顺序指定位置节点替换
    0  不替换,所有数据引入到该节点下
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_ImportXMLString( HXMLTREE hXML,
      const char *szImportXMLString,
      const char *szImportName,
      int bReplace )
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;
int     ret;

  if ( !hXML || !szImportXMLString || !*szImportXMLString )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType&XL_INXML )
  {
    trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ImportXMLString(...,%s).\n",0,szImportName);
    if ( pXML->m_dwTrcType&XL_DATA )
      trc_Write(pXML->m_hTrace,"%h%s%H",szImportXMLString);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( szImportName && *szImportName )
  {
    if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szImportName,NULL,pXML )) )
    {
#ifdef __XML_TRACE__
      if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
        trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(xml_GetLastError()),szImportName);
#endif
      return  FAIL;
    }
  }
  else
  {
    lpNode = pXML->m_pRoot;
  }


  if ( lpNode->m_pszData && *(lpNode->m_pszData) && !bReplace )
  {
    g_xmlErrno = XML_EDATANODE;
    snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szImportName);
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(g_xmlErrno),szImportName);
#endif
    return  FAIL;
  }


  g_xmlErrno = XML_NOERROR;
  ret = xn_ImportXMLString(lpNode,(char*)szImportXMLString,bReplace,pXML);
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType&XL_INXML )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  ret;
}

/**********************************************************************
 *  函数：  xml_ImportXMLStringKVL
 *  功能: 将KVL格式字符串导入到XML结构指定节点下
      XML字串最多32层
 *  参数:
  hXML
    指定接收导入XML设备句柄
  szImportXMLString
    XML格式字串指针
  szImportName
    指定在XML结构接收导入的位置的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
    如果为NULL或""，则导入在根节点下
  bReplace
    是否替换szImportName节点
    >0 替换 用XML字串中第一层上从左到右顺序指定位置节点替换
    <0 替换 用XML字串中第一层上从右到左顺序指定位置节点替换
    0  不替换,所有数据引入到该节点下
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_ImportXMLStringKVL( HXMLTREE hXML,
      const char *szImportXMLString,
      const char *szImportName,
      int bReplace )
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;
int     ret;

  if ( !hXML || !szImportXMLString || !*szImportXMLString )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType&XL_INXML )
  {
    trc_Write(pXML->m_hTrace,">>> %t <<<  xml_ImportXMLString(...,%s).\n",0,szImportName);
    if ( pXML->m_dwTrcType&XL_DATA )
      trc_Write(pXML->m_hTrace,"%h%s%H",szImportXMLString);
  }
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( szImportName && *szImportName )
  {
    if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szImportName,NULL,pXML )) )
    {
#ifdef __XML_TRACE__
      if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
        trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(xml_GetLastError()),szImportName);
#endif
      return  FAIL;
    }
  }
  else
  {
    lpNode = pXML->m_pRoot;
  }


  if ( lpNode->m_pszData && *(lpNode->m_pszData) && !bReplace )
  {
    g_xmlErrno = XML_EDATANODE;
    snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szImportName);
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(g_xmlErrno),szImportName);
#endif
    return  FAIL;
  }


  g_xmlErrno = XML_NOERROR;
  ret = xn_ImportXMLStringKVL(lpNode,(char*)szImportXMLString,bReplace,pXML);
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType&XL_INXML )
    trc_Write(pXML->m_hTrace,"\n");
#endif
  return  ret;
}

/**********************************************************************
 *  函数: xml_asctohex
 *  功能: 将ASCII字串转换为二字节的HEX字串
 *  参数一: 接收转换后的十六进制数据缓冲区指针
 *  参数二: 缓冲区大小
 *  参数三: 待转换的ASCII数据指针
 *  参数四: 数据长度
 *  返回值: 成功,返回转换字符个数;失败返回0
 **********************************************************************/
size_t xml_asctohex( char *szHexBuf, size_t size, char *szAsc, size_t num )
{
unsigned char *p,*q,*pEnd;

  if ( size < num*2 )
  {
    g_xmlErrno = XML_DFENOSPC;
    return  0;
  }


  pEnd = (unsigned char*)szAsc;
  p = pEnd+num-1;
  q = (unsigned char*)szHexBuf+num*2-1;

  for ( ; p>=pEnd; p-- )
  {

    *q = (*p%16)+'0';
    if ( *q>'9' )
      *q+=7;

    *--q = (*p/16)+'0';
    if ( *q>'9' )
      *q+=7;

    q--;
  }
  g_xmlErrno = XML_NOERROR;
  return  num*2;
}

/**********************************************************************
 *  函数: xml_hextoasc
 *  功能: 将二字节的HEX字串转换为ASCII字串
 *  参数一: 接收转换后的ASCII数据缓冲区指针
 *  参数二: 缓冲区大小
 *  参数三: 待转换的二字节的HEX数据指针
 *  参数四: 数据长度
 *  返回值: 成功,返回转换字符个数;失败返回0
 **********************************************************************/
size_t xml_hextoasc( char *szAscBuf, size_t size, char *szHex, size_t num )
{
unsigned char *p,*q;

  if ( size < num/2 )
  {
    g_xmlErrno = XML_DFENOSPC;
    return  0;
  }

  p = (unsigned char*)szHex;
  q = (unsigned char*)szAscBuf;

  for ( ; num; num-=2, q++ )
  {
    if ( *p>='0' && *p <= '9' )
    {
      *q = (*p-'0')*16;
    }
    else if ( *p>='A' && *p <= 'F' )
    {
      *q = (*p-55)*16;
    }
    else if ( *p>='a' && *p <= 'f' )
    {
      *q = (*p-87)*16;
    }
    else
      break;
    p++;
    if ( *p>='0' && *p <= '9' )
    {
      *q += (*p-'0');
    }
    else if ( *p>='A' && *p <= 'F' )
    {
      *q += (*p-55);
    }
    else if ( *p>='a' && *p <= 'f' )
    {
      *q += (*p-87);
    }
    else
      break;
    p++;
  }
  g_xmlErrno = XML_NOERROR;
  return  (char*)q-szAscBuf;
}

/**********************************************************************
 *  函数：  xml_GetLastError
 *  功能: 获取最后一次操作错误代码
 *  参数: 无
 *  返回值: 错误码
 **********************************************************************/
int xml_GetLastError(void)
{
  return  g_xmlErrno;
}


/**********************************************************************
 *  函数：  xml_StringErrno
 *  功能: 将错误代码字串化
 *  参数一: 错误代码
 *  返回值: 错误字串
 **********************************************************************/
char *xml_StringError(int iErrno)
{
static char szErrInfo[256];

  if ( iErrno >=0 )
  {
    switch ( iErrno )
    {
    case  XML_NOERROR:
      return  "无错误(No error)";
    case  XML_EINVAL:
      return  "无效参数(Invalid argument)";
    case  XML_ENOMEM:
      return  "系统内存空间不够(Not enough memory)";

    case  XML_DFENOSPC:
      return  "指定缓冲区空间不够(No space left on device)";

    case  XML_ESYSTEM:
      snprintf(szErrInfo,sizeof(szErrInfo),"系统错误(system error) - [%s]",strerror(errno));
      return  szErrInfo;

    case  XML_ENOROOT:
      return  "无根节点数据(No root data)";
    case  XML_EINVNODENAME:
      snprintf(szErrInfo,sizeof(szErrInfo),"无效节点名称字串(Invalid node name) - [%s]",g_szErrEx);
      return  szErrInfo;
    case  XML_ENONODE:
      snprintf(szErrInfo,sizeof(szErrInfo),"不存在该节点(No such node) - [%s]",g_szErrEx);
      return  szErrInfo;
    case  XML_ENOTDATANODE:
      snprintf(szErrInfo,sizeof(szErrInfo),"不是数据节点(Not data node) - [%s]",g_szErrEx);
      return  szErrInfo;
    case  XML_EDATANODE:
      snprintf(szErrInfo,sizeof(szErrInfo),"是数据节点(Is data node) - [%s]",g_szErrEx);
      return  szErrInfo;
    case  XML_ENOATTR:
      snprintf(szErrInfo,sizeof(szErrInfo),"不存在该属性(No such attribute) - [%s]",g_szErrEx);
      return  szErrInfo;
    case  XML_EATTREXIST:
      snprintf(szErrInfo,sizeof(szErrInfo),"该属性已存在(Attribute exist already) - [%s]",g_szErrEx);
      return  szErrInfo;
    case  XML_EINVDATAPOS:
      return  "无效的数据位置(Invalid data postion)";

    case  XML_EXMLSYNTAX:
      return  "XML字串语法错误(Xml string syntax error)";
    case  XML_EXMLMAXLEV:
      return  "XML字串超过最大层数限制(Xml string max lev error)";
    case  XML_EXMLNONODE:
      snprintf(szErrInfo,sizeof(szErrInfo),"XML字串中不存在进行替换的节点(Xml string special node not exist) - [%s]",g_szErrEx);
      return  szErrInfo;
    case  XML_EOPENMAPFILE:
      return  "打开映射文件错误(Open map file error)";
    case  XML_EFILENOTEXIST:
      snprintf(szErrInfo,sizeof(szErrInfo),"映射文件不存在(Map file not exists) - [%s]",g_szErrEx);
      return  szErrInfo;
    case  XML_EFILENOTREAD:
      snprintf(szErrInfo,sizeof(szErrInfo),"映射文件不可读(Map file can not read) - [%s]",g_szErrEx);
      return  szErrInfo;
    case  XML_EFILECONT:
      return  "映射文件内容错误(Map file content error)";
    case  XML_EFILECONT_FOR:
      return  "映射文件内容循环不匹配(Map file content loop error)";
    case  XML_EFILECONT_FORLVL:
      return  "映射文件内容不支持多重循环(Map file content loop error)";
    case  XML_EFILECONT_FORCOMM:
      return  "DSR映射文件内容循环中不支持多重通讯区(Map file content loop error)";
    case  XML_EFILECONT_FORNUM:
      return  "映射文件内容循环次数错误(Map file content loop num error)";
    case  XML_EFILENOTENG:
      return  "映射文件内容与DSR数据内容不符(Map file and DSR content error)";
    case  XML_EFILECONT_XMLNODE:
      return  "映射文件内容XML节点名超长(Map file XML node name too long)";
    case  XML_EFILEMULDSR:
      return  "DSR映射文件内容中设置了多个DSR文件(Map file dsr file too much)";
    case  XML_EFILENODSR:
      return  "DSR映射文件内容中缺少DSR文件设置(Map file need dsr file)";
    case  XML_EDSRSTRERR:
      return  "DSR字串错误(Dsr content error)";
    case  XML_ELOGFILENOTEXIST:
      snprintf(szErrInfo,sizeof(szErrInfo),"TRACE配置文件不存在(TRACE config file not exists) - [%s]",g_szErrEx);
      return  szErrInfo;
    case  XML_ELOGFILENOTREAD:
      snprintf(szErrInfo,sizeof(szErrInfo),"TRACE配置文件不可读(TRACE config file can not read) - [%s]",g_szErrEx);
      return  szErrInfo;
    case  XML_EBUILDTRACE:
      return  "建立TRACE失败，请检查TRACE文件及备份目录权限(Build Trace fail, please check trace file and bak directory)";

    case  XML_EDATA_NOTENOUGTH:
      return  "引入数据长度不足(import data not enougth)";
    case  XML_EIOTYPE:
      return  "不解的映射文件类型(unknow map file type)";
    case  XML_EFILE_TYPEFIELD:
      return  "映射文件数据类型域字串错误(Map file datatype field content error)";
    case  XML_EFILE_ATTRFIELD:
      return  "映射文件属性域字串错误(Map file attribute field content error)";
    case  XML_E8583NOTEXIST:
      return  "不存在指定的8583域(Special 8583 Field not exist)";
    case  XML_E8583HEAD:
      return  "8583头部数据不匹配(8583 head data error)";
    case  XML_E8583POS:
      return  "8583数据域不匹配(8583 field error)";

    default:
      return  "未知错误代码(Unknow errno)";
    }
  }
  else
  {
    switch ( iErrno )
    {
    case -1:
      return  "通讯端口写错 [socket write error]";
    case -2:
      return  "通讯端口读错 [socket read error]";
    case -7:
      return  "数据长度错误 [data length error]";
    case -8:
      return  "通讯协议错误 [commun procotol error]";
    case -9:
      return  "[server error]";
    case -10:
      return  "数据校验错误 [data crc check error]";
    case -11:
      return  "交易处理超时 [trade deal timeout]";
    case -99:
      return  "通讯初始化失败 [commun init failed]";
    default:
      if ( iErrno>-200 )
      {
        snprintf(szErrInfo,sizeof(szErrInfo),"无法解析的错误 [unknown error],code=[%d]",iErrno);
        return  szErrInfo;
      }
      switch ( iErrno+200 )
      {
      case  -1:
        return  "包溢出 [package overflow]";
      case  -2:
        return  "包不能送达主机 [package can not send to host]";
      case  -3:
        return  "主机回复包错误 [host response error]";
      case  -4:
        return  "送主机包太长 [package too large to host]";
      default:
        snprintf(szErrInfo,sizeof(szErrInfo),"DSR网关机返回状态错误[server error],code=[%d]",iErrno+200);
        return  szErrInfo;
      }
    }
  }
}

#ifdef __XML_TRACE__
/* 获取TRACE句柄 */
int xml_GetTraceHandle( HXMLTREE hXML )
{
PXMLSTRUCT  pXML;

  if ( !hXML  )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }
  pXML =(PXMLSTRUCT)hXML;
  return  pXML->m_hTrace;
}
#endif

/**********************************************************************
 *  函数：  xml_GetVerion
 *  功能: 获取版本号
 *  参数: 无
 *  返回值: 版本号
    高字节为大版本号
    低字节为顺序号
  **********************************************************************/
#define MAKELONG_(a, b)      ((unsigned int)(((unsigned short)(a)) | ((unsigned int)((unsigned short)(b))) << 16))
unsigned int xml_GetVerion()
{
  return  MAKELONG_(97,0);
}

/**********************************************************************
 *  函数：  xml_GetLastVerDate
 *  功能: 获取最后版本日期
 *  参数: 无
 *  返回值: 版本日期串, 格式为 "yyyy-mm-dd"
  **********************************************************************/
char *xml_GetLastVerDate()
{
  return  "2001-12-13";
}

/**********************************************************************
 *  函数: FileFmtOutput
 *  功能: 用 16 进制和字符两种合并形式输出一块缓冲区内容
 *  参数一: 打开的输出文件指针
 *  参数二: 缓冲区指针
 *  参数三: 内容大小
 *  参数四: 地址起始值
 *  返回值: 无
 **********************************************************************/
void FileFmtOutput( FILE *fpFile, const void *pBuf, size_t nLen, int nStart )
{
unsigned char *pt,*p;
int nStep;

  p = pt = (unsigned char*)pBuf;
  while ( nLen ) {

    /* 前地址 */
    nStep=16;
    if ( nStart >= 0 )
      fprintf( fpFile, "[%07X0] ", nStart++ );

    /* 前16个字符 */
    for ( ; nLen && nStep>8; nLen--,nStep--,p++ )
      fprintf( fpFile, "%02X ", *p );

    for ( ; nStep > 8; nStep-- )
      fprintf( fpFile, "   " );

    fprintf( fpFile, " " );

    /* 后16个字符 */
    for ( ; nLen && nStep; nLen--,nStep--,p++ )
      fprintf( fpFile, "%02X ", *p );

    for ( ; nStep; nStep-- )
      fprintf( fpFile, "   " );

    fprintf( fpFile, "  " );

    /* 最后文本字符 */
/*    nStep = p-pt; */
    for ( ; pt < p; pt++ ) {
      if( isprint( *pt ) || ( *pt & 0x80 ) )
        fprintf( fpFile, "%c", *pt );
      else
        fprintf( fpFile, "." );
    }

/*    if ( nStep==16 ) */
      fprintf( fpFile, "\n" );
  }
  fflush( fpFile );
}

#ifdef WIN32
int exchange(long tradetype,char* databuff,long datalen)
{
  return  0;
}
void setcfgfile(char *szFile)
{
}

#endif


/************************************************************************
函数声明:xml_GetElementAttrP
函数功能:获取XML树中指定节点属性数据指针
输入参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要获取属性的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
  szAttributeName
    属性名指针
返    回:
  成功  节点属性数据指针
  失败  NULL
项 目 组:中间业务产品研发组
程 序 员:
发布日期:2000年01月01日
修改日期:2002年04月01日
例:
 **********************************************************************/
char* xml_GetElementAttrP( HXMLTREE hXML,
       const char *szElementName,
       const char *szAttributeName)
{
PXMLSTRUCT  pXML;
PXMLNODE  lpNode;
PXMLNODEATTR lpAttr;

  if ( !hXML || !szAttributeName || !*szAttributeName   )
  {
    g_xmlErrno = XML_EINVAL;
    return  NULL;
  }

  pXML = (PXMLSTRUCT)hXML;

  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
    return  NULL;
  }


  if ( !szElementName || !*szElementName )
  {
    lpNode=pXML->m_pRoot;
  }
  else
  {
    if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName, NULL,pXML )) )
      return  NULL;
  }

  if ( !(lpAttr=sl_FindAttr(&(lpNode->m_AttrList),(unsigned char*)szAttributeName,NULL)) )
  {
    g_xmlErrno = XML_ENOATTR;
    snprintf(g_szErrEx,sizeof(g_szErrEx),"%s",szAttributeName);
    return  NULL;
  }

  g_xmlErrno = XML_NOERROR;
  return  (char*)lpAttr->m_pszData;
}

/************************************************************************
函数声明:
char* xml_GetElementP( HXMLTREE hXML, const char *szElementName)
函数功能:获取XML树中指定节点数据指针
输入参数:
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
返    回:
  成功  节点数据指针
  失败  NULL
项 目 组:中间业务产品研发组
程 序 员:
发布日期:2000年01月01日
修改日期:2002年04月01日
例:
************************************************************************/
char* xml_GetElementP( HXMLTREE hXML, const char *szElementName)
{
PXMLSTRUCT pXML;
PXMLNODE  lpNode;

  if ( !hXML || !szElementName || !*szElementName  )
  {
    g_xmlErrno = XML_EINVAL;
    return  NULL;
  }

  pXML = (PXMLSTRUCT)hXML;
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
    return  NULL;
  }

  if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML )) )
    return  NULL;


  g_xmlErrno = XML_NOERROR;
  return  (char*)lpNode->m_pszData;
}

/**********************************************************************
 *  函数：  xml_GetElementLen
 *  功能: 获取XML树中指定节点数据长度
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
 **********************************************************************/
int xml_GetElementLen( HXMLTREE hXML,
       const char *szElementName)
{
PXMLSTRUCT pXML;
PXMLNODE  lpNode;

  if ( !hXML || !szElementName || !*szElementName )
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;

  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
    return  FAIL;
  }

  if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML )) )
  {
    return  FAIL;
  }

  g_xmlErrno = XML_NOERROR;
  return (lpNode->m_pszData == NULL ? 0 : strlen((char*)lpNode->m_pszData));
}


#ifdef __cplusplus
}
#endif /* end of __cplusplus */

/************************************************************************
函数声明:
HXMLTREE xml_CopyTree(HXMLTREE hXML, const char *szElementName)
函数功能:从句柄中拷贝一个子树，形成一个新的xml树
输入参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要拷贝的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
返    回:
  成功  XML句柄
  失败  FAIL
项 目 组:中间业务产品研发组
程 序 员:
发布日期:2000年01月01日
修改日期:2011-07-28 13:11
例:
************************************************************************/
HXMLTREE xml_CopyTree(HXMLTREE hXML, const char *szElementName)
{
	PXMLSTRUCT pXML, ptXML;
  PXMLNODE  lpNode;
  char *p;

  if ( !hXML || !szElementName || !*szElementName)
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
    trc_Write(pXML->m_hTrace,">>> %t <<<  xml_CopyTree(...,'%s').\n",0,szElementName);
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML )) )
  {
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR && g_xmlErrno==XML_ENONODE )
      trc_Write(pXML->m_hTrace,"error: %s - [%s]\n",xml_StringError(xml_GetLastError()),szElementName);
#endif
    return  FAIL;
  }
  
  if (!(ptXML=(PXMLSTRUCT)calloc(1,sizeof(XMLSTRUCT))) )
	{
		g_xmlErrno = XML_ENOMEM;
		return	FAIL;
	}
	
	if ( !(ptXML->m_pRoot=xn_Copy(lpNode)) )
	{
		free(ptXML);
		return FAIL;
	}
	
#ifdef	__XML_TRACE__
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
			trc_Write(pXML->m_hTrace,">>> %t <<<  xml_CopyTree('%s').\n\n",0,pXML->m_pRoot->m_pszName);
		}
	}
#endif
	return (HXMLTREE) ptXML;
}

/************************************************************************
函数声明:
int xml_LoadTree(HXMLTREE hXML, const char *szElementName, HXMLTREE hLoadXML)
函数功能:装载一个XML句柄，形成一个成为一个子树，被装载的XML句柄被释放
输入参数:
  hXML
    指定XML设备句柄
  szElementName
    装载的位置，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX   指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
  hLoadXML 被装载的XML树指针
返    回:
  成功  SUCC
  失败  FAIL
项 目 组:中间业务产品研发组
程 序 员:
发布日期:2000年01月01日
修改日期:2011-07-28 13:11
例:
************************************************************************/
int xml_LoadTree(HXMLTREE hXML, const char *szElementName, HXMLTREE *hLoadXML)
{
	PXMLSTRUCT pXML, ptXML;
  PXMLNODE  lpNode;
  char *p;

  if ( !hXML || !hLoadXML || !szElementName || !*szElementName)
  {
    g_xmlErrno = XML_EINVAL;
    return  FAIL;
  }

  pXML = (PXMLSTRUCT)hXML;
#ifdef __XML_TRACE__
  if ( pXML->m_dwTrcType & XL_NODE )
    trc_Write(pXML->m_hTrace,">>> %t <<<  xml_LoadTree(...,'%s').\n",0,szElementName);
#endif
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( pXML->m_dwTrcType&XL_ERROR )
      trc_Write(pXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }

  if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML )) )
  {
    if ( g_xmlErrno!=XML_ENONODE )
      return  FAIL;

    /* 不存在该节点添加 */
    lpNode = xn_AddNode( pXML->m_pRoot, (char*)szElementName, "", 0, pXML );
    if ( !lpNode )
    {
      return  FAIL;
    }
  }
  
  ptXML = *(PXMLSTRUCT *)hLoadXML;
#ifdef __XML_TRACE__
  if ( ptXML->m_dwTrcType & XL_NODE )
    trc_Write(ptXML->m_hTrace,">>> %t <<<  xml_LoadTree(...,'%s').\n",0,szElementName);
#endif
  if ( !ptXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
#ifdef __XML_TRACE__
    if ( ptXML->m_dwTrcType&XL_ERROR )
      trc_Write(ptXML->m_hTrace,"error: %s\n",xml_StringError(XML_ENOROOT));
#endif
    return  FAIL;
  }
  
  sl_NodeDestroy( &lpNode->m_DownList );
	sl_AttrDestroy( &lpNode->m_AttrList );
	if ( lpNode->m_pszData )
		free( lpNode->m_pszData );
  
  memcpy(&lpNode->m_DownList, &ptXML->m_pRoot->m_DownList, sizeof(WSEQLIST));
  memcpy(&lpNode->m_AttrList, &ptXML->m_pRoot->m_AttrList, sizeof(WSEQLIST));
  lpNode->m_pszData = ptXML->m_pRoot->m_pszData;
  if (ptXML->m_pRoot->m_pszName)
  	free(ptXML->m_pRoot->m_pszName);
  free(ptXML->m_pRoot);
  free(ptXML);
  *hLoadXML = 0;
	return SUCC;
}


/************************************************************************
函数声明:
PXMLNODE xml_GetNode( HXMLTREE hXML, const char *szElementName)
函数功能:获取XML树中指定节点Node指针
输入参数:
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
返    回:
  成功  节点指针
  失败  NULL
项 目 组:中间业务产品研发组
程 序 员:
发布日期:2000年01月01日
修改日期:2005-08-18 13:11
例:
************************************************************************/
PXMLNODE xml_GetNode( HXMLTREE hXML, const char *szElementName)
{
PXMLSTRUCT pXML;
PXMLNODE  lpNode;

  if ( !hXML || !szElementName || !*szElementName  )
  {
    g_xmlErrno = XML_EINVAL;
    return  NULL;
  }

  pXML = (PXMLSTRUCT)hXML;
  if ( !pXML->m_pRoot )
  {
    g_xmlErrno = XML_ENOROOT;
    return  NULL;
  }

  if ( !(lpNode=xn_LocateNode( pXML->m_pRoot, (char*)szElementName,NULL,pXML )) )
    return  NULL;

  return lpNode;
}


/*
    计算指定节点导出数据(string)时需用的长度
    一般用于ExportToString时，分配缓冲区
    ilen1  结点名长*2+5(<></>)
    ilen2 +数据内容
    ilen3 +属性名+1+2+1(="" )+
    ilen4 +属性值 ...
    只包含该结点及其下所有子结点所需的字符串长度.不包括
    <?xml version="1.0" encoding="GB2312"?> +40
*/
int xml_node_strlen(HXMLTREE lXmlhandle,char *snode)
{
   PXMLNODE  lpNode;
   int iret=0;

	 lpNode = xml_GetNode( lXmlhandle,snode);
	 if (lpNode == NULL )
	 	  return -1;
	 iret = xml_node_explen(lXmlhandle,lpNode);
	 return iret;
}


int xml_node_explen(HXMLTREE lXmlhandle,PXMLNODE lpNode)
{
PWSEQLIST	lpList;
PXMLNODEATTR *lpElemP;
PXMLNODE *lpNodeP;
int		iNameLen,iDataLen,len1,len2;
int		i,n,num;
unsigned char	*p;

	num = 0;


	if ( 1 ) /*自身节点*/
	{
		iNameLen=strlen((char*)lpNode->m_pszName);
		num	+= iNameLen;

		/* 导出属性数据 */
		lpList = &lpNode->m_AttrList;
		if ( i=sl_Length(lpList) )
		{
			lpElemP=(PXMLNODEATTR*)lpList->m_lpBase;
			for ( ; i; i--, lpElemP++ )
			{
				len1 = strlen((char*)(*lpElemP)->m_pszName);
				num += (len1+4);

				if ( (*lpElemP)->m_pszData )
				{
					len2=prv_execlen((*lpElemP)->m_pszData,strlen((char*)(*lpElemP)->m_pszData));
					if (len2 < 0)
						return	FAIL;
					num += len2;
				}
			}
		}
		num++;
	}

	/* 导出子节点数据 */
	lpList = &lpNode->m_DownList;
	if ( i=sl_Length(lpList) )
	{
		lpNodeP		= (PXMLNODE*)lpList->m_lpBase;
		for ( ; i; i--,lpNodeP++ )
		{
			n=xml_node_explen(lXmlhandle,(*lpNodeP));
			if (n<0)
				return	n;
			num+=n;
		}
	}

	if ( 1 ) /*自身节点*/
	{
		/* 导出节点值数据 */
		if ( lpNode->m_pszData )
		{
			if ( (iDataLen=prv_execlen(lpNode->m_pszData,lpNode->m_iDataLen))==FAIL )
				return	FAIL;
			num += iDataLen;
		}

		num += (iNameLen+4);
	}

	return	num;
}

/*
   Tianhc 2006-9-12 11:08
     --原xml_NodeMove有问题,重写,与旧的函数算法不同
   目标结点存在，则先清空目标节点(不是删除,否则同名节点无法处理)
         不存在，建一个空的节点；
   调用sl_Delete() + xn_Move()移动 (清除原节点在其父节点上的登记+移动+free)
*/
int xml_NodeMove( HXMLTREE hXML, const char *snode_dest,const char *snode_src)
{
  PXMLSTRUCT pXML;
  PXMLNODE  lpNode_src,lpNode_dest,lpNode_src_parent;
	char sname_save[255];
	int i,pnUpPos,m_id_save;
  pXML = (PXMLSTRUCT)hXML;
  if ( !pXML->m_pRoot )
     return -99;
	lpNode_src  = xml_GetNode(hXML, snode_src);
  if (lpNode_src==NULL)
  	 return -1;
  if ( !strcmp(snode_src,"/") || !strcmp(snode_dest,"/") )
  	 return -2;
  if ((lpNode_src_parent=xn_LocateNode( pXML->m_pRoot, (char*)snode_src, &pnUpPos, pXML )) == NULL )
  	 return -3;
	if (sl_Delete(&lpNode_src_parent->m_DownList,pnUpPos,NULL) != 0)
		 return -4;
	lpNode_dest  = xml_GetNode(hXML, snode_dest);
  if (lpNode_dest == NULL)  /*不存在则增加一个空的结点*/
  {
  	  if (xml_SetElement(hXML,snode_dest,"") < 0)
         return -5;
  	  lpNode_dest  = xml_GetNode(hXML, snode_dest);
  }
  snprintf(sname_save,sizeof(sname_save),lpNode_dest->m_pszName);
  m_id_save = lpNode_dest->m_id;
  if (xn_Move(lpNode_dest,lpNode_src) != 0)
  	 return -6;
  /*节点改名为目标名*/
  if (xn_Rename(lpNode_dest,sname_save) != 0)
  	 return -7;
  lpNode_dest->m_id=m_id_save;
  return 0;
}

int prv_execlen(char *content,int isize)
{
	  char *pstr;
	  int ilen2=0,i;

	  ilen2 = isize;
	   for (i=0,pstr=content; i<isize; i++,pstr++)
	   {
	   	  if (*pstr == '<' || *pstr == '>')
	   	  	 ilen2 += 3;
	   	  if (*pstr == '&')
	   	  	 ilen2 += 4;
	   	  if (*pstr == '\'')
	   	  	 ilen2 += 5;
	   	  if (*pstr == '"')
	   	  {
	   	  	if (g_xmlquot == 1)
	   	  	 ilen2 += 5;
	   	  	else
	   	  	 ilen2 += 6;
	   	  }
	   }
	 return ilen2;
}


/**********************************************************************
 *	函数:	调用xn_Setquot
 *	功能:	设置内部对"字符的转换标准,1-导出为&quot;(XML标准),0-导出为&quote;旧系统错误的标准
 *	参数一:	模式
 *	备注：	只为兼容旧模式的一个补丁
 *       如果改为新标准，则旧系统需合部重新编译,包括ASPK+IFI所有应用,有较大代价且易出问题
 *       xn_定义为内部函数,不对外引用,故在外再包一层
 **********************************************************************/
void xml_Setquot(int imode)
{
	 xn_Setquot(imode);
}

