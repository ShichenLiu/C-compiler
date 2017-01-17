#include <iostream>
#include <fstream>
#include <unistd.h>
#include "AST_node.h"

using namespace std;
using namespace llvm;

extern FILE* yyin;
extern BlockExprNode* root;
extern int yyparse();
extern void linkExternalFunctions(GenContext &context);

void usage() {
	cout << "\nusage: ./compiler [-fhpv] <filename.c>\n" << endl;
	cout << "optional arguments:" << endl;
	cout << " -h\thelp" << endl;
	cout << " -v\tshow output in terminal" << endl;
	cout << " -f\tshow output in *.ll or *.py" << endl;
	cout << " -p\toutput python version" << endl;
}

int main(int argc, char **argv) {
	char *filename;
	bool f = false, p = false, v = false;
	// check args
	if (argc == 2)
		filename = argv[1];
	else if (argc >= 3) {
		filename = argv[argc-1];
		int ch;
		while((ch = getopt(argc, argv, "fhpv")) != -1) {
			switch(ch) {
			case 'h':
				usage();
				return 0;
			case 'f':
				f = true;
				break;
			case 'p':
				p = true;
				break;
			case 'v':
				v = true;
				break;
			default:
				usage();
				return 0;
			}
		}
	} else {
		usage();
		return 0;
	}
	// check filename
	int len = strlen(filename);
	if (filename[len - 1] != 'c' || filename[len - 2] != '.') {
		usage();
		return 0;
	}
	yyin = fopen(filename, "r");
	if (!yyin) {
        perror("File opening failed");
        return EXIT_FAILURE;
    }
	if (yyparse()){
		cout << "ERROR!" << endl;
		return EXIT_FAILURE;
	}
	// implement options
	if (p) {
		GenContextP context;
		cout << "Generating Python code" << endl;
		cout << "----------------------" << endl;
		context.CodeGen(*root);
		cout << "----------------------" << endl;
		cout << "Finished" << endl;
		if (f) {
			filename[len-1] = 'p';
			filename[len] = 'y';
			filename[len+1] = '\0';
			ofstream outfile;
			outfile.open(filename, ios::out);
			context.OutputCode(outfile);
			outfile.close();
		}
		if (v) {
			cout << "Python code:" << endl;
			cout << "------------" << endl;
			context.OutputCode(cout);
			cout << "------------" << endl;
			cout << "Python code ends" << endl;
		}
		cout << "+=====================================+" << endl;
		cout << "| To Run Python code, please using:   |" << endl;
		cout << "|     python c_code/<filename.py>     |" << endl;
		cout << "+=====================================+" << endl;
		cout << "finished" << endl;
	} else {
		GenContext context;
		InitializeNativeTarget();
		InitializeNativeTargetAsmPrinter();
		InitializeNativeTargetAsmParser();
		linkExternalFunctions(context);
		cout << "Generating LLVM code" << endl;
		cout << "--------------------" << endl;
		context.CodeGen(*root);
		cout << endl;
		cout << "--------------------" << endl;
		cout << "Finished" << endl;
		if (f) {
			filename[len-1] = 'l';
			filename[len] = 'l';
			filename[len+1] = '\0';
			ofstream outfile;
			outfile.open(filename, ios::out);
			context.OutputCode(outfile);
			outfile.close();
		}
		if (v) {
			cout << "LLVM code:" << endl;
			cout << "----------" << endl;
			context.OutputCode(cout);
			cout << "----------" << endl;
			cout << "LLVM code ends" << endl;
		}
		cout << "Run LLVM code" << endl;
		cout << "-------------" << endl;
		context.run();
		cout << endl;
		cout << "-------------" << endl;
		cout << "Rnd LLVM code ends" << endl;
		cout << "Finished" << endl;
	}
	fclose(yyin);
	return 0;
}
