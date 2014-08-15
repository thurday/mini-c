
#ifndef _MINI_COMPILE_H_
#define _MINI_COMPILE_H_

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <list>
#include <stack>
#include <ctype.h>
#include <string>
#include <stdlib.h>

using namespace std;

class Symbol 
{
	public:
		Symbol(){}
		Symbol(const Symbol &b);
        virtual ~Symbol(){}

	public:
		Symbol& operator =(const Symbol &b);
	
	public:
		int line;
		string word;
		char group;
		string code;
};

class Label 
{
	public:
		Label();
		virtual ~Label(){}

	public:
		string text;

	private:
		int n;
        static int _label;

	private:
		static int next();
};

class Action
{
	public:
		static int lookUp(char v,int s);
	private:
		Action(){}
		~Action(){}

	private:
		static int Table[54][19];
		static string vs;
};

class Goto
{
	public:
		static int lookUp(char v,int s);

	private:
		Goto();
		~Goto();

	private:
		static int Table[54][9];
		static string vs;
};


class Compiler 
{
	public:
		Compiler(string& CmdLine);
        virtual ~Compiler(){}
	
	public:
        void preProcess();//预处理器
        Symbol *lexer();//词法分析器
        void parser();//语法分析器
		void optimize();
        void emitter();//生成器

		char nextChar();
		void err(int no,int line);
        void print_log();

	public:
		string code;
		int hasError;//错误发生状态

	private:
		Compiler();
		int lookup(string& m);
	
	private:
		char currentChar;
		string fileName;
		int line;//行数状态
		int hasFile;//源文件打开状态
		ifstream m_in_file;//输入CRR文件
		ostringstream m_log;//输出日志文件
		ofstream m_out_file;//输出ASM文件
		list<string> symbolList;//符号表
};

#endif

