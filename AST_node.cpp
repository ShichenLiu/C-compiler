#include "AST_node.h"
#include "C_syntax.hpp"

extern int yyparse();
extern BlockExprNode* root;
bool error = false;

static Value* cast(Value* value, Type* type, GenContext& context);

Function* printfFunction(GenContext& context) {
    vector<Type*> printfArgs;
    printfArgs.push_back(Type::getInt8PtrTy(getGlobalContext()));
    FunctionType* printfType = FunctionType::get(Type::getInt32Ty(getGlobalContext()), printfArgs, true);
    Function *printfFunc = Function::Create(printfType, Function::ExternalLinkage, Twine("printf"), context.module);
    printfFunc->setCallingConv(CallingConv::C);
    return printfFunc;
}

Function* strlenFunction(GenContext& context) {
    vector<Type*> strlenArgs;
    strlenArgs.push_back(Type::getInt8PtrTy(getGlobalContext()));
    FunctionType* strlenType = FunctionType::get(Type::getInt64Ty(getGlobalContext()), strlenArgs, false);
    Function *strlenFunc = Function::Create(strlenType, Function::ExternalLinkage, Twine("strlen"), context.module);
    strlenFunc->setCallingConv(CallingConv::C);
    return strlenFunc;
}

Function* isdigitFunction(GenContext& context) {
    vector<Type*> isdigitArgs;
    isdigitArgs.push_back(Type::getInt8Ty(getGlobalContext()));
    FunctionType* isdigitType = FunctionType::get(Type::getInt1Ty(getGlobalContext()), isdigitArgs, false);
    Function *isdigitFunc = Function::Create(isdigitType, Function::ExternalLinkage, Twine("isdigit"), context.module);
    isdigitFunc->setCallingConv(CallingConv::C);
    return isdigitFunc;
}

Function* atoiFunction(GenContext& context) {
    vector<Type*> atoiArgs;
    atoiArgs.push_back(Type::getInt8PtrTy(getGlobalContext()));
    FunctionType* atoiType =FunctionType::get(Type::getInt64Ty(getGlobalContext()), atoiArgs, false);
    Function *atoiFunc = Function::Create(atoiType, Function::ExternalLinkage, Twine("atoi"), context.module);
    atoiFunc->setCallingConv(CallingConv::C);
    return atoiFunc;
}

void linkExternalFunctions(GenContext& context) {
    printfFunction(context);
    strlenFunction(context);
    isdigitFunction(context);
    atoiFunction(context);
}

void GenContext::CodeGen(BlockExprNode& root) {
    vector<Type*> arg_types;
    FunctionType *ftype = FunctionType::get(Type::getVoidTy(getGlobalContext()), makeArrayRef(arg_types), false);
    SmallVector<AttributeSet, 4> attrs;
    AttrBuilder builder;
    builder.addAttribute(Attribute::NoUnwind);
    attrs.push_back(AttributeSet::get(getGlobalContext(), ~0U, builder));
    AttributeSet funcFunc = AttributeSet::get(getGlobalContext(), attrs);
    root.CodeGen(*this);
    PassManager<Module> pm;
    raw_string_ostream *out = new raw_string_ostream(code);
    pm.addPass(PrintModulePass(*out));
    pm.run(*module);
}

void GenContext::OutputCode(ostream &out) {
    out << code;
}

GenericValue GenContext::run() {
    ExecutionEngine *ee = EngineBuilder(unique_ptr<Module>(module)).create();
    vector<GenericValue> noargs;
    GenericValue v;
    ee->finalizeObject();
    mainFunction = module->getFunction("main");
    v = ee->runFunction(mainFunction, noargs);
    return v;
}

static Type *typeOf(VariableExprNode *var) {
    Type *type;
    if (var->name == "int")
        type = Type::getInt64Ty(getGlobalContext());
    else if (var->name == "char")
        type = Type::getInt8Ty(getGlobalContext());
    else if (var->name == "double")
        type = Type::getDoubleTy(getGlobalContext());
    else if (var->name == "void")
        type = Type::getVoidTy(getGlobalContext());
    return type;
}

