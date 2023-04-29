#pragma once

#include "common.h"
#include "intercode.h"

/*******************************************************************************
                                   变量结构体
*******************************************************************************/
class Var
{
    // 特殊标记
    bool literal;          // 是否字面量,字面量可以初始化定义的变量
    vector<int> scopePath; // 作用域路径

    // 基本声明形式
    bool externed; // extern声明或定义
    Tag type;      // 变量类型
    string name;   // 变量名称
    bool isPtr;    // 是否是指针
    bool isArray;  // 是否是数组
    int arraySize; // 数组长度

    // 初始值部分
    bool isLeft;   // 是否可以作为左值
    Var *initData; // 缓存初值数据，延迟处置处理
    bool inited;   // 是否初始化数据，字面量一定是初始化了的特殊变量
    union
    {
        int intVal;
        char charVal;
    };
    string ptrVal; // 初始化字符指针常量字符串的名称
    string strVal; // 字符串常量的值
    Var *ptr;      // 指向当前变量指针变量

    // 附加信息
    int size;   // 变量的大小
    int offset; // 局部变量，参数变量的栈帧偏移，默认值0为无效值——表示全局变量

    // 内部使用函数
    void setExtern(bool ext); // 设置extern
    void setType(Tag t);      // 设置类型
    void setPtr(bool ptr);    // 设置指针
    void setName(string n);   // 设置名字
    void setArray(int len);   // 设定数组
    void clear();             // 清除关键字段信息

public:
    // 特殊变量1,4-步长
    static Var *getStep(Var *v); // 获取步长变量
    static Var *getVoid();       // 获取void特殊变量
    static Var *getTrue();       // 获取true变量

    // 构造函数
    Var(vector<int> &sp, bool ext, Tag t, bool ptr, string name, Var *init = NULL); // 变量
    Var(vector<int> &sp, bool ext, Tag t, string name, int len);                    // 数组
    Var(Token *lt);                                                                 // 设定字面量
    Var(int val);                                                                   // 整数变量
    Var(vector<int> &sp, Tag t, bool ptr);                                          // 临时变量
    Var(vector<int> &sp, Var *v);                                                   // 拷贝变量
    Var();                                                                          // void变量

    // 外界调用接口
    bool setInit();          // 设定初始化，由调用者决定初始化方式和顺序
    Var *getInitData();      // 获取初始化变量数据
    vector<int> &getPath();  // 获取scopePath
    string getName();        // 获取名字
    Tag getType();           // 获取类型
    void setLeft(bool lf);   // 设置变量的左值属性
    bool getLeft();          // 获取左值属性
    void setOffset(int off); // 设置栈帧偏移
    int getOffset();         // 获取栈帧偏移
    int getSize();           // 获取变量大小
    bool getArray();         // 获取数组
    bool getPtr();           // 获取指针
    Var *getPointer();       // 获取指针
    void setPointer(Var *p); // 设置指针变量
    string getStrVal();      // 获取字符串常量内容
    bool isBase();           // 是基本类型
    bool isVoid();           // 是void——唯一静态存储区变量getVoid()使用
    bool isRef();            // 是引用类型
    void toString();         // 输出信息

    // 数据流分析接口
    int getVal(); // 获取常量值
};

/*******************************************************************************
                                   函数结构
*******************************************************************************/

class Fun
{
    // 基本信息
    bool externed;         // 声明或定义
    Tag type;              // 变量类型
    string name;           // 变量名称
    vector<Var *> paraVar; // 形参变量列表

    // 临时变量地址分配
    int maxDepth;   // 栈的最大深度，初始0,标识函数栈分配的最大空间
    int curEsp;     // 当前栈指针位置，初始化为0，即ebp存储点
    bool relocated; // 栈帧重定位标记

    // 作用域管理
    vector<int> scopeEsp;   // 当前作用域初始esp，动态控制作用域的分配和释放
    InterCode interCode;    // 中间代码
    InterInst *returnPoint; // 返回点

public:
    // 构造函数与析构函数
    Fun(bool ext, Tag t, string n, vector<Var *> &paraList);
    ~Fun();

    // 声明定义与使用
    bool match(Fun *f);              // 声明定义匹配
    bool match(vector<Var *> &args); // 行参实参匹配
    void define(Fun *def);           // 将函数声明转换为定义，需要拷贝参数列表，设定extern

    // 作用域管理，局部变量地址计算
    void enterScope();     // 进入一个新的作用域
    void leaveScope();     // 离开当前作用域
    void locate(Var *var); // 定位局部变了栈帧偏移

    // 中间代码
    void addInst(InterInst *inst);        // 添加一条中间代码
    void setReturnPoint(InterInst *inst); // 设置函数返回点
    InterInst *getReturnPoint();          // 获取函数返回点

    // 外部调用掉口
    bool getExtern();         // 获取extern
    void setExtern(bool ext); // 设置extern
    Tag getType();            // 获取函数类型
    string &getName();        // 获取名字
    void toString();          // 输出信息
};