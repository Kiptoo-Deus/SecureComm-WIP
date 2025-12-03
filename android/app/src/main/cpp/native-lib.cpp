#include <jni.h>
#include <string>
#include <memory>
#include <android/log.h>
#include "securecomm/dispatcher.hpp"

#define LOG_TAG "CarrierBridge"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Global dispatcher instance
static std::shared_ptr<securecomm::Dispatcher> g_dispatcher = nullptr;

extern "C" {

/**
 * Initialize the dispatcher with a device ID and in-memory transport
 */
JNIEXPORT jlong JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_initDispatcher(
        JNIEnv *env, jclass clazz, jstring device_id) {
    try {
        const char *device_id_str = env->GetStringUTFChars(device_id, nullptr);
        
        // Create in-memory transport (for local testing)
        // In production, this would be WebSocket transport
        auto transport = std::make_shared<securecomm::InMemoryTransport>();
        
        // Create and initialize dispatcher
        auto dispatcher = std::make_shared<securecomm::Dispatcher>(transport);
        dispatcher->register_device(device_id_str);
        dispatcher->start();
        
        // Store globally
        g_dispatcher = dispatcher;
        
        env->ReleaseStringUTFChars(device_id, device_id_str);
        
        LOGD("Dispatcher initialized for device: %s", device_id_str);
        return reinterpret_cast<jlong>(dispatcher.get());
        
    } catch (const std::exception &e) {
        LOGE("Error initializing dispatcher: %s", e.what());
        return 0;
    }
}

/**
 * Create an encrypted session with another device
 */
JNIEXPORT jboolean JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_createSession(
        JNIEnv *env, jclass clazz, jstring remote_device_id) {
    try {
        if (!g_dispatcher) {
            LOGE("Dispatcher not initialized");
            return JNI_FALSE;
        }
        
        const char *remote_id_str = env->GetStringUTFChars(remote_device_id, nullptr);
        auto session = g_dispatcher->create_session_with(remote_id_str);
        env->ReleaseStringUTFChars(remote_device_id, remote_id_str);
        
        LOGD("Session created with: %s", remote_id_str);
        return session.has_value() ? JNI_TRUE : JNI_FALSE;
        
    } catch (const std::exception &e) {
        LOGE("Error creating session: %s", e.what());
        return JNI_FALSE;
    }
}

/**
 * Send an encrypted message to a recipient
 */
JNIEXPORT jboolean JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_sendMessage(
        JNIEnv *env, jclass clazz, jstring recipient_id, jbyteArray plaintext) {
    try {
        if (!g_dispatcher) {
            LOGE("Dispatcher not initialized");
            return JNI_FALSE;
        }
        
        const char *recipient_str = env->GetStringUTFChars(recipient_id, nullptr);
        
        // Convert jbyteArray to vector<uint8_t>
        jbyte *bytes = env->GetByteArrayElements(plaintext, nullptr);
        jsize length = env->GetArrayLength(plaintext);
        std::vector<uint8_t> data(bytes, bytes + length);
        env->ReleaseByteArrayElements(plaintext, bytes, JNI_ABORT);
        
        // Send through dispatcher
        bool result = g_dispatcher->send_message_to_device(recipient_str, data);
        
        env->ReleaseStringUTFChars(recipient_id, recipient_str);
        
        LOGD("Message sent to %s: %s", recipient_str, result ? "success" : "failed");
        return result ? JNI_TRUE : JNI_FALSE;
        
    } catch (const std::exception &e) {
        LOGE("Error sending message: %s", e.what());
        return JNI_FALSE;
    }
}

/**
 * Set a callback to receive inbound messages
 */
JNIEXPORT void JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_setInboundCallback(
        JNIEnv *env, jclass clazz, jobject callback) {
    try {
        if (!g_dispatcher) {
            LOGE("Dispatcher not initialized");
            return;
        }
        
        // Store global reference to callback
        jobject globalCallback = env->NewGlobalRef(callback);
        
        // Set C++ callback that will invoke the Java method
        g_dispatcher->set_on_inbound([env, globalCallback](
                const std::string &sender_id, 
                const std::vector<uint8_t> &plaintext) {
            
            // Get callback class and method
            jclass callbackClass = env->GetObjectClass(globalCallback);
            jmethodID onMessageReceived = env->GetMethodID(
                callbackClass, 
                "onMessageReceived",
                "(Ljava/lang/String;[B)V"
            );
            
            if (onMessageReceived == nullptr) {
                LOGE("Could not find onMessageReceived callback method");
                return;
            }
            
            // Create Java String for sender
            jstring sender_jstring = env->NewStringUTF(sender_id.c_str());
            
            // Create byte array for data
            jbyteArray data_array = env->NewByteArray(plaintext.size());
            env->SetByteArrayRegion(data_array, 0, plaintext.size(),
                                   reinterpret_cast<const jbyte *>(plaintext.data()));
            
            // Call Java callback method
            env->CallVoidMethod(globalCallback, onMessageReceived, sender_jstring, data_array);
            
            // Cleanup
            env->DeleteLocalRef(sender_jstring);
            env->DeleteLocalRef(data_array);
            env->DeleteLocalRef(callbackClass);
        });
        
        LOGD("Inbound callback registered");
        
    } catch (const std::exception &e) {
        LOGE("Error setting inbound callback: %s", e.what());
    }
}

/**
 * Stop the dispatcher and cleanup resources
 */
JNIEXPORT void JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_stopDispatcher(
        JNIEnv *env, jclass clazz) {
    try {
        if (g_dispatcher) {
            g_dispatcher->stop();
            g_dispatcher = nullptr;
            LOGD("Dispatcher stopped");
        }
    } catch (const std::exception &e) {
        LOGE("Error stopping dispatcher: %s", e.what());
    }
}

/**
 * Get the version of the native library
 */
JNIEXPORT jstring JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_getVersion(
        JNIEnv *env, jclass clazz) {
    return env->NewStringUTF("1.0.0");
}

/**
 * Test function - echo data back (useful for debugging)
 */
JNIEXPORT jbyteArray JNICALL
Java_com_example_carrierbridge_jni_CarrierBridgeNative_testEncrypt(
        JNIEnv *env, jclass clazz, jbyteArray data) {
    try {
        jbyte *bytes = env->GetByteArrayElements(data, nullptr);
        jsize length = env->GetArrayLength(data);
        
        // In real code, this would encrypt - for now just echo back
        jbyteArray result = env->NewByteArray(length);
        env->SetByteArrayRegion(result, 0, length, bytes);
        
        env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);
        
        return result;
        
    } catch (const std::exception &e) {
        LOGE("Error in testEncrypt: %s", e.what());
        return nullptr;
    }
}

} // extern "C"
