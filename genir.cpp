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
    双目运算语句
*/
Var *GenIR::genTwoOp(Var *lval, Tag opt, Var *rval)
{
    if (!lval || !rval)
        return NULL;
    if (lval->isVoid() || rval->isVoid())
    {
        SEMERROR(EXPR_IS_VOID); // void函数返回值不能出现在表达式中
        return NULL;
    }
    // 赋值单独处理
    if (opt == ASSIGN)
        return genAssign(lval, rval); // 赋值

    // 先处理(*p)变量
    if (lval->isRef())
        lval = genAssign(lval);
    if (rval->isRef())
        rval = genAssign(rval);
    if (opt == OR)
        return genOr(lval, rval); // 或
    if (opt == AND)
        return genAnd(lval, rval); // 与
    if (opt == EQU)
        return genEqu(lval, rval); // 等于
    if (opt == NEQU)
        return genNequ(lval, rval); // 不等于
    if (opt == ADD)
        return genAdd(lval, rval); // 加
    if (opt == SUB)
        return genSub(lval, rval); // 减
    if (!lval->isBase() || !rval->isBase())
    {
        SEMERROR(EXPR_NOT_BASE); // 不是基本类型
        return lval;
    }
    if (opt == GT)
        return genGt(lval, rval); // 大于
    if (opt == GE)
        return genGe(lval, rval); // 大于等于
    if (opt == LT)
        return genLt(lval, rval); // 小于
    if (opt == LE)
        return genLe(lval, rval); // 小于等于
    if (opt == MUL)
        return genMul(lval, rval); // 乘
    if (opt == DIV)
        return genDiv(lval, rval); // 除
    if (opt == MOD)
        return genMod(lval, rval); // 取模
    return lval;
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
    指针取值语句
*/
Var *GenIR::genPtr(Var *val)
{
    if (val->isBase())
    {
        SEMERROR(EXPR_IS_BASE); // 基本类型不能取值
        return val;
    }
    Var *tmp = new Var(symtab.getScopePath(), val->getType(), false);
    tmp->setLeft(true);   // 指针运算结果为左值
    tmp->setPointer(val); // 设置指针变量
    symtab.addVar(tmp);   // 产生表达式需要根据使用者判断，推迟！

    return tmp;
}

/*
    赋值语句
*/
Var *GenIR::genAssign(Var *lval, Var *rval)
{
    // 被赋值对象必须是左值
    if (!lval->getLeft())
    {
        SEMERROR(EXPR_NOT_LEFT_VAL); // 左值错误
        return rval;
    }
    // 类型检查
    if (!typeCheck(lval, rval))
    {
        SEMERROR(ASSIGN_TYPE_ERR); // 赋值类型不匹配
        return rval;
    }
    // 考虑右值(*p)
    if (rval->isRef())
    {
        if (!lval->isRef())
        {
            // 中间代码lval=*(rval->ptr)
            symtab.addInst(new InterInst(OP_GET, lval, rval->getPointer()));
            return lval;
        }
        else
        {
            // 中间代码*(lval->ptr)=*(rval->ptr),先处理右值
            rval = genAssign(rval);
        }
    }
    // 赋值运算
    if (lval->isRef())
    {
        // 中间代码*(lval->ptr)=rval
        symtab.addInst(new InterInst(OP_SET, rval, lval->getPointer()));
    }
    else
    {
        // 中间代码lval=rval
        symtab.addInst(new InterInst(OP_AS, lval, rval));
    }
    return lval;
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
    或运算语句
*/
Var *GenIR::genOr(Var *lval, Var *rval)
{
    Var *tmp = new Var(symtab.getScopePath(), KW_INT, false); // 基本类型
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(OP_OR, tmp, lval, rval)); // 中间代码tmp=lval||rval
    return tmp;
}

/*
    与运算语句
*/
Var *GenIR::genAnd(Var *lval, Var *rval)
{
    Var *tmp = new Var(symtab.getScopePath(), KW_INT, false); // 基本类型
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(OP_AND, tmp, lval, rval)); // 中间代码tmp=lval&&rval
    return tmp;
}

