#include "parser.h"
#include "token.h"
#include "lexer.h"
#include "error.h"
#include "compiler.h"

/*******************************************************************************
                                   语法分析器
*******************************************************************************/
Parser::Parser(Lexer &lex) : lexer(lex) {}

/*
    语法分析主程序
*/
void Parser::analyse()
{
    move(); // 预先读入
    program();
}

/*
    移进
*/
void Parser::move()
{
    look = lexer.tokenize();
    if (Args::showToken)
        printf("%s\n", look->toString().c_str()); // 输出词法记号——测试
}

/*
    匹配,查看并移动
*/
bool Parser::match(Tag need)
{
    if (look->tag == need)
    {
        move();
        return true;
    }
    else
        return false;
}

// 打印语法错误
#define SYNERROR(code, t) Error::synError(code, t)

/*
    错误修复
*/
#define _(T) || look->tag == T
#define F(C) look->tag == C

void Parser::recovery(bool cond, SynError lost, SynError wrong)
{
    if (cond) /*在给定的Follow集合内*/
        SYNERROR(lost, look);
    else
    {
        SYNERROR(wrong, look);
        move();
    }
}

// 类型
#define TYPE_FIRST F(KW_INT) _(KW_CHAR) _(KW_VOID)
// 表达式
#define EXPR_FIRST F(LPAREN) _(NUM) _(CH) _(STR) _(ID) _(NOT) _(SUB) _(LEA) _(MUL) _(INC) _(DEC)
// 左值运算
#define LVAL_OPR F(ASSIGN) _(OR) _(AND) _(GT) _(GE) _(LT) _(LE) _(EQU) _(NEQU) _(ADD) _(SUB) _(MUL) _(DIV) \
    _(MOD) _(INC) _(DEC)
// 右值运算
#define RVAL_OPR F(OR) _(AND) _(GT) _(GE) _(LT) _(LE) _(EQU) _(NEQU) _(ADD) _(SUB) _(MUL) _(DIV) \
    _(MOD)
// 语句
#define STATEMENT_FIRST (EXPR_FIRST) _(SEMICON) _(KW_WHILE) _(KW_FOR) _(KW_DO) _(KW_IF) \
    _(KW_SWITCH) _(KW_RETURN) _(KW_BREAK) _(KW_CONTINUE)

/*
    <program>			->	<segment><program>|^
*/
void Parser::program()
{
    if (F(END)) // 分析结束
    {
        return;
    }
    else
    {
        segment();
        program();
    }
}

/*
    <segment>			->	rsv_extern <type><def>|<type><def>
*/
void Parser::segment()
{
    bool ext = match(KW_EXTERN); // 记录声明属性
    Tag t = type();
    def(ext, t);
}

/*
    <type>				->	rsv_int|rsv_char|rsv_bool|rsv_void
*/
Tag Parser::type()
{
    Tag tmp = KW_INT;
    if (TYPE_FIRST)
    {
        tmp = look->tag; // 记录类型
        move();          // 移进
    }
    else // 报错
        recovery(F(ID) _(MUL), TYPE_LOST, TYPE_WRONG);
    return tmp; // 记录类型
}

/*
    <defdata>			->	ident <varrdef>|mul ident <init>
*/
Var *Parser::defdata(bool ext, Tag t)
{
    string name = "";
    if (F(ID))
    {
        name = (((Id *)look)->name);
        move();
        return varrdef(ext, t, false, name);
    }
    else if (match(MUL))
    {
        if (F(ID))
        {
            name = (((Id *)look)->name);
            move();
        }
        else
            recovery(F(SEMICON) _(COMMA) _(ASSIGN), ID_LOST, ID_WRONG);
        return init(ext, t, true, name);
    }
    else
    {
        recovery(F(SEMICON) _(COMMA) _(ASSIGN) _(LBRACK), ID_LOST, ID_WRONG);
        return varrdef(ext, t, false, name);
    }
}

