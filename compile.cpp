#include "compile.h"

Compiler::Compiler(string& CmdLine)
{
	line=1;
	hasError=0;
	hasFile=0;
	fileName=CmdLine;
}

Symbol* Compiler::lexer()
{
	char c;
	int s=1;
	Symbol *r;
	r=new Symbol();
	c=currentChar;
	while(s){
		switch(s){
			case 1:
				if(c=='\x0A'){
					line++;
				}else if(isspace(c)){
					s=1;
				}else if(isalpha(c)||c=='_'){
					s=2;
					r->word=c;
				}else if(isdigit(c)){
					s=3;
					r->word=c;
				}else{
					switch(c){
						case '+':
						case '-':
							s=0;
							r->word=c;
							r->group='+';
							r->line=line;
							break;
						case '*':
						case '%':
							s=0;
							r->word=c;
							r->group='*';
							r->line=line;
							break;
						case '&':
						case '|':
							r->word=c;
							s=4;
							break;
						case '>':
						case '<':
							r->word=c;
							s=5;
							break;
						case '!':
							r->word=c;
							s=6;
							break;
						case '=':
							r->word=c;
							s=7;
							break;
						case ',':
						case ';':
						case '{':
						case '}':
						case '(':
						case ')':
						case '$':
							s=0;
							r->word=c;
							r->group=c;
							r->line=line;
							break;
						case '/':
							r->word=c;
							s=8;
							break;
						case '#':
							s=11;
							break;
						default:
							s=1;
							err(1,line);
					}
				}
				c=nextChar();
				break;
			case 2:
				if(isalnum(c)||c=='_'){
					s=2;
					r->word+=c;
					c=nextChar();
				}else{
					s=0;
					r->line=line;
					if(r->word=="int")
						r->group='z';
					else if(r->word=="if")
						r->group='i';
					else if(r->word=="else")
						r->group='e';
					else if(r->word=="do")
						r->group='d';
					else if(r->word=="while")
						r->group='w';
					else if(r->word=="return")
						r->group='r';
					else
						r->group='@';
				}
				break;
			case 3:
				if(isdigit(c)){
					s=3;
					r->word+=c;
					c=nextChar();
				}else if(isalpha(c)||c=='_'){
					s=2;
					r->word+=c;
					err(2,line);
					c=nextChar();
				}else{
					s=0;
					r->line=line;
					r->group='n';
				}
				break;
			case 4:
				if((r->word.c_str())[0]==c){
					s=0;
					r->word+=c;
					r->group='&';
					r->line=line;
					c=nextChar();
				}else{
					s=1;
					err(3,line);
				}
				break;
			case 5: 
				if(c=='='){
					r->word+=c;
					c=nextChar();
				}
				s=0;
				r->line=line;
				r->group='>';
				break;
			case 6:
				if(c=='='){
					r->word+=c;
					r->group='>';
					c=nextChar();
				}else{
					r->group='!';
				}
				s=0;
				r->line=line;
				break;
			case 7:
				if(c=='='){
					r->word+=c;
					r->group='>';
					c=nextChar();
				}else{
					r->group='=';
				}
				s=0;
				r->line=line;
				break;
			case 8:
				if(c=='*'){
					s=9;
					r->word="";
					c=nextChar();
				}else{
					r->line=line;
					r->group='*';
					s=0;
				}
				break;
			case 9:
				if(c=='*'){
					s=10;
				}else{
					s=9;
				}
				c=nextChar();
				break;
			case 10:
				if(c=='/'){
					s=1;
				}else if(c=='*'){
					s=10;
				}else{
					s=9;
				}
				c=nextChar();
				break;
			case 11:
				if(c=='\n'){
					s=1;
					line++;
				}else{
					s=11;
				}
				c=nextChar();
				break;
			default:
				s=1;
				err(0,line);
				c=nextChar();
		}
	}
	currentChar=c;
	m_log<<"词法分析:"<<r->word<<endl;
	return r;
}

