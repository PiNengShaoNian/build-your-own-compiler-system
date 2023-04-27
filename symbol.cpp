#include "symbol.h"
#include "error.h"
#include "token.h"
#include "symtab.h"
#include "compiler.h"
#include "genir.h"

// 打印语义错误
#define SEMERROR(code, name) Error::semError(code, name)

/*******************************************************************************
                                   变量结构
*******************************************************************************/

/*
    获取VOID特殊变量
*/
Var *Var::getVoid()
{
    return SymTab::voidVar;
}

/*
    void变量
*/
Var::Var()
{
    clear();
    setName("<void>"); // 特殊变量名字
    setLeft(false);
    intVal = 0;      // 记录数字数值
    literal = false; // 消除字面量标志
    type = KW_VOID;  // hack类型
    isPtr = true;    // 消除基本类型标志
}

/*
    拷贝出一个临时变量
*/
Var::Var(vector<int> &sp, Var *v)
{
    clear();
    scopePath = sp; // 初始化路径
    setType(v->type);
    setPtr(v->isPtr || v->isArray); // 数组 指针都是指针
    setName("");
    setLeft(false);
}

/*
    关键信息初始化
*/
void Var::clear()
{
    scopePath.push_back(-1); // 默认全局作用域
    externed = false;
    isPtr = false;
    isArray = false;
    isLeft = true; // 默认变量是可以作为左值的
    inited = false;
    literal = false;
    size = 0;
    offset = 0;
    ptr = NULL; // 没有指向当前变量的指针变量
    initData = NULL;
    // TODO
}

/*
    获取true变量
*/
Var *Var::getTrue()
{
    return SymTab::one;
}

/*
    获取步长变量
*/
Var *Var::getStep(Var *v)
{
    if (v->isBase())
        return SymTab::one;
    else if (v->type == KW_CHAR)
        return SymTab::one;
    else if (v->type == KW_INT)
        return SymTab::four;
    else
        return NULL;
}

/*
    临时变量
*/
Var::Var(vector<int> &sp, Tag t, bool ptr)
{
    clear();
    scopePath = sp; // 初始化路径
    setType(t);
    setPtr(ptr);
    setName("");
    setLeft(false);
}
/*
    变量，指针
*/
Var::Var(vector<int> &sp, bool ext, Tag t, bool ptr, string name, Var *init)
{
    clear();
    scopePath = sp; // 初始化路径
    setExtern(ext);
    setType(t);
    setPtr(ptr);
    setName(name);
    initData = init;
}

/*
    数组
*/
Var::Var(vector<int> &sp, bool ext, Tag t, string name, int len)
{
    clear();
    scopePath = sp; // 初始化路径
    setExtern(ext);
    setType(t);
    setName(name);
    setArray(len);
}

/*
    整数变量
*/
Var::Var(int val)
{
    clear();
    setName("<int>"); // 特殊变量名字
    literal = true;
    setLeft(false);
    setType(KW_INT);
    intVal = val; // 记录数字数值
}

/*
    常量,不涉及作用域的变化，字符串存储在字符串表，其他常量作为初始值(使用完删除)
*/
Var::Var(Token *lt)
{
    clear();
    literal = true;
    setLeft(false);
    switch (lt->tag)
    {
    case NUM:
        setType(KW_INT);
        name = "<int>";            // 类型作为名字
        intVal = ((Num *)lt)->val; // 记录数字数值
        break;
    case CH:
        setType(KW_CHAR);
        name = "<char>";            // 类型作为名字
        intVal = 0;                 // 高位置0
        charVal = ((Char *)lt)->ch; // 记录字符值
        break;
    case STR:
        setType(KW_CHAR);
        name = GenIR::genLb(); // 产生一个新的名字

        strVal = ((Str *)lt)->str;   // 记录字符串值
        setArray(strVal.size() + 1); // 字符串作为字符数组存储
        break;
    }
}

/*
    设置extern
*/
void Var::setExtern(bool ext)
{
    externed = ext;
    size = 0;
}

/*
    设置类型
*/
void Var::setType(Tag t)
{
    type = t;
    if (type == KW_VOID) // 无void变量
    {
        SEMERROR(VOID_VAR, ""); // 不允许使用void变量
        type = KW_INT;          // 默认为int
    }
    if (!externed && type == KW_INT)
        size = 4; // 整数4字节
    else if (!externed && type == KW_CHAR)
        size = 1; // 字符1字节
}

