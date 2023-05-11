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