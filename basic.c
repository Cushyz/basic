#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <time.h>

static double var[24]={0.0};

#define VAR(c) 		(var[c-'A'])
#define STR_SIZE	512
#define BUFF_SIZE 	1024
#define LABLE_SIZE 	512
#define STACK_SIZE 	128
#define MAKE(s) 	{#s,s}
#define M_NULL  	{NULL,0}
#define ERROR(args...) {\
	fprintf(stderr,args);\
	fputc('\n',stderr);\
	exit(0);}
static char src[BUFF_SIZE],*at=src;
static char str[STR_SIZE];
static struct{
	unsigned int num;
	char*pos;
}lables[LABLE_SIZE];
char*call_stk[STACK_SIZE],**sp=call_stk;
struct{
	int end,step,sop;
	char who;
	char*pos;
}for_stk[26],*fsp=for_stk;
static int lable_num=0;
enum kind{
	DELIMITER=1,
	VARIABLE,
	NUMBER,
	SFUNC,
	IDENTITY,
	STRING,
	FUNCTION
};
enum{
	EOL=-2,EOS=-3,
	NIL=0,

	BE=256,
	LE,
	NE,
	END=512,
	AND,	OR,
	NOT,
	PRINT,	INPUT,
	RANDOM,
	IF,		THEN,
	FOR,	TO,
	STEP,	NEXT,
	GOTO,	GOSUB,
	RETURN
};

static struct{
	char*p; int k;
}IDtable[]={
	MAKE(END),
	MAKE(PRINT),	MAKE(INPUT),
	MAKE(RANDOM),
	MAKE(PRINT),	MAKE(INPUT),
	MAKE(RANDOM),
	MAKE(IF),		MAKE(THEN),
	MAKE(FOR),		MAKE(TO),
	MAKE(STEP),		MAKE(NEXT),
	MAKE(GOTO),		MAKE(GOSUB),
	MAKE(RETURN),
	M_NULL
};
/*
static struct{
	char*p; double (*f)(double);
}functable[]={
	MAKE(sin),		MAKE(asin),
	MAKE(cos),		MAKE(acos),
	MAKE(tan),		MAKE(atan),
	MAKE(sinh),		MAKE(cosh),
	MAKE(tanh),
	MAKE(log),		MAKE(exp),
	MAKE(sqrt),
	MAKE(abs),		MAKE(ceil),
	MAKE(floor),	MAKE(round),
	M_NULL
};
*/
static struct{
	enum kind k;
	union{
		double d;
		int v;
		void *s;
	};
	int n;
}token;

int ifind(char*t){
	int i;
	for (i = 0; IDtable[i].k; ++i)
		if (strcasecmp(IDtable[i].p,t)==0)
			break;
	return IDtable[i].k;
}
char*lfind(unsigned int lab){
	int mi=0,ma=lable_num-1,mid;
	while(1){
		mid=(mi+ma)/2;
		if(lab==lables[mid].num)
			break;
		if(lab==lables[ma].num){
			mid=ma;
			break;
		}
		else if(lables[mi].num<=lab &&
			lab<lables[mid].num)
			ma=mid;
		else if(lables[mid].num<lab &&
			lab<=lables[ma].num)
			mi=mid;
		else ERROR("Lable %u not found.",lab)
	}
	return lables[mid].pos;
}

void scanflable(){
	char *p=src;
	int n,i;
	unsigned int labn;
	while(*p!=0){
		if(sscanf(p,"%u%n",&labn,&n)==1){
			p+=n-1;
			if(lable_num+1 == LABLE_SIZE)
				ERROR("To many lables"); 
			for(i = lable_num; i>0; --i){
				if(labn < lables[i-1].num)
					lables[i]=lables[i-1];
				else if(labn > lables[i-1].num)
					break;
				else ERROR("Two same lables: %u."
					,labn)
			}
			++lable_num;
			lables[i].num=labn;
			lables[i].pos=p;
		}
		while(*p++!='\n'&&*p!=0);
	}
}

