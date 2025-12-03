package com.example.carrierbridge.transport

import android.util.Log
import java.nio.ByteBuffer
import java.util.zip.CRC32

/**
 * Secure SMS Envelope Format (Binary, encrypted end-to-end)
 * 
 * Design Goal: Minimize overhead, maximize payload per SMS (carriers see only ciphertext)
 * 
 * Format (total ~20 bytes overhead for header + AEAD tag):
 * ┌─────────────────────────────────────────────────────────────┐
 * │ Field           │ Size │ Description                         │
 * ├─────────────────┼──────┼─────────────────────────────────────┤
 * │ Version         │ 1    │ Protocol version (0x01)             │
 * │ SenderIdHash    │ 2    │ Sender device ID hash (CRC16)       │
 * │ MsgId           │ 4    │ Message ID (for dedup/reassembly)   │
 * │ TotalFragments  │ 1    │ Total # of fragments for this msg   │
 * │ FragmentIndex   │ 1    │ This fragment's index (0-based)     │
 * │ CiphertextLen   │ 2    │ Ciphertext length in this fragment  │
 * │ Ciphertext      │ N    │ Encrypted payload (ChaCha20)        │
 * │ AeadTag         │ 16   │ AEAD tag (Poly1305)                 │
 * └─────────────────────────────────────────────────────────────┘
 * Total overhead: 27 bytes
 * Max SMS binary PDU: 140 bytes → ~113 bytes ciphertext per fragment
 * Multipart text SMS: ~67 bytes per segment → ~40 bytes per fragment (less efficient)
 */

data class SmsFragment(
    val version: Byte = 0x01,
    val senderIdHash: Short = 0,
    val msgId: Int = 0,
    val totalFragments: Byte = 0,
    val fragmentIndex: Byte = 0,
    val ciphertext: ByteArray = ByteArray(0),
    val aeadTag: ByteArray = ByteArray(16) // Poly1305 tag
) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other !is SmsFragment) return false
        if (version != other.version) return false
        if (senderIdHash != other.senderIdHash) return false
        if (msgId != other.msgId) return false
        if (totalFragments != other.totalFragments) return false
        if (fragmentIndex != other.fragmentIndex) return false
        if (!ciphertext.contentEquals(other.ciphertext)) return false
        if (!aeadTag.contentEquals(other.aeadTag)) return false
        return true
    }

    override fun hashCode(): Int {
        var result = version.toInt()
        result = 31 * result + senderIdHash
        result = 31 * result + msgId
        result = 31 * result + totalFragments
        result = 31 * result + fragmentIndex
        result = 31 * result + ciphertext.contentHashCode()
        result = 31 * result + aeadTag.contentHashCode()
        return result
    }
}

object SmsEnvelopeCodec {
    private const val TAG = "SmsEnvelope"

    const val VERSION = 0x01.toByte()
    const val HEADER_SIZE = 11 // version(1) + senderIdHash(2) + msgId(4) + totalFragments(1) + fragmentIndex(1) + ciphertextLen(2)
    const val AEAD_TAG_SIZE = 16 // Poly1305
    const val MAX_CIPHERTEXT_PER_FRAGMENT = 113 // 140 bytes SMS - header - tag

    /**
     * Encode a fragment to binary for SMS transmission.
     * Format: [version][senderIdHash(2)][msgId(4)][totalFragments][fragmentIndex][ciphertextLen(2)][ciphertext][aeadTag(16)]
     */
    fun encode(fragment: SmsFragment): ByteArray {
        val buffer = ByteBuffer.allocate(
            HEADER_SIZE + fragment.ciphertext.size + AEAD_TAG_SIZE
        )
        buffer.put(fragment.version)
        buffer.putShort(fragment.senderIdHash)
        buffer.putInt(fragment.msgId)
        buffer.put(fragment.totalFragments)
        buffer.put(fragment.fragmentIndex)
        buffer.putShort(fragment.ciphertext.size.toShort())
        buffer.put(fragment.ciphertext)
        buffer.put(fragment.aeadTag)
        return buffer.array()
    }

