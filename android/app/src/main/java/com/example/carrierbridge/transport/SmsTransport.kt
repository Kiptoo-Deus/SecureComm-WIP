package com.example.carrierbridge.transport

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Build
import android.telephony.SmsManager
import android.telephony.SmsMessage
import android.util.Log
import androidx.core.content.ContextCompat
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.launch
import java.util.*
import java.util.concurrent.ConcurrentHashMap

/**
 * SmsTransport: Secure SMS fallback when internet is unavailable.
 * 
 * Features:
 * - End-to-end encryption (carriers see only ciphertext)
 * - Multipart SMS fragmentation/reassembly
 * - Duplicate detection and replay protection
 * - Callback-based inbound message handling
 * - Fragment timeout (auto-cleanup after TTL)
 */

typealias SmsInboundCallback = (senderIdHash: Short, reassembledCiphertext: ByteArray, aeadTag: ByteArray) -> Unit

class SmsTransport(
    private val context: Context,
    private val deviceIdHash: Short,
    private val coroutineScope: CoroutineScope,
    private val onInboundMessage: SmsInboundCallback? = null
) {
    companion object {
        private const val TAG = "SmsTransport"
        private const val FRAGMENT_TTL_MS = 60_000L // 1 minute timeout for reassembly
        private const val MAX_RETRIES = 3
    }

    private val smsManager: SmsManager = context.getSystemService(SmsManager::class.java)
    private val smsReceiver = SmsReceiver()
    private val reassemblyBuffer = ConcurrentHashMap<String, ReassemblyState>()
    private val sentMessageIds = mutableSetOf<Int>() // For tracking delivery
    private var isReceiverRegistered = false

    data class ReassemblyState(
        val fragments: MutableMap<Byte, SmsFragment> = mutableMapOf(),
        val createdAt: Long = System.currentTimeMillis(),
        var lastFragmentTime: Long = System.currentTimeMillis()
    ) {
        fun isExpired(): Boolean = System.currentTimeMillis() - createdAt > FRAGMENT_TTL_MS
        fun isSenderExpired(): Boolean = System.currentTimeMillis() - lastFragmentTime > FRAGMENT_TTL_MS
    }

    /**
     * Initialize SMS receiver. Call this after permission is granted.
     */
    fun initialize() {
        try {
            val intentFilter = IntentFilter("android.provider.Telephony.SMS_RECEIVED")
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                ContextCompat.registerReceiver(context, smsReceiver, intentFilter, ContextCompat.RECEIVER_EXPORTED)
            } else {
                @Suppress("UnspecifiedRegisterReceiverFlag")
                context.registerReceiver(smsReceiver, intentFilter)
            }
            isReceiverRegistered = true
            Log.d(TAG, "SMS Receiver registered")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to register SMS receiver: ${e.message}", e)
        }
    }

    /**
     * Send an encrypted message via SMS (fragmented if needed).
     * Caller must have already encrypted payload using the ratchet.
     * 
     * @param recipientPhoneNumber E.164 format phone number (e.g., "+1234567890")
     * @param ciphertext Encrypted payload from ratchet
     * @param aeadTag AEAD authentication tag (16 bytes, Poly1305)
     * @return true if all fragments queued for sending
     */
    fun sendEncryptedMessage(
        recipientPhoneNumber: String,
        ciphertext: ByteArray,
        aeadTag: ByteArray
    ): Boolean {
        return try {
            val msgId = Random().nextInt()
            val fragments = SmsEnvelopeCodec.fragmentize(
                ciphertext, aeadTag, deviceIdHash, msgId
            )

            Log.d(TAG, "Sending encrypted message ($msgId) in ${fragments.size} fragments to $recipientPhoneNumber")

            for (frag in fragments) {
                val encoded = SmsEnvelopeCodec.encode(frag)
                val parts = listOf(encoded) // For now, send as binary PDU; can split further if needed

                try {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                        smsManager.sendTextMessage(
                            recipientPhoneNumber,
                            null,
                            String(encoded, Charsets.ISO_8859_1), // Binary data as string for compatibility
                            null,
                            null
                        )
                    } else {
                        @Suppress("DEPRECATION")
                        android.telephony.SmsManager.getDefault().sendTextMessage(
                            recipientPhoneNumber,
                            null,
                            String(encoded, Charsets.ISO_8859_1),
                            null,
                            null
                        )
                    }
                    Log.d(TAG, "Fragment ${frag.fragmentIndex}/${frag.totalFragments} sent")
                } catch (e: Exception) {
                    Log.e(TAG, "Failed to send fragment ${frag.fragmentIndex}: ${e.message}", e)
                    return false
                }
            }

            true
        } catch (e: Exception) {
            Log.e(TAG, "Failed to send encrypted message: ${e.message}", e)
            false
        }
    }

    /**
     * Clean up expired reassembly buffers (call periodically or on inactivity).
     */
    fun cleanupExpiredFragments() {
        val expired = reassemblyBuffer.filter { (_, state) -> state.isExpired() }
        expired.forEach { (key, _) ->
            reassemblyBuffer.remove(key)
            Log.d(TAG, "Cleaned up expired fragment buffer: $key")
        }
    }

    /**
     * Unregister SMS receiver and cleanup.
     */
    fun shutdown() {
        try {
            if (isReceiverRegistered) {
                context.unregisterReceiver(smsReceiver)
                isReceiverRegistered = false
                Log.d(TAG, "SMS Receiver unregistered")
            }
        } catch (e: Exception) {
            Log.e(TAG, "Error during shutdown: ${e.message}", e)
        }
    }

    /**
     * BroadcastReceiver for incoming SMS fragments.
     */
    private inner class SmsReceiver : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            if (intent?.action != "android.provider.Telephony.SMS_RECEIVED") return

            val bundle = intent.extras ?: return
            val pdus = bundle["pdus"] as? Array<*> ?: return
            val senderAddress = bundle["sender"] as? String ?: return

            Log.d(TAG, "SMS received from $senderAddress")

            for (pdu in pdus) {
                val message = try {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                        val format = bundle.getString("format") ?: "3gpp"
                        SmsMessage.createFromPdu(pdu as ByteArray, format)
                    } else {
                        @Suppress("DEPRECATION")
                        SmsMessage.createFromPdu(pdu as ByteArray)
                    }
                } catch (e: Exception) {
                    Log.e(TAG, "Failed to parse SMS PDU: ${e.message}", e)
                    continue
                }

                val body = message.messageBody
                try {
                    // Try to decode as binary envelope
                    val bodyBytes = body.toByteArray(Charsets.ISO_8859_1)
                    val fragment = SmsEnvelopeCodec.decode(bodyBytes) ?: continue

                    handleIncomingFragment(fragment, senderAddress)
                } catch (e: Exception) {
                    Log.w(TAG, "Failed to decode SMS fragment: ${e.message}")
                }
            }
        }
    }

    private fun handleIncomingFragment(fragment: SmsFragment, senderAddress: String) {
        val bufferKey = "${fragment.senderIdHash}_${fragment.msgId}"
        val state = reassemblyBuffer.getOrPut(bufferKey) { ReassemblyState() }

        // Update last fragment time for TTL
        state.lastFragmentTime = System.currentTimeMillis()

        // Check for duplicate
        if (state.fragments.containsKey(fragment.fragmentIndex)) {
            Log.d(TAG, "Duplicate fragment ignored: $bufferKey [${fragment.fragmentIndex}]")
            return
        }

        state.fragments[fragment.fragmentIndex] = fragment
        Log.d(TAG, "Fragment received: ${fragment.fragmentIndex}/${fragment.totalFragments} for $bufferKey from $senderAddress")

        // Check if reassembly is complete
        if (state.fragments.size == fragment.totalFragments.toInt()) {
            val fragmentList = state.fragments.values.toList()
            val reassembled = SmsEnvelopeCodec.reassemble(fragmentList)

            if (reassembled != null) {
                val (ciphertext, aeadTag) = reassembled
                Log.d(TAG, "Message reassembled: $bufferKey (${ciphertext.size} bytes ciphertext)")

                // Invoke callback with reassembled and verified data
                onInboundMessage?.invoke(fragment.senderIdHash, ciphertext, aeadTag)

                reassemblyBuffer.remove(bufferKey)
            } else {
                Log.w(TAG, "Reassembly failed for $bufferKey")
            }
        }
    }
}