Value* VariableExprNode::CodeGen(GenContext& context) {
    Value* val;
    if (!context.funcDeclaring || context.locals().find(name) == context.locals().end()) {
        if (context.globals().find(name) == context.globals().end()) {
            cerr << "Undeclared Variable " << name << endl;
            return NULL;
        } else
            val = context.globals()[name];
    } else
        val = context.locals()[name];
    if (((AllocaInst*)val)->getAllocatedType()->isArrayTy()) {
        ConstantInt* constInt = ConstantInt::get(getGlobalContext(), APInt(32, StringRef("0"), 10));
        vector<Value*> args;
        args.push_back(constInt);
        args.push_back(constInt);
        Type* type;
        type = ((AllocaInst*)val)->getAllocatedType();
        val = GetElementPtrInst::Create(type, val, args, "", context.context());
        return val;
    } else
        return new LoadInst(val, "", false, context.context());
}

Value* CharExprNode::CodeGen(GenContext& context) {
    return ConstantInt::get(Type::getInt8Ty(getGlobalContext()), val, true);
}

Value* IntExprNode::CodeGen(GenContext& context) {
    return ConstantInt::get(Type::getInt64Ty(getGlobalContext()), val, true);
}

Value* DoubleExprNode::CodeGen(GenContext& context) {
    return ConstantFP::get(Type::getDoubleTy(getGlobalContext()), val);
}

Value* BlockExprNode::CodeGen(GenContext& context) {
    Value *returnValue = NULL;
    for (auto it = statements->begin(); it != statements->end(); it++)
        returnValue = (*it)->CodeGen(context);
    return returnValue;
}

Value* OperatorExprNode::CodeGen(GenContext& context) {
    Instruction::BinaryOps instr;
    ICmpInst::Predicate pred;
    bool floatOp = false;
    bool mathOP = false;
    Value* leftVal = left->CodeGen(context);
    Value* rightVal = right->CodeGen(context);
    if (leftVal->getType()->isDoubleTy() || rightVal->getType()->isDoubleTy()) {
        leftVal = cast(leftVal, Type::getDoubleTy(getGlobalContext()), context);
        rightVal = cast(rightVal, Type::getDoubleTy(getGlobalContext()), context);
        floatOp = true;
    } else if (leftVal->getType() == rightVal->getType()) {
    } else {
        leftVal = cast(leftVal, Type::getInt64Ty(getGlobalContext()), context);
        rightVal = cast(rightVal, Type::getInt64Ty(getGlobalContext()), context);
    }
    if (!floatOp) {
        switch (op) {
            case EQ:
                pred = ICmpInst::ICMP_EQ;
                break;
            case NE:
                pred = ICmpInst::ICMP_NE;
                break;
            case GR:
                pred = ICmpInst::ICMP_SGT;
                break;
            case LW:
                pred = ICmpInst::ICMP_SLT;
                break;
            case GE:
                pred = ICmpInst::ICMP_SGE;
                break;
            case LE:
                pred = ICmpInst::ICMP_SLE;
                break;
            case ADD:
            case SADD:
                instr = Instruction::Add;
                mathOP=true;
                break;
            case SUB:
            case SSUB:
                instr = Instruction::Sub;
                mathOP=true;
                break;
            case MUL:
            case SMUL:
                instr = Instruction::Mul;
                mathOP=true;
                break;
            case DIV:
            case SDIV:
                instr = Instruction::SDiv;
                mathOP=true;
                break;
            case OR:
                instr = Instruction::Or;
                mathOP=true;
                break;
            case AND:
                instr = Instruction::And;
                mathOP=true;
                break;
        }
    } else {
        switch (op) {
            case EQ:
                pred = ICmpInst::FCMP_OEQ;
                break;
            case NE:
                pred = ICmpInst::FCMP_ONE;
                break;
            case GR:
                pred = ICmpInst::FCMP_OGT;
                break;
            case LW:
                pred = ICmpInst::FCMP_OLT;
                break;
            case GE:
                pred = ICmpInst::FCMP_OGE;
                break;
            case LE:
                pred = ICmpInst::FCMP_OLE;
                break;
            case ADD:
            case SADD:
                instr = Instruction::FAdd;
                mathOP=true;
                break;
            case SUB:
            case SSUB:
                instr = Instruction::FSub;
                mathOP=true;
                break;
            case MUL:
            case SMUL:
                instr = Instruction::FMul;
                mathOP=true;
                break;
            case DIV:
            case SDIV:
                instr = Instruction::FDiv;
                mathOP=true;
                break;
        }
    }
    if (mathOP)
        return BinaryOperator::Create(instr, leftVal, rightVal, "", context.context());
    else
        return new ICmpInst(*context.context(), pred, leftVal, rightVal, "");
}

