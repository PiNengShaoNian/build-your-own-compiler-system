#include "elf_file.h"
#include <stdio.h>
#include "semantic.h"
#include "common.h"

Elf_file obj; // 输出目标文件，使用fout输出

RelInfo::RelInfo(string seg, int addr, string lb, int t)
{
    offset = addr;
    tarSeg = seg;
    lbName = lb;
    type = t;
}

Elf_file::Elf_file()
{
    shstrtab = NULL;
    strtab = NULL;
    addShdr("", 0, 0, 0, 0, 0, 0, 0, 0, 0); // 空段表项
    addSym("", NULL);                       // 空符号表项
}

int Elf_file::getSegIndex(string segName)
{
    for (int i = 0; i < shdrNames.size(); ++i)
    {
        if (shdrNames[i] == segName) // 找到段
            return i;
    }
    return 0;
}

// sh_name和sh_offset都需要重新计算
void Elf_file::addShdr(string sh_name, int size)
{
    int off = 52 + dataLen;
    if (sh_name == ".text")
    {
        addShdr(sh_name, SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, 0, off, size, 0, 0, 4, 0);
    }
    else if (sh_name == ".data")
    {
        addShdr(sh_name, SHT_PROGBITS, SHF_ALLOC | SHF_WRITE, 0, off, size, 0, 0, 4, 0);
    }
    else if (sh_name == ".bss")
    {
        addShdr(sh_name, SHT_NOBITS, SHF_ALLOC | SHF_WRITE, 0, off, size, 0, 0, 4, 0);
    }
}

void Elf_file::addShdr(string sh_name, Elf32_Word sh_type, Elf32_Word sh_flags, Elf32_Addr sh_addr, Elf32_Off sh_offset,
                       Elf32_Word sh_size, Elf32_Word sh_link, Elf32_Word sh_info, Elf32_Word sh_addralign,
                       Elf32_Word sh_entsize) // 添加一个段表项
{
    Elf32_Shdr *sh = new Elf32_Shdr();
    sh->sh_name = 0;
    sh->sh_type = sh_type;
    sh->sh_flags = sh_flags;
    sh->sh_addr = sh_addr;
    sh->sh_offset = sh_offset;
    sh->sh_size = sh_size;
    sh->sh_link = sh_link;
    sh->sh_info = sh_info;
    sh->sh_addralign = sh_addralign;
    sh->sh_entsize = sh_entsize;
    shdrTab[sh_name] = sh;
    shdrNames.push_back(sh_name);
}

void Elf_file::addSym(lb_record *lb)
{
    // 解析符号的全局性局部性，避免符号冲突
    bool glb = false;
    string name = lb->lbName;
    if (name.find("@lab_") == 0 || name.find("@if_") == 0 || name.find("@while_") == 0 || name.find("@cal_") == 0 || name == "@s_stack")
        return;

    if (lb->segName == ".text") // 代码段
    {
        if (name == "@str2long" || name == "@procBuf")
            glb = true;
        else if (name[0] != '@') // 不带@符号的，都是定义的函数或者_start,全局的
            glb = true;
    }
    else if (lb->segName == ".data") // 数据段
    {
        int index = name.find("@str_");
        if (index == 0) //@str_开头符号
        {
            glb = !(name[5] >= '0' && name[5] <= '9'); // 不是紧跟数字，全局str
        }
        else // 其他类型全局符号
            glb = true;
    }
    else if (lb->segName == "") // 外部符号
    {
        glb = lb->externed; // false
    }
    Elf32_Sym *sym = new Elf32_Sym();
    sym->st_name = 0;
    sym->st_value = lb->addr;                                 // 符号段偏移,外部符号地址为0
    sym->st_size = lb->times * lb->len * lb->cont_len;        // 函数无法通过目前的设计确定，而且不必关心
    if (glb)                                                  // 统一记作无类型符号，和链接器lit协议保持一致
        sym->st_info = ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE); // 全局符号
    else
        sym->st_info = ELF32_ST_INFO(STB_LOCAL, STT_NOTYPE); // 局部符号，避免名字冲突
    sym->st_other = 0;
    if (lb->externed)
        sym->st_shndx = STN_UNDEF;
    else
        sym->st_shndx = getSegIndex(lb->segName);
    addSym(lb->lbName, sym);
}

void Elf_file::addSym(string st_name, Elf32_Sym *s)
{
    Elf32_Sym *sym = symTab[st_name] = new Elf32_Sym();
    if (st_name == "")
    {
        sym->st_name = 0;
        sym->st_value = 0;
        sym->st_size = 0;
        sym->st_info = 0;
        sym->st_other = 0;
        sym->st_shndx = 0;
    }
    else
    {
        sym->st_name = 0;
        sym->st_value = s->st_value;
        sym->st_size = s->st_size;
        sym->st_info = s->st_info;
        sym->st_other = s->st_other;
        sym->st_shndx = s->st_shndx;
    }
    symNames.push_back(st_name);
}

RelInfo *Elf_file::addRel(string seg, int addr, string lb, int type)
{
    RelInfo *rel = new RelInfo(seg, addr, lb, type);
    relTab.push_back(rel);
    return rel;
}

Elf_file::~Elf_file()
{
    // 清空段表
    for (hash_map<string, Elf32_Shdr *, string_hash>::iterator i = shdrTab.begin(); i != shdrTab.end(); ++i)
    {
        delete i->second;
    }

    shdrTab.clear();
    shdrNames.clear();

    // 清空符号表
    for (hash_map<string, Elf32_Sym *, string_hash>::iterator i = symTab.begin(); i != symTab.end(); ++i)
    {
        delete i->second;
    }
    symTab.clear();

    // 清空重定位表
    for (vector<RelInfo *>::iterator i = relTab.begin(); i != relTab.end(); ++i)
    {
        delete *i;
    }
    relTab.clear();
    // 清空缓冲区
    if (shstrtab)
        delete shstrtab;
    if (strtab)
        delete strtab;
}

void Elf_file::printAll()
{
    if (!showAss)
        return;
    cout << "------------段信息------------\n";
    for (hash_map<string, Elf32_Shdr *, string_hash>::iterator i = shdrTab.begin(); i != shdrTab.end(); ++i)
    {
        if (i->first == "")
            continue;
        cout << i->first << ":" << i->second->sh_size << endl;
    }
    cout << "------------符号信息------------\n";
    for (hash_map<string, Elf32_Sym *, string_hash>::iterator i = symTab.begin(); i != symTab.end(); ++i)
    {
        if (i->first == "")
            continue;
        cout << i->first << ":";
        if (i->second->st_shndx == 0)
            cout << "外部";
        if (ELF32_ST_BIND(i->second->st_info) == STB_GLOBAL)
            cout << "全局";
        else if (ELF32_ST_BIND(i->second->st_info) == STB_LOCAL)
            cout << "局部";
        cout << endl;
    }
    cout << "------------重定位信息------------\n";
    for (vector<RelInfo *>::iterator i = relTab.begin(); i != relTab.end(); ++i)
    {
        cout << (*i)->tarSeg << ":" << (*i)->offset << "<-" << (*i)->lbName << endl;
    }
}