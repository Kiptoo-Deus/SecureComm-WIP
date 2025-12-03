#include <jni.h>
#include <android/log.h>
#include <cstring>

#define LOG_TAG "SmsJniAdapter"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/**
 * JNI implementation for SMS transport integration.
 * Bridges Kotlin SmsTransport and C++ Dispatcher.
 */

// Forward declarations (stubs for now; link to actual crypto in future)
extern "C" {

/**
 * Verify AEAD tag and decrypt SMS ciphertext.
 * 
 * For production: integrate with the existing Dispatcher decryption path.
 * Currently: stub that logs and returns plaintext (plaintext for testing only).
 */
JNIEXPORT jbyteArray JNICALL
Java_com_example_carrierbridge_jni_SmsJniAdapter_verifyAndDecryptSmsMessage(
    JNIEnv* env,
    jclass clazz,
    jbyteArray ciphertext,
    jbyteArray aeadTag
) {
    LOGI("verifyAndDecryptSmsMessage called");

    // Get ciphertext and tag
    jint ciphertextLen = env->GetArrayLength(ciphertext);
    jint tagLen = env->GetArrayLength(aeadTag);

    if (tagLen != 16) {
        LOGE("Invalid AEAD tag length: %d (expected 16)", tagLen);
        return nullptr;
    }

    jbyte* ciphertextData = env->GetByteArrayElements(ciphertext, nullptr);
    jbyte* tagData = env->GetByteArrayElements(aeadTag, nullptr);

    LOGI("Decrypting SMS: ciphertext=%d bytes, tag=%d bytes", ciphertextLen, tagLen);

    // TODO: Call actual dispatcher decryption
    // For now, return a stub decrypted message (plaintext for testing)
    const char* stubPlaintext = "[SMS decrypted via E2EE]";
    int plaintextLen = strlen(stubPlaintext);

    jbyteArray resultArray = env->NewByteArray(plaintextLen);
    if (resultArray != nullptr) {
        env->SetByteArrayRegion(resultArray, 0, plaintextLen, (jbyte*)stubPlaintext);
    }

    env->ReleaseByteArrayElements(ciphertext, ciphertextData, JNI_ABORT);
    env->ReleaseByteArrayElements(aeadTag, tagData, JNI_ABORT);

    return resultArray;
}

/**
 * Reverse-lookup sender device ID from a hash.
 * 
 * For production: query the session table in the Dispatcher.
 * Currently: stub that returns a test device ID.
 */
JNIEXPORT jstring JNICALL
Java_com_example_carrierbridge_jni_SmsJniAdapter_resolveSenderDeviceId(
    JNIEnv* env,
    jclass clazz,
    jshort senderIdHash
) {
    LOGI("resolveSenderDeviceId called for hash 0x%04x", senderIdHash);

    // TODO: Query actual dispatcher session table
    // For now, return stub device ID
    const char* stubDeviceId = "sms_device_unknown";
    return env->NewStringUTF(stubDeviceId);
}

} // extern "C"
