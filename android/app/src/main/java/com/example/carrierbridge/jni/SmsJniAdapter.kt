package com.example.carrierbridge.jni

import android.util.Log

/**
 * JNI Adapter for SMS Transport
 * 
 * Bridges Kotlin SmsTransport and C++ Dispatcher via JNI.
 * Handles reassembly, AEAD verification, and routing to native decryption.
 */
object SmsJniAdapter {
    private const val TAG = "SmsJniAdapter"

    /**
     * Called from SmsTransport when an SMS message is fully reassembled.
     * Verifies AEAD tag via native dispatcher, then invokes decryption.
     * 
     * @param senderIdHash Device ID hash from SMS header
     * @param ciphertext Encrypted payload
     * @param aeadTag Authentication tag (Poly1305, 16 bytes)
     */
    fun onSmsMessageReceived(senderIdHash: Short, ciphertext: ByteArray, aeadTag: ByteArray) {
        try {
            Log.d(TAG, "SMS message received from device hash $senderIdHash (${ciphertext.size} bytes)")

            // Verify AEAD tag and decrypt via native dispatcher
            val plaintext = verifyAndDecryptSmsMessage(ciphertext, aeadTag)
            if (plaintext != null) {
                Log.d(TAG, "SMS message decrypted successfully (${plaintext.size} bytes plaintext)")
                // TODO: route plaintext to app message handler or chat UI
            } else {
                Log.w(TAG, "AEAD verification failed for SMS message")
            }
        } catch (e: Exception) {
            Log.e(TAG, "Error processing SMS message: ${e.message}", e)
        }
    }

    /**
     * Native method: verify AEAD tag and decrypt SMS ciphertext.
     * Calls into C++ Dispatcher which performs Poly1305 tag verification
     * and ChaCha20 decryption.
     * 
     * @param ciphertext Encrypted payload
     * @param aeadTag 16-byte Poly1305 authentication tag
     * @return Decrypted plaintext or null if verification fails
     */
    external fun verifyAndDecryptSmsMessage(ciphertext: ByteArray, aeadTag: ByteArray): ByteArray?

    /**
     * Native method: get sender device ID from SMS metadata.
     * Reverse-looks up the device ID hash from the session table.
     * 
     * @param senderIdHash Hash from SMS header
     * @return Device ID string or null
     */
    external fun resolveSenderDeviceId(senderIdHash: Short): String?
}
