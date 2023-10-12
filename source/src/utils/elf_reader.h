#pragma once

#include <stdio.h>
#include <string.h>
#include <elf.h>
#include "dobby.h"

void* FindSymbolAddress(const char* path, const char* name)
{
    void* address = nullptr;

    auto exec_fp = fopen(path, "rb");
    if (!exec_fp) {
        return nullptr;
    }

    Elf64_Ehdr ehdr{};
    fread(&ehdr, sizeof(Elf64_Ehdr), 1, exec_fp);
    if (ehdr.e_ehsize == 0) {
        fclose(exec_fp);
        return nullptr;
    }

    Elf64_Shdr strtab_section{};
    Elf64_Shdr symtab_section{};
    for (uint16_t i = 0; i < ehdr.e_shnum; i++) {
        fseek(exec_fp, ehdr.e_shoff + i * ehdr.e_shentsize, SEEK_SET);
        Elf64_Shdr section{};
        fread(&section, sizeof(Elf64_Shdr), 1, exec_fp);
        if (section.sh_type == SHT_STRTAB) {
            memcpy(&strtab_section, &section, sizeof(Elf64_Shdr));
        } else if (section.sh_type == SHT_SYMTAB) {
            memcpy(&symtab_section, &section, sizeof(Elf64_Shdr));
        }
    }
    if (strtab_section.sh_offset == 0 || symtab_section.sh_offset == 0) {
        fclose(exec_fp);
        return nullptr;
    }

    char* strtab_buf = new char[strtab_section.sh_size];
    if (!strtab_buf) {
        fclose(exec_fp);
        return nullptr;
    }
    fseek(exec_fp, strtab_section.sh_offset, SEEK_SET);
    fread(strtab_buf, strtab_section.sh_size, 1, exec_fp);

    size_t sym_count = symtab_section.sh_size / sizeof(Elf64_Sym);
    fseek(exec_fp, symtab_section.sh_offset, SEEK_SET);
    for (size_t i = 0; i < sym_count; i++) {
        Elf64_Sym symbol{};
        fread(&symbol, sizeof(Elf64_Sym), 1, exec_fp);

        char symbol_name[1024] = { 0 };
        strcpy(symbol_name, &strtab_buf[symbol.st_name]);
        if (strstr(symbol_name, name)) {
            address = DobbySymbolResolver(nullptr, symbol_name);
            break;
        }
    }

    delete[] strtab_buf;
    fclose(exec_fp);

    return address;
}