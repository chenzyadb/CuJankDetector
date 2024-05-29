// Based on AndroidPtraceInject by SsageParuders.

#include "utils/ptrace_utils.h"

int inject_remote_process(pid_t pid, const char* lib_path, const char* func_symbol)
{
    if (ptrace_attach(pid) < 0) {
        LOG("[-] ptrace attach failed.");
        return -1;
    }

    struct user_pt_regs cur_regs{};
    struct user_pt_regs orig_regs{};
    if (ptrace_getregs(pid, &orig_regs) < 0) {
        LOG("[-] ptrace getregs failed.");
        ptrace_detach(pid);
        return -1;
    }
    memcpy(&cur_regs, &orig_regs, sizeof(struct user_pt_regs));

    auto mmap_addr = get_mmap_address(pid);
    LOG("[+] mmap remote func addr: %p.", mmap_addr);

    // void* mmap(void* start, size_t length, int prot, int flags, int fd, off_t offsize);
    uint64_t mmap_params[6] = { 0 };
    mmap_params[0] = 0;
    mmap_params[1] = PATH_MAX;
    mmap_params[2] = PROT_READ | PROT_WRITE | PROT_EXEC;
    mmap_params[3] = MAP_ANONYMOUS | MAP_PRIVATE;
    mmap_params[4] = 0;
    mmap_params[5] = 0;
    if (ptrace_call(pid, mmap_addr, mmap_params, 6, &cur_regs) < 0) {
        LOG("[-] ptrace_call mmap func failed.");
        ptrace_detach(pid);
        return -1;
    }
    auto remote_map_mem_addr = (void*)ptrace_getret(&cur_regs);
    LOG("[+] ptrace_call mmap success, return addr: %p, pc: %" PRIu64 ".", remote_map_mem_addr, ptrace_getpc(&cur_regs));

    auto dlopen_addr = get_dlopen_address(pid);
    auto dlsym_addr = get_dlsym_address(pid);
    auto dlclose_addr = get_dlclose_address(pid);
    auto dlerror_addr = get_dlerror_address(pid);
    LOG("[+] libdl func addr: dlopen: %p, dlsym: %p, dlclose: %p, dlerror: %p.", dlopen_addr, dlsym_addr, dlclose_addr, dlerror_addr);

    if (ptrace_writedata(pid, remote_map_mem_addr, lib_path, strlen(lib_path) + 1) < 0) {
        LOG("[-] Write lib_path to remote process error.");
        ptrace_detach(pid);
        return -1;
    }

    // void* dlopen(const char* filename, int flag);
    uint64_t dlopen_params[2] = { 0 };
    dlopen_params[0] = (uint64_t)remote_map_mem_addr;
    dlopen_params[1] = RTLD_NOW | RTLD_GLOBAL;
    if (ptrace_call(pid, dlopen_addr, dlopen_params, 2, &cur_regs) < 0) {
        LOG("[-] ptrace_call dlopen func failed.");
        ptrace_detach(pid);
        return -1;
    }

    auto remote_module_addr = (void*)ptrace_getret(&cur_regs);
    if (remote_module_addr == nullptr) {
        LOG("[-] ptrace_call dlopen error.");
        if (ptrace_call(pid, dlerror_addr, nullptr, 0, &cur_regs) == -1) {
            LOG("[-] ptrace_call dlerror func failed.");
            ptrace_detach(pid);
            return -1;
        }
        auto err_addr = (void*)ptrace_getret(&cur_regs);
        char err_buf[4096] = { 0 };
        ptrace_readdata(pid, err_addr, err_buf, sizeof(err_buf));
        LOG("[-] dlopen error: %s.", err_buf);
        ptrace_detach(pid);
        return -1;
    }
    LOG("[+] ptrace_call dlopen success, remote module dddr: %p.", remote_module_addr);

    if (strlen(func_symbol) > 0) {
        LOG("[+] func symbol is %s.", func_symbol);

        auto func_name_addr = (void*)((uint64_t)remote_map_mem_addr + strlen(lib_path) + 2);
        if (ptrace_writedata(pid, func_name_addr, func_symbol, strlen(func_symbol) + 1) < 0) {
            LOG("[-] Write function name to remote process error.");
            ptrace_detach(pid);
            return -1;
        }

        // void* dlsym(void* handle, const char* symbol);
        uint64_t dlsym_params[2] = { 0 };
        dlsym_params[0] = (uint64_t)remote_module_addr;
        dlsym_params[1] = (uint64_t)func_name_addr;
        if (ptrace_call(pid, dlsym_addr, dlsym_params, 2, &cur_regs) < 0) {
            LOG("[-] ptrace_call dlsym func failed.");
            ptrace_detach(pid);
            return -1;
        }

        auto remote_module_func_addr = (void*)ptrace_getret(&cur_regs);
        LOG("[+] ptrace_call dlsym success, remote module func addr: %p.", remote_module_func_addr);
        if (ptrace_call(pid, remote_module_func_addr, nullptr, 0, &cur_regs) < 0) {
            LOG("[-] ptrace_call injected func failed.");
            ptrace_detach(pid);
            return -1;
        }
    } else {
        LOG("[+] No func_symbol.");
    }

    if (ptrace_setregs(pid, &orig_regs) < 0) {
        LOG("[-] ptrace_setregs error.");
        ptrace_detach(pid);
        return -1;
    }
    ptrace_getregs(pid, &cur_regs);
    if (memcmp(&orig_regs, &cur_regs, sizeof(struct user_pt_regs)) != 0) {
        LOG("[-] Recover regs failed.");
    }
    LOG("[+] Recover regs success.");
    
    ptrace_detach(pid);
    return 0;
}

