#pragma once
#include "common.h"

/*
    中间代码生成器
*/
class GenIR
{
    static int lbNum; // 标签号码，用于产生唯一的标签

    SymTab &symtab; // 符号表

public:
    GenIR(SymTab &tab); // 重置内部数据

    Var *genAssign(Var *val); // 变量拷贝赋值，用于指针左值引用和变量复制

    // 全局函数
    static string genLb();                       // 产生唯一的标签
    static bool typeCheck(Var *lval, Var *rval); // 检查类型是否可以转换

    // 产生特殊语句
    void genReturn(Var *ret);       // 产生return语句
    void genFunHead(Fun *function); // 产生函数入口语句
    void genFunTail(Fun *function); // 产生函数出口语句
};