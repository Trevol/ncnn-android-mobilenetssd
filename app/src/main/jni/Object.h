//
// Created by trevol on 9/7/20.
//

#ifndef NCNN_ANDROID_MOBILENETSSD_OBJECT_H
#define NCNN_ANDROID_MOBILENETSSD_OBJECT_H

#include <jni.h>
#include <vector>

struct Object {
    float x;
    float y;
    float w;
    float h;
    int labelId;
    const char *label;
    float prob;
};

struct ObjectMeta {
    jclass objCls;
    jmethodID constructor;
    jfieldID x;
    jfieldID y;
    jfieldID w;
    jfieldID h;
    jfieldID labelId;
    jfieldID label;
    jfieldID prob;
};

extern "C"
{

JNIEXPORT jboolean JNICALL
Java_com_tencent_mobilenetssdncnn_Obj_jniInit(JNIEnv *env, jclass clazz);

ObjectMeta getObjectMeta();

jobjectArray toJObjArray(JNIEnv *env, jobject thiz, std::vector<Object> objects);

}
#endif //NCNN_ANDROID_MOBILENETSSD_OBJECT_H
