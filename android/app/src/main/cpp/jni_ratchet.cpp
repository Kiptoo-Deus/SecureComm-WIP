#include <jni.h>
#include <android/log.h>

#define LOG_TAG "CarrierBridge_Ratchet"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

extern "C" {

JNIEXPORT jbyteArray JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_ratchetGetState(
        JNIEnv *env, jclass clazz) {
    LOGD("Getting ratchet state");
    return nullptr;
}

} // extern "C"
