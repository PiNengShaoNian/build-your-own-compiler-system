#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdio.h>
#include <iostream>
using namespace std;
/**  *****************************************************************************************************************************
 ***类型定义***
 ********************************************************************************************************************************/
enum symbol // 所有符号的枚举
{
    null,
    ident,
    excep,
    number,
    strings, // 空，标识符，异常字符，数字,串
    addi,
    subs, // 加，减
    comma,
    lbrac,
    rbrac,
    colon,
    br_al,
    br_cl,
    br_dl,
    br_bl,
    br_ah,
    br_ch,
    br_dh,
    br_bh,
    dr_eax,
    dr_ecx,
    dr_edx,
    dr_ebx,
    dr_esp,
    dr_ebp,
    dr_esi,
    dr_edi,
    i_mov,
    i_cmp,
    i_sub,
    i_add,
    i_lea, // 2p
    i_call,
    i_int,
    i_imul,
    i_idiv,
    i_neg,
    i_inc,
    i_dec,
    i_jmp,
    i_je,
    i_jg,
    i_jl,
    i_jge,
    i_jle,
    i_jne,
    i_jna,
    i_push,
    i_pop, // 1p
    i_ret, // 0p
    a_sec,
    a_glb,
    a_equ,
    a_times,
    a_db,
    a_dw,
    a_dd // assamble instructions
};

#define idLen 30      /*标识符的最大长度30*/
#define numLen 9      /*数字的最大位数9*/
#define stringLen 255 /*字符串的最大长度255*/

// 宏定义
#define GET_CHAR         \
    if (-1 == getChar()) \
        return -1;

/**  *****************************************************************************************************************************
 ***全局声明***
 ********************************************************************************************************************************/
extern FILE *fin;       // 全局变量，文件输入指针
extern enum symbol sym; // 当前符号，getSym()->给语法分析使用
extern char str[];      // 记录当前string，给erorr处理
extern char id[];       // 记录当前ident
extern int num;         // 记录当前num
#define immd 1
#define memr 3
#define regs 2

extern int lineNum; // 记录行数

extern int scanLop;

int getSym();
void program();

#endif
