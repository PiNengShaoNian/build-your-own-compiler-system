#pragma once
#include "common.h"

/*
    基本块类
*/
class Block
{
public:
    // 基本信息和关系
    list<InterInst *> insts; // 基本块的指令序列
    list<Block *> prevs;     // 基本块的前驱序列
    list<Block *> succs;     // 基本块的后继序列

    // 构造与初始化
    Block(vector<InterInst *> &codes);

    // 外部调用接口
    void toString(); // 输出基本块指令
};

/*
    数据流图类
*/
class DFG
{
    void createBlocks(); // 创建基本块
    void linkBlocks();   // 链接基本块

public:
    vector<InterInst *> codeList; // 中间代码序列
    vector<Block *> blocks;       // 流图的所有基本块

    // 构造与初始化
    DFG(InterCode &code); // 初始化
    ~DFG();               // 清理操作

    // 外部调用接口
    void toString(); // 输出基本块
};