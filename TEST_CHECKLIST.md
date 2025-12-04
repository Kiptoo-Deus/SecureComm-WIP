# Device Testing Checklist

## Pre-Test Setup

- [ ] Server built: `server/server_binary` exists
- [ ] APK built: `android/app/build/outputs/apk/debug/app-debug.apk` exists
- [ ] Two Android devices connected via USB or two emulators running
- [ ] Devices on same Wi-Fi network as your machine
- [ ] Machine IP noted (from `ifconfig`)

## Test Execution

### Phase 1: Server Startup

- [ ] Terminal 1: Run `./server/server_binary`
  - [ ] Verify: "Server starting on :8080"
  - [ ] Keep running throughout testing

### Phase 2: Device 1 Setup

- [ ] Install APK: `adb install android/app/build/outputs/apk/debug/app-debug.apk`
- [ ] Launch app on Device 1
- [ ] Verify app opens without crashes
- [ ] Note the user ID displayed (should be auto-generated)

### Phase 3: Device 1 Connection

- [ ] Tap "Inbox" or "Messages" screen
- [ ] Find connection settings (usually gear icon or "Connect" button)
- [ ] Enter server URL: `ws://192.168.1.100:8080/ws` (use your actual IP)
- [ ] Tap "Connect"
- [ ] Wait 2 seconds
- [ ] Verify status changes to "Connected" or similar
- [ ] Check logcat: `adb logcat | grep carrier` for connection logs

### Phase 4: Device 2 Setup

- [ ] Repeat Phase 2 on second device
- [ ] Repeat Phase 3 on second device
- [ ] Both devices should show "Connected"

### Phase 5: Messaging Test

**Device 1 → Device 2:**
- [ ] Type test message: "Hello from Device 1"
- [ ] Tap "Send"
- [ ] Check Device 2 screen
- [ ] Verify message appears within 2 seconds

**Device 2 → Device 1:**
- [ ] Type response: "Hello from Device 2"
- [ ] Tap "Send"
- [ ] Check Device 1 screen
- [ ] Verify message appears within 2 seconds

**Multi-message stress test:**
- [ ] Send 5 quick messages from Device 1
- [ ] Verify all appear on Device 2 in order
- [ ] Repeat in reverse

### Phase 6: Edge Cases

- [ ] **Disconnect & Reconnect:**
  - [ ] Turn off Wi-Fi on Device 1
  - [ ] Tap "Disconnect" or wait for timeout
  - [ ] Turn Wi-Fi back on
  - [ ] Tap "Connect" again
  - [ ] Verify reconnection succeeds

- [ ] **Background message:**
  - [ ] Send message from Device 1
  - [ ] Switch Device 2 to another app
  - [ ] Send another message
  - [ ] Return to CarrierBridge on Device 2
  - [ ] Verify both messages are there

- [ ] **Offline message (if implemented):**
  - [ ] Device 1 offline (airplane mode)
  - [ ] Device 2 sends message
  - [ ] Device 1 goes online
  - [ ] Verify message is delivered

## Success Criteria

✅ **MVP Success** (all required):
1. Both devices install APK without errors
2. Both devices connect to server
3. Messages send and receive in both directions
4. Messages deliver within 2 seconds
5. No app crashes during basic messaging

⭐ **Nice to Have**:
- Message ordering is correct
- Reconnection works smoothly
- 10+ messages can be exchanged without issues

## Failure Diagnosis

| Symptom | Likely Cause | Fix |
|---------|--------------|-----|
| "Connection refused" | Server not running | Start: `./server/server_binary` |
| "Cannot resolve host" | Wrong IP or network | Use correct IP, check Wi-Fi |
| JNI crash on startup | Missing .so | Verify CMake built correctly |
| Messages don't appear | Transport issue | Check logcat and server logs |
| App won't install | APK corrupted or old ABI | Rebuild: `./gradlew clean assembleDebug` |

## Logging Commands

Get detailed diagnostics:

```bash
# App logs (in terminal)
adb logcat | grep -i "carrier\|websocket\|jni"

# Server logs (check output)
# - Look for "[WebSocket] upgrade" messages
# - Look for "[MSG]" relay messages

# Database check
ls -la server/carrier.db
# Should be > 1KB if messages are stored

# Network check on device
adb shell ping 192.168.1.100
adb shell curl -v ws://192.168.1.100:8080/healthz
```

## After Testing

- [ ] Note any crashes or unusual behavior
- [ ] Save logcat output: `adb logcat > device1.log`
- [ ] Take screenshots of successful messaging
- [ ] Update issues/next-steps list in `ARCHITECTURE.md`

---

**Expected time:** 10-15 minutes for full test cycle

**Questions?** See `READY_TO_TEST.md` for detailed walkthrough
