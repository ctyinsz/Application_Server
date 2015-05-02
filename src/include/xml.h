/************************************************************************
版权所有:杭州恒生电子股份有限公司
项目名称:公共项目
版    本:V1.0
操作系统:AIX4.2,SCO5
文件名称:xml.h
文件描述:简单XML操作函数库
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
#ifndef __XML_H__
#define __XML_H__

#include <stdio.h>
#include <sys/types.h>

#define    FAIL      (-1)
#define     SUCC       (0)

typedef long    HXMLTREE;

#define    IO_TYPE_NORMAL    1  /* 定长或分隔符类型 */
#define    IO_TYPE_8583    2  /* 8583类型 */
#define    IO_TYPE_XML      3  /* XML类型 */
#define    IO_TYPE_DSR      4  /* DSR数据类型 */


/**********************************************************************
 *  函数:  xml_Create
 *  功能:  初始化XML结构
 *  参数:
  szRootName
          指定XML结构根元素的名称。
 *  返回值:
  成功  返回XML结构句柄
  失败  返回FAIL
 **********************************************************************/
HXMLTREE xml_Create( const char *szRootName);


/**********************************************************************
 *  函数:  xml_Destroy
 *  功能:  删除XML结构
 *  参数:
  hXML
          待删除的XML设备句柄。
 *  返回值:
  成功  返回字节数
  失败  返回FAIL
 **********************************************************************/
int xml_Destroy( HXMLTREE hXML );

/**********************************************************************
 *  函数:  xml_Clear
 *  功能:  清除XML结构中所有数据，只保留根节点
 *  参数:
  hXML
          待清除的XML设备句柄。
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_Clear( HXMLTREE hXML );

/**********************************************************************
 *  函数:  xml_AddElement
 *  功能:  在XML树中添加一个叶子节点
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
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
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
       const char *szElementValue );

/**********************************************************************
 *  函数:  xml_SetElement
 *  功能:  在XML树中设置一个叶子节点
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
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
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
       const char *szElementValue );

/**********************************************************************
 *  函数:  xml_GetElement
 *  功能:  获取XML树中指定节点数据
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要获取的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
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
       const size_t uiElementBuffSize );


/**********************************************************************
 *  函数:  xml_ModifyElement
 *  功能:  修改XML树中指定节点数据
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要修改的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
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
       const char *szElementValue );


/**********************************************************************
 *  函数:  xml_DelElement
 *  功能:  删除XML树中指定节点数据
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要删除的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_DelElement( HXMLTREE hXML, const char *szElementName );



/**********************************************************************
 *  函数:  xml_ElementExist
 *  功能:  判断XML树中指定节点是否存在
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要删除的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
 *  返回值:
  成功  存在,返回1; 不存在,返回0
  失败  返回FAIL
 **********************************************************************/
int xml_ElementExist( HXMLTREE hXML, const char *szElementName );

/**********************************************************************
 *  函数:  xml_IsLeafNode
 *  功能:  判断XML树中指定节点是否为叶子节点(数据节点)
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要判断的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
 *  返回值:
  成功  是叶子节点,返回1; 不是,返回0
  失败  返回FAIL
 **********************************************************************/
int xml_IsLeafNode( HXMLTREE hXML, const char *szElementName );



