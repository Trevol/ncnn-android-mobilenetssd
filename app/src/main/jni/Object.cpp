#include <jni.h>
#include "Object.h"

ObjectMeta objectMeta;

extern "C"
{

ObjectMeta getObjectMeta() {
    return objectMeta;
}

jobjectArray toJObjArray(JNIEnv *env, jobject thiz, std::vector<Object> objects){
    auto meta = getObjectMeta();
    jobjectArray jObjArray = env->NewObjectArray(objects.size(), meta.objCls, NULL);

    for (size_t i = 0; i < objects.size(); i++) {
        jobject jObj = env->NewObject(meta.objCls, meta.constructor, thiz);

        env->SetFloatField(jObj, meta.x, objects[i].x);
        env->SetFloatField(jObj, meta.y, objects[i].y);
        env->SetFloatField(jObj, meta.w, objects[i].w);
        env->SetFloatField(jObj, meta.h, objects[i].h);
        env->SetIntField(jObj, meta.labelId, objects[i].labelId);
        env->SetObjectField(jObj, meta.label, env->NewStringUTF(objects[i].label));
        env->SetFloatField(jObj, meta.prob, objects[i].prob);

        env->SetObjectArrayElement(jObjArray, i, jObj);
    }

    return jObjArray;
}

JNIEXPORT jboolean JNICALL
Java_com_tencent_mobilenetssdncnn_Obj_jniInit(JNIEnv *env, jclass clazz) {
    static bool initialized = false;
    if (initialized) {
        return JNI_TRUE;
    }
    jclass localObjCls = env->FindClass("com/tencent/mobilenetssdncnn/Obj");
    objectMeta.objCls = reinterpret_cast<jclass>(env->NewGlobalRef(localObjCls));

    objectMeta.constructor = env->GetMethodID(objectMeta.objCls, "<init>",
                                              "()V");

    objectMeta.x = env->GetFieldID(objectMeta.objCls, "x", "F");
    objectMeta.y = env->GetFieldID(objectMeta.objCls, "y", "F");
    objectMeta.w = env->GetFieldID(objectMeta.objCls, "w", "F");
    objectMeta.h = env->GetFieldID(objectMeta.objCls, "h", "F");
    objectMeta.labelId = env->GetFieldID(objectMeta.objCls, "labelId", "I");
    objectMeta.label = env->GetFieldID(objectMeta.objCls, "label", "Ljava/lang/String;");
    objectMeta.prob = env->GetFieldID(objectMeta.objCls, "prob", "F");

    initialized = true;

    return JNI_TRUE;
}
}