/*
1: D->@()S.  程序->主函数
2: S->@=E;. 赋值语句
3: S->{W}.  组合语句
4: S->i(G)S. if语句
5: S->i(G)SeS. if-else语句
6: S->w(G)S. while语句
7: S->zL;.  变量定义
8: S->@(E);. 函数调用
9: W->WS.  多条语句列表
10: W->S.  语句列表
11: L->@.  变量名列表
12: L->L,@.  多个变量名列表
13: G->G&M.  逻辑表达式
14: G->M.
15: M->E>E.  关系表达式
16: M->!M.  逻辑非表达式
17: M->(G).  带括号的逻辑表达式
18: E->E+T.  加法表达式
19: E->T.
20: T->T*F.  乘法表达式
21: T->F.
22: F->(E).  带括号的算术表达式
23: F->@.  变量表达式
24: F->n.   数字表达式
*/

void Compiler::parser()
{
	m_log<<endl;
	m_log<<"*******************************************"<<endl;
	m_log<<"语法分析开始..."<<endl;
	if(hasFile){
		m_in_file.open(fileName.c_str(),ios::in);
	}
	currentChar=nextChar();//词法分析器初始化

	int r,s=1,t=1;
	Symbol *ip,*iq,*it=NULL;
	Symbol *s1,*s2,*s3,*s4,*s5,*s6,*s7,*m;
	Label *l1,*l2;
	stack<int> ss;
	stack<Symbol*> sos;
	ss.push(s);
	ip=lexer();
	while(t){
		s=ss.top();
		t=Action::lookUp(ip->group,s);
		if(t>0){
			sos.push(ip);
			ss.push(t);
			if(it==NULL)
				ip=lexer();
			else
				ip=it;
			it=NULL;
		} else if(t<0) {
			switch(-t) {
				case 1://OK
					//D->@()S
					m_log<<"语法分析:D->@()S"<<endl;
					ss.pop();
					ss.pop();
					ss.pop();
					ss.pop();
					s4=sos.top();
					sos.pop();
					s3=sos.top();
					sos.pop();
					s2=sos.top();
					sos.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='D';
					m->line=s1->line;
					sos.push(m);
					//动作
					if(s1->word!="main"){
						err(5,s1->line);
					}
					code=s4->code;
					delete s1;
					delete s2;
					delete s3;
					delete s4;
					break;
				case 2://OK
					//S->@=E;
					m_log<<"语法分析:S->@=E;"<<endl;
					ss.pop();
					ss.pop();
					ss.pop();
					ss.pop();
					s4=sos.top();
					sos.pop();
					s3=sos.top();
					sos.pop();
					s2=sos.top();
					sos.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='S';
					m->line=s1->line;
					sos.push(m);
					//动作
					if(lookup(s1->word))
						m->code=s3->code+"\tpop ax\n\tmov "+s1->word+",ax\n";
					else
						err(10,s1->line);
					delete s1;
					delete s2;
					delete s3;
					delete s4;
					break;
				case 3://OK
					//S->{W}
					m_log<<"语法分析:S->{W}"<<endl;
					ss.pop();
					ss.pop();
					ss.pop();
					s3=sos.top();
					sos.pop();
					s2=sos.top();
					sos.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='S';
					m->line=s1->line;
					sos.push(m);
					//动作
					m->code=s2->code;
					delete s1;
					delete s2;
					delete s3;
					break;
				case 4://OK
					//S->i(G)S
					ss.pop();
					ss.pop();
					ss.pop();
					ss.pop();
					ss.pop();
					s5=sos.top();
					sos.pop();
					s4=sos.top();
					sos.pop();
					s3=sos.top();
					sos.pop();
					s2=sos.top();
					sos.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='S';
					m->line=s1->line;
					sos.push(m);
					//动作
					if(s1->word=="if"){
						l1=new Label;
						m->code=s3->code+"\tpop ax\n\tcmp ax,1\n\tjne "+l1->text+"\n"+s5->code+l1->text+":\n";
						delete l1;
						m_log<<"语法分析:S->i(G)S"<<endl;
					}
					if(s1->word=="while"){
						l1=new Label;
						l2=new Label;
						m->code=l1->text+":\n"+s3->code+"\tpop ax\n\tcmp ax,1\n\tjne "+l2->text+"\n"+s5->code+"\tjmp "+l1->text+"\n"+l2->text+":\n";
						delete l1;
						delete l2;
						m_log<<"语法分析:S->w(G)S"<<endl;
					}
					delete s1;
					delete s2;
					delete s3;
					delete s4;
					delete s5;
					break;
				case 5://OK
                    //S->i(G)SeS
                    m_log<<"语法分析:S->i(G)SeS"<<endl;
					ss.pop();
					ss.pop();
					ss.pop();
					ss.pop();
					ss.pop();
					ss.pop();
					ss.pop();
					s7=sos.top();
					sos.pop();
					s6=sos.top();
					sos.pop();
					s5=sos.top();
					sos.pop();
					s4=sos.top();
					sos.pop();
					s3=sos.top();
					sos.pop();
					s2=sos.top();
					sos.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='S';
					m->line=s1->line;
					sos.push(m);
					//动作
					l1=new Label;
					l2=new Label;
					if(s1->word=="if"){
						m->code=s3->code+"\tpop ax\n\tcmp ax,1\n\tjnz "+l1->text+"\n"+s5->code+"\tjmp "+l2->text+"\n"+l1->text+":\n"+s7->code+l2->text+":\n";
					}else{
						err(8,s6->line);
                    }
					delete l1;
					delete l2;
					delete s1;
					delete s2;
					delete s3;
					delete s4;
					delete s5;
					delete s6;
					delete s7;
					break;
				case 6://ok
					//S->w(G)S
					m_log<<"语法分析:S->w(G)S"<<endl;
					ss.pop();
					ss.pop();
					ss.pop();
					ss.pop();
					ss.pop();
					s5=sos.top();
					sos.pop();
					s4=sos.top();
					sos.pop();
					s3=sos.top();
					sos.pop();
					s2=sos.top();
					sos.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='S';
					m->line=s1->line;
					sos.push(m);
					//动作
					delete s1;
					delete s2;
					delete s3;
					delete s4;
					delete s5;
					break;
				case 7://OK
					//S->zL;
					m_log<<"语法分析:S->zL;"<<endl;
					ss.pop();
					ss.pop();
					ss.pop();
					s3=sos.top();
					sos.pop();
					s2=sos.top();
					sos.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='S';
					m->line=s1->line;
					sos.push(m);
					//动作
					delete s1;
					delete s2;
					delete s3;
					break;
				case 8://OK
					//S->@(E);
					m_log<<"语法分析:S->@(E);"<<endl;
					ss.pop();
					ss.pop();
					ss.pop();
					ss.pop();
					ss.pop();
					s5=sos.top();
					sos.pop();
					s4=sos.top();
					sos.pop();
					s3=sos.top();
					sos.pop();
					s2=sos.top();
					sos.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='S';
					m->line=s1->line;
					sos.push(m);
					//动作
					delete s1;
					delete s2;
					delete s3;
					delete s4;
					delete s5;
					break;
				case 9://OK
					//W->WS
					m_log<<"语法分析:W->WS"<<endl;
					ss.pop();
					ss.pop();
					s2=sos.top();
					sos.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='W';
					m->line=s1->line;
					sos.push(m);
					//动作
					m->code=s1->code+s2->code;
					delete s1;
					delete s2;
					break;
				case 10://OK
					//W->S
					m_log<<"语法分析:W->S"<<endl;
					ss.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='W';
					m->line=s1->line;
					sos.push(m);
					//动作
					m->code=s1->code;
					delete s1;
					break;
				case 11://OK
					//L->@
					m_log<<"语法分析:L->@"<<endl;
					ss.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='L';
					m->line=s1->line;
					sos.push(m);
					//动作
					if(!lookup(s1->word)){
						symbolList.insert(symbolList.end(),s1->word);
					}else{
						err(11,s1->line);
					}
					delete s1;
					break;
				case 12://OK
					//L->L,@
					m_log<<"语法分析:L->L,@"<<endl;
					ss.pop();
					ss.pop();
					ss.pop();
					s3=sos.top();
					sos.pop();
					s2=sos.top();
					sos.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='L';
					m->line=s1->line;
					sos.push(m);
					if(!lookup(s3->word)){
						symbolList.insert(symbolList.end(),s3->word);
					}else{
						err(11,s3->line);
					}
					delete s1;
					delete s2;
					delete s3;
					break;
				case 13://OK
					//G->G&M
					m_log<<"语法分析:G->G&M"<<endl;
					ss.pop();
					ss.pop();
					ss.pop();
					s3=sos.top();
					sos.pop();
					s2=sos.top();
					sos.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='G';
					m->line=s1->line;
					sos.push(m);
					//动作
					if(s2->word=="&&")
						m->code=s3->code+s1->code+"\tpop ax\n\tpop bx\n\tand ax,bx\n\tpush ax\n";
					if(s2->word=="||")
						m->code=s3->code+s1->code+"\tpop ax\n\tpop bx\n\tor ax,bx\n\tpush ax\n";
					delete s1;
					delete s2;
					delete s3;
					break;
				case 14://OK
					//G->M
					m_log<<"语法分析:G->M"<<endl;
					ss.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='G';
					m->line=s1->line;
					sos.push(m);
					//动作
					m->code=s1->code;
					delete s1;
					break;
				case 15:
					//M->E>E
					m_log<<"语法分析:M->E>E"<<endl;
					ss.pop();
					ss.pop();
					ss.pop();
					s3=sos.top();
					sos.pop();
					s2=sos.top();
					sos.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='M';
					m->line=s1->line;
					sos.push(m);
					//动作
					l1=new Label;
					l2=new Label;
					m->code=s3->code+s1->code+"\tpop ax\n\tpop bx\n\tcmp ax,bx\n";
					if(s2->word==">")
						m->code+="\tjg "+l1->text+"\n";
					if(s2->word=="<")
						m->code+="\tjl "+l1->text+"\n";
					if(s2->word=="==")
						m->code+="\tje "+l1->text+"\n";
					if(s2->word=="!=")
						m->code+="\tjne "+l1->text+"\n";
					if(s2->word==">=")
						m->code+="\tjge "+l1->text+"\n";
					if(s2->word=="<=")
						m->code+="\tjle "+l1->text+"\n";
					m->code+="\tmov ax,0\n\tjmp "+l2->text+"\n"+l1->text+":\tmov ax,1\n"+l2->text+":\tpush ax\n";
					delete l1;
					delete l2;
					delete s1;
					delete s2;
					delete s3;
					break;
				case 16://OK
					//M->!M
					m_log<<"语法分析:M->!M"<<endl;
					ss.pop();
					ss.pop();
					s2=sos.top();
					sos.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='M';
					m->line=s1->line;
					sos.push(m);
					//动作
					m->code="\tpop ax\n\txor ax,ax\n\tpush ax\n";
					delete s1;
					delete s2;
					break;
				case 17://OK
					//M->(G)
					m_log<<"语法分析:M->(G)"<<endl;
					ss.pop();
					ss.pop();
					ss.pop();
					s3=sos.top();
					sos.pop();
					s2=sos.top();
					sos.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='M';
					m->line=s1->line;
					sos.push(m);
					//动作
					m->code=s2->code;
					delete s1;
					delete s2;
					delete s3;
					break;
				case 18://OK
					//E->E+T
					m_log<<"语法分析:E->E+T"<<endl;
					ss.pop();
					ss.pop();
					ss.pop();
					s3=sos.top();
					sos.pop();
					s2=sos.top();
					sos.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->line=s1->line;
					m->group='E';
					sos.push(m);
					//动作
					if(s2->word=="+")
						m->code=s3->code+s1->code+"\tpop ax\n\tpop bx\n\tadd ax,bx\n\tpush ax\n";
					if(s2->word=="-")
						m->code=s3->code+s1->code+"\tpop ax\n\tpop bx\n\tsub ax,bx\n\tpush ax\n";
					delete s1;
					delete s2;
					delete s3;
					break;
				case 19://OK
					//E->T
					m_log<<"语法分析:E->T"<<endl;
					ss.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='E';
					m->line=s1->line;    
					sos.push(m);
					//动作
					m->code=s1->code;
					delete s1;
					break;
				case 20://OK
					//T->T*F
					m_log<<"语法分析:T->T*F"<<endl;
					ss.pop();
					ss.pop();
					ss.pop();
					s3=sos.top();
					sos.pop();
					s2=sos.top();
					sos.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='T';
					m->line=s1->line;
					sos.push(m);
					//动作
					if(s2->word=="*")
						m->code=s3->code+s1->code+"\tpop ax\n\tpop bx\n\tmov dx,0\n\tmul bx\n\tpush ax\n";
					if(s2->word=="/")
						m->code=s3->code+s1->code+"\tpop ax\n\tpop bx\n\tmov dx,0\n\tdiv bx\n\tpush ax\n";
					if(s2->word=="%")
						m->code=s3->code+s1->code+"\tpop ax\n\tpop bx\n\tmov dx,0\n\tdiv bx\n\tpush dx\n";
					delete s1;
					delete s2;
					delete s3;
					break;
				case 21://OK
					//T->F
					m_log<<"语法分析:T->F"<<endl;
					ss.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='T';
					m->line=s1->line;
					sos.push(m);
					//动作
					m->code=s1->code;
					delete s1;
					break;
				case 22://OK
					//F->(E)
					m_log<<"语法分析:F->(E)"<<endl;
					ss.pop();
					ss.pop();
					ss.pop();
					s3=sos.top();
					sos.pop();
					s2=sos.top();
					sos.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='F';
					m->line=s1->line;
					sos.push(m);
					//动作
					m->code=s2->code;
					delete s1;
					delete s2;
					delete s3;
					break;
				case 23://OK
					//F->@
					m_log<<"语法分析:F->@"<<endl;
					ss.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='F';
					m->line=s1->line;
					sos.push(m);
					//动作
					if(lookup(s1->word))
						m->code="\tmov ax,"+s1->word+"\n\tpush ax\n";
					else
						err(10,s1->line);
					delete s1;
					break;
				case 24://OK
					//F->n
					m_log<<"语法分析:F->n"<<endl;
					ss.pop();
					s1=sos.top();
					sos.pop();
					m=new Symbol;
					m->group='F';
					m->line=s1->line;
					sos.push(m);
					//动作
					m->code="\tmov ax,"+s1->word+"\n\tpush ax\n";
					delete s1;
					break;
				case 30:
					//else
					err(8,line);
					ss.push(0);
					ss.push(0);
					ss.push(0);
					ss.push(0);
					ss.push(45);
					s1=new Symbol;
					s1->group='i';
					s1->line=line;
					sos.push(s1);
					s1=new Symbol;
					s1->group='(';
					s1->line=line;
					sos.push(s1);
					s1=new Symbol;
					s1->group='G';
					s1->line=line;
					sos.push(s1);
					s1=new Symbol;
					s1->group=')';
					s1->line=line;
					sos.push(s1);
					s1=new Symbol;
					s1->group='S';
					s1->line=line;
					sos.push(s1);
					continue;
				case 31:
					err(7,line);
					it=ip;
					ip=new Symbol;
					ip->group=';';
					ip->line=line;
					continue;
				case 32:
					err(9,line);
					s1=new Symbol;
					s1->line=line;
					s1->group='S';
					sos.push(s1);
					break;
				default:
					err(4,ip->line);
					goto label1;
			}//switch
			r=ss.top();
			iq=sos.top();
			r=Goto::lookUp(iq->group,r);
			if(r){
				ss.push(r);
			}else{
				err(4,iq->line);
			}
		} else if(t==0&&!hasError){
			m_log<<endl;
			m_log<<"代码通过语法检查,未发现语法错误."<<endl;
		}
	}//while

label1:
	m_in_file.close();
	m_log<<"词法分析完毕..."<<endl;
	m_log<<"*******************************************"<<endl;
}

