package com.example.carrierbridge.jni

import android.content.Context
import android.util.Log
import com.example.carrierbridge.transport.SmsEnvelopeCodec
import com.example.carrierbridge.transport.SmsTransport
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers

/**
 * High-level Kotlin API for CarrierBridge encrypted messaging
 * 
 * This class provides a user-friendly interface to the native C++ cryptography library.
 * It handles:
 * - Device registration and session management
 * - Message encryption/decryption (internet)
 * - SMS fallback transport (when offline)
 * - Message callbacks
 * - Error handling and logging
 */
class CarrierBridgeClient(private val deviceId: String) {
    
    companion object {
        private const val TAG = "CarrierBridgeClient"
    }
    
    private var dispatcherHandle: Long = 0L
    private var messageCallback: MessageCallback? = null
    private var smsTransport: SmsTransport? = null
    private var smsEnabled: Boolean = false
    private val smsScope = CoroutineScope(Dispatchers.Default)
    
    /**
     * Interface for app code to receive messages
     */
    interface MessageCallback {
        fun onMessageReceived(senderId: String, message: String)
    }
    
    /**
     * Initialize the client and register the device
     * 
     * Must be called before sending/receiving messages
     * 
     * @param context Android context for SMS transport initialization (optional)
     * @return true if initialization successful
     */
    fun initialize(context: Context? = null): Boolean {
        return try {
            Log.d(TAG, "Initializing CarrierBridge for device: $deviceId")
            if (!CarrierBridgeNative.libraryLoaded) {
                Log.w(TAG, "Native library not loaded; native features will be disabled")
                // Still initialize SMS transport if context provided so app can function in degraded mode
                if (context != null) {
                    initializeSmsTransport(context)
                }
                return false
            }

            dispatcherHandle = CarrierBridgeNative.initDispatcher(deviceId)
            val success = dispatcherHandle != 0L
            if (success) {
                Log.d(TAG, "CarrierBridge initialized successfully")
                // Initialize SMS transport if context available
                if (context != null) {
                    initializeSmsTransport(context)
                }
            } else {
                Log.e(TAG, "Failed to initialize CarrierBridge dispatcher")
            }
            success
        } catch (e: Exception) {
            Log.e(TAG, "Exception during initialization", e)
            false
        }
    }

    /**
     * Enable or disable SMS fallback transport.
     * 
     * @param context Android context for SMS operations
     * @param enabled true to enable SMS fallback
     */
    fun setSmsEnabled(context: Context, enabled: Boolean) {
        smsEnabled = enabled
        if (enabled && smsTransport == null) {
            initializeSmsTransport(context)
        } else if (!enabled && smsTransport != null) {
            smsTransport?.shutdown()
            smsTransport = null
        }
        Log.d(TAG, "SMS fallback ${if (enabled) "enabled" else "disabled"}")
    }

    /**
     * Send an encrypted message via SMS (fallback transport).
     * For testing/fallback when internet unavailable.
     * 
     * @param recipientPhoneNumber E.164 format (+1234567890)
     * @param ciphertext Already encrypted by ratchet
     * @param aeadTag 16-byte Poly1305 tag
     * @return true if queued for sending
     */
    fun sendViaSms(recipientPhoneNumber: String, ciphertext: ByteArray, aeadTag: ByteArray): Boolean {
        return if (smsEnabled && smsTransport != null) {
            Log.d(TAG, "Sending message via SMS to $recipientPhoneNumber")
            smsTransport!!.sendEncryptedMessage(recipientPhoneNumber, ciphertext, aeadTag)
        } else {
            Log.w(TAG, "SMS not enabled or not initialized")
            false
        }
    }

    /**
     * Initialize SMS transport (called internally).
     */
    private fun initializeSmsTransport(context: Context) {
        try {
            val deviceIdHash = SmsEnvelopeCodec.deviceIdHash(deviceId)
            smsTransport = SmsTransport(
                context = context,
                deviceIdHash = deviceIdHash,
                coroutineScope = smsScope,
                onInboundMessage = { _, ciphertext, aeadTag ->
                    // Forward to JNI adapter for decryption
                    SmsJniAdapter.onSmsMessageReceived(deviceIdHash, ciphertext, aeadTag)
                }
            )
            smsTransport?.initialize()
            Log.d(TAG, "SMS transport initialized")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to initialize SMS transport: ${e.message}", e)
        }
    }
    
    /**
     * Create an encrypted session with another device
     * 
     * Must be done before messaging that device
     * 
     * @param remoteDeviceId Device ID to connect to
     * @return true if session created
     */
    fun createSession(remoteDeviceId: String): Boolean {
        return try {
            if (dispatcherHandle == 0L) {
                Log.e(TAG, "Client not initialized")
                return false
            }
            Log.d(TAG, "Creating session with device: $remoteDeviceId")
            CarrierBridgeNative.createSession(remoteDeviceId)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to create session with $remoteDeviceId", e)
            false
        }
    }
    
    /**
     * Send an encrypted message to a device
     * 
     * @param recipientId Recipient device ID
     * @param message Message text (will be encrypted)
     * @return true if sent successfully
     */
    fun sendMessage(recipientId: String, message: String): Boolean {
        return try {
            if (dispatcherHandle == 0L) {
                Log.e(TAG, "Client not initialized")
                return false
            }
            Log.d(TAG, "Sending encrypted message to: $recipientId (${message.length} chars)")
            CarrierBridgeNative.sendMessage(recipientId, message.toByteArray(Charsets.UTF_8))
        } catch (e: Exception) {
            Log.e(TAG, "Failed to send message to $recipientId", e)
            false
        }
    }
    
    /**
     * Set a callback to receive incoming messages
     * 
     * @param callback Interface implementation to receive messages
     */
    fun setMessageCallback(callback: MessageCallback) {
        this.messageCallback = callback
        try {
            Log.d(TAG, "Setting message callback")
            CarrierBridgeNative.setInboundCallback(object : CarrierBridgeNative.MessageCallback {
                override fun onMessageReceived(senderId: String, data: ByteArray) {
                    try {
                        val message = String(data, Charsets.UTF_8)
                        Log.d(TAG, "Received message from $senderId: ${message.length} chars")
                        callback.onMessageReceived(senderId, message)
                    } catch (e: Exception) {
                        Log.e(TAG, "Error decoding message from $senderId", e)
                    }
                }
            })
        } catch (e: Exception) {
            Log.e(TAG, "Failed to set message callback", e)
        }
    }
    
    /**
     * Get the version of the native library
     * 
     * @return Version string
     */
    fun getVersion(): String {
        return try {
            CarrierBridgeNative.getVersion()
        } catch (e: Exception) {
            "unknown"
        }
    }
    
    /**
     * Shutdown the client and cleanup resources
     * 
     * Should be called when app closes
     */
    fun shutdown() {
        try {
            if (dispatcherHandle != 0L) {
                Log.d(TAG, "Shutting down CarrierBridge")
                CarrierBridgeNative.stopDispatcher()
                dispatcherHandle = 0L
            }
            smsTransport?.shutdown()
            smsTransport = null
        } catch (e: Exception) {
            Log.e(TAG, "Error during shutdown", e)
        }
    }
    
    /**
     * Check if client is initialized
     */
    fun isInitialized(): Boolean = dispatcherHandle != 0L

    /**
     * Check if SMS fallback is enabled
     */
    fun isSmsEnabled(): Boolean = smsEnabled
}
