package com.tencent.mobilenetssdncnn;

import android.content.res.AssetManager;
import android.graphics.Bitmap;

public class MobilenetV2SSDliteNcnn {

    public native boolean Init(AssetManager mgr);

    public native Obj[] Detect(Bitmap bitmap, boolean use_gpu);

    static {
        System.loadLibrary("ncnndetectors");
    }
}
