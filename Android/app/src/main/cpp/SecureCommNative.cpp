#include <jni.h>
#include <vector>
#include <cstdint>
#include "securecomm/crypto.hpp"

static securecomm::AEAD aead;

extern "C" JNIEXPORT void JNICALL
Java_com_example_secure_1carrier_crypto_SecureCommNative_setKey(JNIEnv* env, jobject, jbyteArray key) {
    jsize keyLen = env->GetArrayLength(key);
    std::vector<uint8_t> keyVec(keyLen);
    env->GetByteArrayRegion(key, 0, keyLen, reinterpret_cast<jbyte*>(keyVec.data()));
    aead.set_key(keyVec);
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_example_secure_1carrier_crypto_SecureCommNative_encrypt(JNIEnv* env, jobject, jbyteArray plaintext, jbyteArray aad) {
    jsize ptLen = env->GetArrayLength(plaintext);
    std::vector<uint8_t> ptVec(ptLen);
    env->GetByteArrayRegion(plaintext, 0, ptLen, reinterpret_cast<jbyte*>(ptVec.data()));
    std::vector<uint8_t> aadVec;
    if (aad != nullptr) {
        jsize aadLen = env->GetArrayLength(aad);
        aadVec.resize(aadLen);
        env->GetByteArrayRegion(aad, 0, aadLen, reinterpret_cast<jbyte*>(aadVec.data()));
    }
    std::vector<uint8_t> ctVec = aead.encrypt(ptVec, aadVec);
    jbyteArray ciphertext = env->NewByteArray(ctVec.size());
    env->SetByteArrayRegion(ciphertext, 0, ctVec.size(), reinterpret_cast<const jbyte*>(ctVec.data()));
    return ciphertext;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_example_secure_1carrier_crypto_SecureCommNative_decrypt(JNIEnv* env, jobject, jbyteArray ciphertext, jbyteArray aad) {
    jsize ctLen = env->GetArrayLength(ciphertext);
    std::vector<uint8_t> ctVec(ctLen);
    env->GetByteArrayRegion(ciphertext, 0, ctLen, reinterpret_cast<jbyte*>(ctVec.data()));
    std::vector<uint8_t> aadVec;
    if (aad != nullptr) {
        jsize aadLen = env->GetArrayLength(aad);
        aadVec.resize(aadLen);
        env->GetByteArrayRegion(aad, 0, aadLen, reinterpret_cast<jbyte*>(aadVec.data()));
    }
    auto ptOpt = aead.decrypt(ctVec, aadVec);
    if (!ptOpt.has_value()) return nullptr;
    const std::vector<uint8_t>& ptVec = ptOpt.value();
    jbyteArray plaintext = env->NewByteArray(ptVec.size());
    env->SetByteArrayRegion(plaintext, 0, ptVec.size(), reinterpret_cast<const jbyte*>(ptVec.data()));
    return plaintext;
}