/*
    设置指针
*/
void Var::setPtr(bool ptr)
{
    if (!ptr)
        return;
    isPtr = true;
    if (!externed)
        size = 4; // 指针都是4字节
}

/*
    设置名称
*/
void Var::setName(string n)
{
    if (n == "")
        n = GenIR::genLb();
    name = n;
}

/*
    设置数组
*/
void Var::setArray(int len)
{
    if (len <= 0)
    {
        SEMERROR(ARRAY_LEN_INVALID, name); // 数组长度小于等于0错误
        return;
    }
    else
    {
        isArray = true;
        isLeft = false; // 数组不能作为左值
        arraySize = len;
        if (!externed)
            size *= len; // 变量大小乘以数组长度
    }
}

/*
    获取作用域路径
*/
vector<int> &Var::getPath()
{
    return scopePath;
}

/*
    设置指针变量
*/
void Var::setPointer(Var *p)
{
    ptr = p;
}

/*
    获取指针变量
*/
Var *Var::getPointer()
{
    return ptr;
}

/*
    获取指针
*/
bool Var::getPtr()
{
    return isPtr;
}

/*
    获取名字
*/
string Var::getName()
{
    return name;
}

/*
    获取数组
*/
bool Var::getArray()
{
    return isArray;
}

/*******************************************************************************
                                   函数结构
*******************************************************************************/
/*
    构造函数声明，返回值+名称+参数列表
*/
Fun::Fun(bool ext, Tag t, string n, vector<Var *> &paraList)
{
    externed = ext;
    type = t;
    name = n;
    paraVar = paraList;
    // TODO
}
Fun::~Fun()
{
    // TODO
}

/*
    进入一个新的作用域
*/
void Fun::enterScope()
{
    scopeEsp.push_back(0);
}

/*
    离开当前作用域
*/
void Fun::leaveScope()
{
    maxDepth = (curEsp > maxDepth) ? curEsp : maxDepth; // 计算最大深度
    curEsp -= scopeEsp.back();
    scopeEsp.pop_back();
}

/*
    设置extern
*/
void Fun::setExtern(bool ext)
{
    externed = ext;
}

/*
    获取extern
*/
bool Fun::getExtern()
{
    return externed;
}

Tag Fun::getType()
{
    return type;
}

/*
    获取名字
*/
string &Fun::getName()
{
    return name;
}

/*
    设置变量的左值
*/
void Var::setLeft(bool lf)
{
    isLeft = lf;
}

/*
    获取变量的左值
*/
bool Var::getLeft()
{
    return isLeft;
}

/*
    声明定义匹配
*/
#define SEMWARN(code, name) Error::semWarn(code, name)
bool Fun::match(Fun *f)
{
    // 区分函数的返回值
    if (name != f->name)
        return false;

    if (paraVar.size() != f->paraVar.size())
        return false;

    int len = paraVar.size();
    for (int i = 0; i < len; i++)
    {
        if (GenIR::typeCheck(paraVar[i], f->paraVar[i])) // 类型兼容
        {
            if (paraVar[i]->getType() != f->paraVar[i]->getType())
            {
                SEMWARN(FUN_DEC_CONFLICT, name); // 函数声明冲突——警告
            }
        }
        else
            return false;
    }

    // 匹配成功后再验证返回类型
    if (type != f->type)
    {
        SEMWARN(FUN_RET_CONFLICT, name); // 函数返回值冲突——警告
    }
    return true;
}

/*
    行参实参匹配
*/
bool Fun::match(vector<Var *> &args)
{
    if (paraVar.size() != args.size())
        return false;
    int len = paraVar.size();
    for (int i = 0; i < len; i++)
    {
        if (!GenIR::typeCheck(paraVar[i], args[i])) // 类型检查不兼容
            return false;
    }
    return true;
}

/*
    将函数声明转换为定义，需要拷贝参数列表，设定extern
*/
void Fun::define(Fun *def)
{
    externed = false;       // 定义
    paraVar = def->paraVar; // 拷贝参数
}

/*
    添加一条中间代码
*/
void Fun::addInst(InterInst *inst)
{
    interCode.addInst(inst);
}

/*
    设置函数返回点
*/
void Fun::setReturnPoint(InterInst *inst)
{
    returnPoint = inst;
}

/*
    获取函数返回点
*/
InterInst *Fun::getReturnPoint()
{
    return returnPoint;
}

/*
    是数字
*/
bool Var::isVoid()
{
    return type == KW_VOID;
}

/*
    是引用类型
*/
bool Var::isRef()
{
    return !!ptr;
}