/*
    <def>					->	mul id <init><deflist>|ident <idtail>
*/
void Parser::def(bool ext, Tag t)
{
    string name = "";
    if (match(MUL)) // 指针
    {
        if (F(ID))
        {
            name = ((Id *)look)->name;
            move();
        }
        else
            recovery(F(SEMICON) _(COMMA) _(ASSIGN), ID_LOST, ID_WRONG);
        deflist(ext, t);
    }
    else
    {
        if (F(ID)) // 变量 数组 函数
        {
            name = ((Id *)look)->name;
            move();
        }
        else
            recovery(F(SEMICON) _(COMMA) _(ASSIGN) _(LPAREN) _(LBRACK), ID_LOST, ID_WRONG);
        idtail(ext, t, false, name);
    }
}

/*
    <idtail>			->	<varrdef><deflist>|lparen <para> rparen <funtail>
*/
void Parser::idtail(bool ext, Tag t, bool ptr, string name)
{
    if (match(LPAREN)) // 函数
    {
        // TODO
        vector<Var *> paraList; // 参数列表
        para(paraList);
        if (!match(RPAREN))
            recovery(F(LBRACK) _(SEMICON), RPAREN_LOST, RPAREN_WRONG);
        funtail(NULL);
    }
    else
    {
        deflist(ext, t);
    }
}

/*
    <paradatatail>->	lbrack rbrack|lbrack num rbrack|^
*/
Var *Parser::paradatatail(Tag t, string name)
{
    if (match(LBRACK))
    {
        int len = 1; // 参数数组忽略长度
        if (F(NUM))
        {
            len = ((Num *)look)->val;
            move();
        } // 可以没有指定长度
        if (!match(RBRACK))
            recovery(F(COMMA) _(RPAREN), RBRACK_LOST, RBRACK_WRONG);
        // TODO
        return NULL;
    }
    // TODO
    return NULL;
}

/*
    <paradata>		->	mul ident|ident <paradatatail>
*/
Var *Parser::paradata(Tag t)
{
    string name = "";
    if (match(MUL))
    {
        if (F(ID))
        {
            name = ((Id *)look)->name;
            move();
        }
        else
            recovery(F(COMMA) _(RPAREN), ID_LOST, ID_WRONG);
        // TODO
        return NULL;
    }
    else if (F(ID))
    {
        name = ((Id *)look)->name;
        move();
        return paradatatail(t, name);
    }
    else
    {
        recovery(F(COMMA) _(RPAREN) _(LBRACK), ID_LOST, ID_WRONG);
        // TODO
        return NULL;
    }
}

/*
    <para>				->	<type><paradata><paralist>|^
*/
void Parser::para(vector<Var *> &list)
{
    if (F(RPAREN))
        return;
    Tag t = type();
    Var *v = paradata(t);
    // TODO
    list.push_back(v);
    paralist(list);
}

/*
    <paralist>		->	comma<type><paradata><paralist>|^
*/
void Parser::paralist(vector<Var *> &list)
{
    if (match(COMMA))
    { // 下一个参数
        Tag t = type();
        Var *v = paradata(t);
        // TODO
        list.push_back(v);
        paralist(list);
    }
}

/*
    <funtail>			->	<block>|semicon
*/
void Parser::funtail(Fun *f)
{
    if (match(SEMICON)) // 函数声明
    {
        // TODO
    }
    else // 函数定义
    {
        // TODO
        block();
    }
}

/*
    <block>				->	lbrac<subprogram>rbrac
*/
void Parser::block()
{
    if (!match(LBRACE))
        recovery(TYPE_FIRST || STATEMENT_FIRST || F(RBRACE), LBRACE_LOST, LBRACE_WRONG);
    subprogram();
    if (!match(RBRACE))
        recovery(TYPE_FIRST || STATEMENT_FIRST || F(KW_EXTERN) _(KW_ELSE) _(KW_CASE) _(KW_DEFAULT),
                 RBRACE_LOST, RBRACE_WRONG);
}

