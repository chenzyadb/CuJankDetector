#pragma once

#include "misc.h"

inline int ptrace_attach(pid_t pid) noexcept
{
    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) < 0) {
        return -1;
    }
    waitpid(pid, nullptr, WUNTRACED);
    return 0;
}

inline int ptrace_continue(pid_t pid) noexcept
{
    return ptrace(PTRACE_CONT, pid, nullptr, nullptr);
}

inline int ptrace_detach(pid_t pid) noexcept
{
    return ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
}

inline int ptrace_getregs(pid_t pid, struct user_pt_regs* regs) noexcept
{
    struct iovec ioVec{};
    ioVec.iov_base = regs;
    ioVec.iov_len = sizeof(*regs);
    return ptrace(PTRACE_GETREGSET, pid, (void*)NT_PRSTATUS, &ioVec);
}

inline int ptrace_setregs(pid_t pid, struct user_pt_regs* regs) noexcept
{
    struct iovec ioVec{};
    ioVec.iov_base = regs;
    ioVec.iov_len = sizeof(*regs);
    return ptrace(PTRACE_SETREGSET, pid, (void*)NT_PRSTATUS, &ioVec);
}

inline uint64_t ptrace_getret(const struct user_pt_regs* regs) noexcept
{
    return regs->regs[0];
}

inline uint64_t ptrace_getpc(const struct user_pt_regs* regs) noexcept
{
    return regs->pc;
}

inline void* get_mmap_address(pid_t pid) noexcept
{
    return get_remote_func_addr(pid, process_libs.libc_path, (void*)mmap);
}

inline void* get_dlopen_address(pid_t pid) noexcept
{
    return get_remote_func_addr(pid, process_libs.libdl_path, (void*)dlopen);
}

inline void* get_dlclose_address(pid_t pid) noexcept
{
    return get_remote_func_addr(pid, process_libs.libdl_path, (void*)dlclose);
}

inline void* get_dlsym_address(pid_t pid) noexcept
{
    return get_remote_func_addr(pid, process_libs.libdl_path, (void*)dlsym);
}

inline void* get_dlerror_address(pid_t pid) noexcept
{
    return get_remote_func_addr(pid, process_libs.libdl_path, (void*)dlerror);
}

inline int ptrace_readdata(pid_t pid, void* pReadAddr, char* pReadBuf, size_t size) noexcept
{
    size_t nReadOffset = 0;
    size_t nReadCount = size / sizeof(long);
    for (size_t count = 0; count < nReadCount; count++) {
        long lTmpBuf = ptrace(PTRACE_PEEKTEXT, pid, (void*)((uint64_t)pReadAddr + nReadOffset), 0);
        if (lTmpBuf < 0) {
            return -1;
        }
        memcpy((void*)((uint64_t)pReadBuf + nReadOffset), &lTmpBuf, sizeof(long));
        nReadOffset += sizeof(long);
    }
    size_t nRemainByte = size % sizeof(long);
    if (nRemainByte > 0) {
        long lTmpBuf = ptrace(PTRACE_PEEKTEXT, pid, (void*)((uint64_t)pReadAddr + nReadOffset), 0);
        if (lTmpBuf < 0) {
            return -1;
        }
        memcpy((void*)((uint64_t)pReadBuf + nReadOffset), &lTmpBuf, nRemainByte);
    }
    return 0;
}

inline int ptrace_writedata(pid_t pid, void* pWriteAddr, const char* pWriteBuf, size_t size) noexcept
{
    size_t nWriteOffset = 0;
    size_t nWriteCount = size / sizeof(long);
    for (size_t count = 0; count < nWriteCount; count++){
        long lTmpBuf = 0;
        memcpy(&lTmpBuf, (void*)((uint64_t)pWriteBuf + nWriteOffset), sizeof(long));
        if (ptrace(PTRACE_POKETEXT, pid, (void*)((uint64_t)pWriteAddr + nWriteOffset), (void*)lTmpBuf) < 0) {
            return -1;
        }
        nWriteOffset += sizeof(long);
    }
    size_t nRemainByte = size % sizeof(long);
    if (nRemainByte > 0){
        long lTmpBuf = ptrace(PTRACE_PEEKTEXT, pid, (void*)((uint64_t)pWriteAddr + nWriteOffset), 0);
        memcpy(&lTmpBuf, (void*)((uint64_t)pWriteBuf + nWriteOffset), nRemainByte);
        if (ptrace(PTRACE_POKETEXT, pid, (void*)((uint64_t)pWriteAddr + nWriteOffset), lTmpBuf) < 0){
            return -1;
        }
    }
    return 0;
}

inline int ptrace_call(pid_t pid, void* executeAddr, const uint64_t* params, int paramsCount, struct user_pt_regs* regs) noexcept
{
    static constexpr int num_param_regs = sizeof(uint64_t);
    static constexpr uint64_t cpsr_t_mask = (1U << 5);

    for (int count = 0; count < paramsCount && count < num_param_regs; count++){
        regs->regs[count] = params[count];
    }
    if (paramsCount > num_param_regs){
        size_t size = (paramsCount - num_param_regs) * sizeof(long);
        regs->sp -= size;
        if (ptrace_writedata(pid, (void*)regs->sp, (char*)&params[num_param_regs], size) < 0) {
             return -1;
        }
    }

    regs->pc = (uint64_t)executeAddr;
    if (regs->pc & 1){
        regs->pc &= (~1U);
        regs->pstate |= cpsr_t_mask;
    } else {
        regs->pstate &= ~cpsr_t_mask;
    }
    
    regs->regs[30] = (uint64_t)get_module_base_addr(pid, process_libs.libc_path);

    if (ptrace_setregs(pid, regs) < 0 || ptrace_continue(pid) < 0) {
        return -1;
    }

    int stat = 0;
    waitpid(pid, &stat, WUNTRACED);
    while ((stat & 0xFF) != 0x7F){
        if (ptrace_continue(pid) < 0) {
            return -1;
        }
        waitpid(pid, &stat, WUNTRACED);
    }

    if (ptrace_getregs(pid, regs) < 0) {
        return -1;
    }

    return 0;
}
