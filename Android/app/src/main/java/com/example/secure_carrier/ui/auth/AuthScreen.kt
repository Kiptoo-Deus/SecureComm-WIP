package com.example.secure_carrier.ui.auth

import android.content.Context
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.ui.Alignment
import androidx.compose.ui.unit.sp
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.navigation.NavController
@Composable
fun AuthScreen(viewModel: AuthViewModel, onAuthSuccess: () -> Unit, navController: NavController) {
    val ctx = LocalContext.current
    // Autofill display name from SharedPreferences if available
    val prefs = ctx.getSharedPreferences("secure_carrier", Context.MODE_PRIVATE)
    LaunchedEffect(Unit) {
        val storedName = prefs.getString("display_name", null)
        if (!storedName.isNullOrBlank() && viewModel.displayName.isBlank()) {
            viewModel.displayName = storedName
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
        OutlinedTextField(
            value = viewModel.phone,
            onValueChange = { viewModel.phone = it },
            label = { Text("Phone") },
            modifier = Modifier.fillMaxWidth()
        )
        OutlinedTextField(
            value = viewModel.displayName,
            onValueChange = { viewModel.displayName = it },
            label = { Text("Display name") },
            modifier = Modifier.fillMaxWidth()
        )
        Button(
            onClick = { viewModel.requestOtp() },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Request OTP")
        }
        OutlinedTextField(
            value = viewModel.otp,
            onValueChange = { viewModel.otp = it },
            label = { Text("OTP (auto-filled after request)") },
            modifier = Modifier.fillMaxWidth()
        )
        Button(
            onClick = {
                viewModel.verifyOtp(ctx) { userId, token ->
                    saveAuth(ctx, userId, token)
                    onAuthSuccess()
                }
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Verify OTP")
        }
        val status = viewModel.status
        if (status != null) {
            Text(text = status, modifier = Modifier.padding(top = 8.dp), fontSize = 16.sp)
        }
        Spacer(modifier = Modifier.height(32.dp))
    }
}

fun saveAuth(ctx: Context, userId: String, token: String) {
    val prefs = ctx.getSharedPreferences("secure_carrier", Context.MODE_PRIVATE)
    prefs.edit().putString("auth_token", token).putString("user_id", userId).apply()
}