/*
    <subprogram>	->	<localdef><subprogram>|<statements><subprogram>|^
*/
void Parser::subprogram()
{
    if (TYPE_FIRST) // 局部变量
    {
        localdef();
        subprogram();
    }
    else if (STATEMENT_FIRST) // 语句
    {
        statement();
        subprogram();
    }
}

/*
    <localdef>		->	<type><defdata><deflist>
*/
void Parser::localdef()
{
    Tag t = type();
    // TODO
    defdata(false, t);
    deflist(false, t);
}

/*
    <statement>		->	<altexpr>semicon
                                        |<whilestat>|<forstat>|<dowhilestat>
                                        |<ifstat>|<switchstat>
                                        |rsv_break semicon
                                        |rsv_continue semicon
                                        |rsv_return<altexpr>semicon
*/
void Parser::statement()
{
    switch (look->tag)
    {
    case KW_WHILE:
        whilestat();
        break;
    case KW_FOR:
        forstat();
        break;
    case KW_DO:
        dowhilestat();
        break;
    case KW_IF:
        ifstat();
        break;
    case KW_SWITCH:
        switchstat();
        break;
    case KW_BREAK:
        // TODO
        move();
        if (!match(SEMICON))
            recovery(TYPE_FIRST || STATEMENT_FIRST || F(RBRACE), SEMICON_LOST, SEMICON_WRONG);
        break;
    case KW_CONTINUE:
        // TODO
        move();
        if (!match(SEMICON))
            recovery(TYPE_FIRST || STATEMENT_FIRST || F(RBRACE), SEMICON_LOST, SEMICON_WRONG);
        break;
    case KW_RETURN:
        move();
        // TODO
        altexpr();
        if (!match(SEMICON))
            recovery(TYPE_FIRST || STATEMENT_FIRST || F(RBRACE), SEMICON_LOST, SEMICON_WRONG);
        break;
    default:
        altexpr();
        if (!match(SEMICON))
            recovery(TYPE_FIRST || STATEMENT_FIRST || F(RBRACE), SEMICON_LOST, SEMICON_WRONG);
    }
}

/*
    <whilestat>		->	rsv_while lparen<altexpr>rparen<block>
    <block>				->	<block>|<statement>
*/
void Parser::whilestat()
{
    // TODO
    match(KW_WHILE);
    if (!match(LPAREN))
        recovery(EXPR_FIRST || F(RPAREN), LPAREN_LOST, LPAREN_WRONG);
    Var *cond = altexpr();
    if (!match(RPAREN))
        recovery(F(LBRACE), RPAREN_LOST, RPAREN_WRONG);
    if (F(LBRACE))
        block();
    else
        statement();
}

/*
    <dowhilestat> -> 	rsv_do <block> rsv_while lparen<altexpr>rparen semicon
    <block>				->	<block>|<statement>
*/
void Parser::dowhilestat()
{
    // TODO
    match(KW_DO);
    if (F(LBRACE))
        block();
    else
        statement();
    if (!match(KW_WHILE))
        recovery(F(LPAREN), WHILE_LOST, WHILE_WRONG);
    if (!match(LPAREN))
        recovery(EXPR_FIRST || F(RPAREN), LPAREN_LOST, LPAREN_WRONG);
    Var *cond = altexpr();
    if (!match(RPAREN))
        recovery(F(SEMICON), RPAREN_LOST, RPAREN_WRONG);
    if (!match(SEMICON))
        recovery(TYPE_FIRST || STATEMENT_FIRST || F(RBRACE), SEMICON_LOST, SEMICON_WRONG);
}

/*
    <forstat> 		-> 	rsv_for lparen <forinit> semicon <altexpr> semicon <altexpr> rparen <block>
    <block>				->	<block>|<statement>
*/
void Parser::forstat()
{
    // TODO
    match(KW_FOR);
    if (!match(LPAREN))
        recovery(TYPE_FIRST || EXPR_FIRST || F(SEMICON), LPAREN_LOST, LPAREN_WRONG);
    forinit(); // 初始语句
    // TODO
    Var *cond = altexpr(); // 循环条件
    if (!match(SEMICON))
        recovery(EXPR_FIRST, SEMICON_LOST, SEMICON_WRONG);
    altexpr();
    if (!match(RPAREN))
        recovery(F(LBRACE), RPAREN_LOST, RPAREN_WRONG);
    if (F(LBRACE))
        block();
    else
        statement();
}

