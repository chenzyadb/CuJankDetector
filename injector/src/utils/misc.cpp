#include "misc.h"

struct lib_params process_libs{};

#ifndef __aarch64__

__attribute__((constructor)) 
void handle_libs()
{
    LOG("[-] Unsupported android arch.");
    exit(-1);
}

#else

__attribute__((constructor)) 
void handle_libs()
{
    LOG("[+] Android Ptrace Inject");

    char buffer[PROP_VALUE_MAX] = { 0 };
    __system_property_get("ro.build.version.sdk", buffer);
    int android_sdk_version = atoi(buffer);
    LOG("[+] Android SDK version: %d.", android_sdk_version);

    if (android_sdk_version >= __ANDROID_API_Q__) {
        process_libs.libc_path = "/apex/com.android.runtime/lib64/bionic/libc.so";
        process_libs.linker_path = "/apex/com.android.runtime/bin/linker64";
        process_libs.libdl_path = "/apex/com.android.runtime/lib64/bionic/libdl.so";
    } else if (android_sdk_version >= __ANDROID_API_O__) {
        process_libs.libc_path = "/system/lib64/libc.so";
        process_libs.linker_path = "/system/bin/linker64";
        process_libs.libdl_path = "/system/lib64/libdl.so";
    } else {
        LOG("[-] Unsupported android version.");
        exit(-1);
    }

    LOG("[+] libc_path: %s.", process_libs.libc_path);
    LOG("[+] linker_path: %s.", process_libs.linker_path);
    LOG("[+] libdl_path: %s.", process_libs.libdl_path);
}

#endif
