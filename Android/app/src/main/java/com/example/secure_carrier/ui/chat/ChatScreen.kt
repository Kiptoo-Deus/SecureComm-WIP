@file:OptIn(androidx.compose.material3.ExperimentalMaterial3Api::class)
package com.example.secure_carrier.ui.chat

import android.content.Context
import android.util.Base64
import androidx.navigation.NavController
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.ui.Alignment
import androidx.compose.ui.unit.sp
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Button
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.example.secure_carrier.net.WebSocketManager
import org.json.JSONObject
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults

@Composable
fun ChatScreen(authViewModel: com.example.secure_carrier.ui.auth.AuthViewModel, navController: NavController) {
    val ctx = LocalContext.current
    val prefs = ctx.getSharedPreferences("secure_carrier", Context.MODE_PRIVATE)
    val token = prefs.getString("auth_token", null)
    var recipient by remember { mutableStateOf("") }
    var message by remember { mutableStateOf("") }
    val messages = remember { mutableStateListOf<String>() }
    data class OnlineUser(val userId: String, val displayName: String)
    val onlineUsers = remember { mutableStateListOf<OnlineUser>() }
    var expanded by remember { mutableStateOf(false) }

    LaunchedEffect(token) {
        token?.let {
            WebSocketManager.connect(it) { msg ->
                try {
                    val obj = JSONObject(msg)
                    if (obj.optString("type") == "online_users") {
                        onlineUsers.clear()
                        val arr = obj.optJSONArray("users")
                        if (arr != null) {
                            for (i in 0 until arr.length()) {
                                val userObj = arr.getJSONObject(i)
                                val userId = userObj.optString("userId")
                                val displayName = userObj.optString("displayName")
                                onlineUsers.add(OnlineUser(userId, displayName))
                            }
                        }
                    } else {
                        messages.add(msg)
                    }
                } catch (e: Exception) {
                    messages.add(msg)
                }
            }
        }
    }

    Column(
        modifier = Modifier
            .padding(16.dp)
            .verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.spacedBy(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        androidx.compose.material3.Button(
            onClick = { navController.navigate("settings") },
            modifier = Modifier.fillMaxWidth()
        ) {
            androidx.compose.material3.Text("Settings")
        }
        Box(modifier = Modifier.height(240.dp).fillMaxWidth()) {
            LazyColumn {
                items(messages) { m ->
                    Text(m, fontSize = 16.sp)
                }
            }
        }
        Text("Online Users:", modifier = Modifier.align(Alignment.Start))
        ExposedDropdownMenuBox(
            expanded = expanded,
            onExpandedChange = { expanded = !expanded }
        ) {
            OutlinedTextField(
                value = recipient,
                onValueChange = { recipient = it },
                label = { Text("Recipient ID") },
                modifier = Modifier.fillMaxWidth(),
                readOnly = false,
                trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = expanded) }
            )
            DropdownMenu(
                expanded = expanded,
                onDismissRequest = { expanded = false }
            ) {
                onlineUsers.forEach { user ->
                    DropdownMenuItem(
                        text = { Text("${user.displayName} (${user.userId})") },
                        onClick = {
                            recipient = user.userId
                            expanded = false
                        }
                    )
                }
            }
        }
        OutlinedTextField(
            value = message,
            onValueChange = { message = it },
            label = { Text("Message") },
            modifier = Modifier.fillMaxWidth()
        )
        Button(
            onClick = {
                val userId = prefs.getString("user_id", "") ?: ""
                val obj = JSONObject()
                obj.put("type", "chat")
                obj.put("message_id", System.currentTimeMillis().toString())
                obj.put("sender_id", userId)
                obj.put("recipient", recipient)
                val payload = Base64.encodeToString(message.toByteArray(), Base64.NO_WRAP)
                obj.put("payload", payload)
                WebSocketManager.send(obj.toString())
                messages.add("me → $recipient: $message")
                message = ""
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Send")
        }
        Spacer(modifier = Modifier.height(32.dp))
    }
}