/*
    <forinit> 		->  <localdef> | <altexpr>
*/
void Parser::forinit()
{
    if (TYPE_FIRST)
        localdef();
    else
    {
        altexpr();
        if (!match(SEMICON))
            recovery(EXPR_FIRST, SEMICON_LOST, SEMICON_WRONG);
    }
}

/*
    <ifstat>			->	rsv_if lparen<expr>rparen<block><elsestat>
*/
void Parser::ifstat()
{
    // TODO
    match(KW_IF);
    if (!match(LPAREN))
        recovery(EXPR_FIRST, LPAREN_LOST, LPAREN_WRONG);
    Var *cond = expr();
    // TODO
    if (!match(RPAREN))
        recovery(F(LBRACE), RPAREN_LOST, RPAREN_WRONG);
    if (F(LBRACE))
        block();
    else
        statement();
    // TODO

    if (F(KW_ELSE))
    { // 有else
        elsestat();
    }
    // TODO
    // 不对if-else的else部分优化，妨碍冗余删除算法的效果
    //  if(F(KW_ELSE)){//有else
    //  	ir.genElseHead(_else,_exit);//else头部
    //  	elsestat();
    //  	ir.genElseTail(_exit);//else尾部
    //  }
    //  else{//无else
    //  	ir.genIfTail(_else);
    //  }
}

/*
    <elsestat>		-> 	rsv_else<block>|^
*/
void Parser::elsestat()
{
    if (match(KW_ELSE))
    {
        // TODO
        if (F(LBRACE))
            block();
        else
            statement();
    }
}

/*
    <switchstat>	-> 	rsv_switch lparen <expr> rparen lbrac <casestat> rbrac
*/
void Parser::switchstat()
{
    // TODO
    match(KW_SWITCH);
    if (!match(LPAREN))
        recovery(EXPR_FIRST, LPAREN_LOST, LPAREN_WRONG);
    Var *cond = expr();
    // TODO
    if (!match(RPAREN))
        recovery(F(LBRACE), RPAREN_LOST, RPAREN_WRONG);
    if (!match(LBRACE))
        recovery(F(KW_CASE) _(KW_DEFAULT), LBRACE_LOST, LBRACE_WRONG);
    casestat(cond);
    if (!match(RBRACE))
        recovery(TYPE_FIRST || STATEMENT_FIRST, RBRACE_LOST, RBRACE_WRONG);
    // TODO
}

/*
    <casestat> 		-> 	rsv_case <caselabel> colon <subprogram><casestat>
                                        |rsv_default colon <subprogram>
*/
void Parser::casestat(Var *cond)
{
    if (match(KW_CASE))
    {
        Var *lb = caselabel();
        if (!match(COLON))
            recovery(TYPE_FIRST || STATEMENT_FIRST, COLON_LOST, COLON_WRONG);
        subprogram();
        casestat(cond);
    }
    else if (match(KW_DEFAULT))
    { // default默认执行
        if (!match(COLON))
            recovery(TYPE_FIRST || STATEMENT_FIRST, COLON_LOST, COLON_WRONG);
        subprogram();
    }
}

/*
    <caselabel>		->	<literal>
*/
Var *Parser::caselabel()
{
    return literal();
}

/*
    <altexpr>			->	<expr>|^
*/
Var *Parser::altexpr()
{
    if (EXPR_FIRST)
        return expr();
    // TODO
    return NULL; // 返回特殊VOID变量
}

