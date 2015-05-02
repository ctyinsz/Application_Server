#include <stdio.h>
#include <stdlib.h>
//#include <float.h>
#include <string.h>
#define TRUE 1
#define FALSE 0
struct stack
{
	double opnum[100];
	char op[100];
	int iopnum;
	int iop;
};
int stack_init(struct stack* s)
{
	memset(s->opnum,0x00,sizeof(s->opnum));
	memset(s->op,0x00,sizeof(s->op));
	s->iop=-1;
	s->iopnum=-1;
	return 0;
}
int push_opnum(struct stack* s,double num)
{
	int maxlen=sizeof(s->opnum)/sizeof(double)-1;
	if(s->iopnum>=maxlen)
		return -1;
	s->iopnum++;
	s->opnum[s->iopnum]=num;
	return 0;
}
int push_op(struct stack* s,const char ch)
{
	int maxlen=sizeof(s->op)/sizeof(char)-1;
	if(s->iop>=maxlen)
		return -1;
	s->iop++;
	s->op[s->iop]=ch;
	return 0;
}
int pop_opnum(struct stack* s,double *num)
{
	if(s->iopnum<0)
		return -1;
	*num=s->opnum[s->iopnum];
	s->opnum[s->iopnum]=0;
	s->iopnum--;
	return 0;
}
int pop_op(struct stack* s, char *ch)
{
	if(s->iop<0)
		return -1;
	*ch=s->op[s->iop];
	s->op[s->iop]=0;
	s->iop--;
	return 0;
}
int try_topop(struct stack* s,char *ch)
{
	if(s->iop<0)
		return -1;
	*ch=s->op[s->iop];
	return 0;
}

int isempty(struct stack* s)
{
	if(s->iopnum>=0)
		return FALSE;
	else
		return TRUE;
}
int isoptr(char ch)
{
	if((ch=='+')||(ch=='-')||(ch=='*')||(ch=='/')||(ch=='(')||(ch==')'))
		return TRUE;
	else
		return FALSE;
}
int isopnd(char ch)
{
	if(((ch>='0')&&(ch<='9'))||(ch=='.'))
		return TRUE;
	else
		return FALSE;
}
int TryItem(char ch)
{
	if(isoptr(ch))
		return 1;
	else if(isopnd(ch))
		return 0;
	else 
		return -1;
}
char* GetNextOpnd(char *str,double *ret)
{
	int i=0;
	char opnd[30]={0};

	while(isopnd(str[i]))
		i++;
	if(i>0)
	{
		strncpy(opnd,str,i);
		*ret=atof(opnd);
		return str+i;
	}
	else
		return NULL;
}
char* GetNextOptr(char *str,char *ret)
{
	if(str[0]!=0)
	{
		*ret=str[0];
		return str+1;
	}
	else
		return NULL;
}
int calclevel(char ch)
{
	if((ch=='+')||(ch=='-'))
		return 0;
	else if((ch=='*')||(ch=='/'))
		return 1;
	else if(ch=='(')
		return 2;
	else
		return -1;
}
int comppri(char ch1,char ch2)
{
	int Lch1=calclevel(ch1);
	int Lch2=calclevel(ch2);
	if(Lch1>=0)
		return Lch1-Lch2;
	else
		return -3;
}
double resolve(double opnd1,double opnd2,char op)
{
	if(op=='+')
		return opnd1+opnd2;
	else if(op=='-')
		return opnd1-opnd2;
	else if(op=='*')
		return opnd1*opnd2;
	else if(op=='/')
		return opnd1/opnd2;
	else if(op=='%')
		return (int)opnd1%(int)opnd2;
}
double resolve_all(struct stack* s)
{
	double opnd1,opnd2;
	char op;
	int iret;
	while(!isempty(s))
	{
		iret = pop_op(s,&op);
		if(pop_opnum(s,&opnd2)<0)
			opnd2 = 0;
		if(iret<0)
			return opnd2;
		if(pop_opnum(s,&opnd1)<0)
			opnd1 = 0;
		if(!isempty(s))
			push_opnum(s,resolve(opnd1,opnd2,op));
		else
			return resolve(opnd1,opnd2,op);
	}
	return 0;
}
double resolve_last(struct stack* s)
{
	double opnd1,opnd2;
	char op;
	
	if(pop_opnum(s,&opnd2)<0)
		opnd2 = 0;
	if(pop_opnum(s,&opnd1)<0)
		opnd1 = 0;
	if(pop_op(s,&op)<0)
		return opnd2;
	return push_opnum(s,resolve(opnd1,opnd2,op));
}

int calc(char *str,double *ret)
{
	struct stack s;
	char *p=str;
	double opnd;
	char optr;
	stack_init(&s);

	while((p!=NULL)&&(*p!=0))
	{
		if(isopnd(p[0]))
		{
			p=GetNextOpnd(p,&opnd);
			push_opnum(&s,opnd);
		}
		else if(isoptr(p[0]))
		{
			char preop;
			int iret;
			p=GetNextOptr(p,&optr);
			if(try_topop(&s,&preop)<0)
				preop=0;
			if(optr==')')
			{
				while((preop!='(')&&(preop!=0))
				{
					resolve_last(&s); 
					if(try_topop(&s,&preop)<0)
						preop=0;
				}
				if(preop=='(')
				{
					pop_op(&s,&preop);
					continue;
				}	
			}
			else
			{
				iret = comppri(optr,preop);
				if((iret>0)||(preop=='('))
				{
					push_op(&s,optr);
				}
				else if(iret==0||iret==-1)
				{
					resolve_last(&s);
					push_op(&s,optr);
				}
				else
					return -1;
			}
		}
		else
			return -1;
	}
	if(*p==0)
	{
		*ret = resolve_all(&s); 
		return 0;
	}
	return -1;
}
int checkstr(char *str)
{
	int i=0;

	while(str[i]!=0)
	{
		if((str[i]==' ')||(str[i]=='\n')||(str[i]=='\r')||(str[i]=='\t')||(str[i]=='\v'))
		{
			int j=i;
			while(str[j]!=0)
			{
				str[j]=str[j+1];
				j++;
			}
		}
		else if(TryItem(str[i])<0)
			return -1;
		else 
		{
			i++;
			continue;
		}
	}
	return 0;
}
int main(int argc, char **argv)
{
	double ret;
	int i;

	  if ( 2 == argc )
  {
      if( (checkstr(argv[1])) < 0 )
      {
          fprintf(stderr,"非法运算表达式%s\n",argv[1]);
          return 1;
      }
  }
  else
  {
      fprintf(stderr,"Usage:%s <运算表达式>\n",argv[0]);
      return 1;
  }

  calc(argv[1],&ret);
  char buf[100]={0};
  printf("%s=%.*lf\n",argv[1],3,ret);
  return 0;
}
