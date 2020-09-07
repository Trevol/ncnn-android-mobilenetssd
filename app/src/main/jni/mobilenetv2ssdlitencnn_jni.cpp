//
// Created by trevol on 9/7/20.
//

#include <jni.h>
#include <android/asset_manager_jni.h>
#include <benchmark.h>
#include "net.h"
#include "Object.h"

class Noop : public ncnn::Layer {
};

DEFINE_LAYER_CREATOR(Noop)

static ncnn::Net mobilenetv2;

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_tencent_mobilenetssdncnn_MobilenetV2SSDliteNcnn_Init(JNIEnv *env, jobject thiz,
                                                              jobject assetManager) {
    mobilenetv2.register_custom_layer("Silence", Noop_layer_creator);

    ncnn::UnlockedPoolAllocator g_blob_pool_allocator;
    ncnn::PoolAllocator g_workspace_pool_allocator;

    ncnn::Option opt;
    opt.lightmode = true;
    opt.num_threads = 4;
    opt.blob_allocator = &g_blob_pool_allocator;
    opt.workspace_allocator = &g_workspace_pool_allocator;
    opt.use_packing_layout = true;
    opt.use_vulkan_compute = true;
    mobilenetv2.opt = opt;

    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);

    mobilenetv2.load_param("mobilenetv2_ssdlite_voc.param");
    mobilenetv2.load_model("mobilenetv2_ssdlite_voc.bin");

    {
        int ret = mobilenetv2.load_param(mgr, "mobilenetv2_ssdlite_voc.param");
        if (ret != 0) {
            __android_log_print(ANDROID_LOG_DEBUG, "MobilenetV2SSDliteNcnn", "load_param failed");
            return JNI_FALSE;
        }
    }

    {
        int ret = mobilenetv2.load_model(mgr, "mobilenetv2_ssdlite_voc.bin");
        if (ret != 0) {
            __android_log_print(ANDROID_LOG_DEBUG, "MobilenetV2SSDliteNcnn", "load_model failed");
            return JNI_FALSE;
        }
    }

    // init jni glue
    jclass localObjCls = env->FindClass("com/tencent/mobilenetssdncnn/Obj");
    objectMeta.objCls = reinterpret_cast<jclass>(env->NewGlobalRef(localObjCls));

    objectMeta.constructorId = env->GetMethodID(objectMeta.objCls, "<init>",
                                                "(Lcom/tencent/mobilenetssdncnn/Obj;)V");

    objectMeta.xId = env->GetFieldID(objectMeta.objCls, "x", "F");
    objectMeta.yId = env->GetFieldID(objectMeta.objCls, "y", "F");
    objectMeta.wId = env->GetFieldID(objectMeta.objCls, "w", "F");
    objectMeta.hId = env->GetFieldID(objectMeta.objCls, "h", "F");
    objectMeta.labelId = env->GetFieldID(objectMeta.objCls, "label", "Ljava/lang/String;");
    objectMeta.probId = env->GetFieldID(objectMeta.objCls, "prob", "F");

    return JNI_TRUE;
}

JNIEXPORT jobjectArray JNICALL
Java_com_tencent_mobilenetssdncnn_MobilenetV2SSDliteNcnn_Detect(JNIEnv *env, jobject thiz,
                                                                jobject bitmap,
                                                                jboolean use_gpu) {
    double start_time = ncnn::get_current_time();

    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, bitmap, &info);
    int width = info.width;
    int height = info.height;
    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
        return NULL;

    // ncnn from bitmap
    ncnn::Mat in = ncnn::Mat::from_android_bitmap_resize(env, bitmap, ncnn::Mat::PIXEL_BGR, 300,
                                                         300);

    // mobilenetssd
    std::vector<Object> objects;
    {
        const float mean_vals[3] = {127.5f, 127.5f, 127.5f};
        const float norm_vals[3] = {1.0 / 127.5, 1.0 / 127.5, 1.0 / 127.5};
        in.substract_mean_normalize(mean_vals, norm_vals);

        ncnn::Extractor ex = mobilenetv2.create_extractor();

        ex.set_vulkan_compute(use_gpu);

        ex.input("data", in);

        ncnn::Mat out;
        ex.extract("detection_out", out);

        for (int i = 0; i < out.h; i++) {
            const float *values = out.row(i);

            Object object;
            object.label = values[0];
            object.prob = values[1];
            object.x = values[2] * width;
            object.y = values[3] * height;
            object.w = values[4] * width - object.x;
            object.h = values[5] * height - object.y;

            objects.push_back(object);
        }
    }

    // objects to Obj[]
    static const char *class_names[] = {"background",
                                        "aeroplane", "bicycle", "bird", "boat",
                                        "bottle", "bus", "car", "cat", "chair",
                                        "cow", "diningtable", "dog", "horse",
                                        "motorbike", "person", "pottedplant",
                                        "sheep", "sofa", "train", "tvmonitor"};

    jobjectArray jObjArray = env->NewObjectArray(objects.size(), objectMeta.objCls, NULL);

    for (size_t i = 0; i < objects.size(); i++) {
        jobject jObj = env->NewObject(objectMeta.objCls, objectMeta.constructorId, thiz);

        env->SetFloatField(jObj, objectMeta.xId, objects[i].x);
        env->SetFloatField(jObj, objectMeta.yId, objects[i].y);
        env->SetFloatField(jObj, objectMeta.wId, objects[i].w);
        env->SetFloatField(jObj, objectMeta.hId, objects[i].h);
        env->SetObjectField(jObj, objectMeta.labelId,
                            env->NewStringUTF(class_names[objects[i].label]));
        env->SetFloatField(jObj, objectMeta.probId, objects[i].prob);

        env->SetObjectArrayElement(jObjArray, i, jObj);
    }

    double elasped = ncnn::get_current_time() - start_time;
    __android_log_print(ANDROID_LOG_DEBUG, "MobilenetV2SSDliteNcnn", "%.2fms   detect", elasped);

    return jObjArray;
}

}