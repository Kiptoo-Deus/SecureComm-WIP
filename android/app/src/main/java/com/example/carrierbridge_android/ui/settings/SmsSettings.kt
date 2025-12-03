package com.example.carrierbridge_android.ui.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.carrierbridge_android.ui.theme.BrandBlue
import com.example.carrierbridge_android.ui.theme.SurfaceLight
import com.example.carrierbridge_android.ui.theme.TextPrimary
import com.example.carrierbridge_android.ui.theme.TextSecondary

/**
 * SMS Fallback Settings and Consent Screen
 * 
 * User education and consent for SMS transport when internet is unavailable.
 * Explains costs, privacy, and requests necessary permissions.
 */

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SmsSettingsScreen(
    onBackClick: () -> Unit,
    onEnableSms: (Boolean) -> Unit,
    isEnabled: Boolean = false
) {
    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("SMS Fallback Settings", fontWeight = FontWeight.SemiBold) },
                navigationIcon = {
                    IconButton(onClick = onBackClick) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, "Back")
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = BrandBlue,
                    titleContentColor = Color.White,
                    navigationIconContentColor = Color.White
                )
            )
        },
        containerColor = SurfaceLight
    ) { innerPadding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(innerPadding)
                .verticalScroll(rememberScrollState())
                .padding(16.dp)
        ) {
            // Header
            Text(
                "End-to-End Encrypted SMS",
                fontSize = 20.sp,
                fontWeight = FontWeight.Bold,
                color = TextPrimary
            )
            Spacer(modifier = Modifier.height(8.dp))
            Text(
                "When you don't have internet, CarrierBridge can send encrypted messages via SMS (cellular network).",
                color = TextSecondary
            )

            Spacer(modifier = Modifier.height(24.dp))

            // Feature Card
            Card(
                modifier = Modifier.fillMaxWidth(),
                colors = CardDefaults.cardColors(containerColor = Color.White)
            ) {
                Column(modifier = Modifier.padding(16.dp)) {
                    Text("✅ Features", fontWeight = FontWeight.SemiBold, color = TextPrimary)
                    Spacer(modifier = Modifier.height(8.dp))
                    listOf(
                        "All messages remain end-to-end encrypted",
                        "Carrier cannot read your messages",
                        "Automatic fallback when WiFi/data unavailable",
                        "Works cross-carrier and internationally",
                        "Message delivery via base station network"
                    ).forEach { feature ->
                        Row(modifier = Modifier.padding(vertical = 4.dp)) {
                            Text("• ", color = BrandBlue)
                            Text(feature, color = TextPrimary)
                        }
                    }
                }
            }

            Spacer(modifier = Modifier.height(16.dp))

            // Cost & Privacy Card
            Card(
                modifier = Modifier.fillMaxWidth(),
                colors = CardDefaults.cardColors(containerColor = Color(0xFFFFF3CD))
            ) {
                Column(modifier = Modifier.padding(16.dp)) {
                    Text("⚠️ Important", fontWeight = FontWeight.SemiBold, color = TextPrimary)
                    Spacer(modifier = Modifier.height(8.dp))
                    listOf(
                        "SMS may incur carrier charges (check your plan)",
                        "Carrier sees phone numbers and timestamps",
                        "Messages fragmented into multiple SMS",
                        "Requires SEND_SMS and RECEIVE_SMS permissions"
                    ).forEach { note ->
                        Row(modifier = Modifier.padding(vertical = 4.dp)) {
                            Text("• ", color = TextPrimary)
                            Text(note, color = TextPrimary, fontSize = 13.sp)
                        }
                    }
                }
            }

            Spacer(modifier = Modifier.height(16.dp))

            // Security Card
            Card(
                modifier = Modifier.fillMaxWidth(),
                colors = CardDefaults.cardColors(containerColor = Color(0xE8F5E9))
            ) {
                Column(modifier = Modifier.padding(16.dp)) {
                    Text("🔒 Security", fontWeight = FontWeight.SemiBold, color = TextPrimary)
                    Spacer(modifier = Modifier.height(8.dp))
                    listOf(
                        "ChaCha20-Poly1305 AEAD encryption per message",
                        "Double Ratchet protocol (forward secrecy)",
                        "AEAD tag prevents tampering/replay attacks",
                        "Device ID hashing in SMS header"
                    ).forEach { sec ->
                        Row(modifier = Modifier.padding(vertical = 4.dp)) {
                            Text("• ", color = TextPrimary)
                            Text(sec, color = TextPrimary, fontSize = 13.sp)
                        }
                    }
                }
            }

            Spacer(modifier = Modifier.height(24.dp))

            // Enable Toggle
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(Color.White, shape = RoundedCornerShape(8.dp))
                    .padding(16.dp),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column {
                    Text("Enable SMS Fallback", fontWeight = FontWeight.SemiBold, color = TextPrimary)
                    Text("Use SMS when offline", color = TextSecondary, fontSize = 12.sp)
                }
                Switch(
                    checked = isEnabled,
                    onCheckedChange = { enabled ->
                        onEnableSms(enabled)
                    },
                    modifier = Modifier.padding(start = 16.dp)
                )
            }

            Spacer(modifier = Modifier.height(24.dp))

            // Consent Text
            Text(
                "By enabling SMS fallback, you consent to:\n" +
                "• Sending and receiving SMS messages\n" +
                "• Potential SMS charges from your carrier\n" +
                "• CarrierBridge storing SMS-related data locally",
                fontSize = 12.sp,
                color = TextSecondary
            )

            Spacer(modifier = Modifier.height(32.dp))
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SmsConsentDialog(
    onAccept: () -> Unit,
    onDecline: () -> Unit
) {
    AlertDialog(
        onDismissRequest = onDecline,
        title = { Text("Enable SMS Messaging?") },
        text = {
            Column {
                Text(
                    "CarrierBridge can send encrypted messages via SMS when WiFi/data is unavailable.",
                    modifier = Modifier.padding(bottom = 12.dp)
                )
                Text(
                    "Carrier charges may apply. Your messages stay encrypted (carrier cannot read them).",
                    style = MaterialTheme.typography.bodySmall,
                    color = TextSecondary
                )
            }
        },
        confirmButton = {
            Button(onClick = onAccept, colors = ButtonDefaults.buttonColors(containerColor = BrandBlue)) {
                Text("Enable", color = Color.White)
            }
        },
        dismissButton = {
            TextButton(onClick = onDecline) {
                Text("Not Now")
            }
        }
    )
}