void Compiler::preProcess()
{
	string tmpstr;
	ifstream ins(fileName.c_str(),ios::in);
	m_log<<"*******************************************"<<endl;
	m_log<<"程序预处理开始"<<endl;
	if(ins.is_open()){
		hasFile=1;
		m_log<<"打开文件 \'"<<fileName<<"\' 成功!"<<endl;
	}else{
		hasFile=0;
		m_log<<"打开文件失改!"<<endl;
	}
	m_log<<"程序预处理完成!"<<endl;
	m_log<<"*******************************************"<<endl;
	ins.close();
}

char Compiler::nextChar()
{
	char ch;
	if(m_in_file.get(ch)){
		currentChar=ch;
		return ch;
	}else{
		currentChar=ch;
		return '$';
	}
}

void Compiler::emitter()
{
	if(hasError){
		m_log<<endl;
		m_log<<"*******************************************"<<endl;
		m_log<<"源代码中有错误,不能生成目标代码!"<<endl;
		m_log<<"*******************************************"<<endl;
		return;
	}
	m_log<<endl;
	m_log<<"*******************************************"<<endl;
	m_log<<"开始生成目标代码..."<<endl;
	m_out_file.open((fileName + ".s").c_str(),ios::out);
	if(m_out_file.is_open()){
		m_log<<"创建文件"<<(fileName + ".s").c_str()<<"成功!"<<endl;

		m_out_file<<"\t.data"<<endl;
		while(!symbolList.empty()){
			m_out_file<<"\t\t" << symbolList.front().c_str()<<"\tdw ?"<<endl;
			symbolList.pop_front();
		}

		m_out_file<<"\t.text"<<endl;
		m_out_file<<".globl main"<<endl;
		m_out_file<<"\t.type   main, @function "<<endl;
		m_out_file<<endl;

		m_out_file<<"main:"<<endl;
		m_out_file<<"\t;***CODE START***"<<endl;

		m_log<<"开始优化代码."<<endl;
		optimize();
		m_log<<"优化代码完毕."<<endl;

		m_out_file<<code;
		m_log<<"写入目标代码."<<endl;

		m_out_file<<"\t;***CODE END***"<<endl;
		m_out_file<<"\tret"<<endl;

		m_out_file.close();
		m_log<<endl;
		m_log<<"代码生成完成!"<<endl;
		m_log<<"*******************************************"<<endl;
	}else{
		m_log<<endl;
		m_log<<"无法创建文件!"<<endl;
		m_log<<"*******************************************"<<endl;
	}
	return;
}