/*
    大于语句
*/
Var *GenIR::genGt(Var *lval, Var *rval)
{
    Var *tmp = new Var(symtab.getScopePath(), KW_INT, false); // 基本类型
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(OP_GT, tmp, lval, rval)); // 中间代码tmp=lval>rval
    return tmp;
}

/*
    大于等于语句
*/
Var *GenIR::genGe(Var *lval, Var *rval)
{
    Var *tmp = new Var(symtab.getScopePath(), KW_INT, false); // 基本类型
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(OP_GE, tmp, lval, rval)); // 中间代码tmp=lval>=rval
    return tmp;
}

/*
    小于语句
*/
Var *GenIR::genLt(Var *lval, Var *rval)
{
    Var *tmp = new Var(symtab.getScopePath(), KW_INT, false); // 基本类型
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(OP_LT, tmp, lval, rval)); // 中间代码tmp=lval<rval
    return tmp;
}

/*
    小于等于语句
*/
Var *GenIR::genLe(Var *lval, Var *rval)
{
    Var *tmp = new Var(symtab.getScopePath(), KW_INT, false); // 基本类型
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(OP_LE, tmp, lval, rval)); // 中间代码tmp=lval<=rval
    return tmp;
}

/*
    等于语句
*/
Var *GenIR::genEqu(Var *lval, Var *rval)
{
    Var *tmp = new Var(symtab.getScopePath(), KW_INT, false); // 基本类型
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(OP_EQU, tmp, lval, rval)); // 中间代码tmp=lval==rval
    return tmp;
}

/*
    不等于语句
*/
Var *GenIR::genNequ(Var *lval, Var *rval)
{
    Var *tmp = new Var(symtab.getScopePath(), KW_INT, false); // 基本类型
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(OP_NE, tmp, lval, rval)); // 中间代码tmp=lval!=rval
    return tmp;
}

/*
    加法语句
*/
Var *GenIR::genAdd(Var *lval, Var *rval)
{
    Var *tmp = NULL;
    // 指针和数组只能和基本类型相加
    if ((lval->getArray() || lval->getPtr()) && rval->isBase())
    {
        tmp = new Var(symtab.getScopePath(), lval);
        rval = genMul(rval, Var::getStep(lval));
    }
    else if (lval->isBase() && (rval->getArray() || rval->getPtr()))
    {
        tmp = new Var(symtab.getScopePath(), rval);
        lval = genMul(lval, Var::getStep(rval));
    }
    else if (lval->isBase() && rval->isBase()) // 基本类型
    {
        tmp = new Var(symtab.getScopePath(), KW_INT, false); // 基本类型
    }
    else
    {
        SEMERROR(EXPR_NOT_BASE); // 加法类型不兼容
        return lval;
    }
    // 加法命令
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(OP_ADD, tmp, lval, rval)); // 中间代码tmp=lval+rval
    return tmp;
}

/*
    减法语句
*/
Var *GenIR::genSub(Var *lval, Var *rval)
{
    Var *tmp = NULL;
    if (!rval->isBase())
    {
        SEMERROR(EXPR_NOT_BASE); // 类型不兼容,减数不是基本类型
        return lval;
    }

    // 指针和数组
    if ((lval->getArray() || lval->getPtr()))
    {
        tmp = new Var(symtab.getScopePath(), lval);
        rval = genMul(rval, Var::getStep(lval));
    }
    else // 基本类型
    {
        tmp = new Var(symtab.getScopePath(), KW_INT, false); // 基本类型
    }
    // 减法命令
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(OP_SUB, tmp, lval, rval)); // 中间代码tmp=lval-rval
    return tmp;
}

/*
    乘法语句
*/
Var *GenIR::genMul(Var *lval, Var *rval)
{
    Var *tmp = new Var(symtab.getScopePath(), KW_INT, false); // 基本类型
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(OP_MUL, tmp, lval, rval)); // 中间代码tmp=lval*rval
    return tmp;
}

