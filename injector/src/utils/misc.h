#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include <dirent.h>
#include <elf.h>
#include <fcntl.h>
#include <asm/unistd.h>
#include <asm/ptrace.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/system_properties.h>

#define LOG(...)\
    do {\
        printf(__VA_ARGS__);\
        printf("\n");\
    } while (0)


struct lib_params {
    const char* libc_path;
    const char* linker_path;
    const char* libdl_path;
};

extern lib_params process_libs;


inline void* get_module_base_addr(pid_t pid, const char* module_name) noexcept
{
    void* module_base_addr = nullptr;
    char maps_path[128] = "/proc/self/maps";
    if (pid > 0) {
        snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    }
    auto fp = fopen(maps_path, "r");
    if (fp != nullptr) {
        char buffer[1024] = { 0 };
        while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
            if (strstr(buffer, module_name) != nullptr) {
                auto divide_ptr = strchr(buffer, '-');
                if (divide_ptr != nullptr) {
                    char addr_str[1024] = { 0 };
                    size_t addr_str_len = divide_ptr - &buffer[0];
                    memcpy(addr_str, buffer, addr_str_len);
                    addr_str[addr_str_len] = '\0';
                    auto module_addr = (void*)strtoul(addr_str, nullptr, 16);
                    if (module_addr != (void*)ULLONG_MAX) {
                        module_base_addr = module_addr;
                        break;
                    }
                }
            }
        }
        fclose(fp);
    }
    return module_base_addr;
}

inline void* get_remote_func_addr(pid_t pid, const char* module_name, void* local_func_addr) noexcept
{
    auto local_module_addr = get_module_base_addr(0, module_name);
    auto remote_module_addr = get_module_base_addr(pid, module_name);
    return (void*)((uint64_t)remote_module_addr + ((uint64_t)local_func_addr - (uint64_t)local_module_addr));
}