/**********************************************************************
 *  函数:  xml_ChildElementCount
 *  功能:  获取XML树中指定节点的子节点个数
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构中的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_ChildElementCount( HXMLTREE hXML,
       const char *szElementName );



/**********************************************************************
 *  函数:  xml_GetChildElementName
 *  功能:  获取XML树节点中指定位置子节点名
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要获取的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
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
       const size_t uiNameBuffSize );


/**********************************************************************
 *  函数:  xml_AddElementAttr
 *  功能:  在XML树中添加一个节点属性
    属性节点不可以重复存在
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要添加属性的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  表示取最后一个元素，
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
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
       const char *szAttributeValue );


/**********************************************************************
 *  函数:  xml_SetElementAttr
 *  功能:  在XML树中设置一个节点属性
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
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
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
       const char *szAttributeValue );


/**********************************************************************
 *  函数:  xml_GetElementAttr
 *  功能:  获取XML树中指定节点数据
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要获取属性的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
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
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_GetElementAttr( HXMLTREE hXML,
       const char *szElementName,
       const char *szAttributeName,
       char *pucAttributeBuff,
       const size_t uiAttrBuffSize );


/**********************************************************************
 *  函数:  xml_DelElementAttr
 *  功能:  删除XML树中指定节点属性
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要删除属性的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
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
       const char *szAttributeName);



/**********************************************************************
 *  函数:  xml_ElementAttrExist
 *  功能:  判断XML树中指定节点属性是否存在
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要判断属性的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
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
       const char *szAttributeName);


/**********************************************************************
 *  函数:  xml_ModifyElementAttr
 *  功能:  修改XML树中指定节点属性数据
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要获取属性的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
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
       const char *szAttributeValue );



/**********************************************************************
 *  函数:  xml_GetAttributeName
 *  功能:  获取XML树节点中指定位置属性名
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要获取的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
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
       const size_t uiAttrNameBuffSize );



/**********************************************************************
 *  函数:  xml_AttributeCount
 *  功能:  获取XML树中指定节点的属性个数
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构中的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
 *  返回值:
  成功  返回该节点子元素个数
  失败  返回FAIL
 **********************************************************************/
int xml_AttributeCount( HXMLTREE hXML,
       const char *szElementName );



/**********************************************************************
 *  函数:  xml_ElementCount
 *  功能:  统计XML树中指定节点元素同一层上的个数
 *  参数:
  hXML
    指定XML设备句柄
  szElementName
    指定在XML结构要统计的元素的名称，形如"/PRIVATE|2/电费单价"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn/FINDNAME"
    NMAE  指定每一层元素名,
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
 *  返回值:
  成功  返回个数
  失败  返回FAIL
 **********************************************************************/
int xml_ElementCount( HXMLTREE hXML,
       const char *szElementName );




/**********************************************************************
 *  函数:  xml_ExportXMLString
 *  功能:  导出XML字串数据
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
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
    如果为NULL或""，则导出所有数据
  bNodeSelf
    是否包含节点自身数据
    非0    导出时包含该节点元素。
    0    导出时导出该节点所有子节点数据，不包含该节点本身。
  *  返回值:
  成功  返回导出的字节数
  失败  返回FAIL
 **********************************************************************/
int xml_ExportXMLString( HXMLTREE hXML,
      char *pucXMLBuff,
      size_t uiXMLBuffLen,
      const char *szExportName,
      int bNodeSelf );



/**********************************************************************
 *  函数:  xml_ImportXMLString
 *  功能:  将XML格式字符串导入到XML结构指定节点下
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
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
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
      const char *szImportXML,
      const char *szImportName,
      int bReplace );



/**********************************************************************
 *  函数:  xml_ExportDSRString
 *  功能:  导出DSR格式字串数据
 *  参数:
  hXML
    指定XML设备句柄
  szMapFile
    DSR <=> XML 映射文件
  pucDSRBuff
    接收导出取字串的缓冲区指针
  uiDSRBuffLen
    缓冲区大小
 *  返回值:
  成功  返回导出的字节数
  失败  返回FAIL
 **********************************************************************/
int xml_ExportDSRString( HXMLTREE hXML,
      const char *szMapFile,
      char *pucDSRBuff,
      size_t uiDSRBuffLen );



