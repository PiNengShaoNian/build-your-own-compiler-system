#include "linker.h"
#include <stdio.h>
#include <string.h>

extern bool showLink;

Block::Block(char *d, unsigned int off, unsigned int s)
{
    data = d;
    offset = off;
    size = s;
}

Block::~Block()
{
    delete[] data;
}

SegList::~SegList()
{
    ownerList.clear();
    for (int i = 0; i < blocks.size(); ++i)
    {
        delete blocks[i];
    }
    blocks.clear();
}

/*
    name:段名
    off:文件偏移地址
    base:加载基址，修改后提供给其他段
*/
void SegList::allocAddr(string name, unsigned int &base, unsigned int &off)
{
    begin = off; // 记录对齐前偏移

    // 虚拟地址对齐，让所有的段按照4k字节对齐
    //.bss段直接紧跟上一个段，一般是.data,因此定义处理段时需要将.data和.bss放在最后
    if (name != ".bss")
        base += (MEM_ALIGN - base % MEM_ALIGN) % MEM_ALIGN;

    // 偏移地址对齐，让一般段按照4字节对齐，文本段按照16字节对齐
    int align = DISC_ALIGN;
    if (name == ".text")
        align = 16;
    off += (align - off % align) % align;
    // 使虚址和偏移按照4k模同余
    base = base - base % MEM_ALIGN + off % MEM_ALIGN;
    // 累加地址和偏移
    baseAddr = base;
    offset = off;
    size = 0;
    for (int i = 0; i < ownerList.size(); ++i)
    {
        size += (DISC_ALIGN - size % DISC_ALIGN) % DISC_ALIGN; // 对齐每个小段，按照4字节
        Elf32_Shdr *seg = ownerList[i]->shdrTab[name];
        // 读取需要合并段的数据
        if (name != ".bss")
        {
            char *buf = new char[seg->sh_size];                       // 申请数据缓存
            ownerList[i]->getData(buf, seg->sh_offset, seg->sh_size); // 读取数据
            blocks.push_back(new Block(buf, size, seg->sh_size));     // 记录数据，对于.bss段数据是空的，不能重定位！没有common段！！！
        }
        // 修改每个文件中对应段的addr
        // 修改每个文件的段虚拟，为了方便计算符号或者重定位的虚址，不需要保存合并后文件偏移
        seg->sh_addr = base + size;
        size += seg->sh_size; // 累加段大小
    }
    base += size;       // 累加基址
    if (name != ".bss") //.bss段不修改偏移
        off += size;
}

Linker::Linker()
{
    segNames.push_back(".text");
    segNames.push_back(".data");
    segNames.push_back(".bss"); //.bss段有尾端对齐功能，不能删除
    for (int i = 0; i < segNames.size(); ++i)
        segLists[segNames[i]] = new SegList();
}

void Linker::addElf(const char *dir)
{
    Elf_file *elf = new Elf_file();
    elf->readElf(dir);   // 读入目标文件，构造elf文件对象
    elfs.push_back(elf); // 添加目标文件对象
}

void Linker::allocAddr()
{
    unsigned int curAddr = BASE_ADDR;                // 当前加载基址
    unsigned int curOff = 52 + 32 * segNames.size(); // 默认文件偏移,PHT保留.bss段
    if (showLink)
        printf("----------地址分配----------\n");

    for (int i = 0; i < segNames.size(); ++i) // 按照类型分配地址，不紧邻.data与.bss段
    {
        segLists[segNames[i]]->allocAddr(segNames[i], curAddr, curOff); // 自动分配
        if (showLink)
            printf("%s\taddr=%08x\toff=%08x\tsize=%08x(%d)\n", segNames[i].c_str(),
                   segLists[segNames[i]]->baseAddr,
                   segLists[segNames[i]]->offset,
                   segLists[segNames[i]]->size,
                   segLists[segNames[i]]->size);
    }
}

Linker::~Linker()
{
    // 清空合并段序列
    for (hash_map<string, SegList *, string_hash>::iterator i = segLists.begin(); i != segLists.end(); ++i)
    {
        delete i->second;
    }
    segLists.clear();

    // 清空符号引用序列
    for (vector<SymLink *>::iterator i = symLinks.begin(); i != symLinks.end(); ++i)
    {
        delete *i;
    }
    symLinks.clear();

    // 清空符号定义序列
    for (vector<SymLink *>::iterator i = symDef.begin(); i != symDef.end(); ++i)
    {
        delete *i;
    }
    symDef.clear();

    // 清空目标文件
    for (int i = 0; i < elfs.size(); ++i)
    {
        delete elfs[i];
    }
    elfs.clear();
}