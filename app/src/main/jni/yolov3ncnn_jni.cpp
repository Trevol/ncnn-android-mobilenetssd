//
// Created by trevol on 9/7/20.
//

#include <jni.h>
#include <android/asset_manager_jni.h>
#include <benchmark.h>
#include "net.h"
#include "Object.h"

static ncnn::Net yolov3;

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_tencent_mobilenetssdncnn_YoloV3Ncnn_Init(JNIEnv *env, jobject thiz,
                                                  jobject assetManager) {
    static bool initialized = false;
    if (initialized) {
        return JNI_TRUE;
    }

    static ncnn::UnlockedPoolAllocator g_blob_pool_allocator;
    static ncnn::PoolAllocator g_workspace_pool_allocator;


    ncnn::Option opt;

    opt.lightmode = true;
    opt.num_threads = 4;
    opt.blob_allocator = &g_blob_pool_allocator;
    opt.workspace_allocator = &g_workspace_pool_allocator;
    opt.use_packing_layout = false;
    opt.use_vulkan_compute = true;

//    opt.use_winograd_convolution = true;
//    opt.use_sgemm_convolution = true;
//    opt.use_fp16_packed = true;
//    opt.use_fp16_storage = true;
//    opt.use_fp16_arithmetic = true;
//    opt.use_packing_layout = true;
//    opt.use_shader_pack8 = false;
//    opt.use_image_storage = false;

    yolov3.opt = opt;

    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);

    {
//        const char *param = "mobilenetv2_yolov3.param";
        const char *param = "yolov3-tiny-2cls-320.param";
        int ret = yolov3.load_param(mgr, param);
        if (ret != 0) {
            __android_log_print(ANDROID_LOG_DEBUG, "YoloV3Ncnn", "load_param failed");
            return JNI_FALSE;
        }
    }

    {
//        const char *bin = "mobilenetv2_yolov3.bin";
        const char *bin = "yolov3-tiny-2cls-320.bin";
        int ret = yolov3.load_model(mgr, bin);
        if (ret != 0) {
            __android_log_print(ANDROID_LOG_DEBUG, "YoloV3Ncnn", "load_model failed");
            return JNI_FALSE;
        }
    }

    initialized = true;

    return JNI_TRUE;
}

JNIEXPORT jobjectArray JNICALL
Java_com_tencent_mobilenetssdncnn_YoloV3Ncnn_Detect(JNIEnv *env, jobject thiz,
                                                    jobject bitmap,
                                                    jboolean use_gpu) {
    static const char *class_names[] = {"background",
                                        "aeroplane", "bicycle", "bird", "boat",
                                        "bottle", "bus", "car", "cat", "chair",
                                        "cow", "diningtable", "dog", "horse",
                                        "motorbike", "person", "pottedplant",
                                        "sheep", "sofa", "train", "tvmonitor"};

    double start_time = ncnn::get_current_time();

    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, bitmap, &info);
    int width = info.width;
    int height = info.height;
    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
        return NULL;

    // ncnn from bitmap
//    int targetSize = 352;
//    int targetSize = 320;
//    int targetWidth = 352;
//    int targetHeight = 416;

    int targetWidth = 256;
    int targetHeight = 320;

    ncnn::Mat in = ncnn::Mat::from_android_bitmap_resize(
            env, bitmap, ncnn::Mat::PIXEL_RGB, targetWidth, targetHeight);

    std::vector<Object> objects;
    {
//        const float mean_vals[3] = {127.5f, 127.5f, 127.5f};
//        const float norm_vals[3] = {1.0 / 127.5, 1.0 / 127.5, 1.0 / 127.5};
        const float mean_vals[3] = {0, 0, 0};
        const float norm_vals[3] = {1 / 255., 1 / 255., 1 / 255.};
        in.substract_mean_normalize(mean_vals, norm_vals);

        ncnn::Extractor ex = yolov3.create_extractor();

        ex.set_vulkan_compute(use_gpu);

        ex.input("data", in);

        ncnn::Mat out;
        ex.extract("output", out);

        for (int i = 0; i < out.h; i++) {
            const float *values = out.row(i);

            /*char buff[200];
            snprintf(buff, sizeof(buff), "%f %f %f %f %f %f",
                     values[0],
                     values[1],
                     values[2],
                     values[3],
                     values[4],
                     values[5]);*/

            Object object;
            object.labelId = values[0];
            object.label = class_names[object.labelId];
            object.prob = values[1];
            object.x = values[2] * width;
            object.y = values[3] * height;
            object.w = values[4] * width - object.x;
            object.h = values[5] * height - object.y;

            objects.push_back(object);
        }
    }

    jobjectArray jObjArray = toJObjArray(env, thiz, objects);

    double elasped = ncnn::get_current_time() - start_time;
    __android_log_print(ANDROID_LOG_DEBUG, "YoloV3Ncnn", "%.2fms   detect", elasped);

    return jObjArray;
}

}