#include "AST_node.h"
#include "C_syntax.hpp"

#define INDENTION_LENGHT 4

extern int yyparse();
extern BlockExprNode *root;

// bool error = false;

string modify_funcname(string name) {
    if (name == "strlen") {
        return "len";
    } else if (name == "isdigit") {
        return "str.isdigit";
    }
    return name;
}

GenBlockP::GenBlockP() {
    isFunction = false;
}

GenContextP::GenContextP() {
    funcDeclaring = false;
    indent_num = 0;
    blocks.push(new GenBlockP());
}

GenContextP::~GenContextP() {

}

void GenContextP::CodeGen(BlockExprNode &root) {
    code << "# -*- coding:utf-8" << endl;
    code << "# Authors: Shichen Liu" << endl;
    code << "#          Mengyang Lv" << endl;
    code << "#          Dayang Li" << endl;
    code << "# Date: 2016.12" << endl;
    code << "# Project: Compiler final project" << endl;
    code << "def printf(format, *args):\n    if len(args):\n        new_args = []\n        for i,arg in enumerate(args):\n            if type(arg) == list and len(arg) > 0 and type(arg[0]) == str:\n                new_args.append(''.join(arg))\n            else:\n                new_args.append(arg)\n        print ''.join(format) % tuple(new_args),\n    else:\n        print ''.join(format),\n\n";
    code << "def atoi(s):\n    new_s = []\n    for i in s:\n        if type(i) != str:\n            break\n        else:\n            new_s.append(i)\n    return int(''.join(new_s))\n\n";
    root.CodeGenP(*this);
    code << "\nif __name__ == \"__main__\":\n    main()\n";
}

void GenContextP::OutputCode(ostream& out) {
    string c = code.str();
    out << c;
}

void GenContextP::clearBuf() {
    code_buf.str("");
}

void GenContextP::applyBuf() {
    string s = code_buf.str();
    code << s;
    clearBuf();
}

set<string>& GenContextP::locals() {
    return blocks.top()->locals;
}

set<string>& GenContextP::declared_globals() {
    return blocks.top()->declared_globals;
}

set<string>& GenContextP::globals() {
    return globalVariables;
}

GenBlockP* GenContextP::currentBlock() {
    return blocks.top();
}

void GenContextP::PushBlock(bool copy_locals, bool isFunction) {
    GenBlockP *block = new GenBlockP();
    if (copy_locals) {
        block->locals = blocks.top()->locals;
    }
    if (!isFunction) {
        block->declared_globals = blocks.top()->declared_globals;
    }
    block->isFunction = isFunction;
    blocks.push(block);
    indent_num = indent_num + INDENTION_LENGHT;
}

void GenContextP::popBlock() {
    GenBlockP *top = blocks.top();
    blocks.pop();
    indent_num = indent_num - INDENTION_LENGHT;
    delete top;
}

void GenContextP::indent(bool use_buf) {
    for (int i = 0; i < indent_num; ++i) {
        if (use_buf) {
            code_buf << ' ';
        } else {
            code << ' ';
        }
    }
}

void GenContextP::declare_global(string name) {
    code << "global " << name;
}

void GenContextP::nextLine(bool use_buf) {
    if (use_buf) {
        code_buf << '\n';
    } else {
        code << '\n';
    }
}

void VariableExprNode::CodeGenP(GenContextP &context) {
    if (context.funcDeclaring) {
        if (context.globals().count(name) == 1) {
            fprintf(stderr, "duplicate symbol %s\n", name.c_str());
        } else {
            context.locals().insert(name);
            context.code << name;
        }
    } else {
        if (context.locals().count(name) == 1) {
            context.code_buf << name;
        } else if (context.globals().count(name) == 1) {
            if (context.declared_globals().count(name) == 1) {
                context.code_buf << name;
            } else {
                context.declared_globals().insert(name);
                context.code_buf << name;
            }
        } else {
            fprintf(stderr, "undeclared symbol %s\n", name.c_str());
        }
    }
}

void CharExprNode::CodeGenP(GenContextP &context) {
    if (val == '\'' || val == '\\') {
        context.code_buf << "\'\\" << val << "\'";
    } else {
        context.code_buf << "\'" << val << "\'";
    }
}

void IntExprNode::CodeGenP(GenContextP &context) {
    context.code_buf << val;
}

void DoubleExprNode::CodeGenP(GenContextP &context) {
    context.code_buf << val;
}

void BlockExprNode::CodeGenP(GenContextP &context) {
    for(auto it = statements->begin(); it != statements->end(); it++) {
        context.indent();
        (*it)->CodeGenP(context);
        context.nextLine();
    }
    if (context.currentBlock()->isFunction) {
        for (set<string>::iterator it = context.declared_globals().begin(); it != context.declared_globals().end(); it++) {
            context.indent(false);
            context.declare_global(*it);
            context.nextLine(false);
        }
        context.applyBuf();
    }
}