Value* AssignExprNode::CodeGen(GenContext& context) {
    Value* rightVal;
    Value* leftVal;
    if (!context.funcDeclaring || context.locals().find(left->name) == context.locals().end()) {
        if (context.globals().find(left->name) == context.globals().end())
            return NULL;
        else
            leftVal = context.globals()[left->name];
    }
    else
        leftVal = context.locals()[left->name];
    rightVal = right->CodeGen(context);
    if (leftVal->getType() == Type::getInt64PtrTy(getGlobalContext()))
        rightVal = cast(rightVal, Type::getInt64Ty(getGlobalContext()), context);
    else if (leftVal->getType() == Type::getDoublePtrTy(getGlobalContext()))
        rightVal = cast(rightVal, Type::getDoubleTy(getGlobalContext()), context);
    else if (leftVal->getType() == Type::getInt8PtrTy(getGlobalContext()))
        rightVal = cast(rightVal, Type::getInt8Ty(getGlobalContext()), context);
    return new StoreInst(rightVal, leftVal, false, context.context());
}

Value* FuncExprNode::CodeGen(GenContext& context) {
    // Get functor
    Function *function = context.module->getFunction(functor->name.c_str());
    if (function == NULL)
        cerr << "No such function " << functor->name << endl;
    vector<Value*> argsRef;
    for (auto it = args->begin(); it != args->end(); it++)
        argsRef.push_back((*it)->CodeGen(context));
    CallInst *call = CallInst::Create(function, makeArrayRef(argsRef), "", context.context());
    return call;
}

Value* CastExprNode::CodeGen(GenContext& context) {
    Value* value = expr->CodeGen(context);
    Type* castType = typeOf(type);
    value = cast(value, castType, context);
    return value;
}

Value* IndexExprNode::CodeGen(GenContext& context) {
    Value* array = name->CodeGen(context);
    Value* num = cast(expr->CodeGen(context), Type::getInt64Ty(getGlobalContext()), context);
    num = new TruncInst(num, Type::getInt32Ty(getGlobalContext()), "", context.context());
    Type* arrayType = cast<PointerType>(array->getType()->getScalarType())->getElementType();
    Instruction* instr;
    Value* retInst;
    instr = GetElementPtrInst::Create(arrayType, array, num, "", context.context());
    // whether read or write
    if (assign == NULL)
        retInst = new LoadInst(instr, "", false, context.context());
    else
        retInst = new StoreInst(assign->CodeGen(context), instr, false, context.context());
    return retInst;
}

Value* ExprStatementNode::CodeGen(GenContext& context) {
    return expr->CodeGen(context);
}

Value* VarDecStatementNode::CodeGen(GenContext& context) {
    Value* newVar;
    newVar = new AllocaInst(typeOf(type), name->name.c_str(), context.context());
    context.locals()[name->name] = newVar;
    if (expr != NULL) {
        AssignExprNode assign(name, expr);
        assign.CodeGen(context);
    }
    return newVar;
}