void Compiler::err(int no,int line)
{
	string errText;
	hasError=1;
	switch(no){
		case 1:
			errText="出现无法识别的符号.";
			break;
		case 2:
			errText="错误的标识符.";
			break;
		case 3:
			errText="运算符\'&&\'(\'||\')写成\'&\'(\'|\')了.";
			break;
		case 4:
			errText="语法错误,编译停止!";
			break;
		case 5:
			errText="找不到main()函数.";
			break;
		case 6:
			errText="所使用的函数未定义";
			break;
		case 7:
			errText="语句后缺少\';\'.";
			break;
		case 8:
			errText="else没有与之匹配的if.";
			break;
		case 9:
			errText="缺少语句.";
			break;
		case 10:
			errText="变量未定义.";
			break;
		case 11:
			errText="变量以定义过,不能重新定义.";
			break;
		default:
			errText="未知错误!";
	}
	m_log<<"发生错误:文件"<<fileName<<".crr第"<<line<<"行:"<<errText<<endl;
}


int Compiler::lookup(string& m)
{
	list<string>::iterator i;
	for (i=symbolList.begin(); i != symbolList.end(); ++i){
		if(*i==m){
			return 1;
		}
	}
	return 0;

}

Symbol::Symbol(const Symbol &b)
{
	group=b.group;
	line=b.line;
	word=b.word;
}