int next(){
	char*p;	int i;
	while(*at==' '||*at=='\t')++at;
	if(*at=='\n'||*at=='\''||*at==0){
		while(*at++!='\n'&&*at!=0);
		token.v=*at?EOL:EOS;
		token.n=1;
		return (token.k=DELIMITER);
	}
	if(*at=='"'){
		token.s=p=str,i=0;
		while(*++at!='"'&&i++<STR_SIZE){
			*p++=*at;
			if (*at=='\n'||!*at)
				ERROR("Unexpect char"
					" in string: [\\%d].",*at);
		}
		at++;
		*p=0;
		return (token.k=STRING);
	}
	if((p=strchr("+-*/%^=<>(),;",*at))) {
		if(!p)
			ERROR("Unexpect token: '%c'.",*at);
		token.n=1;
		if(*at=='<'){
			if(*(at+1)=='>'){
				token.v=NE;
				token.n=2;
				++at;
			}
			else if (*(at+1)=='='){
				token.v=LE;
				token.n=2;
				++at;
			}
			else token.v=*at;
		}
		else if(*at=='>'&&*(at+1)=='='){
			token.n+=1;
			token.v=BE;
			++at;
		}
		else token.v=*p;
		++at;
		return (token.k=DELIMITER);
	}
	if(isdigit(*at)){
		sscanf(at,"%lf%n",&token.d,&i);
		token.n=i;
		at+=i;
		return (token.k=NUMBER);
	}
	if(isalpha(*at)){
		token.s=p=str;
		while(isalpha(*at))
			*p++=*at++;
		*p=0;
		if(*(str+1)==0){
			token.n=1;
			token.v=toupper(*str);
			return (token.k=VARIABLE);
		}
		i=ifind(str);
		if(!i)
			ERROR("Unexpect token: '%s'.",str);
		token.n=strlen(str);
		if(i==RANDOM){
			token.d=(double)rand()/RAND_MAX;
			return (token.k=SFUNC);
		}
		token.v=i;
		return (token.k=IDENTITY);
	}
	ERROR("Unexpect char: '%c'.",*at);
	exit(1);
}

