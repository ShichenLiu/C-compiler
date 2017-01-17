#ifndef AST_NODE_H
#define AST_NODE_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <stack>
#include <set>
#include <string>
#include <algorithm>
#include <typeinfo>

#include <llvm/IR/Value.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Pass.h>

using namespace llvm;
using namespace std;

enum E_TYPE {
    E_UNKNOWN = -1,
    E_VOID = 0,
    E_CHAR,
    E_INT,
    E_DOUBLE,
    E_PTR,
    E_FUNC,
};

class GenBlock {
public:
    BasicBlock *block;
    map<string, Value*> locals;
};

class GenBlockP {
public:
    GenBlockP();
    set<string> locals;
    set<string> declared_globals;
    bool isFunction;
};

class BlockExprNode;

class GenContext {
public:
    stack<GenBlock *> blocks;
    map<string, Value*> globalVariables;
    Function *mainFunction;
    Function *tempFunction;
    string code;
    bool funcDeclaring;
    Module *module;
public:
    GenContext() {
        funcDeclaring = false;
        module = new Module("main", getGlobalContext());
    }
    void CodeGen(BlockExprNode& root);
    void OutputCode(ostream &out);
    GenericValue run();
    map<string, Value*>& locals()                   { return blocks.top()->locals; }
    map<string, Value*>& globals()                  { return globalVariables; }
    BasicBlock *context()                           { return blocks.top()->block; }
    Function* currentFunction()                     { return tempFunction; }
    void currentFunction(Function *function)        { tempFunction = function; }
    void ret(BasicBlock* block)                     { blocks.top()->block = block; }
    void push(BasicBlock *block, bool copy_locals = true) {
        GenBlock* new_block = new GenBlock();
        new_block->block = block;
        if(copy_locals) {
            map<string, Value*> prev_locals = map<string, Value*>(blocks.top()->locals);
            new_block->locals = prev_locals;
        }
        blocks.push(new_block);
    }
    void pop() {
        GenBlock *top = blocks.top();
        blocks.pop();
        delete top;
    }
};

class GenContextP {
public:
    stack<GenBlockP *> blocks;
    set<string> globalVariables;
    stringstream code;
    stringstream code_buf;
    int indent_num;
    bool funcDeclaring;

public:
    GenContextP();
    ~GenContextP();
    void CodeGen(BlockExprNode &root);
    void OutputCode(ostream &out);
    void clearBuf();
    void applyBuf();
    set<string>& locals();
    set<string>& declared_globals();
    set<string>& globals();
    void declare_global(string name);
    GenBlockP *currentBlock();
    void PushBlock(bool copy_locals = true, bool isFunction = false);
    void popBlock();
    void indent(bool use_buf = true);
    void nextLine(bool use_buf = true);
};

class ASTNode {
public:
    ASTNode() {};
    virtual ~ASTNode(){};
    virtual Value *CodeGen(GenContext&) = 0;
    virtual void CodeGenP(GenContextP&) = 0;
};

class ExprNode: public ASTNode {
public:
    E_TYPE _type;
};

class StatementNode: public ASTNode {};

