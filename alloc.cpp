#include "alloc.h"
#include "intercode.h"
#include "livevar.h"
#include <algorithm>
#include "symbol.h"
#include "platform.h"

/*******************************************************************************
                                   节点
*******************************************************************************/

Node::Node(Var *v, Set &E) : var(v), degree(0), color(-1)
{
    exColors = E; // 排斥色集合为空
}

/*
    添加一条边，有序插入，度+1！保证二分查找
*/
void Node::addLink(Node *node)
{
    vector<Node *>::iterator pos = lower_bound(links.begin(), links.end(), node);
    if (pos == links.end() || *pos != node) // 没有找到，则插入
    {
        links.insert(pos, node);
        degree++; // 度增加
    }
}

/*
    添加一个排除色,度-1
*/
void Node::addExColor(int color)
{
    if (degree == -1) // 已经着色节点，不再处理！
        return;
    exColors.set(color); // 添加排除色
    degree--;
}

/*
    根据exColors选择第一个有效的颜色，度=-1
*/
void Node::paint(Set &colorBox)
{
    Set availColors = colorBox - exColors;      // 可用的颜色集合
    for (int i = 0; i < availColors.count; i++) // 遍历所有可用颜色
    {
        if (availColors.get(i)) // 还有颜色可用
        {
            color = i;          // 着色
            var->regId = color; // 记录颜色（寄存器ID）到变量
            // 度置为小于任何一个未处理节点的度值，这样保证不会重复访问——每次选大度节点！
            degree = -1;
            for (int j = 0; j < links.size(); j++) // 遍历邻居
            {
                links[j]->addExColor(color); // 邻居添加排除色
            }
            return; // 着色成功！
        }
    }
    // 着色失败！！！
    degree = -1; // 已经尝试着色，未成功，标记为处理节点
}

/*******************************************************************************
                                   作用域
*******************************************************************************/

Scope::Scope(int i, int addr) : id(i), esp(addr), parent(NULL)
{
}

Scope::~Scope()
{
    for (int i = 0; i < children.size(); i++)
        delete children[i];
}

Scope *Scope::find(int i)
{
    // 二分查找
    Scope *sc = new Scope(i, esp); // 先创建子作用域，拷贝父esp
    vector<Scope *>::iterator pos = lower_bound(children.begin(), children.end(), sc, scope_less());
    if (pos == children.end() || (*pos)->id != i) // 没有找到，则插入
    {
        children.insert(pos, sc); // 有序插入
        sc->parent = this;        // 记录父节点
    }
    else
    {
        delete sc; // 删除查找对象
        sc = *pos; // 找到了
    }
    return sc;
}

/*******************************************************************************
                                   冲突图
*******************************************************************************/

/*
    构造函数
*/
CoGraph::CoGraph(list<InterInst *> &optCode, vector<Var *> &para, LiveVar *lv, Fun *f)
{
    fun = f;
    this->optCode = optCode; // 记录代码
    this->lv = lv;           // 记录活跃变量分析对象指针

    U.init(Plat::regNum, 1); // 初始化颜色集合,全集
    E.init(Plat::regNum, 0); // 初始化颜色集合,空集

    // 函数参数变量
    for (int i = 0; i < para.size(); ++i)
        varList.push_back(para[i]); // 将参数放入列表

    // 所有的局部变量
    for (list<InterInst *>::iterator i = optCode.begin(); i != optCode.end(); ++i)
    {
        InterInst *inst = *i;        // 遍历指令
        Operator op = inst->getOp(); // 获取操作符
        if (op == OP_DEC)            // 局部变量声明
        {
            Var *arg1 = inst->getArg1();
            varList.push_back(arg1); // 记录变量
        }
        if (op == OP_LEA) // 标记只能放在内存的变量！
        {
            Var *arg1 = inst->getArg1();
            if (arg1)
                arg1->inMem = true; // p=&a, a只能放在内存
        }
    }

    Set &liveE = lv->getE(); // 活跃变量分析时的空集合
    // 计算当前变量列表想对于活跃变量分析时变量列表的掩码
    Set mask = liveE;                        // 初始化为空集
    for (int i = 0; i < varList.size(); i++) // 遍历当前变量列表
    {
        mask.set(varList[i]->index); // 取出变量在活跃变量分析时的列表索引
    }

    // 构建图节点
    for (int i = 0; i < varList.size(); i++)
    {
        Node *node;
        if (varList[i]->getArray() || varList[i]->inMem) // 数组或者只能在内存的变量，不着色
            node = new Node(varList[i], U);              // 创建数组的节点
        else
            node = new Node(varList[i], E); // 创建变量的节点

        varList[i]->index = i; // 修正新的变量列表索引，方便查找变量所在的新的列表位置！
        nodes.push_back(node); // 保存节点
    }

    // 构建冲突边
    Set buf = liveE; // 缓冲上次处理的live.out集合，减少匹配运算
                     // 逆序遍历live.out集合
    for (list<InterInst *>::reverse_iterator i = optCode.rbegin(); i != optCode.rend(); ++i)
    {
        Set &liveout = (*i)->liveInfo.out; // live.out集合
        if (liveout != buf)                // 新的冲突关系
        {
            buf = liveout; // 缓存
            // 根据指令活跃变量集合生成完全图的边，可以通过“逆序遍历缓存集合”的方式减少计算
            vector<Var *> coVar = lv->getCoVar(liveout & mask); // 冲突变量序列，先使用掩码过滤
            for (int j = 0; j < (int)coVar.size() - 1; j++)     // n(n-1)/2个组合
            {
                for (int k = j + 1; k < coVar.size(); k++)
                {
                    nodes[coVar[j]->index]->addLink(nodes[coVar[k]->index]); // 添加关系，不会重复添加
                    nodes[coVar[k]->index]->addLink(nodes[coVar[j]->index]); // 相互添加关系
                }
            }
        }
    }
}