/*
    <deflist>			->	comma <defdata> <deflist>|semicon
*/
void Parser::deflist(bool ext, Tag t)
{
    if (match(COMMA)) // 下一个声明
    {
        defdata(ext, t);
        deflist(ext, t);
    }
    else if (match(SEMICON)) // 最后一个声明
        return;
    else // 出错了
    {
        if (F(ID) _(MUL)) // 逗号
        {
            recovery(1, COMMA_LOST, COMMA_WRONG);
            defdata(ext, t);
            deflist(ext, t);
        }
        else
            recovery(TYPE_FIRST || STATEMENT_FIRST || F(KW_EXTERN) _(RBRACE), SEMICON_LOST, SEMICON_WRONG);
    }
}

/*
    <varrdef>			->	lbrack num rbrack | <init>
*/
Var *Parser::varrdef(bool ext, Tag t, bool ptr, string name)
{
    if (match(LBRACK))
    {
        int len = 0;
        if (F(NUM))
        {
            len = ((Num *)look)->val;
            move();
        }
        else
            recovery(F(RBRACK), NUM_LOST, NUM_WRONG);
        if (!match(RBRACK))
            recovery(F(COMMA) _(SEMICON), RBRACK_LOST, RBRACK_WRONG);
        // TODO
        return NULL;
    }
    else
        return init(ext, t, ptr, name);
}

/*
    <init>				->	assign <expr>|^
*/
Var *Parser::init(bool ext, Tag t, bool ptr, string name)
{
    Var *initVal = NULL;
    if (match(ASSIGN))
    {
        initVal = expr();
    }
    // TODO
    return NULL;
}

/*
    <expr> 				-> 	<assexpr>
*/
Var *Parser::expr()
{
    return assexpr();
}

/*
    <assexpr>			->	<orexpr><asstail>
*/
Var *Parser::assexpr()
{
    Var *lval = orexpr();
    return asstail(lval);
}

/*
    <asstail>			->	assign<assexpr>|^
*/
Var *Parser::asstail(Var *lval)
{
    if (match(ASSIGN))
    {
        Var *rval = assexpr();
        // TODO
        return asstail(NULL);
    }
    return lval;
}

/*
    <orexpr> 			-> 	<andexpr><ortail>
*/
Var *Parser::orexpr()
{
    Var *lval = andexpr();
    return ortail(lval);
}

/*
    <ortail> 			-> 	or <andexpr> <ortail>|^
*/
Var *Parser::ortail(Var *lval)
{
    if (match(OR))
    {
        Var *rval = andexpr();
        // TODO
        return ortail(NULL);
    }
    return lval;
}

/*
    <andexpr> 		-> 	<cmpexpr><andtail>
*/
Var *Parser::andexpr()
{
    Var *lval = cmpexpr();
    return andtail(lval);
}

/*
    <andtail> 		-> 	and <cmpexpr> <andtail>|^
*/
Var *Parser::andtail(Var *lval)
{
    if (match(AND))
    {
        Var *rval = cmpexpr();
        // TODO
        return andtail(NULL);
    }
    return lval;
}

/*
    <cmpexpr>			->	<aloexpr><cmptail>
*/
Var *Parser::cmpexpr()
{
    Var *lval = aloexpr();
    return cmptail(lval);
}

/*
    <cmptail>			->	<cmps><aloexpr><cmptail>|^
*/
Var *Parser::cmptail(Var *lval)
{
    if (F(GT) _(GE) _(LT) _(LE) _(EQU) _(NEQU))
    {
        Tag opt = cmps();
        Var *rval = aloexpr();
        // TODO
        return cmptail(NULL);
    }
    return lval;
}

/*
    <cmps>				->	gt|ge|ls|le|equ|nequ
*/
Tag Parser::cmps()
{
    Tag opt = look->tag;
    move();
    return opt;
}

/*
    <aloexpr>			->	<item><alotail>
*/
Var *Parser::aloexpr()
{
    Var *lval = item();
    return alotail(lval);
}