Value* ArrayDecStatementNode::CodeGen(GenContext& context) {
    ArrayType* arrayType = ArrayType::get(typeOf(type), size);
    AllocaInst *alloc = new AllocaInst(arrayType, name->name.c_str(), context.context());
    context.locals()[name->name] = alloc;
    if (init->size() != 0) {
        for (auto it = init->begin(); it != init->end(); ++it) {
            ExprNode* num = new IntExprNode(it - init->begin());
            IndexExprNode a(name, num, (*it));
            a.CodeGen(context);
            delete num;
        }
    }
    return alloc;
}

Value* ReturnStatementNode::CodeGen(GenContext& context) {
    return expr->CodeGen(context);
}

Value* FuncDecStatementNode::CodeGen(GenContext& context) {
    // Function type
    vector<Type*> argTypeRef;
    for (auto it = args->begin(); it != args->end(); it++)
        argTypeRef.push_back(typeOf((*it)->type));
    FunctionType *funcType = FunctionType::get(typeOf(type), ArrayRef<Type*>(argTypeRef), false);
    Function *function = Function::Create(funcType, GlobalValue::ExternalLinkage, name->name.c_str(), context.module);
    function->setCallingConv(CallingConv::C);
    SmallVector<AttributeSet, 4> Attrs;
    AttrBuilder Builder;
    Builder.addAttribute(Attribute::NoUnwind);
    Attrs.push_back(AttributeSet::get(getGlobalContext(), ~0U, Builder));
    AttributeSet funcFuncAttrSet = AttributeSet::get(getGlobalContext(), Attrs);
    function->setAttributes(funcFuncAttrSet);
    context.currentFunction(function);
    context.funcDeclaring = true;
    // Start function
    BasicBlock *funcBlock = BasicBlock::Create(getGlobalContext(), "", function, 0);
    context.push(funcBlock, false);
    Function::arg_iterator argsValues = function->arg_begin();
    for (auto it = args->begin(); it != args->end(); it++, argsValues++) {
        (*it)->CodeGen(context);
        Value *argumentValue = &(*argsValues);
        argumentValue->setName((*it)->name->name.c_str());
        StoreInst *inst = new StoreInst(argumentValue, context.locals()[(*it)->name->name], false, funcBlock);
    }
    // Get return value
    Value* returnValue = block->CodeGen(context);
    context.funcDeclaring = false;
    // Return
    BasicBlock *returnBlock = BasicBlock::Create(getGlobalContext(), "", function, 0);
    BranchInst::Create(returnBlock, context.context());
    ReturnInst::Create(getGlobalContext(), returnValue, returnBlock);
    context.pop();
    return function;
}

Value* ExternFuncDecStatementNode::CodeGen(GenContext& context) {
    vector<Type*> argTypes;
    FunctionType *ftype;
    Function *function;
    for (auto it = args->begin(); it != args->end(); it++)
        argTypes.push_back(typeOf((*it)->type));
    ftype = FunctionType::get(Type::getVoidTy(getGlobalContext()), makeArrayRef(argTypes), false);
    function = Function::Create(ftype, GlobalValue::ExternalLinkage, name->name.c_str(), context.module);
    return function;
}

Value* IfStatementNode::CodeGen(GenContext& context) {
    BasicBlock* ifTrue = BasicBlock::Create(getGlobalContext(), "", context.currentFunction(), 0);
    BasicBlock* ifFalse = BasicBlock::Create(getGlobalContext(), "", context.currentFunction(), 0);
    BasicBlock* ifEnd = BasicBlock::Create(getGlobalContext(), "", context.currentFunction(), 0);
    BranchInst::Create(ifTrue, ifFalse, condExpr->CodeGen(context), context.context());
    // Entering IF
    context.push(ifTrue);
    trueBlock->CodeGen(context);
    // JMP to END
    BranchInst::Create(ifEnd, context.context());
    context.pop();
    // Entering ELSE
    context.push(ifFalse);
    falseBlock->CodeGen(context);
    // JMP to END
    BranchInst::Create(ifEnd, context.context());
    context.pop();
    // Return END
    context.ret(ifEnd);
    return ifEnd;
}

