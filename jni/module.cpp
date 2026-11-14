#include <jni.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <fstream>
#include <string>
#include <map>
#include <android/log.h>
#include "zygisk.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ModuleBase;

#define LOG_TAG "PPIDSpoofer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

static std::map<std::string, pid_t> g_config;

// 假的 getppid
static pid_t fake_getppid() {
    return 1000; 
}

// 极简 ARM64 Inline Hook
void inline_hook_getppid() {
    void* handle = dlopen("libc.so", RTLD_LAZY);
    if (!handle) return;
    void* target = dlsym(handle, "getppid");
    if (!target) return;

    // 允许修改内存页
    uintptr_t page_start = (uintptr_t)target & ~(4096 - 1);
    mprotect((void*)page_start, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);

    // 写入跳转指令：LDR X0, #8; BR X0;
    uint32_t* code = (uint32_t*)target;
    uintptr_t func_addr = (uintptr_t)fake_getppid;
    
    // 覆盖前 16 字节（ARM64典型函数头）
    code[0] = 0x58000040; 
    code[1] = 0xd61f0000; 
    *(uintptr_t*)(code + 2) = func_addr;
    
    __builtin___clear_cache((char*)target, (char*)(code + 4));
    LOGD("Hooked getppid successfully!");
}

class PPIDSpooferModule : public ModuleBase {
public:
    void onLoad(Api* api, JNIEnv* env) override {
        this->api = api;
        this->env = env;
        // 读取配置文件
        std::ifstream file("/data/adb/modules/ppid_spoofer/packages.txt");
        std::string line;
        if (file.is_open()) {
            while (std::getline(file, line)) {
                if (line.empty() || line[0] == '#') continue;
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string pkg = line.substr(0, pos);
                    pid_t pid = std::stoi(line.substr(pos + 1));
                    g_config[pkg] = pid;
                }
            }
            file.close();
        }
    }

    void preAppSpecialize(AppSpecializeArgs* args) override {
        const char* process = env->GetStringUTFChars(args->nice_name, nullptr);
        if (process) {
            LOGD("Checking app: %s", process);
            // 如果这个包名在配置表里
            if (g_config.find(process) != g_config.end()) {
                LOGD("Hooking target: %s with PID %d", process, g_config[process]);
                inline_hook_getppid();
            }
            env->ReleaseStringUTFChars(args->nice_name, process);
        }
    }
private:
    Api* api;
    JNIEnv* env;
};

REGISTER_ZYGISK_MODULE(PPIDSpooferModule)