/*
    除法语句
*/
Var *GenIR::genDiv(Var *lval, Var *rval)
{
    Var *tmp = new Var(symtab.getScopePath(), KW_INT, false); // 基本类型
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(OP_DIV, tmp, lval, rval)); // 中间代码tmp=lval/rval
    return tmp;
}

/*
    模语句
*/
Var *GenIR::genMod(Var *lval, Var *rval)
{
    Var *tmp = new Var(symtab.getScopePath(), KW_INT, false); // 基本类型
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(OP_MOD, tmp, lval, rval)); // 中间代码tmp=lval%rval
    return tmp;
}

/*
    左单目运算语句
*/
Var *GenIR::genOneOpLeft(Tag opt, Var *val)
{
    if (!val)
        return NULL;
    if (val->isVoid())
    {
        SEMERROR(EXPR_IS_VOID); // void函数返回值不能出现在表达式中
        return NULL;
    }
    //&x *p 运算单独处理

    if (opt == LEA)
        return genLea(val); // 取址语句
    if (opt == MUL)
        return genPtr(val); // 指针取值语句

    //++ --
    if (opt == INC)
        return genIncL(val); // 左自加语句
    if (opt == DEC)
        return genDecL(val); // 左自减语句

    // not minus ++ --
    if (val->isRef())
        val = genAssign(val); // 处理(*p)
    if (opt == NOT)
        return genNot(val); // not语句
    if (opt == SUB)
        return genMinus(val); // 取负语句
    return val;
}

/*
    取反
*/
Var *GenIR::genNot(Var *val)
{
    Var *tmp = new Var(symtab.getScopePath(), KW_INT, false); // 生成整数
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(OP_NOT, tmp, val)); // 中间代码tmp=-val
    return tmp;
}

/*
    取负
*/
Var *GenIR::genMinus(Var *val)
{
    if (!val->isBase())
    {
        SEMERROR(EXPR_NOT_BASE); // 运算对象不是基本类型
        return val;
    }
    Var *tmp = new Var(symtab.getScopePath(), KW_INT, false); // 生成整数
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(OP_NEG, tmp, val)); // 中间代码tmp=-val
    return tmp;
}

/*
    左自加
*/
Var *GenIR::genIncL(Var *val)
{
    if (!val->getLeft())
    {
        SEMERROR(EXPR_NOT_LEFT_VAL);
        return val;
    }
    if (val->isRef()) //++*p情况 => t1=*p t2=t1+1 *p=t2
    {
        Var *t1 = genAssign(val);                // t1=*p
        Var *t2 = genAdd(t1, Var::getStep(val)); // t2 = t1 + 1
        return genAssign(val, t2);               // *p = t2
    }
    symtab.addInst(new InterInst(OP_ADD, val, val, Var::getStep(val)));
    return val;
}

/*
    左自减
*/
Var *GenIR::genDecL(Var *val)
{
    if (!val->getLeft())
    {
        SEMERROR(EXPR_NOT_LEFT_VAL);
        return val;
    }
    if (val->isRef()) // --*p情况 => t1=*p t2=t1-1 *p=t2
    {
        Var *t1 = genAssign(val);                // t1=*p
        Var *t2 = genSub(t1, Var::getStep(val)); // t2=t1-1
        return genAssign(val, t2);               // *p=t2
    }
    symtab.addInst(new InterInst(OP_SUB, val, val, Var::getStep(val))); // 中间代码--val
    return val;
}

/*
    取址语句
*/
Var *GenIR::genLea(Var *val)
{
    if (!val->getLeft())
    {
        SEMERROR(EXPR_NOT_LEFT_VAL); // 不能取地址
        return val;
    }
    if (val->isRef())             // 类似&*p运算
        return val->getPointer(); // 取出变量的指针,&*(val->ptr)等价于ptr
    else                          // 一般取地址运算
    {
        Var *tmp = new Var(symtab.getScopePath(), val->getType(), true); // 产生局部变量tmp
        symtab.addVar(tmp);                                              // 插入声明
        symtab.addInst(new InterInst(OP_LEA, tmp, val));                 // 中间代码tmp=&val
        return tmp;
    }
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