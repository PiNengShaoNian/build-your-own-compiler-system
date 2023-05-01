#include "dfg.h"
#include "intercode.h"

/*******************************************************************************
                                   基本块
*******************************************************************************/

/*
    构造与初始化
*/
Block::Block(vector<InterInst *> &codes)
{
    for (int i = 0; i < codes.size(); ++i)
    {
        codes[i]->block = this;    // 记录指令所在的基本块
        insts.push_back(codes[i]); // 转换为list
    }
}

/*
    输出基本块指令
*/
void Block::toString()
{
    printf("-----------%p----------\n", this);
    printf("前驱：");
    for (list<Block *>::iterator i = prevs.begin(); i != prevs.end(); ++i)
    {
        printf("%p ", (*i));
    }
    printf("\n");
    printf("后继：");
    for (list<Block *>::iterator i = succs.begin(); i != succs.end(); ++i)
    {
        printf("%p ", (*i));
    }
    printf("\n");
    for (list<InterInst *>::iterator i = insts.begin(); i != insts.end(); ++i)
    {
        (*i)->toString();
    }
    printf("-----------------------------\n");
}

/*******************************************************************************
                                   数据流图
*******************************************************************************/

/*
    初始化
*/
DFG::DFG(InterCode &code)
{
    code.markFirst();          // 标识首指令
    codeList = code.getCode(); // 获取代码序列
    createBlocks();            // 创建基本块
    linkBlocks();              // 链接基本块关系
}

/*
    创建基本块
*/
void DFG::createBlocks()
{
    vector<InterInst *> tmpList; // 记录一个基本块的指令临时列表
    for (int i = 0; i < codeList.size(); ++i)
    {
        if (tmpList.empty() && codeList[i]->isFirst())
        {
            tmpList.push_back(codeList[i]); // 添加第一条首指令
            continue;
        }
        if (!tmpList.empty())
        {
            if (codeList[i]->isFirst())
            {                                         // 新的首指令
                blocks.push_back(new Block(tmpList)); // 发现添加基本块
                tmpList.clear();                      // 清除上个临时列表
            }
            tmpList.push_back(codeList[i]); // 添加新的首指令或者基本块后继的指令
        }
    }
    blocks.push_back(new Block(tmpList)); // 添加最后一个基本块
}

/*
    链接基本块
*/
void DFG::linkBlocks()
{
    // 链接基本块顺序关系
    for (int i = 0; i < blocks.size() - 1; ++i)
    {                                              // 后继关系
        InterInst *last = blocks[i]->insts.back(); // 当前基本块的最后一条指令
        if (!last->isJmp())                        // 不是直接跳转，可能顺序执行
            blocks[i]->succs.push_back(blocks[i + 1]);
    }
    for (int i = 1; i < blocks.size(); ++i)
    {                                                  // 前驱关系
        InterInst *last = blocks[i - 1]->insts.back(); // 前个基本块的最后一条指令
        if (!last->isJmp())                            // 不是直接跳转，可能顺序执行
            blocks[i]->prevs.push_back(blocks[i - 1]);
    }
    for (int i = 0; i < blocks.size(); ++i)
    {                                              // 跳转关系
        InterInst *last = blocks[i]->insts.back(); // 基本块的最后一条指令
        if (last->isJmp() || last->isJcond())
        {                                                         // （直接/条件）跳转
            blocks[i]->succs.push_back(last->getTarget()->block); // 跳转目标块为后继
            last->getTarget()->block->prevs.push_back(blocks[i]); // 相反为前驱
        }
    }
}

/*
    清除所有基本块
*/
DFG::~DFG()
{
    for (int i = 0; i < blocks.size(); ++i)
    {
        delete blocks[i];
    }
    // TODO
}

/*
    输出基本块
*/
void DFG::toString()
{
    for (int i = 0; i < blocks.size(); ++i)
    {
        blocks[i]->toString();
    }
}