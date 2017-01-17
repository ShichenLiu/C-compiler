all: compiler

OBJS =  C_syntax.o  		\
	AST_node.o 				\
	main.o    				\
	C_lexical.o  			\
	AST_node_python.o

LLVMCONFIG = llvm-config
CPPFLAGS = `$(LLVMCONFIG) --cppflags` -std=c++11 -g
LDFLAGS = `$(LLVMCONFIG) --ldflags` -lpthread -ldl -lz -lncurses -rdynamic
LIBS = `$(LLVMCONFIG) --libs`
SYSTEMLIBS = `$(LLVMCONFIG) —-system-libs`

C_syntax.cpp: C_syntax.yacc
	bison -d --no-lines -o $@ $^

C_syntax.hpp: C_syntax.cpp

C_lexical.cpp: C_lexical.lex C_syntax.hpp
	flex -L -o $@ $^

%.o: %.cpp
	g++ -c $(CPPFLAGS) -o $@ $<

compiler: $(OBJS)
	llvm-g++ -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

clean:
	$(RM) -rf C_syntax.cpp C_syntax.hpp compiler y.output c_code/*.py c_code/*.ll C_lexical.cpp $(OBJS)