double expr();
double numpar(){
	double r;
	int f=1;
	next();
	if(token.k==DELIMITER&&
		(token.v=='+'||token.v=='-')){
		if(token.v=='-')f=-1;
		next();
	}	
    if(token.k==NUMBER||token.k==SFUNC)
    	r=token.d;
    else if(token.k==VARIABLE)
    	r=VAR(token.v);
    else if(token.k==DELIMITER){
    	if(token.v=='('){
    		r=expr();
    		next();
    		if(token.k!=DELIMITER||token.v!=')')
    			ERROR("Expect ')' but no.");
    	}
    	else ERROR("Expect expression but no.");
    }
    else ERROR("Unknow token in expression.");
    return r;
}
double mpow()
{
    double a=numpar();
    while (1) {
        next();
        if(token.k==DELIMITER&&token.v=='^')
            a=pow(a, numpar());
        else break;
    }
    return a;
}
double muldiv()
{
    double a=mpow();
    while (1) {
        if(token.k==DELIMITER){
        	if(token.v=='*')
            	a*=numpar();
            else if(token.v=='/')
            	a/=numpar();
            else if(token.v=='%')
            	a/=numpar();
            else break;
        }
        else break;
    }
    return a;
}
double addsub()
{
    double a=muldiv();
    while (1) {
        if(token.k==DELIMITER){
        	if(token.v=='+')
            	a+=muldiv();
            else if(token.v=='-')
            	a-=muldiv();
            else break;
        }
        else break;
    }
    return a;
}
double test(){
	double a=addsub();
	if (token.k==DELIMITER){
		if(token.v=='='){
			a=a==addsub();
		}
		else if(token.v=='<'){
			a=a<addsub();
		}
		else if(token.v=='>'){
			a=a>addsub();
		}
		else if(token.v==NE){
			a=a!=addsub();
		}
		else if(token.v==LE){
			a=a<=addsub();
		}
		else if(token.v==BE){
			a=a>=addsub();
		}
	}
	return a;
}
double expr(){
	double d=test();
	at-=token.n;
	return d;
}
void stmt(){
	int t;
	while(1){
		t=0;
		sscanf(at,"%*u%n",&t);
		at+=t;
	start:
		next();
		if (token.k==VARIABLE){
			t=token.v;
			next();
			if(token.k!=DELIMITER||token.v!='=')
    			ERROR("Expect '=' but no.");
    		VAR(t)=expr();
    		next();
    		if(token.k!=DELIMITER
    			||(token.v!=EOL&&token.v!=EOS))
    			ERROR("Only one statement a line.");
		}
		else if (token.k==IDENTITY){
			switch(token.v){
				case PRINT:
				next();
				while(1){
					if(token.k==VARIABLE||
						token.k==NUMBER||
						token.k==SFUNC){
						at-=token.n;
						printf("%g",expr());
					}
					else if(token.k==STRING)
						printf("%s", token.s);
					else ERROR("Unknow usage of print.");
					next();
					if(token.k==DELIMITER)
						switch(token.v){
							case ',':putchar('\t');
							case ';':break;
							case EOL:
								putchar('\n');
								goto out;
							default:
							ERROR("Unknow usage of print.")
						}
					next();
					if(token.k==DELIMITER&&token.v==EOL)
						goto out;
				}
				out:
				break;
				case INPUT:
				if(next()==VARIABLE)
					scanf("%lf",&VAR(token.v));
				else ERROR("Input need a variable.");
				break;
				case IF:
				t=expr();
				if(token.k!=IDENTITY||token.v!=THEN)
    					ERROR("If need a then.");
    			if(t) goto start;
    			else while(next()!=DELIMITER
    				||(token.v!=EOL&&token.v!=EOF));
				break;
				case FOR:
				if(fsp+1>for_stk+26)
					ERROR("Stack overflow")
				fsp->step=1;
				if(next()!=VARIABLE)
					ERROR("For need a variable.")
				fsp->who=token.v;
				if(next()!=DELIMITER||token.v!='=')
    				ERROR("Expect '=' but no.");
    			VAR(fsp->who)=expr();
    			if(next()!=IDENTITY||token.v!=TO)
    				ERROR("Expect 'To' but no.");
    			fsp->end=expr();
    			fsp->sop=fsp->end-VAR(fsp->who);
    			if(next()!=DELIMITER
    					||(token.v!=EOL&&token.v!=EOS)){
    				if(token.k!=IDENTITY||token.v!=STEP)
    					ERROR("Only one statement a line.");
    				fsp->step=expr();
    				if(next()!=DELIMITER
    					||(token.v!=EOL&&token.v!=EOS))
    					ERROR("Only one statement a line.");
    			}
    			fsp++->pos=at;
				break;
				case NEXT:
				--fsp;
				if(next()!=DELIMITER
    				||(token.v!=EOL&&token.v!=EOS))
    				ERROR("Only one statement a line.");
				VAR(fsp->who)+=fsp->step;
				t=fsp->end-VAR(fsp->who);
				if(t*fsp->sop < 0||fsp->sop==0){
					if(fsp<for_stk)
						ERROR("Stack undererflow");
				}
				else
					at=fsp++->pos;
				break;
				case GOTO:
				if(next()==NUMBER)
					at=lfind(token.d);
				else ERROR("Goto need a lable.")
				break;
				case GOSUB:
				if(next()==NUMBER){
					if(++sp>call_stk+STACK_SIZE)
						ERROR("Stack overflow")
					next();
    				if(token.k!=DELIMITER
    					||(token.v!=EOL&&token.v!=EOS))
    					ERROR("Only one statement a line.");
    				*sp=at;
					at=lfind(token.d);
				}
				else ERROR("GOSUB need a lable.")
				break;
				case RETURN:
				if(sp-1<call_stk)
					ERROR("Stack undererflow")
				next();
    			if(token.k!=DELIMITER
    				||(token.v!=EOL&&token.v!=EOS))
    				ERROR("Only one statement a line.");
    			at=*sp--;
				break;
				case END:
				return;
			}
		}
		else
			ERROR("Unknow statement %d.",token.k)
	}
}
int load(const char *name){
	FILE *f;
	int i;
	f=fopen(name,"r");
	if(strcasecmp(name+strlen(name)-4,".bas")!=0)
		ERROR("\"%s\" should end with '.bas'.",name);
	if(!f) ERROR("Unable to open \"%s\".",name);
	for (i = 0; i<BUFF_SIZE-1 && !feof(f); ++i)
		src[i]=fgetc(f);
	src[i-1]=0; /* the last character is EOF */
	return 1;
}

int main(int argc, char const *argv[]){
	if (argc != 2)
		ERROR("Use \"%s <filename>\" "
			"to excute Basic codes.",argv[0]);
	load(argv[1]);
	srand((unsigned int)clock());
	scanflable();
	stmt();
	return 0;
}