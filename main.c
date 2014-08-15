#include "compile.h"
#include <libgen.h>

int main(int argc, char* argv[])
{
	string file(basename(argv[1]));
	if(!file.empty()){
		Compiler cc(file);
		cc.preProcess();
		cc.parser();
		cc.emitter();
		cc.print_log();
	}
	return 0;
}