Symbol& Symbol::operator =(const Symbol &b)
{
	group=b.group;
	line=b.line;
	word=b.word;
	return *this;
}


Label::Label()
{
	n=next();
	char buffer[6];
	sprintf(buffer, "%d", n); 
	text = buffer;
	text="L"+text;
}

int Label::next()
{
	return ++_label;
}

int Label::_label=0;

string Action::vs ="+*>&=!@n(){};,ziew$";

int Action::lookUp(char v,int s)
{
	int n=vs.find_first_of(v,0);
	return Table[s-1][n];
}

int Action::Table[54][19]={
	-40,-40,-40,-40,-40,-40,3,-40,-40,-40,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state1
	-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-30,-40,0,//state2
	-40,-40,-40,-40,-40,-40,-40,-40,4,-40,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state3
	-40,-40,-40,-40,-40,-40,-40,-40,-40,5,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state4
	-40,-40,-40,-40,-40,-40,7,-40,-40,-40,8,-40,-40,-40,11,9,-30,10,-40,//state5
	-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-30,-40,-1,//state6
	-40,-40,-40,-40,12,-40,-40,-40,13,-40,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state7
	-40,-40,-40,-40,-40,-40,7,-40,-40,-40,8,-32,-32,-40,11,9,-30,10,-32,//state8
	-40,-40,-40,-40,-40,-40,-40,-40,13,-40,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state9
	-40,-40,-40,-40,-40,-40,-40,-40,13,-40,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state10
	-40,-40,-40,-40,-40,-40,17,-40,-40,-40,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state11
	-40,-40,-40,-40,-40,-40,22,23,21,-40,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state12
	-40,-40,-40,-40,-40,27,22,23,28,-40,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state13
	-40,-40,-40,-40,-40,-40,7,-40,-40,-40,8,29,-40,-40,11,9,-30,10,-40,//state14
	-40,-40,-40,-40,-40,-40,-10,-40,-40,-40,-10,-10,-40,-40,-10,-10,-30,-10,-40,//state15
	-40,-40,-40,-40,-40,-40,-31,-40,-40,-40,-31,-31,31,32,-31,-31,-30,-31,-31,//state16
	-40,-40,-40,-40,-40,-40,-31,-40,-40,-40,-31,-31,-11,-11,-31,-31,-30,-31,-31,//state17
	35,-40,-40,-40,-40,-40,-31,-40,-40,34,-31,-31,33,-40,-31,-31,-30,-31,-31,//state18
	-19,36,-19,-19,-40,-40,-31,-40,-40,-19,-31,-31,-19,-40,-31,-31,-30,-31,-31,//state19
	-21,-21,-21,-21,-40,-40,-31,-40,-40,-21,-31,-31,-21,-40,-31,-31,-30,-31,-31,//state20
	-40,-40,-40,-40,-40,-40,22,23,21,-40,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state21
	-23,-23,-23,-23,-40,-40,-31,-40,-40,-23,-31,-31,-23,-40,-31,-31,-30,-31,-31,//state22
	-24,-24,-24,-24,-40,-40,-31,-40,-40,-24,-31,-31,-24,-40,-31,-31,-30,-31,-31,//state23
	-40,-40,-40,38,-40,-40,-40,-40,-40,34,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state24
	35,-40,39,-40,-40,-40,-31,-40,-40,34,-31,-31,33,-40,-31,-31,-30,-31,-31,//state25
	-40,-40,-40,-14,-40,-40,-40,-40,-40,-14,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state26
	-40,-40,-40,-40,-40,27,22,23,28,-40,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state27
	-40,-40,-40,-40,-40,27,22,23,28,-40,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state28
	-40,-40,-40,-40,-40,-40,-3,-40,-40,-40,-3,-3,-40,-40,-3,-3,-3,-3,-3,//state29
	-40,-40,-40,-40,-40,-40,-9,-40,-40,-40,-9,-9,-40,-40,-9,-9,-9,-9,-40,//state30
	-40,-40,-40,-40,-40,-40,-7,-40,-40,-40,-7,-7,-40,-40,-7,-7,-7,-7,-7,//state31
	-40,-40,-40,-40,-40,-40,44,-40,-40,-40,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state32
	-40,-40,-40,-40,-40,-40,-2,-40,-40,-40,-2,-2,-40,-40,-2,-2,-2,-2,-2,//state33
	-40,-40,-40,-40,-40,-40,7,-40,-40,-40,8,-32,46,-40,11,9,-30,10,-32,//state34
	-40,-40,-40,-40,-40,-40,22,23,21,-40,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state35
	-40,-40,-40,-40,-40,-40,22,23,21,-40,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state36
	35,-40,-40,-40,-40,-40,-40,-40,-40,49,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state37
	-40,-40,-40,-40,-40,27,22,23,28,-40,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state38
	-40,-40,-40,-40,-40,-40,22,23,21,-40,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state39
	-40,-40,-40,-16,-40,-40,-40,-40,-40,-16,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state40
	35,-40,39,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state41
	-40,-40,-40,38,-40,-40,-40,-40,-40,52,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state42
	35,-40,39,-40,-40,-40,-31,-40,-40,49,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state43
	-40,-40,-40,-40,-40,-40,-31,-40,-40,-40,-31,-31,-12,-12,-31,-31,-30,-31,-31,//state44
	-40,-40,-40,-40,-40,-40,-4,-40,-40,-40,-4,-4,-40,-40,-4,-4,53,-4,-4,//state45
	-40,-40,-40,-40,-40,-40,-8,-40,-40,-40,-8,-8,-40,-40,-8,-8,-30,-8,-8,//state46
	-18,36,-18,-18,-40,-40,-31,-40,-40,-18,-31,-31,-18,-40,-31,-31,-30,-31,-31,//state47
	-20,-20,-20,-20,-40,-40,-31,-40,-40,-20,-31,-31,-20,-40,-31,-31,-30,-31,-31,//state48
	-22,-22,-22,-22,-40,-40,-31,-40,-40,-22,-31,-31,-22,-40,-31,-31,-30,-31,-31,//state49
	-40,-40,-40,-13,-40,-40,-40,-40,-40,-13,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state50
	35,-40,-40,-15,-40,-40,-40,-40,-40,-15,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state51
	-40,-40,-40,-17,-40,-40,-40,-40,-40,-17,-40,-40,-40,-40,-40,-40,-30,-40,-40,//state52
	-40,-40,-40,-40,-40,-40,7,-40,-40,-40,8,-32,-32,-40,11,9,-30,10,-32,//state53
	-40,-40,-40,-40,-40,-40,-5,-40,-40,-40,-5,-5,-40,-40,-5,-5,-30,-5,-5};//state54