/*
    析够函数
*/
CoGraph::~CoGraph()
{
    for (int i = 0; i < nodes.size(); i++)
    {
        delete nodes[i]; // 清除节点内存
    }
    delete scRoot; // 清除作用域内存
}

/*
    选择度最大的未处理节点，利用最大堆根据节点度堆排序
*/
Node *CoGraph::pickNode()
{
    // 图节点度发生变化，重新构建最大堆
    make_heap(nodes.begin(), nodes.end(), node_less()); // 从小到大O(NlogN) N-->0
    Node *node = nodes.front();                         // 取最大度节点
    return node;
}

/*
    寄存器分配图着色算法,将regNum个寄存器着色到图上
*/
void CoGraph::regAlloc()
{
    Set colorBox = U;                 // 颜色集合
    int nodeNum = nodes.size();       // 节点个数
    for (int i = 0; i < nodeNum; i++) // 处理所有节点
    {
        Node *node = pickNode(); // 选取未处理的最大度节点
        node->paint(colorBox);   // 对该节点着色
    }
}

/*
    根据当前变量的作用域路径获取栈帧偏移地址
*/
int &CoGraph::getEsp(vector<int> &path)
{
    Scope *scope = scRoot;                // 当前作用域初始化为全局作用域
    for (int i = 1; i < path.size(); i++) // 遍历路径
    {
        scope = scope->find(path[i]); // 向下寻找作用域，没有会自动创建！
    }
    return scope->esp; // 返回作用域的偏移的引用
}

/*
    为不能着色的变量分配栈帧地址
*/
void CoGraph::stackAlloc()
{
    int base = Plat::stackBase_protect; // 寄存器分配后需要保护现场，使用保护现场栈帧基址
                                        // 初始化作用域序列
    scRoot = new Scope(0, base);        // 全局作用域,栈起始偏移为ebp-base

    int max = base; // 记录栈帧的最大深度，初始化为base，防止没有内存分配时最大值出错
    // 遍历所有的DEC和ARG指令
    for (list<InterInst *>::iterator i = optCode.begin(); i != optCode.end(); ++i)
    {
        InterInst *inst = *i;        // 遍历指令
        Operator op = inst->getOp(); // 获取操作符
        if (op == OP_DEC)            // 局部变量声明
        {
            Var *arg1 = inst->getArg1();
            if (arg1->regId == -1) // 没有分配到寄存器，计算栈帧地址
            {
                int &esp = getEsp(arg1->getPath()); // 获取变量对应作用与的esp引用
                int size = arg1->getSize();
                size += (4 - size % 4) % 4; // 按照4字节的大小整数倍分配局部变量
                esp += size;                // 累计当前作用域大小
                arg1->setOffset(-esp);      // 局部变量偏移为负数
                if (esp > max)
                    max = esp;
            }
        }
    }
    fun->setMaxDep(max);
}

/*
    分配算法
*/
void CoGraph::alloc()
{
    regAlloc();   // 寄存器分配
    stackAlloc(); // 栈帧地址分配
}