    /**
     * Decode a binary blob back into a fragment.
     * Validates header structure and AEAD tag presence.
     */
    fun decode(data: ByteArray): SmsFragment? {
        return try {
            if (data.size < HEADER_SIZE + AEAD_TAG_SIZE) {
                Log.w(TAG, "Fragment too short: ${data.size} bytes")
                return null
            }

            val buffer = ByteBuffer.wrap(data)
            val version = buffer.get()
            if (version != VERSION) {
                Log.w(TAG, "Unsupported version: $version")
                return null
            }

            val senderIdHash = buffer.short
            val msgId = buffer.int
            val totalFragments = buffer.get()
            val fragmentIndex = buffer.get()
            val ciphertextLen = buffer.short.toInt()

            if (ciphertextLen < 0 || ciphertextLen > MAX_CIPHERTEXT_PER_FRAGMENT) {
                Log.w(TAG, "Invalid ciphertext length: $ciphertextLen")
                return null
            }

            val expectedTotalSize = HEADER_SIZE + ciphertextLen + AEAD_TAG_SIZE
            if (data.size < expectedTotalSize) {
                Log.w(TAG, "Data too short for payload. Expected $expectedTotalSize, got ${data.size}")
                return null
            }

            val ciphertext = ByteArray(ciphertextLen)
            buffer.get(ciphertext)

            val aeadTag = ByteArray(AEAD_TAG_SIZE)
            buffer.get(aeadTag)

            SmsFragment(
                version = version,
                senderIdHash = senderIdHash,
                msgId = msgId,
                totalFragments = totalFragments,
                fragmentIndex = fragmentIndex,
                ciphertext = ciphertext,
                aeadTag = aeadTag
            )
        } catch (e: Exception) {
            Log.e(TAG, "Failed to decode fragment: ${e.message}", e)
            null
        }
    }

    /**
     * Split an encrypted envelope (ciphertext + tag) into SMS fragments.
     * Each fragment includes a copy of the AEAD tag for independent verification.
     */
    fun fragmentize(
        ciphertext: ByteArray,
        aeadTag: ByteArray,
        senderIdHash: Short,
        msgId: Int
    ): List<SmsFragment> {
        val fragments = mutableListOf<SmsFragment>()
        var offset = 0
        var fragIndex = 0

        while (offset < ciphertext.size) {
            val chunkSize = minOf(MAX_CIPHERTEXT_PER_FRAGMENT, ciphertext.size - offset)
            val chunk = ciphertext.sliceArray(offset until offset + chunkSize)

            fragments.add(
                SmsFragment(
                    version = VERSION,
                    senderIdHash = senderIdHash,
                    msgId = msgId,
                    totalFragments = ((ciphertext.size + MAX_CIPHERTEXT_PER_FRAGMENT - 1) / MAX_CIPHERTEXT_PER_FRAGMENT).toByte(),
                    fragmentIndex = fragIndex.toByte(),
                    ciphertext = chunk,
                    aeadTag = aeadTag
                )
            )
            offset += chunkSize
            fragIndex++
        }

        return fragments
    }

    /**
     * Reassemble fragments back into the original ciphertext and AEAD tag.
     * Validates that all fragments have consistent metadata and matching AEAD tags.
     */
    fun reassemble(fragments: List<SmsFragment>): Pair<ByteArray, ByteArray>? {
        if (fragments.isEmpty()) return null

        val first = fragments[0]
        val expectedTotal = first.totalFragments.toInt()

        // Validate all fragments belong to the same message
        if (fragments.any { it.msgId != first.msgId || it.totalFragments != first.totalFragments }) {
            Log.w(TAG, "Fragment metadata mismatch")
            return null
        }

        // Validate AEAD tags match (all fragments should have the same tag)
        if (fragments.any { !it.aeadTag.contentEquals(first.aeadTag) }) {
            Log.w(TAG, "AEAD tag mismatch between fragments")
            return null
        }

        // Check we have all fragments
        if (fragments.size != expectedTotal) {
            Log.w(TAG, "Missing fragments: have ${fragments.size}, need $expectedTotal")
            return null
        }

        // Sort by fragment index and reconstruct
        val sorted = fragments.sortedBy { it.fragmentIndex.toInt() }
        val ciphertext = sorted.flatMap { it.ciphertext.toList() }.toByteArray()

        return ciphertext to first.aeadTag
    }

    /**
     * Helper: compute a short hash of device ID for compact SMS header.
     */
    fun deviceIdHash(deviceId: String): Short {
        val crc = CRC32()
        crc.update(deviceId.toByteArray())
        return (crc.value and 0xFFFF).toShort()
    }
}
