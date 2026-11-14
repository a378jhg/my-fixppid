#include <jni.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <android/log.h>
#include <dobby.h>
#include "zygisk.hpp"

#define LOG_TAG "PPIDSpoofer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

void* zygisk_module_entry_ptr = nullptr; 

// 1. 伪造 getppid 系统调用
static pid_t (*orig_getppid)();
static pid_t fake_getppid() {
    return 1000; 
}

// 2. 拦截文件读取，伪造 /proc/self/status 内容
static char* (*orig_fgets)(char* s, int size, FILE* stream);
static char* fake_fgets(char* s, int size, FILE* stream) {
    char* res = orig_fgets(s, size, stream);
    if (res && s) {
        // 拦截 Seccomp 状态
        if (strstr(s, "Seccomp:")) {
            char* p = strstr(s, "Seccomp:");
            if (p) p[8] = '2'; // 把 0 改成 2
        }
        // 拦截父进程 PID
        if (strstr(s, "PPid:")) {
            char* p = strstr(s, "PPid:");
            if (p) {
                // 找到数字部分，强行覆盖为 1000
                char* num = p + 5;
                while (*num && (*num < '0' || *num > '9')) num++;
                if (*num) {
                    int i = 0;
                    for (; i < 4; i++) {
                        if (num[i] >= '0' && num[i] <= '9') num[i] = "1000"[i];
                    }
                    // 把多出来的位数用空格填满，防止结构错乱
                    for (; num[i] >= '0' && num[i] <= '9'; i++) {
                        num[i] = ' ';
                    }
                }
            }
        }
    }
    return res;
}

class PPIDSpooferModule : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override {
        this->env = env;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
        const char* process = env->GetStringUTFChars(args->nice_name, nullptr);
        if (process) {
            if (strcmp(process, "com.chunqiunativecheck") == 0) {
                LOGD("Hooking target: %s", process);
                
                // 使用 Dobby 进行专业 Hook
                void* handle = dlopen("libc.so", RTLD_LAZY);
                if (handle) {
                    DobbyHook(dlsym(handle, "getppid"), (void*)fake_getppid, (void**)&orig_getppid);
                    DobbyHook(dlsym(handle, "fgets"), (void*)fake_fgets, (void**)&orig_fgets);
                    LOGD("Hooks installed successfully!");
                }
            }
            env->ReleaseStringUTFChars(args->nice_name, process);
        }
    }
private:
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(PPIDSpooferModule)
