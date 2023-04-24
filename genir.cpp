#include "genir.h"
#include <sstream>
#include "symbol.h"
#include "symtab.h"
#include "error.h"

int GenIR::lbNum = 0;

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