/**********************************************************************
 *  函数:  xml_ImportDSRString
 *  功能:  将DSR格式字符串导入到XML结构指定节点下
 *  参数:
  hXML
    指定接收导入XML设备句柄
  szMapFile
    DSR <=> XML 映射文件
  szImportDSRString
    DSR格式字串指针
  size
    DSR格式内容长度
  szImportName
    指定在XML结构接收导入的位置的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
    如果为NULL或""，则导入在根节点下
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_ImportDSRString( HXMLTREE hXML,
      const char *szMapFile,
      const char *szImportDSRString,
      size_t  size,
      const char *szImportName );

/**********************************************************************
 *  函数:  xml_ExchangeWithDSR
 *  功能:  和DSR网关机交换数据
 *  参数:
  hXML
    指定发往网关机数据的XML设备句柄
  szMapFile
    DSR <=> XML 映射文件
  lLUType
    LU类型
  hRecvXML
    接收网关机数据的XML设备句柄
  szImportName
    指定在接收XML结构中位置的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
    如果为NULL或""，则导入在根节点下
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_ExchangeWithDSR( HXMLTREE hXML,
      const char *szMapFile,
      long    lLUType,
      HXMLTREE  hRecvXML,
      const char *szImportName );


/**********************************************************************
 *  函数:  xml_ExportFmtString
 *  功能:  导出指定格式字串数据
 *  参数:
  hXML
    指定XML设备句柄
  szMapFile
    XML <=> 指定格式 映射文件
  pucBuff
    接收导出取字串的缓冲区指针
  uiBuffLen
    缓冲区大小
  iType
    导出的类型
 *  返回值:
  成功  返回导出的字节数
  失败  返回FAIL
 **********************************************************************/
int xml_ExportFmtString( HXMLTREE hXML,
      const char *szMapFile,
      char *pucBuff,
      size_t uiBuffLen,
      int    iType );

/**********************************************************************
 *  函数:  xml_ImportFmtString
 *  功能:  将指定格式字符串导入到XML结构指定节点下
 *  参数:
  hXML
    指定接收导入XML设备句柄
  szMapFile
    XML <=> 指定格式 映射文件
  szImportString
    指定格式字串指针
  size
    指定格式内容长度
  szImportName
    指定在XML结构接收导入的位置的元素的名称，形如"/PRIVATE|2/电费单价|0"
    "/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
    NMAE  指定每一层元素名,
    IDX    指定每一层元素在同名兄弟元素中的排序位置。
      如果iIndex为0，表示取最后一个元素，
      如果为1，2，3……，表示取相应编号的元素。
      缺省为1
    如果为NULL或""，则导入在根节点下
 *  返回值:
  成功  返回SUCC
  失败  返回FAIL
 **********************************************************************/
int xml_ImportFmtString( HXMLTREE hXML,
      const char *szMapFile,
      const char *szImportString,
      size_t  uiStrLen,
      const char *szImportName,
      int    iType );


int xml_Save( HXMLTREE hXML, int iType, char szName );

int xml_Restore( HXMLTREE hXML, int iType, char szName, int iId );


#define  xml_GetLastErrorString()  xml_String(xml_GetLastError())

/**********************************************************************
 *  函数:  xml_GetLastError
 *  功能:  获取最后一次操作错误代码
 *  参数:  无
 *  返回值: 错误码
 **********************************************************************/
int xml_GetLastError(void);

/**********************************************************************
 *  函数:  xml_StringErrno
 *  功能:  将错误代码字串化
 *  参数一:  错误代码
 *  返回值: 错误字串
 **********************************************************************/
char *xml_StringError(int iErrno);

/**********************************************************************
 *  错误代码
 **********************************************************************/
#define XML_NOERROR         0    /* 无错误(No error)                    */
#define XML_EINVAL         22    /* 无效参数(Invalid argument)          */
#define XML_ENOMEM         12    /* 系统内存空间不够(Not enough memory)  */

#define XML_DFENOSPC       28    /* 指定缓冲区空间不够(No space left on device)  */

#define XML_ESYSTEM       200    /* 系统错误(system error)           */

#define XML_ENOROOT       201    /* 无根节点数据(No root data)           */
#define XML_EINVNODENAME  202    /* 无效节点名称字串(Invalid node name)  */
#define XML_ENONODE       203    /* 不存在该节点(No such node)           */
#define XML_ENOTDATANODE  204    /* 不是数据节点(Not data node)          */
#define XML_EDATANODE     205    /* 是数据节点(Is data node)             */
#define XML_ENOATTR       206    /* 不存在该属性(No such attribute)      */
#define XML_EATTREXIST    207    /* 该属性已存在(Attribute exist already)*/
#define XML_EINVDATAPOS   208    /* 无效的数据位置(Invalid data postion) */

