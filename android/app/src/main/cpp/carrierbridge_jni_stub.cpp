#include <jni.h>
#include <android/log.h>
#include <string>
#include <vector>

#define LOG_TAG "carrierbridge_jni_stub"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static JavaVM* g_jvm = nullptr;
static jobject g_messageCallback = nullptr;
static bool g_initialized = false;

jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    g_jvm = vm;
    LOGI("JNI_OnLoad called");
    return JNI_VERSION_1_6;
}

extern "C" {

// Signature: long initDispatcher(String)
JNIEXPORT jlong JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_initDispatcher(JNIEnv* env, jclass /*cls*/, jstring deviceId) {
    const char* id = deviceId ? env->GetStringUTFChars(deviceId, nullptr) : "<null>";
    LOGI("initDispatcher called for deviceId=%s", id);
    if (deviceId) env->ReleaseStringUTFChars(deviceId, id);
    g_initialized = true;
    // return a fake handle
    return (jlong)1;
}

// boolean createSession(String)
JNIEXPORT jboolean JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_createSession(JNIEnv* env, jclass /*cls*/, jstring /*remoteDeviceId*/) {
    LOGI("createSession (stub) called");
    return JNI_TRUE;
}

// boolean sendMessage(String, byte[])
JNIEXPORT jboolean JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_sendMessage(JNIEnv* env, jclass /*cls*/, jstring /*recipientId*/, jbyteArray /*plaintext*/) {
    LOGI("sendMessage (stub) called");
    return JNI_TRUE;
}

// void setInboundCallback(Object)
JNIEXPORT void JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_setInboundCallback(JNIEnv* env, jclass /*cls*/, jobject callback) {
    LOGI("setInboundCallback (stub) called");
    if (g_messageCallback) {
        env->DeleteGlobalRef(g_messageCallback);
        g_messageCallback = nullptr;
    }
    if (callback) {
        g_messageCallback = env->NewGlobalRef(callback);
    }
}

// void stopDispatcher()
JNIEXPORT void JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_stopDispatcher(JNIEnv* env, jclass /*cls*/) {
    LOGI("stopDispatcher (stub) called");
    g_initialized = false;
    if (g_messageCallback) {
        env->DeleteGlobalRef(g_messageCallback);
        g_messageCallback = nullptr;
    }
}

// String getVersion()
JNIEXPORT jstring JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_getVersion(JNIEnv* env, jclass /*cls*/) {
    LOGI("getVersion (stub) called");
    return env->NewStringUTF("carrierbridge_jni-stub-1.0");
}

// boolean dispatcherIsInitialized()
JNIEXPORT jboolean JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_dispatcherIsInitialized(JNIEnv* env, jclass /*cls*/) {
    return g_initialized ? JNI_TRUE : JNI_FALSE;
}

// byte[] ratchetGetState()
JNIEXPORT jbyteArray JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_ratchetGetState(JNIEnv* env, jclass /*cls*/) {
    return nullptr;
}

// boolean transportConnect(String)
JNIEXPORT jboolean JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_transportConnect(JNIEnv* env, jclass /*cls*/, jstring /*url*/) {
    return JNI_FALSE;
}

// boolean meshStartDiscovery()
JNIEXPORT jboolean JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_meshStartDiscovery(JNIEnv* env, jclass /*cls*/) {
    return JNI_FALSE;
}

// int queueGetPendingCount()
JNIEXPORT jint JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_queueGetPendingCount(JNIEnv* env, jclass /*cls*/) {
    return 0;
}

// byte[] testEncrypt(byte[])
JNIEXPORT jbyteArray JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_testEncrypt(JNIEnv* env, jclass /*cls*/, jbyteArray data) {
    if (!data) return nullptr;
    jsize len = env->GetArrayLength(data);
    jbyteArray out = env->NewByteArray(len);
    jbyte* buf = env->GetByteArrayElements(data, nullptr);
    env->SetByteArrayRegion(out, 0, len, buf);
    env->ReleaseByteArrayElements(data, buf, JNI_ABORT);
    return out;
}

} // extern "C"
