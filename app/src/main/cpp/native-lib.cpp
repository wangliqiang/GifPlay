#include <jni.h>
#include <string>
#include <malloc.h>
#include <cstring>
#include "gif_lib.h"
#include <android/log.h>
#include <android/bitmap.h>

#define  LOG_TAG    "GifPlay"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  argb(a, r, g, b) ( ((a) & 0xff) << 24 ) | ( ((b) & 0xff) << 16 ) | ( ((g) & 0xff) << 8 ) | ((r) & 0xff)

typedef struct GifBean {
    int current_frame;// 当前帧数
    int total_frame;// 总帧数
    int *delays;// 单帧延迟时间数组
} GifBean;

// 绘制图片
void drawFrame(GifFileType *gifFileType, GifBean *gifBean, AndroidBitmapInfo info, void *pixels) {
    // 获取当前帧
    SavedImage savedImage = gifFileType->SavedImages[gifBean->current_frame];
    // 图片地址
    int *px = (int *) pixels;
    int pointPixels;
    GifImageDesc frameInfo = savedImage.ImageDesc;
    GifByteType gifByteType;// 压缩数据

    // RGB数据
    ColorMapObject *colorMapObject = frameInfo.ColorMap;
    // Bitmap往下偏移
    px = (int *) ((char *) px + info.stride * frameInfo.Top);

    int *line;// 每行首地址
    for (int y = frameInfo.Top; y < frameInfo.Top + frameInfo.Height; ++y) {
        line = px;
        for (int x = frameInfo.Left; x < frameInfo.Left + frameInfo.Width; ++x) {
            // 每个坐标的位置
            pointPixels = (y - frameInfo.Top) * frameInfo.Width + (x - frameInfo.Left);
            // 索引
            gifByteType = savedImage.RasterBits[pointPixels];
            GifColorType gifColorType = colorMapObject->Colors[gifByteType];
            line[x] = argb(255, gifColorType.Red, gifColorType.Green, gifColorType.Blue);
        }
        px = (int *) ((char *) px + info.stride);
    }
}

extern "C"
JNIEXPORT jlong

JNICALL
Java_com_app_gifplay_GifHandler_loadPath(JNIEnv *env, jobject instance, jstring path_) {
    const char *path = env->GetStringUTFChars(path_, 0);
    int err;
    GifFileType *gifFileType = DGifOpenFileName(path, &err);

    DGifSlurp(gifFileType);

    GifBean *gifBean = (GifBean *) malloc(sizeof(GifBean));
    // 清空内存地址
    memset(gifBean, 0, sizeof(GifBean));
    gifFileType->UserData = gifBean;

    // 初始化数组
    gifBean->delays = (int *) malloc(sizeof(int) * gifFileType->ImageCount);
    memset(gifBean->delays, 0, sizeof(int) * gifFileType->ImageCount);

    // 延迟时间获取
    // Delay Time = 单位1/100秒，如果值不为1，表示暂停规定的时间后再继续往下处理数据流
    gifFileType->UserData = gifBean;
    gifBean->current_frame = 0;
    gifBean->total_frame = gifFileType->ImageCount;
    ExtensionBlock *ext;
    for (int i = 0; i < gifFileType->ImageCount; ++i) {
        SavedImage frame = gifFileType->SavedImages[i];
        for (int j = 0; j < frame.ExtensionBlockCount; ++j) {
            if (frame.ExtensionBlocks[j].Function == GRAPHICS_EXT_FUNC_CODE) {
                ext = &frame.ExtensionBlocks[j];
                break;
            }
        }
        if (ext) {
            int frame_delay = 10 * (ext->Bytes[1] | (ext->Bytes[2] << 8));
            LOGE("时间  %d  ", frame_delay);
            gifBean->delays[i] = frame_delay;
        }
    }
    LOGE("gif  长度大小  %d  ", gifFileType->ImageCount);

    env->ReleaseStringUTFChars(path_, path);
    return (jlong)
    gifFileType;
}

extern "C"
JNIEXPORT jint

JNICALL
Java_com_app_gifplay_GifHandler_getWidth(JNIEnv *env, jobject thiz, jlong ndk_gif) {
    GifFileType *gifFileType = (GifFileType *) ndk_gif;
    return gifFileType->SWidth;
}

extern "C"
JNIEXPORT jint

JNICALL
Java_com_app_gifplay_GifHandler_getHeight(JNIEnv *env, jobject thiz, jlong ndk_gif) {
    GifFileType *gifFileType = (GifFileType *) ndk_gif;
    return gifFileType->SHeight;
}

extern "C"
JNIEXPORT jint

JNICALL
Java_com_app_gifplay_GifHandler_updateFrame(JNIEnv *env, jobject thiz, jlong ndk_gif,
                                            jobject bitmap) {
    GifFileType *gifFileType = (GifFileType *) ndk_gif;
    GifBean *gifBean = (GifBean *) gifFileType->UserData;

    AndroidBitmapInfo info;
    // 像素数组
    AndroidBitmap_getInfo(env, bitmap, &info);

    void *pixels;
    AndroidBitmap_lockPixels(env, bitmap, &pixels);

    drawFrame(gifFileType, gifBean, info, pixels);

    gifBean->current_frame += 1;
    if (gifBean->current_frame >= gifBean->total_frame - 1) {
        gifBean->current_frame = 0;
        LOGE("重新开始   %d   ", gifBean->current_frame);
    }
    AndroidBitmap_unlockPixels(env, bitmap);

    return gifBean->delays[gifBean->current_frame];
}