#define XML_EXMLSYNTAX    209    /* XML字串语法错误(Xml string syntax error)    */
#define XML_EXMLMAXLEV    210    /* XML字串超过最大层数限制(Xml string max lev error)    */
#define XML_EXMLNONODE    211    /* XML字串中不存在进行替换的节点(Xml string special node not exist)    */

#define XML_EOPENMAPFILE  221    /* 打开DSR映射文件错误(Open map file error)*/
#define XML_EFILENOTEXIST 222    /* 映射文件不存在(Map file not exists)*/
#define XML_EFILENOTREAD  223    /* 映射文件不可读(Map file can not read)*/
#define XML_EFILECONT     224    /* 映射文件内容错误(Map file content error)*/
#define XML_EFILECONT_FOR 225    /* 映射文件内容循环不匹配(Map file content loop error)*/
#define XML_EFILECONT_FORLVL  226 /* 映射文件内容不支持多重循环(Map file content loop error)*/
#define XML_EFILECONT_FORCOMM  227 /* DSR映射文件内容循环中不支持多重通讯区(Map file content loop error)*/
#define XML_EFILECONT_FORNUM  228 /* 映射文件内容循环次数错误(Map file content loop num error)*/
#define XML_EFILENOTENG      229 /* 映射文件内容与DSR数据内容不符(Map file and DSR content error)*/
#define XML_EFILECONT_XMLNODE   230 /* 映射文件内容XML节点名超长(Map file XML node name too long)*/
#define XML_EFILEMULDSR      231 /* DSR映射文件内容中设置了多个DSR文件(Map file dsr file too much)*/
#define XML_EFILENODSR      232 /* DSR映射文件内容中缺少DSR文件设置(Map file need dsr file)*/
#define XML_EDSRSTRERR      233 /* DSR字串错误(Dsr content error)       */

#define XML_ELOGFILENOTEXIST  234 /* TRACE配置文件不存在(TRACE config file not exists)*/
#define XML_ELOGFILENOTREAD    235 /* TRACE配置文件不可读(TRACE config file can not read)*/
#define  XML_EBUILDTRACE      236  /* 建立TRACE失败，请检查TRACE文件及备份目录权限(Build Trace fail, please check trace file or bak directory) */

#define  XML_EDATA_NOTENOUGTH  237 /* 引入数据长度不足(import data not enougth) */
#define  XML_EIOTYPE        238 /* 不解的映射文件类型(unknow map file type) */
#define  XML_EFILE_TYPEFIELD    239  /* 映射文件数据类型域字串错误(Map file datatype field content error) */
#define  XML_EFILE_ATTRFIELD    240  /* 映射文件属性域字串错误(Map file attribute field content error) */
#define  XML_E8583NOTEXIST    241 /* 不存在指定的8583域(Special 8583 Field not exist) */
#define  XML_E8583HEAD      242 /* 8583头部数据不匹配(8583 head data error) */
#define  XML_E8583POS      243 /* 8583数据域不匹配(8583 field error) */

/**********************************************************************
 *  函数:  xml_asctohex
 *  功能:  将ASCII字串转换为二字节的HEX字串
 *  参数一:  接收转换后的十六进制数据缓冲区指针
 *  参数二:  缓冲区大小
 *  参数三:  待转换的ASCII数据指针
 *  参数四:  数据长度
 *  返回值:  成功,返回转换字符个数;失败返回0
 **********************************************************************/
size_t xml_asctohex( char *szHexBuf, size_t size, char *szAsc, size_t num );

/**********************************************************************
 *  函数:  xml_hextoasc
 *  功能:  将二字节的HEX字串转换为ASCII字串
 *  参数一:  接收转换后的ASCII数据缓冲区指针
 *  参数二:  缓冲区大小
 *  参数三:  待转换的二字节的HEX数据指针
 *  参数四:  数据长度
 *  返回值:  成功,返回转换字符个数;失败返回0
 **********************************************************************/
size_t xml_hextoasc( char *szAscBuf, size_t size, char *szHex, size_t num );

int xml_GetTraceHandle( HXMLTREE hXML );

/**********************************************************************
 *  函数:  xml_GetVerion
 *  功能:  获取版本号
 *  参数:  无
 *  返回值: 版本号
 **********************************************************************/
