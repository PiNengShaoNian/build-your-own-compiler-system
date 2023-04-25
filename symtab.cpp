#include "symtab.h"
#include "error.h"
#include "symbol.h"
#include "compiler.h"
#include "genir.h"

// 打印语义错误
#define SEMERROR(code, name) Error::semError(code, name);

/*******************************************************************************
                                   符号表
*******************************************************************************/

Var *SymTab::voidVar = NULL; // 特殊变量的初始化
Var *SymTab::zero = NULL;    // 特殊变量的初始化
Var *SymTab::one = NULL;     // 特殊变量的初始化
Var *SymTab::four = NULL;    // 特殊变量的初始化

/*
    初始化符号表
*/
SymTab::SymTab()
{
    /*
    此处产生特殊的常量void，1,4
    */
    voidVar = new Var(); // void变量
    zero = new Var(1);   // 常量0
    one = new Var(1);    // 常量1
    four = new Var(4);   // 常量4

    addVar(voidVar); // 让符号表管理这些特殊变量
    addVar(one);     // 让符号表管理这些特殊变量
    addVar(zero);    // 让符号表管理这些特殊变量
    addVar(four);    // 让符号表管理这些特殊变量

    scopeId = 0;
    curFun = NULL;
    scopePath.push_back(0); // 全局作用域
}

/*
    清除信息
*/
SymTab::~SymTab()
{
    // 清除函数,保证变量信息在指令清除时可用
    hash_map<string, Fun *, string_hash>::iterator funIt, funEnd = funTab.end();
    for (funIt = funTab.begin(); funIt != funEnd; ++funIt)
    {
        delete funIt->second;
    }
    // 清除变量
    hash_map<string, vector<Var *> *, string_hash>::iterator varIt, varEnd = varTab.end();
    for (varIt = varTab.begin(); varIt != varEnd; ++varIt)
    {
        vector<Var *> &list = *varIt->second;
        for (int i = 0; i < list.size(); i++)
            delete list[i];

        delete &list;
    }
    // 清除串
    hash_map<string, Var *, string_hash>::iterator strIt, strEnd = strTab.end();
    for (strIt = strTab.begin(); strIt != strEnd; ++strIt)
        delete strIt->second;
}

/*
    设置中间代码生成器
*/
void SymTab::setIr(GenIR *ir)
{
    this->ir = ir;
}

/*
    获取scopePath
*/
vector<int> &SymTab::getScopePath()
{
    return scopePath;
}

/*
    添加一个变量到符号表
*/
void SymTab::addVar(Var *var)
{
    if (varTab.find(var->getName()) == varTab.end()) // 没有该名字的变量
    {
        varTab[var->getName()] = new vector<Var *>; // 创建链表
        varTab[var->getName()]->push_back(var);     // 添加变量
        varList.push_back(var->getName());
    }
    else
    {
        // 判断同名变量是否都不在一个作用域
        vector<Var *> &list = *varTab[var->getName()];
        int i;
        for (i = 0; i < list.size(); i++)
        {
            if (list[i]->getPath().back() == var->getPath().back()) // 在一个作用域，冲突！
                break;
        }
        if (i == list.size() || var->getName()[0] == '<') // 没有冲突
            list.push_back(var);
        else
        {
            // 同一作用域存在同名的变量的定义，extern是声明外部文件的变量，相当于定义了该全局变量
            SEMERROR(VAR_RE_DEF, var->getName());
            delete var;
            return; // 无效变量，删除，不定位
        }
    }
    // TODO
}

/*
    添加一个字符串常量
*/
void SymTab::addStr(Var *v)
{
    strTab[v->getName()] = v;
}

/*
    获取一个变量
*/
Var *SymTab::getVar(string name)
{
    Var *select = NULL; // 最佳选择
    if (varTab.find(name) != varTab.end())
    {
        vector<Var *> &list = *varTab[name];
        int pathLen = scopePath.size(); // 当前路径长度
        int maxLen = 0;                 // 已经匹配的最大长度
        for (int i = 0; i < list.size(); i++)
        {
            int len = list[i]->getPath().size();
            // 发现候选的变量,路径是自己的前缀
            if (len <= pathLen && list[i]->getPath()[len - 1] == scopePath[len - 1])
            {
                if (len > maxLen) // 选取最长匹配
                {
                    maxLen = len;
                    select = list[i];
                }
            }
        }
    }
    if (!select)
        SEMERROR(VAR_UN_DEC, name); // 变量未声明
    return select;
}

/*
    根据实际参数，获取一个函数
*/
Fun *SymTab::getFun(string name, vector<Var *> &args)
{
    if (funTab.find(name) != funTab.end())
    {
        Fun *last = funTab[name];
        if (!last->match(args))
        {
            SEMERROR(FUN_CALL_ERR, name); // 行参实参不匹配
            return NULL;
        }
        return last;
    }
    SEMERROR(FUN_UN_DEC, name); // 函数未声明
    return NULL;
}

/*
    声明一个函数
*/
void SymTab::decFun(Fun *fun)
{
    fun->setExtern(true);

    if (funTab.find(fun->getName()) == funTab.end()) // 没有该名字的函数
    {
        funTab[fun->getName()] = fun; // 添加函数
        funList.push_back(fun->getName());
    }
    else
    {
        // 判断是否是重复函数声明
        Fun *last = funTab[fun->getName()];
        if (!last->match(fun))
            SEMERROR(FUN_DEC_ERR, fun->getName()); // 函数声明与定义不匹配

        delete fun;
    }
}

/*
    定义一个函数
*/
void SymTab::defFun(Fun *fun)
{
    if (fun->getExtern()) // extern不允许出现在定义
    {
        SEMERROR(EXTERN_FUN_DEF, fun->getName());
        fun->setExtern(false);
    }
    if (funTab.find(fun->getName()) == funTab.end()) // 没有该名字的函数
    {
        funTab[fun->getName()] = fun; // 添加函数
        funList.push_back(fun->getName());
    }
    else // 已经声明
    {
        Fun *last = funTab[fun->getName()];
        if (last->getExtern())
        {
            // 之前是声明
            if (!last->match(fun)) // 匹配的声明
            {
                SEMERROR(FUN_DEC_ERR, fun->getName()); // 函数声明与定义不匹配
            }
            last->define(fun); // 将函数参数拷贝，设定externed为false
        }
        else // 重定义
        {
            SEMERROR(FUN_RE_DEF, fun->getName());
        }
        delete fun; // 删除当前函数对象
        fun = last; // 公用函数结构体
    }
    curFun = fun;           // 当前分析的函数
    ir->genFunHead(curFun); // 产生函数入口
}

/*
    结束定义一个函数
*/
void SymTab::endDefFun()
{
    ir->genFunTail(curFun); // 产生函数出口
    curFun = NULL;          // 当前分析的函数置空
}

/*
    添加一条中间代码
*/
void SymTab::addInst(InterInst *inst)
{
    if (curFun)
        curFun->addInst(inst);
    else
        delete inst;
}

/*
    进入局部作用域
*/
void SymTab::enter()
{
    scopeId++;
    scopePath.push_back(scopeId);
    if (curFun)
        curFun->enterScope();
}

/*
    离开局部作用域
*/
void SymTab::leave()
{
    scopePath.pop_back(); // 撤销更改
    if (curFun)
        curFun->leaveScope();
}

/*
    是基本类型
*/
bool Var::isBase()
{
    return !isArray && !isPtr;
}

/*
    获取类型
*/
Tag Var::getType()
{
    return type;
}
