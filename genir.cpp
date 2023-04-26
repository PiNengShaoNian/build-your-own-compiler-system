#include "genir.h"
#include <sstream>
#include "symbol.h"
#include "symtab.h"
#include "intercode.h"
#include "error.h"

// 打印语义错误
#define SEMERROR(code) Error::semError(code)

int GenIR::lbNum = 0;

/*
    初始化
*/
GenIR::GenIR(SymTab &tab) : symtab(tab)
{
    symtab.setIr(this); // 构建符号表与代码生成器的一一关系
    lbNum = 0;
    // TODO
}

/*
    检查类型是否可以转换
*/
bool GenIR::typeCheck(Var *lval, Var *rval)
{
    bool flag = false;
    if (!rval)
        return false;
    if (lval->isBase() && rval->isBase()) // 都是基本类型
        return true;
    else if (!lval->isBase() && !rval->isBase())   // 都不是基本类型
        flag = rval->getType() == lval->getType(); // 只要求类型相同

    return flag;
}

/*
    获取唯一名字的标签
*/
string GenIR::genLb()
{
    lbNum++;
    string lb = ".L"; // 为了和汇编保持一致！
    stringstream ss;
    ss << lbNum;
    return lb + ss.str();
}

/*
    拷贝赋值语句,处理取*p值的情况
*/
Var *GenIR::genAssign(Var *val)
{
    Var *tmp = new Var(symtab.getScopePath(), val); // 拷贝变量信息
    symtab.addVar(tmp);
    if (val->isRef()) // 中间代码tmp=*(val->ptr)
    {
        // 中间代码tmp=*(val->ptr)
        symtab.addInst(new InterInst(OP_GET, tmp, val->getPointer()));
    }
    else
    {
        symtab.addInst(new InterInst(OP_AS, tmp, val)); // 中间代码tmp=val
    }
    return tmp;
}

/*
    产生return语句
*/
void GenIR::genReturn(Var *ret)
{
    if (!ret)
        return;
    Fun *fun = symtab.getCurFun();
    if (ret->isVoid() && fun->getType() != KW_VOID || ret->isBase() && fun->getType() == KW_VOID) // 类型不兼容
    {
        SEMERROR(RETURN_ERR); // return语句和函数返回值类型不匹配
        return;
    }
    InterInst *returnPoint = fun->getReturnPoint(); // 获取返回点
    if (ret->isVoid())
        symtab.addInst(new InterInst(OP_RET, returnPoint)); // return returnPoint
    else
    {
        if (ret->isRef()) // 处理ret是*p情况
            ret = genAssign(ret);
        symtab.addInst(new InterInst(OP_RETV, returnPoint, ret)); // return returnPoint ret
    }
}

/*
    产生函数入口语句
*/
void GenIR::genFunHead(Fun *function)
{
    function->enterScope();                            // 进入函数作用域
    symtab.addInst(new InterInst(OP_ENTRY, function)); // 添加函数入口指令
    function->setReturnPoint(new InterInst);           // 创建函数的返回点
}

/*
    产生函数出口语句
*/
void GenIR::genFunTail(Fun *function)
{
    symtab.addInst(function->getReturnPoint());       // 添加函数返回点，return的目的标号
    symtab.addInst(new InterInst(OP_EXIT, function)); // 添加函数出口指令
    function->leaveScope();                           // 退出函数作用域
}