Value* ForStatementNode::CodeGen(GenContext& context) {
    // Initialize
    initExpr->CodeGen(context);
    BasicBlock* forIter = BasicBlock::Create(getGlobalContext(), "", context.currentFunction(), 0);
    BasicBlock* forEnd = BasicBlock::Create(getGlobalContext(), "", context.currentFunction(), 0);
    BasicBlock* forCheck = BasicBlock::Create(getGlobalContext(), "", context.currentFunction(), 0);
    // Check condition satisfaction
    BranchInst::Create(forCheck, context.context());
    context.push(forCheck);
    // Whether break the loop
    BranchInst::Create(forIter, forEnd, condExpr->CodeGen(context), forCheck);
    context.pop();
    // Entering loop block
    context.push(forIter);
    block->CodeGen(context);
    // Iteration
    loopExpr->CodeGen(context);
    // Jump back to condition checking
    BranchInst::Create(forCheck, context.context());
    context.pop();
    // Return END
    context.ret(forEnd);
    return forEnd;
}

Value* WhileStatementNode::CodeGen(GenContext& context) {
    BasicBlock* whileIter = BasicBlock::Create(getGlobalContext(), "", context.currentFunction(), 0);
    BasicBlock* whileEnd = BasicBlock::Create(getGlobalContext(), "", context.currentFunction(), 0);
    BasicBlock* whileCheck = BasicBlock::Create(getGlobalContext(), "", context.currentFunction(), 0);
    // Check condition satisfaction
    BranchInst::Create(whileCheck, context.context());
    context.push(whileCheck);
    // Whether break the loop
    BranchInst::Create(whileIter, whileEnd, whileExpr->CodeGen(context), context.context());
    context.pop();
    // Entering loop block
    context.push(whileIter);
    block->CodeGen(context);
    // Jump back to condition checking
    BranchInst::Create(whileCheck, context.context());
    context.pop();
    // Return END
    context.ret(whileEnd);
    return whileEnd;
}

static Value* cast(Value* value, Type* type, GenContext& context) {
    if (type == value->getType())
        return value;
    if (type == Type::getDoubleTy(getGlobalContext())) {
        if (value->getType() == Type::getInt64Ty(getGlobalContext()) || value->getType() == Type::getInt8Ty(getGlobalContext()))
            value = new SIToFPInst(value, type, "", context.context());
        else
            cout << "Cannot cast this value.\n";
    }
    else if (type == Type::getInt64Ty(getGlobalContext())) {
        if (value->getType() == Type::getDoubleTy(getGlobalContext()))
            value = new FPToSIInst(value, type, "", context.context());
        else if (value->getType() == Type::getInt8Ty(getGlobalContext()))
            value = new SExtInst(value, type, "", context.context());
        else if (value->getType() == Type::getInt32Ty(getGlobalContext()))
            value = new ZExtInst(value, type, "", context.context());
        else if (value->getType() == Type::getInt8PtrTy(getGlobalContext()))
            value = new PtrToIntInst(value, type, "", context.context());
        else if (value->getType() == Type::getInt64PtrTy(getGlobalContext()))
            value = new PtrToIntInst(value, type, "", context.context());
        else
            cout << "Cannot cast this value.\n";
    } else if (type == Type::getInt8Ty(getGlobalContext())) {
        if (value->getType() == Type::getDoubleTy(getGlobalContext()))
            value = new FPToSIInst(value, type, "", context.context());
        else if (value->getType() == Type::getInt64Ty(getGlobalContext()))
            value = new TruncInst(value, type, "", context.context());
        else
            cout << "Cannot cast this value.\n";
    } else
        cout << "Cannot cast this value.\n";
    return value;
}