void OperatorExprNode::CodeGenP(GenContextP &context) {
    string opStr = "";
    switch (op) {
    case EQ:
        opStr = "==";
        break;
    case NE:
        opStr = "!=";
        break;
    case GR:
        opStr = ">";
        break;
    case LW:
        opStr = "<";
        break;
    case GE:
        opStr = ">=";
        break;
    case LE:
        opStr = "<=";
        break;
    case AND:
        opStr = "and";
        break;
    case OR:
        opStr = "or";
        break;
    case ADD:
    case SADD:
        opStr = "+";
        break;
    case SUB:
    case SSUB:
        opStr = "-";
        break;
    case MUL:
    case SMUL:
        opStr = "*";
        break;
    case DIV:
    case SDIV:
        opStr = "/";
        break;
    }
    context.code_buf << "(";
    left->CodeGenP(context);
    context.code_buf << " " << opStr << " ";
    right->CodeGenP(context);
    context.code_buf << ")";
}

void AssignExprNode::CodeGenP(GenContextP &context) {
    left->CodeGenP(context);
    context.code_buf << " = ";
    right->CodeGenP(context);
}

void FuncExprNode::CodeGenP(GenContextP &context) {
    context.code_buf << modify_funcname(functor->name);
    context.code_buf << "(";
    vector<ExprNode*>::iterator it = args->begin();
    if (it != args->end()) {
        (*it)->CodeGenP(context);
        for (it = it + 1; it != args->end(); it++) {
            context.code_buf << ", ";
            (*it)->CodeGenP(context);
        }
    }
    context.code_buf << ")";
}

void CastExprNode::CodeGenP(GenContextP &context) {
    // do no extra moves
    expr->CodeGenP(context);
}

void IndexExprNode::CodeGenP(GenContextP &context) {
    name->CodeGenP(context);
    context.code_buf << "[";
    expr->CodeGenP(context);
    context.code_buf << "]";
    if (assign != NULL) {
        context.code_buf << " = ";
        assign->CodeGenP(context);
    }
}

void ExprStatementNode::CodeGenP(GenContextP &context) {
    expr->CodeGenP(context);
}

void VarDecStatementNode::CodeGenP(GenContextP &context) {
    if (context.locals().count(name->name) == 1) {
        fprintf(stderr, "redefinition %s\n", name->name.c_str());
    } else {
        context.locals().insert(name->name);
    }
    name->CodeGenP(context);
    if (!context.funcDeclaring) {
        context.code_buf << " = ";
        if (expr != NULL) {
            expr->CodeGenP(context);
        } else {
            context.code_buf << "None";
        }
    }
}

void ArrayDecStatementNode::CodeGenP(GenContextP &context) {
    if (context.locals().count(name->name) == 1) {
        fprintf(stderr, "redefinition %s\n", name->name.c_str());
    } else {
        context.locals().insert(name->name);
    }
    name->CodeGenP(context);
    if (init->size() == 0) {
        context.code_buf << " = [None]*" << size;
    } else {
        context.code_buf << " = [";
        vector<ExprNode*>::iterator it = init->begin();
        if (it != init->end()) {
            (*it)->CodeGenP(context);
            for (it = it + 1; it != init->end(); it++) {
                if (isString && (it+1) == init->end()) {
                    break;
                }
                context.code_buf << ", ";
                (*it)->CodeGenP(context);
            }
        }
        context.code_buf << "]";
    }
}

void ReturnStatementNode::CodeGenP(GenContextP &context) {
    context.code_buf << "return ";
    expr->CodeGenP(context);
}

void FuncDecStatementNode::CodeGenP(GenContextP &context) {
    context.funcDeclaring = true;
    context.PushBlock(false, true);
    context.code << "def ";
    name->CodeGenP(context);
    context.code << "(";
    vector<VarDecStatementNode*>::iterator it = args->begin();
    if (it != args->end()) {
        (*it)->CodeGenP(context);
        for (it = it + 1; it != args->end(); it++) {
            context.code << ", ";
            (*it)->CodeGenP(context);
        }
    }
    context.code << "):";
    context.nextLine(false);
    context.funcDeclaring = false;
    block->CodeGenP(context);
    context.popBlock();
}

void ExternFuncDecStatementNode::CodeGenP(GenContextP &context) {
    // don't generate code
}

void IfStatementNode::CodeGenP(GenContextP &context) {
    context.code_buf << "if (";
    condExpr->CodeGenP(context);
    context.code_buf << "):";
    context.nextLine();
    context.PushBlock(true, false);
    trueBlock->CodeGenP(context);
    context.popBlock();
    if (falseBlock->statements->size() == 0) {
        return;
    }
    context.indent();
    context.code_buf << "else:";
    context.nextLine();
    context.PushBlock(true, false);
    falseBlock->CodeGenP(context);
    context.popBlock();
}

void ForStatementNode::CodeGenP(GenContextP &context) {
    initExpr->CodeGenP(context);
    context.nextLine();
    context.indent();
    context.code_buf << "while ";
    condExpr->CodeGenP(context);
    context.code_buf << ":";
    context.nextLine();
    context.PushBlock(true, false);
    block->statements->push_back(new ExprStatementNode(loopExpr));
    block->CodeGenP(context);
    context.popBlock();
}

void WhileStatementNode::CodeGenP(GenContextP &context) {
    context.code_buf << "while";
    whileExpr->CodeGenP(context);
    context.code_buf << ":";
    context.nextLine();
    context.PushBlock(true, false);
    block->CodeGenP(context);
    context.popBlock();
}
