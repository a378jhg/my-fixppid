#pragma once
#include <jni.h>

namespace zygisk {
    struct Api { void *impl; };
    struct AppSpecializeArgs {
        jint uid;
        jint gid;
        jintArray gids;
        jint runtime_flags;
        jobjectArray rlimits;
        jstring nice_name; // 关键：必须包含此项，否则读不到包名
        jint mount_external;
        jint se_info;
    };
    struct ServerSpecializeArgs {};
    class ModuleBase {
    public:
        virtual void onLoad(Api *api, JNIEnv *env) {}
        virtual void preAppSpecialize(AppSpecializeArgs *args) {}
        virtual void postAppSpecialize(const AppSpecializeArgs *args) {}
        virtual void preServerSpecialize(ServerSpecializeArgs *args) {}
        virtual void postServerSpecialize(const ServerSpecializeArgs *args) {}
    };
}

extern void* zygisk_module_entry_ptr;

#define REGISTER_ZYGISK_MODULE(clazz) \
    static clazz instance; \
    extern "C" [[gnu::visibility("default")]] void zygisk_module_entry(zygisk::Api *api, JNIEnv *env) { \
        instance.onLoad(api, env); \
        zygisk_module_entry_ptr = &instance; \
}
