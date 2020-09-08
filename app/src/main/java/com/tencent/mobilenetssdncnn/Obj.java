package com.tencent.mobilenetssdncnn;

public class Obj {
    public float x;
    public float y;
    public float w;
    public float h;
    public int labelId;
    public String label;
    public float prob;

    public static native boolean jniInit();

    static {
        System.loadLibrary("ncnndetectors");
        jniInit();
    }
}
