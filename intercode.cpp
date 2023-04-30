#include "genir.h"
#include "intercode.h"
#include "symbol.h"

/*******************************************************************************
                                   四元式
*******************************************************************************/
/*
    初始化
*/
void InterInst::init()
{
    op = OP_NOP;
    this->result = NULL;
    this->target = NULL;
    this->arg1 = NULL;
    this->fun = NULL;
    this->arg2 = NULL;
    // TODO
}

/*
    一般运算指令
*/
InterInst::InterInst(Operator op, Var *rs, Var *arg1, Var *arg2)
{
    init();
    this->op = op;
    this->result = rs;
    this->arg1 = arg1;
    this->arg2 = arg2;
}

/*
    函数调用指令
*/
InterInst::InterInst(Operator op, Fun *fun, Var *rs)
{
    init();
    this->op = op;
    this->result = rs;
    this->fun = fun;
    this->arg2 = NULL;
}

/*
    参数进栈指令
*/
InterInst::InterInst(Operator op, Var *arg1)
{
    init();
    this->op = op;
    this->result = NULL;
    this->arg1 = arg1;
    this->arg2 = NULL;
}

/*
    产生唯一标号
*/
InterInst::InterInst()
{
    init();
    label = GenIR::genLb();
}

/*
    条件跳转指令
*/
InterInst::InterInst(Operator op, InterInst *tar, Var *arg1, Var *arg2)
{
    init();
    this->op = op;
    this->target = tar;
    this->arg1 = arg1;
    this->arg2 = arg2;
}

/*
    获取第一个参数
*/
Var *InterInst::getArg1()
{
    return arg1;
}

/*
    获取第二个参数
*/
Var *InterInst::getArg2()
{
    return arg2;
}

/*
    获取标签
*/
string InterInst::getLabel()
{
    return label;
}

/*
    获取函数对象
*/
Fun *InterInst::getFun()
{
    return fun;
}

/*
    获取操作符
*/
Operator InterInst::getOp()
{
    return op;
}

/*
    获取跳转指令的目标指令
*/
InterInst *InterInst::getTarget()
{
    return target;
}

/*
    获取返回值
*/
Var *InterInst::getResult()
{
    return result;
}

/*
    输出指令信息
*/
void InterInst::toString()
{
    if (label != "")
    {
        printf("%s:\n", label.c_str());
        return;
    }
    switch (op)
    {
    // case OP_NOP:printf("nop");break;
    case OP_DEC:
        printf("dec ");
        arg1->value();
        break;
    case OP_ENTRY:
        printf("entry");
        break;
    case OP_EXIT:
        printf("exit");
        break;
    case OP_AS:
        result->value();
        printf(" = ");
        arg1->value();
        break;
    case OP_ADD:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" + ");
        arg2->value();
        break;
    case OP_SUB:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" - ");
        arg2->value();
        break;
    case OP_MUL:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" * ");
        arg2->value();
        break;
    case OP_DIV:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" / ");
        arg2->value();
        break;
    case OP_MOD:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" %% ");
        arg2->value();
        break;
    case OP_NEG:
        result->value();
        printf(" = ");
        printf("-");
        arg1->value();
        break;
    case OP_GT:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" > ");
        arg2->value();
        break;
    case OP_GE:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" >= ");
        arg2->value();
        break;
    case OP_LT:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" < ");
        arg2->value();
        break;
    case OP_LE:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" <= ");
        arg2->value();
        break;
    case OP_EQU:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" == ");
        arg2->value();
        break;
    case OP_NE:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" != ");
        arg2->value();
        break;
    case OP_NOT:
        result->value();
        printf(" = ");
        printf("!");
        arg1->value();
        break;
    case OP_AND:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" && ");
        arg2->value();
        break;
    case OP_OR:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" || ");
        arg2->value();
        break;
    case OP_JMP:
        printf("goto %s", target->label.c_str());
        break;
    case OP_JT:
        printf("if( ");
        arg1->value();
        printf(" )goto %s", target->label.c_str());
        break;
    case OP_JF:
        printf("if( !");
        arg1->value();
        printf(" )goto %s", target->label.c_str());
        break;
    // case OP_JG:printf("if( ");arg1->value();printf(" > ");arg2->value();printf(" )goto %s",
    // 	target->label.c_str());break;
    // case OP_JGE:printf("if( ");arg1->value();printf(" >= ");arg2->value();printf(" )goto %s",
    // 	target->label.c_str());break;
    // case OP_JL:printf("if( ");arg1->value();printf(" < ");arg2->value();printf(" )goto %s",
    // 	target->label.c_str());break;
    // case OP_JLE:printf("if( ");arg1->value();printf(" <= ");arg2->value();printf(" )goto %s",
    // 	target->label.c_str());break;
    // case OP_JE:printf("if( ");arg1->value();printf(" == ");arg2->value();printf(" )goto %s",
    // 	target->label.c_str());break;
    case OP_JNE:
        printf("if( ");
        arg1->value();
        printf(" != ");
        arg2->value();
        printf(" )goto %s",
               target->label.c_str());
        break;
    case OP_ARG:
        printf("arg ");
        arg1->value();
        break;
    case OP_PROC:
        printf("%s()", fun->getName().c_str());
        break;
    case OP_CALL:
        result->value();
        printf(" = %s()", fun->getName().c_str());
        break;
    case OP_RET:
        printf("return goto %s", target->label.c_str());
        break;
    case OP_RETV:
        printf("return ");
        arg1->value();
        printf(" goto %s", target->label.c_str());
        break;
    case OP_LEA:
        result->value();
        printf(" = ");
        printf("&");
        arg1->value();
        break;
    case OP_SET:
        printf("*");
        arg1->value();
        printf(" = ");
        result->value();
        break;
    case OP_GET:
        result->value();
        printf(" = ");
        printf("*");
        arg1->value();
        break;
    }
    printf("\n");
}

/*******************************************************************************
                                   中间代码
*******************************************************************************/

/*
    添加中间代码
*/
void InterCode::addInst(InterInst *inst)
{
    code.push_back(inst);
}

/*
    输出指令信息
*/
void InterCode::toString()
{
    for (int i = 0; i < code.size(); i++)
    {
        code[i]->toString();
    }
}

/*
    清除内存
*/
InterCode::~InterCode()
{
    for (int i = 0; i < code.size(); i++)
    {
        delete code[i];
    }
}
/*
    获取中间代码序列
*/
vector<InterInst *> &InterCode::getCode()
{
    return code;
}
