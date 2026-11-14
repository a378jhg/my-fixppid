#include <jni.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <android/log.h>
#include "zygisk.hpp"

#define LOG_TAG "PPIDSpoofer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

void* zygisk_module_entry_ptr = nullptr;

// ---------- 1. Hook getppid 返回 1000 ----------
static pid_t fake_getppid() {
    return 1000;
}

void hook_getppid() {
    void* handle = dlopen("libc.so", RTLD_LAZY);
    if (!handle) return;
    void* target = dlsym(handle, "getppid");
    if (!target) return;

    uintptr_t page_start = (uintptr_t)target & ~(4096 - 1);
    mprotect((void*)page_start, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);

    uint32_t* code = (uint32_t*)target;
    uintptr_t func_addr = (uintptr_t)fake_getppid;
    code[0] = 0x58000040;
    code[1] = 0xd61f0000;
    *(uintptr_t*)(code + 2) = func_addr;
    __builtin___clear_cache((char*)target, (char*)(code + 4));
    LOGD("Hook getppid 注入成功！");
}

// ---------- 2. Hook fgets 伪造 /proc/self/status 内容 ----------
static char* (*orig_fgets)(char* s, int size, FILE* stream) = nullptr;

char* fake_fgets(char* s, int size, FILE* stream) {
    char* res = orig_fgets(s, size, stream);
    if (res && s) {
        // 将 Seccomp: 0 修正为 Seccomp: 2
        if (strstr(s, "Seccomp:")) {
            char* p = strstr(s, "Seccomp:");
            if (p) p[8] = '2';
        }
        // 同时修正文件内的 PPid 字段
        if (strstr(s, "PPid:")) {
            char* p = strstr(s, "PPid:");
            if (p) {
                char* num = p + 5;
                while (*num && (*num < '0' || *num > '9')) num++;
                if (*num) {
                    int i = 0;
                    for (; i < 4; i++) {
                        if (num[i] >= '0' && num[i] <= '9') num[i] = "1000"[i];
                    }
                    for (; num[i] >= '0' && num[i] <= '9'; i++) {
                        num[i] = ' ';
                    }
                }
            }
        }
    }
    return res;
}

void hook_fgets() {
    void* handle = dlopen("libc.so", RTLD_LAZY);
    if (!handle) return;
    void* target = dlsym(handle, "fgets");
    if (!target) return;

    uintptr_t page_start = (uintptr_t)target & ~(4096 - 1);
    mprotect((void*)page_start, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);

    uint32_t* code = (uint32_t*)target;
    uintptr_t func_addr = (uintptr_t)fake_fgets;
    code[0] = 0x58000040;
    code[1] = 0xd61f0000;
    *(uintptr_t*)(code + 2) = func_addr;
    __builtin___clear_cache((char*)target, (char*)(code + 4));
    LOGD("Hook fgets 注入成功！");
}

// ---------- 模块入口 ----------
class PPIDSpooferModule : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override { this->env = env; }

    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
        if (!args->nice_name) return;
        const char* process = env->GetStringUTFChars(args->nice_name, nullptr);
        if (process) {
            LOGD("当前启动进程: %s", process);
            if (strcmp(process, "com.chunqiunativecheck") == 0) {
                LOGD("匹配春秋检测，开始注入Hook...");
                hook_getppid();
                hook_fgets();
            }
            env->ReleaseStringUTFChars(args->nice_name, process);
        }
    }
private:
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(PPIDSpooferModule)
