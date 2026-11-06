#include <jni.h>

extern "C" JNIEXPORT jstring JNICALL
Java_io_github_ravi9733_gta5port_MainActivity_nativePortStatus(JNIEnv *env, jobject) {
    return env->NewStringUTF(
        "Android ARM64 shell is building. The iOS engine host and Metal renderer are not "
        "wired into Android yet. Next: port host services, then implement the Vulkan renderer. "
        "This developer build is not playable."
    );
}
