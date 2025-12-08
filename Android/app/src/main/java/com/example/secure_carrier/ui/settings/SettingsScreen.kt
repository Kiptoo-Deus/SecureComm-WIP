package com.example.secure_carrier.ui.settings

import android.content.Context
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp

@Composable
fun SettingsScreen(onThemeChange: (Boolean) -> Unit, isDarkTheme: Boolean) {
    val ctx = LocalContext.current
    var darkTheme by remember { mutableStateOf(isDarkTheme) }

    Column(modifier = Modifier.padding(16.dp)) {
        Text("Settings", modifier = Modifier.padding(bottom = 16.dp))
        Text("Dark Mode")
        Switch(
            checked = darkTheme,
            onCheckedChange = {
                darkTheme = it
                saveThemePref(ctx, it)
                onThemeChange(it)
            },
            modifier = Modifier.fillMaxWidth().padding(top = 8.dp)
        )
    }
}

fun saveThemePref(ctx: Context, dark: Boolean) {
    val prefs = ctx.getSharedPreferences("secure_carrier", Context.MODE_PRIVATE)
    prefs.edit().putBoolean("dark_theme", dark).apply()
}

fun loadThemePref(ctx: Context): Boolean {
    val prefs = ctx.getSharedPreferences("secure_carrier", Context.MODE_PRIVATE)
    return prefs.getBoolean("dark_theme", false)
}
