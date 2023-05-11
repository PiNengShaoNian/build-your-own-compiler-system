#include "linker.h"
#include "stdio.h"

bool showLink = false;
int main(int argc, char *argv[])
{
    Linker linker;
    string desFileName;
    int i = 1;
    while (true)
    {
        string arg = argv[i];
        if (arg.rfind(".o") != arg.length() - 2) // 是输出文件
        {
            desFileName = arg;
            break;
        }
        linker.addElf(arg.c_str()); // 添加目标文件
        i++;
    }
    return 0;
}