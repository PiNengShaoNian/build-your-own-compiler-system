#include "compiler.h"
#include "error.h"
#include "lexer.h"
#include "token.h"
#include "parser.h"
#include "assert.h"
#include "symtab.h"

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
        Parser parser(lexer, symTab);

        parser.analyse();

        assert(lexer.tokenize()->tag == END);
    }
}