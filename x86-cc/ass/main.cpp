#include "common.h"
#include <stdlib.h>
FILE *fin = NULL; // 全局变量，文件输入指针

string finName;       // 输入文件名
bool showAss = false; // 显示汇编信息
/**
  主函数，测试，发布
*/
int main(int argc, char *argv[])
{
    finName = argv[1];
    showAss = (argv[2][0] == 'y');
    fin = fopen((finName + ".s").c_str(), "r"); // 输入文件
    fclose(fin);
    return 0;
}