Goto::Goto()
{

}

Goto::~Goto()
{

}

int Goto::lookUp(char v,int s)
{
	int n=vs.find_first_of(v,0);
	return Table[s-1][n];
}

string Goto::vs="DSWLGMETF";

int Goto::Table[54][9]={
	2,0,0,0,0,0,0,0,0,//state1
	0,0,0,0,0,0,0,0,0,//state2
	0,0,0,0,0,0,0,0,0,//state3
	0,0,0,0,0,0,0,0,0,//state4
	0,6,0,0,0,0,0,0,0,//state5
	0,0,0,0,0,0,0,0,0,//state6
	0,0,0,0,0,0,0,0,0,//state7
	0,15,14,0,0,0,0,0,0,//state8
	0,0,0,0,0,0,0,0,0,//state9
	0,0,0,0,0,0,0,0,0,//state10
	0,0,0,16,0,0,0,0,0,//state11
	0,0,0,0,0,0,18,19,20,//state12
	0,0,0,0,24,26,25,19,20,//state13
	0,30,0,0,0,0,0,0,0,//state14
	0,0,0,0,0,0,0,0,0,//state15
	0,0,0,0,0,0,0,0,0,//state16
	0,0,0,0,0,0,0,0,0,//state17
	0,0,0,0,0,0,0,0,0,//state18
	0,0,0,0,0,0,0,0,0,//state19
	0,0,0,0,0,0,0,0,0,//state20
	0,0,0,0,0,0,37,19,20,//state21
	0,0,0,0,0,0,0,0,0,//state22
	0,0,0,0,0,0,0,0,0,//state23
	0,0,0,0,0,0,0,0,0,//state24
	0,0,0,0,0,0,0,0,0,//state25
	0,0,0,0,0,0,0,0,0,//state26
	0,0,0,0,0,40,41,19,20,//state27
	0,0,0,0,42,26,43,19,20,//state28
	0,0,0,0,0,0,0,0,0,//state29
	0,0,0,0,0,0,0,0,0,//state30
	0,0,0,0,0,0,0,0,0,//state31
	0,0,0,0,0,0,0,0,0,//state32
	0,0,0,0,0,0,0,0,0,//state33
	0,45,0,0,0,0,0,0,0,//state34
	0,0,0,0,0,0,0,47,20,//state35
	0,0,0,0,0,0,0,0,48,//state36
	0,0,0,0,0,0,0,0,0,//state37
	0,0,0,0,0,50,41,19,20,//state38
	0,0,0,0,0,0,51,19,20,//state39
	0,0,0,0,0,0,0,0,0,//state40
	0,0,0,0,0,0,0,0,0,//state41
	0,0,0,0,0,0,0,0,0,//state42
	0,0,0,0,0,0,0,0,0,//state43
	0,0,0,0,0,0,0,0,0,//state44
	0,0,0,0,0,0,0,0,0,//state45
	0,0,0,0,0,0,0,0,0,//state46
	0,0,0,0,0,0,0,0,0,//state47
	0,0,0,0,0,0,0,0,0,//state48
	0,0,0,0,0,0,0,0,0,//state49
	0,0,0,0,0,0,0,0,0,//state50
	0,0,0,0,0,0,0,0,0,//state51
	0,0,0,0,0,0,0,0,0,//state52
	0,54,0,0,0,0,0,0,0,//state53
	0,0,0,0,0,0,0,0,0//state54
};

void Compiler::optimize()
{
	int i,j,f;
	string t="\tpush ax\n\tpop ax\n";
	do{
		f=0;
		j=code.length()-17;
		for(i=0;i<j;i++){
			if(code.substr(i,17)==t){
				f=1;
				code.replace(i,17,"");
				break;
			}
		}
	}while(f);
}

void Compiler::print_log()
{
	std::cout << m_log.str();
}

