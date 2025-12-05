package com.example.secure_carrier.net

import android.util.Log
import java.net.InetSocketAddress
import java.net.Socket
import java.util.concurrent.CountDownLatch
import java.util.concurrent.Executors

object ServerDiscovery {
    private const val TAG = "ServerDiscovery"
    private const val PORT = 8080
    private const val TIMEOUT_MS = 500  // Reduced from 2000
    private var cachedServerUrl: String? = null
    private val executor = Executors.newFixedThreadPool(16)  // Parallel threads

    /**
     * Discovers the server by scanning the local subnet for an open port 8080 in parallel.
     * Returns cached URL if available, otherwise scans and caches the result.
     */
    fun discoverServer(): String? {
        // Return cached URL if available
        cachedServerUrl?.let {
            Log.d(TAG, "Using cached server URL: $it")
            return it
        }

        return try {
            // Get device's own IP to determine subnet
            val deviceIp = getDeviceIpAddress()
            if (deviceIp.isNullOrEmpty()) {
                Log.w(TAG, "Could not determine device IP")
                return null
            }
            Log.d(TAG, "Device IP: $deviceIp")

            // Extract subnet (e.g., "192.168.0" from "192.168.0.x")
            val subnet = deviceIp.substringBeforeLast(".")
            Log.d(TAG, "Scanning subnet: $subnet.0 - $subnet.255 (parallel, 500ms timeout)")

            // Scan subnet in parallel for open port 8080
            val foundUrl = scanSubnetParallel(subnet)
            if (foundUrl != null) {
                cachedServerUrl = foundUrl
                Log.d(TAG, "Server found at: $foundUrl")
            } else {
                Log.w(TAG, "No server found on subnet")
            }
            foundUrl
        } catch (e: Exception) {
            Log.e(TAG, "Error discovering server", e)
            null
        }
    }

    /**
     * Scan subnet in parallel using thread pool.
     */
    private fun scanSubnetParallel(subnet: String): String? {
        val latch = CountDownLatch(254)
        var foundUrl: String? = null
        val lock = Object()

        for (i in 1..254) {
            executor.submit {
                try {
                    val ipToCheck = "$subnet.$i"
                    if (isServerAvailable(ipToCheck)) {
                        synchronized(lock) {
                            if (foundUrl == null) {
                                foundUrl = "http://$ipToCheck:$PORT"
                                Log.d(TAG, "Server found at: $foundUrl")
                            }
                        }
                    }
                } finally {
                    latch.countDown()
                }
            }
        }

        // Wait up to 10 seconds for scan to complete or early exit if found
        latch.await()
        return foundUrl
    }

    /**
     * Check if server is available at the given IP (non-blocking with timeout).
     */
    private fun isServerAvailable(ip: String): Boolean {
        return try {
            val socket = Socket()
            socket.soTimeout = TIMEOUT_MS
            socket.connect(InetSocketAddress(ip, PORT), TIMEOUT_MS)
            socket.close()
            true
        } catch (e: Exception) {
            false
        }
    }

    /**
     * Get the device's local IP address on the current network.
     */
    private fun getDeviceIpAddress(): String? {
        return try {
            val interfaces = java.net.NetworkInterface.getNetworkInterfaces()
            for (intf in interfaces) {
                for (addr in intf.inetAddresses) {
                    // Skip loopback and IPv6
                    val hostAddr = addr.hostAddress
                    if (!addr.isLoopbackAddress && hostAddr != null && hostAddr.contains(".")) {
                        Log.d(TAG, "Found local IP: $hostAddr")
                        return hostAddr
                    }
                }
            }
            null
        } catch (e: Exception) {
            Log.e(TAG, "Error getting device IP", e)
            null
        }
    }

    /**
     * Clear the cached server URL (useful for forcing a re-scan).
     */
    fun clearCache() {
        cachedServerUrl = null
        Log.d(TAG, "Cached server URL cleared")
    }
}
