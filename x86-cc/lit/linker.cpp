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
    if (name != ".bss") //.bss段直接紧跟上一个段，一般是.data,因此定义处理段时需要将.data和.bss放在最后
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
            // 记录数据，对于.bss段数据是空的，不能重定位！没有common段！！！
            blocks.push_back(new Block(buf, size, seg->sh_size));
        }
        // 修改每个文件中对应段的addr
        seg->sh_addr = base + size; // 修改每个文件的段虚拟，为了方便计算符号或者重定位的虚址，不需要保存合并后文件偏移
        size += seg->sh_size;       // 累加段大小
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

void Linker::collectInfo()
{
    for (int i = 0; i < elfs.size(); ++i) // 扫描输入文件
    {
        Elf_file *elf = elfs[i];
        // 记录段表信息
        for (int i = 0; i < segNames.size(); ++i)
        {
            if (elf->shdrTab.find(segNames[i]) != elf->shdrTab.end())
                segLists[segNames[i]]->ownerList.push_back(elf);
        }

        // 记录符号引用信息
        for (hash_map<string, Elf32_Sym *, string_hash>::iterator symIt = elf->symTab.begin();
             symIt != elf->symTab.end(); ++symIt) // 所搜该文件的所有有用符号
        {
            SymLink *symLink = new SymLink();
            symLink->name = symIt->first;             // 记录名字
            if (symIt->second->st_shndx == STN_UNDEF) // 引用符号
            {
                symLink->recv = elf;  // 记录引用文件
                symLink->prov = NULL; // 标记未定义
                symLinks.push_back(symLink);
            }
            else
            {
                symLink->prov = elf;  // 记录定义文件
                symLink->recv = NULL; // 标示该定义符号未被任何文件引用
                symDef.push_back(symLink);
            }
        }
    }
}

bool Linker::symValid()
{
    bool flag = true;
    startOwner = NULL;
    for (int i = 0; i < symDef.size(); ++i)
    {
        // 只考虑全局符号
        if (ELF32_ST_BIND(symDef[i]->prov->symTab[symDef[i]->name]->st_info) != STB_GLOBAL)
            continue;

        if (symDef[i]->name == START) // 记录程序入口文件
            startOwner = symDef[i]->prov;

        for (int j = i + 1; j < symDef.size(); ++j) // 遍历后边定义的符号
        {
            if (ELF32_ST_BIND(symDef[j]->prov->symTab[symDef[j]->name]->st_info) != STB_GLOBAL) // 只考虑全局符号
                continue;

            if (symDef[i]->name == symDef[j]->name)
            {
                printf("符号名%s在文件%s和文件%s中发生链接冲突。\n",
                       symDef[i]->name.c_str(),
                       symDef[i]->prov->elf_dir,
                       symDef[j]->prov->elf_dir);
                flag = false;
            }
        }
    }
    if (startOwner == NULL)
    {
        printf("链接器找不到程序入口%s。\n", START);
        flag = false;
    }
    for (int i = 0; i < symLinks.size(); ++i) // 遍历未定义符号
    {
        for (int j = 0; j < symDef.size(); ++j) // 遍历定义的符号
        {
            if (ELF32_ST_BIND(symDef[j]->prov->symTab[symDef[j]->name]->st_info) != STB_GLOBAL) // 只考虑全局符号
                continue;

            if (symLinks[i]->name == symDef[j]->name) // 同名符号
            {
                symLinks[i]->prov = symDef[j]->prov; // 记录符号定义的文件信息
                symDef[j]->recv = symDef[j]->prov;   // 该赋值没有意义，只是保证recv不为NULL
            }
        }
        if (symLinks[i]->prov == NULL) // 未定义
        {
            unsigned char info = symLinks[i]->recv->symTab[symLinks[i]->name]->st_info;
            string type;
            if (ELF32_ST_TYPE(info) == STT_OBJECT)
                type = "变量";
            else if (ELF32_ST_TYPE(info) == STT_FUNC)
                type = "函数";
            else
                type = "符号";
            printf("文件%s的%s名%s未定义。\n", symLinks[i]->recv->elf_dir, type.c_str(), symLinks[i]->name.c_str());
            if (flag)
                flag = false;
        }
    }
    return flag;
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

void Linker::symParser()
{
    // 扫描所有定义符号，原地计算虚拟地址
    if (showLink)
        printf("----------定义符号解析----------\n");
    for (int i = 0; i < symDef.size(); ++i)
    {
        Elf32_Sym *sym = symDef[i]->prov->symTab[symDef[i]->name];  // 定义的符号信息
        string segName = symDef[i]->prov->shdrNames[sym->st_shndx]; // 段名
        sym->st_value = sym->st_value +                             // 偏移
                        symDef[i]->prov->shdrTab[segName]->sh_addr; // 段基址
        if (showLink)
            printf("%s\t%08x\t%s\n", symDef[i]->name.c_str(),
                   sym->st_value,
                   symDef[i]->prov->elf_dir);
    }

    // 扫描所有符号引用，绑定虚拟地址
    if (showLink)
        printf("----------未定义符号解析----------\n");
    for (int i = 0; i < symLinks.size(); ++i)
    {
        Elf32_Sym *provsym = symLinks[i]->prov->symTab[symLinks[i]->name]; // 被引用的符号信息
        Elf32_Sym *recvsym = symLinks[i]->recv->symTab[symLinks[i]->name]; // 被引用的符号信息
        recvsym->st_value = provsym->st_value;                             // 被引用符号已经解析了
        if (showLink)
            printf("%s\t%08x\t%s\n", symLinks[i]->name.c_str(),
                   recvsym->st_value, symLinks[i]->recv->elf_dir);
    }
}

bool Linker::link(const char *dir)
{
    collectInfo();   // 搜集段/符号信息
    if (!symValid()) // 符号引用验证
        return false;
    allocAddr(); // 分配地址空间
    symParser(); // 符号地址解析
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