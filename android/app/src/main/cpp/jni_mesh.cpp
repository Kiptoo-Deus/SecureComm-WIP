#include <jni.h>
#include <android/log.h>

#define LOG_TAG "CarrierBridge_Mesh"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_meshStartDiscovery(
        JNIEnv *env, jclass clazz) {
    LOGD("Starting mesh discovery");
    return JNI_TRUE;
}

} // extern "C"