#define LOWORD_(l)           ((unsigned short)(l))
#define HIWORD_(l)           ((unsigned short)(((unsigned int)(l) >> 16) & 0xFFFF))
unsigned int xml_GetVerion();

/**********************************************************************
 *  函数:  xml_GetLastVerDate
 *  功能:  获取最后版本日期
 *  参数:  无
 *  返回值: 版本日期串, 格式为 "yyyy-mm-dd"
  **********************************************************************/
char *xml_GetLastVerDate();


/*use commun swap data call outlook*/
int xml_exchange(long lLUType,char* databuff,long datalen);


void FileFmtOutput( FILE *fpFile, const void *pBuf, size_t nLen, int nStart );

/************************************************************************
函数声明:xml_GetElementAttrP
函数功能:获取XML树中指定节点属性数据指针
输入参数: 
	hXML
		指定XML设备句柄
	szElementName
		指定在XML结构要获取属性的元素的名称，形如"/PRIVATE|2/电费单价|0"
		"/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
		NMAE	指定每一层元素名,
		IDX		指定每一层元素在同名兄弟元素中的排序位置。
			如果iIndex为0，表示取最后一个元素，
			如果为1，2，3……，表示取相应编号的元素。
			缺省为1
	szAttributeName
		属性名指针
返    回: 
	成功	节点属性数据指针
	失败	NULL
项 目 组:中间业务产品研发组
程 序 员:
发布日期:2000年01月01日
修改日期:2002年04月01日
例:
 **********************************************************************/
char* xml_GetElementAttrP( HXMLTREE hXML,
		   const char *szElementName,
		   const char *szAttributeName);
		   
/************************************************************************
函数声明:xml_GetElementP
函数功能:获取XML树中指定节点数据指针
输入参数: 
	hXML
		指定XML设备句柄
	szElementName
		指定在XML结构要获取的元素的名称，形如"/PRIVATE|2/电费单价|0"
		"/NAME1|IDX1/NAME2|IDX2/.../NAMEn|IDXn"
		NMAE	指定每一层元素名,
		IDX		指定每一层元素在同名兄弟元素中的排序位置。
			如果iIndex为0，表示取最后一个元素，
			如果为1，2，3……，表示取相应编号的元素。
			缺省为1
返    回: 
	成功	节点数据指针
	失败	NULL
项 目 组:中间业务产品研发组
程 序 员:
发布日期:2000年01月01日
修改日期:2002年04月01日
例:
************************************************************************/
char* xml_GetElementP( HXMLTREE hXML, const char *szElementName);
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
项 目 组:中间业务产品研发组
程 序 员:
发布日期:2000年01月01日
修改日期:2002年04月01日
例:
 **********************************************************************/
int xml_GetElementLen( HXMLTREE hXML,const char *szElementName);

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
int xml_node_strlen(HXMLTREE lXmlhandle,char *snode);

/**********************************************************************
 *	函数:	调用xn_Setquot
 *	功能:	设置内部对"字符的转换标准,1-导出为&quot;(XML标准),0-导出为&quote;旧系统错误的标准
 *	参数一:	模式
 *	备注：	只为兼容旧模式的一个补丁
 *       如果改为新标准，则旧系统需合部重新编译,包括ASPK+IFI所有应用,有较大代价且易出问题
 *       xn_定义为内部函数,不对外引用,故在外再包一层
 **********************************************************************/
void xml_Setquot(int imode);

/*
   Tianhc 2006-9-12 11:08
     --原xml_NodeMove有问题,重写,与旧的函数算法不同
   目标结点存在，则先清空目标节点(不是删除,否则同名节点无法处理)
         不存在，建一个空的节点；
   调用sl_Delete() + xn_Move()移动 (清除原节点在其父节点上的登记+移动+free)     
*/
int xml_NodeMove( HXMLTREE hXML, const char *snode_dest,const char *snode_src);
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
HXMLTREE xml_CopyTree(HXMLTREE hXML, const char *szElementName);

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
int xml_LoadTree(HXMLTREE hXML, const char *szElementName, HXMLTREE *hLoadXML);

#endif /* end of  __XML_H__ */
