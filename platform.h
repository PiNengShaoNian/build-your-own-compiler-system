#pragma once
#include "common.h"

/*
    ARM平台信息
*/
class Plat
{
public:
    /*
     进栈指令序列:
         mov ip,sp
         stmfd sp!,{fp,ip,lr,pc}
         sub fp,ip,#4
     出栈指令序列：
         ldmea fp,{fp,sp,pc}
     栈基址fp指向pc，继续入栈需要在偏移12字节基础之上！
    */
    static const int stackBase = 12; // 不加保护现场的栈基址=12
};