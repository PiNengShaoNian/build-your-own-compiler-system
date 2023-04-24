#pragma once
#include "common.h"

/*
    中间代码生成器
*/
class GenIR
{
    static int lbNum; // 标签号码，用于产生唯一的标签

public:
    GenIR(SymTab &tab); // 重置内部数据

    // 全局函数
    static string genLb();                       // 产生唯一的标签
    static bool typeCheck(Var *lval, Var *rval); // 检查类型是否可以转换
};