/*
    <alotail>			->	<adds><item><alotail>|^
*/
Var *Parser::alotail(Var *lval)
{
    if (F(ADD) _(SUB))
    {
        Tag opt = adds();
        Var *rval = item();
        // TODO
        return alotail(NULL);
    }
    return lval;
}

/*
    <adds>				->	add|sub
*/
Tag Parser::adds()
{
    Tag opt = look->tag;
    move();
    return opt;
}

/*
    <item>				->	<factor><itemtail>
*/
Var *Parser::item()
{
    Var *lval = factor();
    return itemtail(lval);
}

/*
    <itemtail>		->	<muls><factor><itemtail>|^
*/
Var *Parser::itemtail(Var *lval)
{
    if (F(MUL) _(DIV) _(MOD))
    {
        Tag opt = muls();
        Var *rval = factor();
        // TODO
        return itemtail(NULL);
    }
    return lval;
}

/*
    <muls>				->	mul|div|mod
*/
Tag Parser::muls()
{
    Tag opt = look->tag;
    move();
    return opt;
}

/*
    <factor> 			-> 	<lop><factor>|<val>
*/
Var *Parser::factor()
{
    if (F(NOT) _(SUB) _(LEA) _(MUL) _(INC) _(DEC))
    {
        Tag opt = lop();
        Var *v = factor();
        // TODO
        return NULL;
    }
    else
        return val();
}

/*
    <lop> 				-> 	not|sub|lea|mul|incr|decr
*/
Tag Parser::lop()
{
    Tag opt = look->tag;
    move();
    return opt;
}

/*
    <val>					->	<elem><rop>
*/
Var *Parser::val()
{
    Var *v = elem();
    if (F(INC) _(DEC))
    {
        Tag opt = rop();
        // TODO
    }
    return v;
}

/*
    <rop>					->	incr|decr|^
*/
Tag Parser::rop()
{
    Tag opt = look->tag;
    move();
    return opt;
}

/*
    <elem>				->	ident<idexpr>|lparen<expr>rparen|<literal>
*/
Var *Parser::elem()
{
    Var *v = NULL;
    if (F(ID)) // 变量，数组，函数调用
    {
        string name = ((Id *)look)->name;
        move();
        v = idexpr(name);
    }
    else if (match(LPAREN)) // 括号表达式
    {
        v = expr();
        if (!match(RPAREN))
        {
            recovery(LVAL_OPR, RPAREN_LOST, RPAREN_WRONG);
        }
    }
    else // 常量
        v = literal();

    return v;
}

/*
    <literal>			->	number|string|chara
*/
Var *Parser::literal()
{
    Var *v = NULL;
    if (F(NUM) _(STR) _(CH))
    {
        // TODO
        move();
    }
    else
        recovery(RVAL_OPR, LITERAL_LOST, LITERAL_WRONG);
    return v;
}

/*
    <idexpr>			->	lbrack <expr> rbrack|lparen<realarg>rparen|^
*/
Var *Parser::idexpr(string name)
{
    Var *v = NULL;
    if (match(LBRACK))
    {
        Var *index = expr();
        if (!match(RBRACK))
            recovery(LVAL_OPR, LBRACK_LOST, LBRACK_WRONG);
        // TODO
    }
    else if (match(LPAREN))
    {
        vector<Var *> args;
        realarg(args);
        if (!match(RPAREN))
            recovery(RVAL_OPR, RPAREN_LOST, RPAREN_WRONG);
        // TODO
    }
    else
        ;

    return v;
}

/*
    <realarg>			->	<arg><arglist>|^
*/
void Parser::realarg(vector<Var *> &args)
{
    if (EXPR_FIRST)
    {
        args.push_back(arg()); // 压入参数
        arglist(args);
    }
}

/*
    <arglist>			->	comma<arg><arglist>|^
*/
void Parser::arglist(vector<Var *> &args)
{
    if (match(COMMA))
    {
        args.push_back(arg());
        arglist(args);
    }
}

/*
    <arg> 				-> 	<expr>
*/
Var *Parser::arg()
{
    // 添加一个实际参数
    return expr();
}