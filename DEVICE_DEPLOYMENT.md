# CarrierBridge Android MVP - Device Deployment Guide

This guide walks you through building, deploying, and testing the CarrierBridge Android app on physical devices.

## Prerequisites

- macOS with Xcode Command Line Tools
- Android SDK + NDK (API 24+)
- Two Android devices (API 24+) for testing
- Go 1.19+ (for server)
- `adb` command-line tool
- `curl` for downloading prebuilt libraries

## Quick Start (Automated)

### 1. Download and Prepare Native Libraries

```bash
cd /Users/joel/Documents/GitHub/CarrierBridge
chmod +x scripts/download_libsodium.sh
./scripts/download_libsodium.sh
```

This downloads prebuilt `libsodium.so` for all supported ABIs (arm64-v8a, armeabi-v7a, x86, x86_64) and places them in:
```
android/app/src/main/jniLibs/<abi>/libsodium.so
```

### 2. Build the Android APK

```bash
chmod +x scripts/build.sh
./scripts/build.sh
```

This will:
- Verify native libraries are in place
- Build the JNI bridge against `libsodium` and `libsecurecomm`
- Assemble the debug APK

**Output:** `android/app/build/outputs/apk/debug/app-debug.apk`

### 3. Start the Server

In one terminal:

```bash
chmod +x scripts/start_server.sh
./scripts/start_server.sh
```

The server will:
- Listen on `localhost:8080` (WebSocket at `/ws`)
- Create/use `server/carrier.db` for persistence
- Print: `Server starting on :8080`

**Find your machine's IP for Android:**
```bash
ifconfig | grep "inet " | grep -v 127.0.0.1
```

Example: If your IP is `192.168.1.100`, you'll connect to `ws://192.168.1.100:8080/ws`

### 4. Install APK on Devices

Connect your first Android device via USB and enable USB debugging, then:

```bash
adb install android/app/build/outputs/apk/debug/app-debug.apk
```

Do the same for the second device (or use multiple `adb` commands).

### 5. Test on Devices

1. **Device 1:**
   - Open CarrierBridge app
   - Tap **Register** (auto-generates a unique user ID, shown in logs)
   - Wait ~2 seconds, then tap **Connect**
   - In the connection URL field, enter: `ws://192.168.1.100:8080/ws` (use your actual IP)
   - Status should show "Connecting..." then "Connected"

2. **Device 2:**
   - Do the same registration and connection
   - Both devices should now show "Connected" status

3. **Send Messages:**
   - On Device 1, type a message and tap **Send**
   - On Device 2, the message should appear in the message list
   - Vice versa

## Manual Steps (If Automation Fails)

### Option A: Build libsodium Manually

If `download_libsodium.sh` fails, download prebuilt binaries:

1. Visit: https://github.com/jedisct1/libsodium/releases
2. Download the Android prebuilt for each ABI (e.g., `libsodium-1.0.18-android-armv8.tar.gz`)
3. Extract and place `libsodium.so` into:
   ```
   android/app/src/main/jniLibs/arm64-v8a/libsodium.so
   android/app/src/main/jniLibs/armeabi-v7a/libsodium.so
   android/app/src/main/jniLibs/x86/libsodium.so
   android/app/src/main/jniLibs/x86_64/libsodium.so
   ```

### Option B: Build APK Manually

```bash
cd android
./gradlew clean assembleDebug
```

### Option C: Start Server Manually

```bash
cd server
go mod tidy
go run main.go
```

## Troubleshooting

### APK Build Fails

- Ensure NDK is installed: `Android Studio → SDK Manager → SDK Tools → NDK`
- Check CMake path in `android/app/build.gradle.kts`
- Verify libsodium.so exists in `jniLibs`

### "Symbol not found" at Runtime

- Ensure `libsodium.so` is present in the APK: `unzip -l app-debug.apk | grep libsodium`
- Check ABI mismatch between your device and built APK
- Run: `adb shell getprop ro.product.cpu.abi` to see device ABI

### Cannot Connect to Server

- Verify server is running: `curl ws://localhost:8080/healthz` (or check for "Server starting" log)
- Ping your machine's IP from Android: `ping 192.168.1.100`
- Check firewall isn't blocking port 8080
- Ensure Android device is on the same Wi-Fi network

### No Messages Received

- Check server logs for `WebSocket upgrade` and envelope processing
- Verify both devices have unique user IDs (should auto-generate)
- Check device logcat: `adb logcat | grep carrierbridge`

## Code Structure

- **Native JNI:** `android/app/src/main/cpp/carrierbridge_jni.cpp` + `android_jni_transport.cpp`
- **Kotlin Bridge:** `android/app/src/main/java/org/carrierbridge/NativeBridge.kt`
- **WebSocket:** `android/app/src/main/java/org/carrierbridge/NativeWebSocket.kt`
- **UI:** `android/app/src/main/java/org/carrierbridge/ChatScreen.kt` + `ChatViewModel.kt`
- **Server:** `server/main.go` (WebSocket hub + persistence)

## Next Steps After MVP

- [ ] Add end-to-end encryption key material exchange (X3DH prekey upload)
- [ ] Implement persistent message queue (SQLite on device)
- [ ] Add FCM push notifications for offline delivery
- [ ] Secure key storage (Android Keystore)
- [ ] Contact discovery / phone number verification
- [ ] Full message history and conversation UI

---

**Questions?** Check `server/main.go` for HTTP endpoints or `docs/ARCHITECTURE.md` for protocol details.
