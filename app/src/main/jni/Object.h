//
// Created by trevol on 9/7/20.
//

#ifndef NCNN_ANDROID_MOBILENETSSD_OBJECT_H
#define NCNN_ANDROID_MOBILENETSSD_OBJECT_H

#include "../../../../../../../Android/Sdk/ndk/21.3.6528147/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/jni.h"

struct Object {
    float x;
    float y;
    float w;
    float h;
    int label;
    float prob;
};

struct ObjectMeta{
    jclass objCls;
    jmethodID constructorId;
    jfieldID xId;
    jfieldID yId;
    jfieldID wId;
    jfieldID hId;
    jfieldID labelId;
    jfieldID probId;
};

static ObjectMeta objectMeta;

#endif //NCNN_ANDROID_MOBILENETSSD_OBJECT_H
