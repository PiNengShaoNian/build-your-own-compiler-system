#pragma once
#include "common.h"

/*
    四元式类，定义了中间代码的指令的形式
*/
class InterInst
{
private:
    string label; // 标签
    Operator op;  // 操作符
    // union{
    Var *result;       // 运算结果
    InterInst *target; // 跳转标号
    //};
    // union{
    Var *arg1; // 参数1
    Fun *fun;  // 函数
    //};
    Var *arg2; // 参数2

    void init(); // 初始化

public:
    // 构造
    InterInst(Operator op, Var *rs, Var *arg1, Var *arg2 = NULL);               // 一般运算指令
    InterInst(Operator op, Fun *fun, Var *rs = NULL);                           // 函数调用指令,ENTRY,EXIT
    InterInst(Operator op, Var *arg1 = NULL);                                   // 参数进栈指令,NOP
    InterInst(Operator op, InterInst *tar, Var *arg1 = NULL, Var *arg2 = NULL); // 条件跳转指令,return
    InterInst();                                                                // 产生唯一标号
};

/*
    中间代码
*/
class InterCode
{
    vector<InterInst *> code;

public:
    // 内存管理
    ~InterCode(); // 清除内存

    // 管理操作
    void addInst(InterInst *inst); // 添加一条中间代码
};