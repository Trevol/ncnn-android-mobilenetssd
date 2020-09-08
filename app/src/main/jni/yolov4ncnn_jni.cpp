//
// Created by trevol on 9/7/20.
//

#include <jni.h>
#include <android/asset_manager_jni.h>
#include <benchmark.h>
#include "net.h"
#include "Object.h"

static ncnn::Net yolov4;

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_tencent_mobilenetssdncnn_YoloV4Ncnn_Init(JNIEnv *env, jobject thiz,
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
    opt.use_packing_layout = true;
    opt.use_vulkan_compute = true;

    opt.use_winograd_convolution = true;
    opt.use_sgemm_convolution = true;
    opt.use_fp16_packed = true;
    opt.use_fp16_storage = true;
    opt.use_fp16_arithmetic = true;
    opt.use_packing_layout = true;
    opt.use_shader_pack8 = false;
    opt.use_image_storage = false;

    yolov4.opt = opt;


    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);

    {
        int ret = yolov4.load_param(mgr, "yolov4-tiny-opt.param");
        if (ret != 0) {
            __android_log_print(ANDROID_LOG_DEBUG, "YoloV4Ncnn", "load_param failed");
            return JNI_FALSE;
        }
    }

    {
        int ret = yolov4.load_model(mgr, "yolov4-tiny-opt.bin");
        if (ret != 0) {
            __android_log_print(ANDROID_LOG_DEBUG, "YoloV4Ncnn", "load_model failed");
            return JNI_FALSE;
        }
    }

    initialized = true;

    return JNI_TRUE;
}

JNIEXPORT jobjectArray JNICALL
Java_com_tencent_mobilenetssdncnn_YoloV4Ncnn_Detect(JNIEnv *env, jobject thiz,
                                                                jobject bitmap,
                                                                jboolean use_gpu) {
    static const char* class_names[] = {"background", "person", "bicycle",
                                        "car", "motorbike", "aeroplane", "bus", "train", "truck",
                                        "boat", "traffic light", "fire hydrant", "stop sign",
                                        "parking meter", "bench", "bird", "cat", "dog", "horse",
                                        "sheep", "cow", "elephant", "bear", "zebra", "giraffe",
                                        "backpack", "umbrella", "handbag", "tie", "suitcase",
                                        "frisbee", "skis", "snowboard", "sports ball", "kite",
                                        "baseball bat", "baseball glove", "skateboard", "surfboard",
                                        "tennis racket", "bottle", "wine glass", "cup", "fork",
                                        "knife", "spoon", "bowl", "banana", "apple", "sandwich",
                                        "orange", "broccoli", "carrot", "hot dog", "pizza", "donut",
                                        "cake", "chair", "sofa", "pottedplant", "bed", "diningtable",
                                        "toilet", "tvmonitor", "laptop", "mouse", "remote", "keyboard",
                                        "cell phone", "microwave", "oven", "toaster", "sink",
                                        "refrigerator", "book", "clock", "vase", "scissors",
                                        "teddy bear", "hair drier", "toothbrush"
    };

    double start_time = ncnn::get_current_time();

    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, bitmap, &info);
    int width = info.width;
    int height = info.height;
    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
        return NULL;

    // ncnn from bitmap
    ncnn::Mat in = ncnn::Mat::from_android_bitmap_resize(
            env, bitmap, ncnn::Mat::PIXEL_RGB, 416, 416);

    std::vector<Object> objects;
    {
        const float mean_vals[3] = {0, 0, 0};
        const float norm_vals[3] = {1 / 255.f, 1 / 255.f, 1 / 255.f };
        in.substract_mean_normalize(mean_vals, norm_vals);

        ncnn::Extractor ex = yolov4.create_extractor();

        ex.set_vulkan_compute(use_gpu);

        ex.input("data", in);

        ncnn::Mat out;
        ex.extract("output", out);

        for (int i = 0; i < out.h; i++) {
            const float *values = out.row(i);

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
    __android_log_print(ANDROID_LOG_DEBUG, "YoloV4Ncnn", "%.2fms   detect", elasped);

    return jObjArray;
}

}