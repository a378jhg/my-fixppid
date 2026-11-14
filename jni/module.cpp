#include <jni.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <android/log.h>
#include "zygisk.hpp"

#define LOG_TAG "PPIDSpoofer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

void* zygisk_module_entry_ptr = nullptr; 

static pid_t fake_getppid() {
    return 1000; 
}

void inline_hook_getppid() {
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
    LOGD("Hooked getppid successfully!");
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
                inline_hook_getppid();
            }
            env->ReleaseStringUTFChars(args->nice_name, process);
        }
    }
private:
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(PPIDSpooferModule)
