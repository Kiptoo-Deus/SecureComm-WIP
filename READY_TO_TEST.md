# CarrierBridge MVP - Ready to Test

**Status:** Both APK and server built and ready for device testing.

## Built Artifacts

- **APK:** `android/app/build/outputs/apk/debug/app-debug.apk`
- **Server:** `server/server_binary`

## Quick Start (5 minutes)

### 1. Start the Server

Open a terminal and run:

```bash
cd /Users/joel/Documents/GitHub/CarrierBridge/server
./server_binary
```

You should see:
```
Server starting on :8080
```

### 2. Find Your Machine's IP

In another terminal:

```bash
ifconfig | grep "inet " | grep -v 127.0.0.1
```

Example output: `inet 192.168.1.100`

**Note this IP down** — you'll need it in the app.

### 3. Install APK on First Device

Connect your first Android phone via USB (enable Developer Mode & USB Debugging):

```bash
adb install /Users/joel/Documents/GitHub/CarrierBridge/android/app/build/outputs/apk/debug/app-debug.apk
```

Open the app on the phone.

### 4. Install APK on Second Device

Repeat step 3 on a second Android phone, or use an emulator.

### 5. Connect Both Devices

**On each device:**

1. Tap **Inbox** or open the chat screen
2. Look for a "Connect" button or settings
3. Enter server URL: `ws://192.168.1.100:8080/ws` (replace 192.168.1.100 with your actual IP from step 2)
4. Tap **Connect**

**Status should change to "Connected"**

### 6. Send a Test Message

- Type a message on Device 1
- Tap **Send**
- Message appears on Device 2 in real-time

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        Two Android Devices                   │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────────┐           ┌──────────────────┐        │
│  │  Device 1        │           │  Device 2        │        │
│  │  - MainActivity  │           │  - MainActivity  │        │
│  │  - ChatScreen    │           │  - ChatScreen    │        │
│  │  - OkHttp WS     │           │  - OkHttp WS     │        │
│  │  - JNI Bridge    │           │  - JNI Bridge    │        │
│  └────────┬─────────┘           └────────┬─────────┘        │
│           │                               │                   │
│           └─────────────┬─────────────────┘                   │
│                         │                                     │
└─────────────────────────┼─────────────────────────────────────┘
                          │
                   WebSocket (encrypted)
                          │
         ┌────────────────┼────────────────┐
         │                │                │
         ▼                ▼                ▼
  ┌────────────────────────────────────────────┐
  │         Go Server (localhost:8080)         │
  │                                            │
  │  - WebSocket Hub (/ws)                     │
  │  - User Registration (/v1/register)        │
  │  - Key Provisioning (/v1/keys/{user})      │
  │  - Persistent Queue (SQLite)               │
  │  - Message Relay                           │
  │                                            │
  │  Database: carrier.db                      │
  └────────────────────────────────────────────┘
```

## Code Locations

### JNI Bridge (C++ Native)
- **JNI Stub:** `android/app/src/main/cpp/carrierbridge_jni.cpp`
  - Minimal MVP mode (no libsecurecomm dependency)
  - Methods: init, send, register_device, create_session
  - Handles socket data routing

### Transport (Kotlin)
- **WebSocket Manager:** `android/app/src/main/java/com/example/carrierbridge/transport/WebSocketTransport.kt` (or similar)
- **OkHttp Client:** Added to Gradle dependencies
- Manages TLS/WSS connections to server

### Server
- **Main Logic:** `server/main.go`
  - WebSocket upgrade and message relay
  - SQLite persistence for users and queued messages
  - Registration endpoint: `POST /v1/register`
  - Key fetch endpoint: `GET /v1/keys/{user}`
  - Message streaming: `GET /ws?token=<token>`

### UI
- **MainActivity:** `android/app/src/main/java/com/example/carrierbridge_android/MainActivity.kt`
- **ChatScreen:** `android/app/src/main/java/com/example/carrierbridge_android/ui/ChatScreen.kt`

## Troubleshooting

### "Connection refused" on device

- Make sure both device and server are on same Wi-Fi network
- Use correct IP from `ifconfig` (not localhost)
- Check server is still running: `curl -v ws://localhost:8080/healthz` on your machine

### JNI library not found

- Ensure `libcarrierbridge_jni.so` was built: check `android/app/build/outputs/lib` contains it
- APK bundles it automatically; verify with: `unzip -l app-debug.apk | grep carrierbridge_jni.so`

### Messages not syncing

- Check device logcat: `adb logcat | grep -i "carrier\|websocket"`
- Server logs should show `[MSG] device_a → device_b` when messages relay
- SQLite database: `ls -la server/carrier.db`

### Rebuild APK

If you make code changes:

```bash
cd android
./gradlew clean assembleDebug
adb install app/build/outputs/apk/debug/app-debug.apk
```

## Next Steps (Phase 2+)

- [ ] **Full X3DH Handshake:** Integrate real prekey exchange from `src/libsecurecomm/src/x3dh.cpp`
- [ ] **Ratchet State:** Use `libsecurecomm` Double Ratchet for ongoing encryption
- [ ] **Secure Storage:** Android Keystore + encrypted SQLite for key material
- [ ] **Offline Queue:** Persist messages locally when offline, deliver on reconnect
- [ ] **FCM Push:** Send notifications for new messages (requires Firebase setup)
- [ ] **Phone Verification:** OTP-based registration instead of auto-ID
- [ ] **Contact Discovery:** Sync contacts and show availability

## Testing Commands

### From your machine (test server API)

```bash
# Check server is alive
curl http://localhost:8080/healthz

# Register a test user
curl -X POST http://localhost:8080/v1/register \
  -H "Content-Type: application/json" \
  -d '{
    "phone": "+1234567890",
    "display_name": "Alice",
    "identity_pub": "...",
    "signed_prekey": "...",
    "one_time_prekeys": []
  }'

# Fetch keys for a user
curl http://localhost:8080/v1/keys/user123
```

### On device (check connectivity)

```bash
adb shell ping 192.168.1.100
adb shell curl -v ws://192.168.1.100:8080/ws
```

---

**Questions or issues?** Check `DEVICE_DEPLOYMENT.md` for detailed setup instructions.