class IntExprNode: public ExprNode {
public:
    long long val;
public:
    IntExprNode(long long val): val(val) {
        _type = E_INT;
    }
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class CharExprNode: public ExprNode {
public:
    char val;
public:
    CharExprNode(char val): val(val) {
        _type = E_CHAR;
    }
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class DoubleExprNode: public ExprNode {
    double val;
public:
    DoubleExprNode(double val): val(val) {
        _type = E_DOUBLE;
    }
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class VariableExprNode: public ExprNode {
public:
    string name;
public:
    VariableExprNode(const string &name, E_TYPE type = E_UNKNOWN): name(name) {
        _type = type;
    }
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class OperatorExprNode: public ExprNode {
public:
    int op;
    ExprNode *left, *right;
public:
    OperatorExprNode(ExprNode *left, int op, ExprNode *right): left(left), right(right), op(op) {}
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class BlockExprNode: public ExprNode {
public:
    vector<StatementNode*> *statements;
public:
    BlockExprNode(): statements(new vector<StatementNode*>()) {}
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class AssignExprNode: public ExprNode {
public:
    VariableExprNode *left;
    ExprNode *right;
public:
    AssignExprNode(VariableExprNode *left, ExprNode *right): left(left), right(right) {}
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class FuncExprNode: public ExprNode {
public:
    VariableExprNode *functor;
    vector<ExprNode*> *args;
public:
    FuncExprNode(VariableExprNode *functor): functor(functor), args(new vector<ExprNode*>()) {}
    FuncExprNode(VariableExprNode *functor, vector<ExprNode*> *args): functor(functor), args(args) {}
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class CastExprNode: public ExprNode {
public:
    VariableExprNode *type;
    ExprNode *expr;
public:
    CastExprNode(VariableExprNode *type, ExprNode *expr): type(type), expr(expr) {}
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class ExprStatementNode : public StatementNode {
public:
    ExprNode *expr;
public:
    ExprStatementNode(ExprNode *expr): expr(expr) {}
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class ReturnStatementNode : public StatementNode {
public:
    ExprNode *expr;
public:
    ReturnStatementNode(ExprNode *expr): expr(expr) {}
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class VarDecStatementNode : public StatementNode {
public:
    VariableExprNode *type;
    VariableExprNode *name;
    ExprNode *expr;
public:
    VarDecStatementNode(VariableExprNode *type, VariableExprNode *name): type(type), name(name), expr(NULL) {}
    VarDecStatementNode(VariableExprNode *type, VariableExprNode *name, ExprNode *expr): type(type), name(name), expr(expr) {}
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class ArrayDecStatementNode : public StatementNode {
public:
    VariableExprNode *type;
    VariableExprNode *name;
    vector<ExprNode*> *init;
    long long size;
    bool isString;
public:
    ArrayDecStatementNode(VariableExprNode *type, VariableExprNode *name, long long size): type(type), name(name), init(new vector<ExprNode*>()), size(size), isString(false) {}
    ArrayDecStatementNode(VariableExprNode *type, VariableExprNode *name, vector<ExprNode*> *init): type(type), name(name), init(init), size(init->size()), isString(false) {}
    ArrayDecStatementNode(VariableExprNode *type, VariableExprNode *name, const string &str): type(type), name(name), init(new vector<ExprNode*>()), isString(true) {
        for(auto it = str.begin(); it != str.end(); it++)
            init->push_back((ExprNode*)(new CharExprNode(*it)));
        init->push_back((ExprNode*)(new CharExprNode(0)));
        size = init->size() + 1;
    }
   virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class IndexExprNode : public ExprNode {
public:
    VariableExprNode *name;
    ExprNode *expr;
    ExprNode *assign;
public:
    IndexExprNode(VariableExprNode *name, ExprNode *expr): name(name), expr(expr), assign(NULL) {}
    IndexExprNode(VariableExprNode *name, ExprNode *expr, ExprNode *assign): name(name), expr(expr), assign(assign) {}
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class FuncDecStatementNode : public StatementNode {
public:
    VariableExprNode *type;
    VariableExprNode *name;
    vector<VarDecStatementNode*> *args;
    BlockExprNode *block;
public:
    FuncDecStatementNode(VariableExprNode *type, VariableExprNode *name, vector<VarDecStatementNode*> *args, BlockExprNode *block): type(type), name(name), args(args), block(block) {}
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class ExternFuncDecStatementNode : public StatementNode {
public:
    VariableExprNode *type;
    VariableExprNode *name;
    vector<VarDecStatementNode*> *args;
public:
    ExternFuncDecStatementNode(VariableExprNode *type, VariableExprNode *name, vector<VarDecStatementNode*> *_args): type(type), name(name), args(_args) {
        vector<VarDecStatementNode*>::const_iterator it;
    }
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class IfStatementNode : public StatementNode {
public:
    ExprNode *condExpr;
    BlockExprNode *trueBlock;
    BlockExprNode *falseBlock;
public:
    IfStatementNode(ExprNode *condExpr, BlockExprNode *trueBlock): condExpr(condExpr), trueBlock(trueBlock), falseBlock(new BlockExprNode()) {}
    IfStatementNode(ExprNode *condExpr, BlockExprNode *trueBlock, BlockExprNode *falseBlock): condExpr(condExpr), trueBlock(trueBlock), falseBlock(falseBlock) {}
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class ForStatementNode : public StatementNode {
public:
    ExprNode *initExpr;
    ExprNode *condExpr;
    ExprNode *loopExpr;
    BlockExprNode *block;
public:
    ForStatementNode(ExprNode *initExpr, ExprNode *condExpr, ExprNode *loopExpr, BlockExprNode *block): initExpr(initExpr), condExpr(condExpr), loopExpr(loopExpr), block(block) {}
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

class WhileStatementNode : public StatementNode {
public:
    ExprNode *whileExpr;
    BlockExprNode *block;
public:
    WhileStatementNode(ExprNode *whileExpr, BlockExprNode *block): whileExpr(whileExpr), block(block) {}
    virtual Value *CodeGen(GenContext&);
    virtual void CodeGenP(GenContextP&);
};

#endif
