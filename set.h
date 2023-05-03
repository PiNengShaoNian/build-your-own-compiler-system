#pragma once
#include "common.h"

/*
    集合类——使用位图表达集合运算
*/
class Set
{
    vector<unsigned int> bmList; // 位图列表：位图长度大于32时

public:
    int count; // 记录集合元素个数

    // 构造与初始化
    Set();
    Set(int size, bool val); // 构造一个容纳size个元素的集合，并初始化
    void init(int size, bool val);
    void p(); // 调试输出函数

    // 集合基本运算
    Set operator&(Set val);    // 交集运算
    Set operator|(Set val);    // 并集运算
    Set operator-(Set val);    // 差集运算
    Set operator^(Set val);    // 异或运算
    Set operator~();           // 补集运算
    bool operator==(Set &val); // 比较运算
    bool operator!=(Set &val); // 比较运算
    bool get(int i);           // 索引运算
    void set(int i);           // 置位运算
    void reset(int i);         // 复位运算
    int max();                 // 返回最高位的1的索引
    bool empty()
    {
        return bmList.size() == 0;
    }
};

/*
    复写传播的数据流信息
*/
struct CopyInfo
{
    Set in;   // 输入集合
    Set out;  // 输出集合
    Set gen;  // 产生复写表达式集合
    Set kill; // 杀死复写表达式集合
};