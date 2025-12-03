package com.example.carrierbridge.jni

import android.util.Log

/**
 * JNI Native methods for CarrierBridge C++ library
 * 
 * This class provides direct access to the native C++ cryptography and messaging engine.
 * All methods here are implemented in native-lib.cpp and linked C++ sources.
 */
object CarrierBridgeNative {

    private const val TAG = "CarrierBridgeNative"
    // Indicates whether the native JNI library was successfully loaded
    var libraryLoaded: Boolean = false
        private set

    init {
        try {
            // Load the native JNI library
            System.loadLibrary("carrierbridge_jni")
            Log.d(TAG, "Native library 'carrierbridge_jni' loaded successfully")
            libraryLoaded = true
        } catch (e: UnsatisfiedLinkError) {
            Log.e(TAG, "Failed to load native library: ${e.message}", e)
        }
    }
    
    // =========================================================================
    // DISPATCHER METHODS - Session and Message Management
    // =========================================================================
    
    /**
     * Initialize the dispatcher for a given device ID
     * 
     * @param deviceId Unique identifier for this device
     * @return Handle to dispatcher (0 on failure)
     */
    external fun initDispatcher(deviceId: String): Long
    
    /**
     * Create an encrypted session with another device
     * 
     * @param remoteDeviceId Device ID of the remote party
     * @return true if session created successfully
     */
    external fun createSession(remoteDeviceId: String): Boolean
    
    /**
     * Send an encrypted message to a recipient
     * 
     * @param recipientId Device ID of the recipient
     * @param plaintext Message content (will be encrypted)
     * @return true if message sent successfully
     */
    external fun sendMessage(recipientId: String, plaintext: ByteArray): Boolean
    
    /**
     * Set a callback to receive inbound encrypted messages
     * 
     * @param callback Object implementing MessageCallback interface
     */
    external fun setInboundCallback(callback: MessageCallback)
    
    /**
     * Stop the dispatcher and cleanup resources
     */
    external fun stopDispatcher()
    
    /**
     * Get version of the native library
     */
    external fun getVersion(): String
    
    // =========================================================================
    // DISPATCHER HELPER METHODS
    // =========================================================================
    
    external fun dispatcherIsInitialized(): Boolean
    
    // =========================================================================
    // RATCHET METHODS - Cryptographic Key Management
    // =========================================================================
    
    external fun ratchetGetState(): ByteArray?
    
    // =========================================================================
    // TRANSPORT METHODS - Network Communication
    // =========================================================================
    
    external fun transportConnect(url: String): Boolean
    
    // =========================================================================
    // MESH NETWORKING METHODS
    // =========================================================================
    
    external fun meshStartDiscovery(): Boolean
    
    // =========================================================================
    // OFFLINE QUEUE METHODS
    // =========================================================================
    
    external fun queueGetPendingCount(): Int
    
    // =========================================================================
    // TEST METHODS
    // =========================================================================
    
    /**
     * Test encryption/decryption (echoes data back for testing)
     */
    external fun testEncrypt(data: ByteArray): ByteArray?
    
    // =========================================================================
    // CALLBACK INTERFACE
    // =========================================================================
    
    /**
     * Interface for receiving inbound messages
     * Implement this to handle incoming encrypted messages
     */
    interface MessageCallback {
        /**
         * Called when an encrypted message is received and decrypted
         * 
         * @param senderId Device ID of the sender
         * @param data Decrypted message content
         */
        fun onMessageReceived(senderId: String, data: ByteArray)
    }
}
