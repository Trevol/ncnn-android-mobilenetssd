#include <jni.h>
#include <android/log.h>
#include "net.h"

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    __android_log_print(ANDROID_LOG_DEBUG, "ncnndetectors", "JNI_OnLoad");

    ncnn::create_gpu_instance();

    return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    __android_log_print(ANDROID_LOG_DEBUG, "ncnndetectors", "JNI_OnUnload");

    ncnn::destroy_gpu_instance();
}

