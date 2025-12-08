package com.example.secure_carrier

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.runtime.mutableStateOf
import androidx.compose.ui.tooling.preview.Preview
import androidx.lifecycle.viewmodel.compose.viewModel
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.example.secure_carrier.ui.auth.AuthScreen
import com.example.secure_carrier.ui.chat.ChatScreen
import com.example.secure_carrier.ui.theme.Secure_CarrierTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
            setContent {
                val ctx = this
                val darkThemeState = remember { mutableStateOf(com.example.secure_carrier.ui.settings.loadThemePref(ctx)) }
                Secure_CarrierTheme(darkTheme = darkThemeState.value) {
                    Surface(color = MaterialTheme.colorScheme.background) {
                        val navController = rememberNavController()
                        NavHost(navController = navController, startDestination = "auth") {
                            composable("auth") {
                                val vm: com.example.secure_carrier.ui.auth.AuthViewModel = viewModel()
                                AuthScreen(viewModel = vm, onAuthSuccess = { navController.navigate("chat") }, navController = navController)
                            }
                            composable("chat") {
                                val vm: com.example.secure_carrier.ui.auth.AuthViewModel = viewModel()
                                ChatScreen(authViewModel = vm, navController = navController)
                            }
                            composable("settings") {
                                com.example.secure_carrier.ui.settings.SettingsScreen(
                                    onThemeChange = { dark -> darkThemeState.value = dark },
                                    isDarkTheme = darkThemeState.value
                                )
                            }
                        }
                    }
                }
            }
    }
}

@Preview(showBackground = true)
@Composable
fun PreviewMain() {
    Secure_CarrierTheme {
        GreetingPreview()
    }
}

@Composable
fun GreetingPreview() {}