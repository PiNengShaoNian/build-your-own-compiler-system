#include "copyprop.h"
#include "dfg.h"
#include "intercode.h"

CopyPropagation::CopyPropagation(DFG *g) : dfg(g)
{
    dfg->toCode(optCode);
    int j = 0;
    // 统计所有的表达式

    for (list<InterInst *>::iterator i = optCode.begin(); i != optCode.end(); ++i)
    {
        InterInst *inst = *i;        // 遍历指令
        Operator op = inst->getOp(); // 获取操作符
        if (op == OP_AS)             // 复写表达式
        {
            copyExpr.push_back(inst); // 记录复写表达式
        }
    }

    // 没有复写表达式，无法进行进一步优化了，在这里提前退出
    if (copyExpr.size() == 0)
        return;

    int index = 0;
    for (int i = 0; i < copyExpr.size(); ++i)
    {
        InterInst *inst = copyExpr[i]; // 遍历指令
        Var *rs = inst->getResult();   // 获取结果
        Var *arg1 = inst->getArg1();

        if (varToIndex.find(rs) == varToIndex.end())
            varToIndex[rs] = index++;
        if (varToIndex.find(arg1) == varToIndex.end())
            varToIndex[arg1] = index++;
    }
    uf.resize(index);
    for (int i = 0; i < index; ++i)
    {
        uf[i] = i;
    }

    for (int i = 0; i < copyExpr.size(); ++i)
    {
        InterInst *inst = copyExpr[i]; // 遍历指令
        Var *rs = inst->getResult();   // 获取结果
        Var *arg1 = inst->getArg1();

        int rsIndex = varToIndex[rs];
        int arg1Index = varToIndex[arg1];
        uf[findParent(rsIndex)] = findParent(arg1Index);
    }

    U.init(copyExpr.size(), 1); // 初始化基本集合,全集
    E.init(copyExpr.size(), 0); // 初始化基本集合,空集

    // 初始化指令的指令的gen和kill集合
    for (list<InterInst *>::iterator i = optCode.begin(); i != optCode.end(); ++i)
    {
        InterInst *inst = *i;   // 遍历指令
        inst->copyInfo.gen = E; // 初始化使用集合与杀死集合
        inst->copyInfo.kill = E;
        Var *rs = inst->getResult(); // 获取结果
        Operator op = inst->getOp(); // 获取操作符
        if (inst->unknown())
        {                            // 取值运算 *p=x / 函数调用<非指针参数应该是仅仅杀死全局变量，忽略>
            inst->copyInfo.kill = U; // 杀死所有表达式
        }
        else if (op >= OP_AS && op <= OP_GET)
        {
            int rsParent = findParent(varToIndex[rs]);
            // 其他所有的表达式
            for (int i = 0; i < copyExpr.size(); i++) // 扫描所有复写表达式
            {
                if (copyExpr[i] == inst) // 产生表达式
                {
                    inst->copyInfo.gen.set(i); // 产生了复写表达式，设置产生标记
                }
                else // 杀死表达式
                {
                    if (rsParent == findParent(varToIndex[copyExpr[i]->getResult()]))
                    {
                        inst->copyInfo.kill.set(i); // 设置杀死复写表达式标记
                    }
                    // if (rs == copyExpr[i]->getResult() || rs == copyExpr[i]->getArg1())
                    // {
                    //     inst->copyInfo.kill.set(i); // 设置杀死复写表达式标记
                    // }
                }
            }
        }
    }
}

int CopyPropagation::findParent(int x)
{
    if (uf[x] == x)
        return x;
    return uf[x] = findParent(uf[x]);
}

/*
    复写传播传递函数：f(x)=(x - kill(B)) | gen(B)
*/
bool CopyPropagation::translate(Block *block)
{
    Set tmp = block->copyInfo.in; // 记录B.in作为第一个指令的输入
    for (list<InterInst *>::iterator i = block->insts.begin(); i != block->insts.end(); ++i)
    {
        InterInst *inst = *i; // 取出指令
        if (inst->isDead)     // 跳过死指令
            continue;
        Set &in = inst->copyInfo.in;   // 指令的in
        Set &out = inst->copyInfo.out; // 指令的out
        in = tmp;                      // 设置指令的in

        out = (in - inst->copyInfo.kill) | inst->copyInfo.gen; // 指令级传递函数out=f(in)
        tmp = out;                                             // 用当前指令的out设置下次使用的in
    }
    bool flag = tmp != block->copyInfo.out; // 是否变化
    block->copyInfo.out = tmp;              // 设定B.out集合
    return flag;
}

