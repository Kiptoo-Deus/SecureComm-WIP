#include <jni.h>
#include <android/log.h>

#define LOG_TAG "CarrierBridge_Queue"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

extern "C" {

JNIEXPORT jint JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_queueGetPendingCount(
        JNIEnv *env, jclass clazz) {
    LOGD("Getting pending message count");
    return 0;
}

} // extern "C"
