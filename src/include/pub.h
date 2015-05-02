#ifndef _PUB_
#define _PUB_
#include	"gas.inc"


#define MAXPARANUM 100		//最大参数数量
#define CFGFILELEN 16384	//资源文件长度
#define COMMBUFLEN 16384			//通讯缓冲区长度
#define BUF_SIZE 512			//一般缓冲区长度
#define MSG_NUM 3  				//并发数量
#define LINKMODE 0 				//1 短连接 0长连接
#define CFGFILENAME 512		//资源文件名长度
#define NODEPATHLEN 512		//参数树节点路径长度
#define NODELEN 2048			//参数树节点长度

#define FLOW "/sys/flow"
#define COMP FLOW"/comp"
#define COMMBUF "/commbuf"

#define RESPATH "/home/traxex/app/myproject/cfg"

//资源文件操作
mxml_type_t	type_cb(mxml_node_t *node);
int loadfile(char *filename,char *buf,unsigned long inlen);
int gbk2utf8(char *srcBuf);
int code_convert(char *from_charset, char *to_charset, char *inbuf, unsigned long inlen, char *outbuf, unsigned long outlen);

//流程资源处理
//int exeflow(HXMLTREE lXmlhandle,char *flowbuf);
//int flow_GetParas(mxml_node_t *node_comp, mxml_node_t *tree,const char **paras_out, int maxnum);
//const char *flow_GetStatus(mxml_node_t *node_comp, mxml_node_t *tree, char *statuscfg);
//char *doProcess(const char **paras, const char *snodename, char *statuscfg_out);

int ExeFlow(HXMLTREE lXmlhandle);
int RetrievePara( HXMLTREE lXmlhandle , mxml_node_t *node_comp, mxml_node_t *tree );
const char* NextFlowsn( HXMLTREE lXmlhandle ,mxml_node_t *node_comp, mxml_node_t *tree );


//组件资源处理
int ExeComp(HXMLTREE lXmlhandle);

#endif
