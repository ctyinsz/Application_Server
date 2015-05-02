#include	"gas.inc"
int SDATA_MSetValue(HXMLTREE hXml)
{
	printf("this is comp %s\n",__func__);
	char status[10]={0};
	char parabuf[NODELEN]={0};
	char nodepath[NODEPATHLEN]={0};	
	int i,num;
	if((num =  xml_ElementCount(hXml,COMP"/complist/para"))==FAIL)
		return FAIL;
	
	for(i=0;i<num;i++)
	{
		memset(parabuf,0x00,sizeof(parabuf));
		memset(nodepath,0x00,sizeof(nodepath));
		snprintf(nodepath,sizeof(nodepath)-1,COMP"/complist/para|%d",i+1);
		if(xml_GetElement(hXml,nodepath,parabuf,sizeof(parabuf)-1)==FAIL)
			return FAIL;
		printf("para %d:[%s]\n",i,parabuf);
	}	
	
	strcpy(status,"0");
	if(xml_SetElement(hXml , COMP"/compstatus",status)==FAIL)
		return FAIL;
	return SUCC;
}

int SDATA_ValCompare(HXMLTREE hXml)
{
	printf("this is comp %s\n",__func__);
	char status[10]={0};
	
	strcpy(status,"0");
	char parabuf[NODELEN]={0};
	char nodepath[NODEPATHLEN]={0};	
	int i,num;
	if((num =  xml_ElementCount(hXml,COMP"/complist/para"))==FAIL)
		return FAIL;
	
	for(i=0;i<num;i++)
	{
		memset(parabuf,0x00,sizeof(parabuf));
		memset(nodepath,0x00,sizeof(nodepath));
		snprintf(nodepath,sizeof(nodepath)-1,COMP"/complist/para|%d",i+1);
		if(xml_GetElement(hXml,nodepath,parabuf,sizeof(parabuf)-1)==FAIL)
			return FAIL;
		printf("para %d:[%s]\n",i,parabuf);
	}

	if(xml_SetElement(hXml , COMP"/compstatus",status)==FAIL)
		return FAIL;
	return SUCC;
}