struct inject_params {
    pid_t pid;
    const char* lib_path;
    const char* func_symbol;
};

struct inject_params process_inject = {0, "", ""};

void handle_params(int argc, char* argv[])
{
    pid_t pid = 0;
    const char* lib_path = nullptr;
    const char* func_symbol = nullptr;

    int count = 0;
    while (count < argc) {
        if (strcmp(argv[count], "-p") == 0) {
            if ((count + 1) >= argc){
                LOG("[-] Missing parameter -p.");
                exit(-1);
            }
            count++;
            pid = atoi(argv[count]);
        } else if (strcmp(argv[count], "-l") == 0) {
            if ((count + 1) >= argc){
                LOG("[-] Missing parameter -l.");
                exit(-1);
            }
            count++;
            lib_path = argv[count];
        } else if (strcmp(argv[count], "-s") == 0) {
            if ((count + 1) >= argc){
                LOG("[-] Missing parameter -s.");
                exit(-1);
            }
            count++;
            func_symbol = argv[count];
        }
        count++;
    }

    if (pid <= 0) {
        LOG("[-] Invaild pid.");
        exit(-1);
    } else {
        process_inject.pid = pid;
        LOG("[+] Inject task pid: %d.", process_inject.pid);
    }

    if (lib_path == nullptr) {
        LOG("[-] Invaild lib_path.");
        exit(-1);
    } else {
        process_inject.lib_path = strdup(lib_path);
        LOG("[+] Inject library path: %s.", process_inject.lib_path);
    }

    if (func_symbol != nullptr) {
        process_inject.func_symbol = strdup(func_symbol);
        LOG("[+] Call function symbol: %s.", process_inject.func_symbol);
    }
}

int init_inject(int argc, char* argv[]) 
{
    handle_params(argc, argv);
    LOG("[+] Finish handle params.");
    return inject_remote_process(process_inject.pid, process_inject.lib_path, process_inject.func_symbol);
}

// -p 目标进程pid
// -l 注入的动态链接库文件路径
// -s 指定动态链接库中的函数名
int main(int argc, char* argv[]) 
{
    LOG("[+] Start Inject.");
    if (init_inject(argc, argv) == 0){
        LOG("[+] Injection Finished.");
    } else {
        LOG("[-] Injection Failed.");
    }
    return 0;
}
