package com.example.carrierbridge_android.ui.navigation

import android.content.Context
import androidx.compose.runtime.Composable
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.example.carrierbridge.jni.CarrierBridgeClient
import com.example.carrierbridge_android.ui.ChatScreen
import com.example.carrierbridge_android.ui.settings.SmsSettingsScreen
import com.example.carrierbridge_android.ui.settings.AppSettingsScreen

sealed class Screen(val route: String) {
    object Chat : Screen("chat")
    object SmsSettings : Screen("sms_settings")
    object AppSettings : Screen("app_settings")
}

@Composable
fun AppNavigation(
    carrierClient: CarrierBridgeClient?,
    context: Context
) {
    val navController = rememberNavController()

    NavHost(navController = navController, startDestination = Screen.Chat.route) {
        composable(Screen.Chat.route) {
            ChatScreen(
                deviceId = "alice",
                recipientId = "bob",
                carrierClient = carrierClient,
                onRecipientChange = {}
            )
        }

        composable(Screen.SmsSettings.route) {
            SmsSettingsScreen(
                onBackClick = { navController.popBackStack() },
                onEnableSms = { enabled ->
                    carrierClient?.setSmsEnabled(context, enabled)
                },
                isEnabled = carrierClient?.isSmsEnabled() ?: false
            )
        }

        composable(Screen.AppSettings.route) {
            AppSettingsScreen(
                onBackClick = { navController.popBackStack() },
                onSmsSettingsClick = { navController.navigate(Screen.SmsSettings.route) },
                carrierClient = carrierClient
            )
        }
    }
}