/*
    复写传播数据流分析函数
    direct : 数据流方向,前向
    init : 初始化集合,B.out=U
    bound : 边界集合，Entry.out=E
    join : 交汇运算，交集
    translate : 传递函数,f(x)=(x - kill(B)) | gen(B)
*/
void CopyPropagation::analyse()
{
    // 正常的数据流分析
    dfg->blocks[0]->copyInfo.out = E; // 初始化边界集合Entry.out=E
    for (int i = 1; i < dfg->blocks.size(); ++i)
    {
        dfg->blocks[i]->copyInfo.out = U; // 初始化基本块B.out=U
    }
    bool change = true;
    while (change) // B.out集合发生变化
    {
        change = false;
        for (int i = 1; i < dfg->blocks.size(); ++i) // 任意B!=Entry
        {
            if (!dfg->blocks[i]->canReach) // 块不可达不处理
                continue;
            Set tmp = U; // 保存交汇运算结果
            for (list<Block *>::iterator j = dfg->blocks[i]->prevs.begin();
                 j != dfg->blocks[i]->prevs.end(); ++j) // prev[i]
            {
                tmp = tmp & (*j)->copyInfo.out;
            }
            // B.in=& prev[j].out
            dfg->blocks[i]->copyInfo.in = tmp;
            if (translate(dfg->blocks[i])) // 传递函数执行时比较前后集合是否有差别
                change = true;
        }
    }
}

/*
    递归检测var赋值的源头的实现函数
*/
Var *CopyPropagation::__find(Set &in, Var *var, Var *src)
{
    if (!var)
        return NULL;
    for (int i = 0; i < copyExpr.size(); i++) // 扫描所有复写表达式
    {
        if (in.get(i) && var == copyExpr[i]->getResult())
        {
            var = copyExpr[i]->getArg1(); // 查找参数源头

            if (src == var) // 查找过程中出现环！比如：x=y;y=x;查找y的时候。
                break;
            return __find(in, copyExpr[i]->getArg1(), src); // 继续查找赋值源头
        }
    }
    return var; // 找不到可替换的变量，返回自身，或者上层结果
}

/*
    递归检测var赋值的源头的封装函数
*/
Var *CopyPropagation::find(Set &in, Var *var)
{
    __find(in, var, var);
}

/*
    执行复写传播
*/
void CopyPropagation::propagate()
{
    analyse();
    // 处理每一个指令
    for (list<InterInst *>::iterator i = optCode.begin(); i != optCode.end(); ++i)
    {
        InterInst *inst = *i;
        Var *rs = inst->getResult();        // 获取结果
        Operator op = inst->getOp();        // 获取操作符
        Var *arg1 = inst->getArg1();        // 获取操作数1
        Var *arg2 = inst->getArg2();        // 获取操作数2
        InterInst *tar = inst->getTarget(); // 获取跳转目标

        if (op == OP_SET) // 取值运算，检查 *arg1=result
        {
            Var *newRs = find(inst->copyInfo.in, rs); // 找到result赋值源头，肯定不是空
            inst->replace(op, newRs, arg1);           // 不论是否是新变量，更新指令
        }
        else if (op >= OP_AS && op <= OP_GET && op != OP_LEA) // 一般表达式，排除p=&x运算，检查arg1/2
        {
            Var *newArg1 = find(inst->copyInfo.in, arg1); // 找arg1源头
            Var *newArg2 = find(inst->copyInfo.in, arg2); // 找arg2源头
            inst->replace(op, rs, newArg1, newArg2);      // 不论是否是新变量，更新指令
        }
        else if (op == OP_JT || op == OP_JF || op == OP_ARG || op == OP_RETV) // 条件表达式,参数表达式，返回表达式
        {
            Var *newArg1 = find(inst->copyInfo.in, arg1); // 找arg1源头
            inst->setArg1(newArg1);                       // 更新参数变量，返回值
        }
    }
}