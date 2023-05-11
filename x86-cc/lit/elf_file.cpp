#include "elf_file.h"
#include <stdio.h>
#include <string.h>

extern bool showLink;

RelItem::RelItem(string sname, Elf32_Rel *r, string rname)
{
    segName = sname;
    rel = r;
    relName = rname;
}

RelItem::~RelItem()
{
    delete rel;
}

void Elf_file::getData(char *buf, Elf32_Off offset, Elf32_Word size)
{
    FILE *fp = fopen(elf_dir, "rb");
    rewind(fp);
    fseek(fp, offset, 0);    // 读取位置
    fread(buf, size, 1, fp); // 读取数据
    fclose(fp);
}

Elf_file::Elf_file()
{
    shstrtab = NULL;
    strtab = NULL;
    elf_dir = NULL;
}

void Elf_file::readElf(const char *dir)
{
    string d = dir;
    elf_dir = new char[d.length() + 1];
    strcpy(elf_dir, dir);
    FILE *fp = fopen(dir, "rb");
    rewind(fp);
    fread(&ehdr, sizeof(Elf32_Ehdr), 1, fp); // 读取文件头

    if (ehdr.e_type == ET_EXEC) // 可执行文件拥有程序头表
    {
        fseek(fp, ehdr.e_phoff, 0);            // 程序头表位置
        for (int i = 0; i < ehdr.e_phnum; ++i) // 读取程序头表
        {
            Elf32_Phdr *phdr = new Elf32_Phdr();
            fread(phdr, ehdr.e_phentsize, 1, fp); // 读取程序头
            phdrTab.push_back(phdr);              // 加入程序头表
        }
    }

    fseek(fp, ehdr.e_shoff + ehdr.e_shentsize * ehdr.e_shstrndx, 0); // 段表字符串表位置
    Elf32_Shdr shstrTab;
    fread(&shstrTab, ehdr.e_shentsize, 1, fp); // 读取段表字符串表项
    char *shstrTabData = new char[shstrTab.sh_size];
    fseek(fp, shstrTab.sh_offset, 0);             // 转移到段表字符串表内容
    fread(shstrTabData, shstrTab.sh_size, 1, fp); // 读取段表字符串表

    fseek(fp, ehdr.e_shoff, 0);            // 段表位置
    for (int i = 0; i < ehdr.e_shnum; ++i) // 读取段表
    {
        Elf32_Shdr *shdr = new Elf32_Shdr();
        fread(shdr, ehdr.e_shentsize, 1, fp); // 读取段表项[非空]
        string name(shstrTabData + shdr->sh_name);
        shdrNames.push_back(name);
        if (name.empty())
            delete shdr; // 删除空段表项
        else
        {
            shdrTab[name] = shdr; // 加入段表
        }
    }
    delete[] shstrTabData; // 清空段表字符串表

    Elf32_Shdr *strTab = shdrTab[".strtab"]; // 字符串表信息
    char *strTabData = new char[strTab->sh_size];
    fseek(fp, strTab->sh_offset, 0);           // 转移到字符串表内容
    fread(strTabData, strTab->sh_size, 1, fp); // 读取字符串表

    Elf32_Shdr *sh_symTab = shdrTab[".symtab"]; // 符号表信息
    fseek(fp, sh_symTab->sh_offset, 0);         // 转移到符号表内容
    int symNum = sh_symTab->sh_size / 16;       // 符号个数[非空],16最好用2**sh_symTab->sh_entsize代替
    vector<Elf32_Sym *> symList;                // 按照序列记录符号表所有信息，方便重定位符号查询
    for (int i = 0; i < symNum; ++i)            // 读取符号
    {
        Elf32_Sym *sym = new Elf32_Sym();
        fread(sym, 16, 1, fp);  // 读取符号项[非空]
        symList.push_back(sym); // 添加到符号序列
        string name(strTabData + sym->st_name);
        if (name.empty()) // 无名符号，对于链接没有意义,按照链接器设计需要记录全局和局部符号，避免名字冲突
            delete sym;   // 删除空符号项
        else
        {
            symTab[name] = sym; // 加入符号表
        }
    }

    if (showLink)
        printf("----------%s重定位数据:----------\n", elf_dir);

    for (hash_map<string, Elf32_Shdr *, string_hash>::iterator i = shdrTab.begin(); i != shdrTab.end(); ++i) // 所有段的重定位项整合
    {
        if (i->first.find(".rel") == 0) // 是重定位段
        {
            Elf32_Shdr *sh_relTab = shdrTab[i->first]; // 重定位表信息
            fseek(fp, sh_relTab->sh_offset, 0);        // 转移到重定位表内容
            int relNum = sh_relTab->sh_size / 8;       // 重定位项数
            for (int j = 0; j < relNum; ++j)
            {
                Elf32_Rel *rel = new Elf32_Rel();
                fread(rel, 8, 1, fp);                                                 // 读取重定位项
                string name(strTabData + symList[ELF32_R_SYM(rel->r_info)]->st_name); // 获得重定位符号名字

                // 使用shdrNames[sh_relTab->sh_info]访问目标段更标准
                relTab.push_back(new RelItem(i->first.substr(4), rel, name)); // 添加重定位项
                if (showLink)
                    printf("%s\t%08x\t%s\n", i->first.substr(4).c_str(), rel->r_offset, name.c_str());
            }
        }
    }

    delete[] strTabData; // 清空字符串表
    fclose(fp);
}

Elf_file::~Elf_file()
{
    // 清空程序头表
    for (vector<Elf32_Phdr *>::iterator i = phdrTab.begin(); i != phdrTab.end(); ++i)
    {
        delete *i;
    }
    phdrTab.clear();

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
    for (vector<RelItem *>::iterator i = relTab.begin(); i != relTab.end(); ++i)
    {
        delete *i;
    }
    relTab.clear();

    // 清空临时存储数据
    if (shstrtab != NULL)
        delete[] shstrtab;
    if (strtab != NULL)
        delete[] strtab;
    if (elf_dir)
        delete elf_dir;
}