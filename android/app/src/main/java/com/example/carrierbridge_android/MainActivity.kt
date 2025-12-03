package com.example.carrierbridge_android

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.Scaffold
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.core.content.ContextCompat
import com.example.carrierbridge.jni.CarrierBridgeClient
import com.example.carrierbridge_android.ui.ChatScreen
import com.example.carrierbridge_android.ui.theme.Carrierbridge_AndroidTheme

class MainActivity : ComponentActivity() {
    companion object {
        private const val TAG = "MainActivity"
    }

    private var carrierClient: CarrierBridgeClient? = null
    private var smsPermissionGranted = false

    // SMS permission launcher
    private val smsPermissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { permissions ->
        smsPermissionGranted = permissions[Manifest.permission.SEND_SMS] == true &&
                permissions[Manifest.permission.RECEIVE_SMS] == true
        
        Log.d(TAG, "SMS permissions granted: $smsPermissionGranted")
        
        if (smsPermissionGranted && carrierClient != null) {
            carrierClient!!.setSmsEnabled(this, true)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        
        Log.d(TAG, "MainActivity onCreate")
        
        // Initialize CarrierBridge client
        initializeCarrierBridge()
        
        // Request SMS permissions if needed
        requestSmsPermissionsIfNeeded()

        setContent {
            Carrierbridge_AndroidTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) {
                    ChatScreen(
                        deviceId = "alice",
                        recipientId = "bob",
                        carrierClient = carrierClient
                    )
                }
            }
        }
    }

    override fun onResume() {
        super.onResume()
        Log.d(TAG, "MainActivity onResume")
        // Any reconnection logic if needed
    }

    override fun onPause() {
        super.onPause()
        Log.d(TAG, "MainActivity onPause")
        // Pause background tasks if needed
    }

    override fun onDestroy() {
        super.onDestroy()
        Log.d(TAG, "MainActivity onDestroy - shutting down CarrierBridge")
        // Cleanup and shutdown
        carrierClient?.shutdown()
        carrierClient = null
    }

    private fun initializeCarrierBridge() {
        try {
            carrierClient = CarrierBridgeClient("alice")
            val success = carrierClient!!.initialize(context = this)
            if (success) {
                Log.d(TAG, "CarrierBridge initialized successfully")
            } else {
                Log.e(TAG, "Failed to initialize CarrierBridge")
            }
        } catch (e: Exception) {
            Log.e(TAG, "Exception during CarrierBridge initialization", e)
        }
    }

    private fun requestSmsPermissionsIfNeeded() {
        val sendSmsGranted = ContextCompat.checkSelfPermission(
            this,
            Manifest.permission.SEND_SMS
        ) == PackageManager.PERMISSION_GRANTED

        val receiveSmsGranted = ContextCompat.checkSelfPermission(
            this,
            Manifest.permission.RECEIVE_SMS
        ) == PackageManager.PERMISSION_GRANTED

        smsPermissionGranted = sendSmsGranted && receiveSmsGranted

        if (!smsPermissionGranted) {
            Log.d(TAG, "Requesting SMS permissions")
            smsPermissionLauncher.launch(
                arrayOf(
                    Manifest.permission.SEND_SMS,
                    Manifest.permission.RECEIVE_SMS
                )
            )
        } else {
            Log.d(TAG, "SMS permissions already granted")
            carrierClient?.setSmsEnabled(this, true)
        }
    }
}
