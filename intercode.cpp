#include "genir.h"
#include "intercode.h"

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
    产生唯一标号
*/
InterInst::InterInst()
{
    init();
    label = GenIR::genLb();
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
    清除内存
*/
InterCode::~InterCode()
{
    for (int i = 0; i < code.size(); i++)
    {
        delete code[i];
    }
}