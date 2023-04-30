#include "compiler.h"
#include "error.h"
#include "lexer.h"
#include "token.h"
#include "parser.h"
#include "assert.h"
#include "symtab.h"
#include "genir.h"

int main(int argc, char *argv[])
{
    vector<char *> srcfiles; // 源文件
    if (argc > 1)            // 至少1个参数
    {
        for (int i = 1; i < argc; i++)
        { // 取最后一个参数之前的参数
            srcfiles.push_back(argv[i]);
        }
    }

    if (srcfiles.size() >= 1)
    {
        Scanner scanner(srcfiles[0]);
        Lexer lexer(scanner);
        Error err(&scanner);
        SymTab symTab;
        GenIR genIr(symTab);
        Parser parser(lexer, symTab, genIr);

        parser.analyse();

        // 报错
        if (Error::getErrorNum() + Error::getWarnNum())
            return -1; // 出错不进行后续操作

        symTab.toString();       // 输出符号表
        symTab.printInterCode(); // 输出中间代码

        assert(lexer.tokenize()->tag == END);

        // 生成汇编代码
        symTab.genAsm(srcfiles[0]);
    }
}