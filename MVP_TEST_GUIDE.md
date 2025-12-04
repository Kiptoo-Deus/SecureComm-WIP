# MVP Feature Test Guide

## New Features Added

### Mobile App (Android)

#### 1. **Authentication System**
- **AuthStartScreen**: Entry point - user chooses between phone or email signup
- **PhoneSignupScreen**: 2-step phone signup with OTP verification
  - Step 1: Enter phone number + display name
  - Step 2: Enter 6-digit OTP received from server
- **EmailSignupScreen**: 2-step email signup with token verification
  - Step 1: Enter email + display name
  - Step 2: Enter verification token sent to email
- **AuthRepository**: Manages auth state, stores credentials in SharedPreferences, handles API calls

#### 2. **Contact Discovery**
- **ContactListScreen**: Shows available contacts (users registered on platform)
  - Displays contact name, phone/email, online status (green dot), last seen time
  - Pull to refresh syncs with server
  - Tap contact to open chat (placeholder)
- **ContactRepository**: Reads device contacts, discovers available users via backend
  - Privacy-preserving: Hashes phone/email before sending to server
  - Uses SHA-256 for contact hashing

#### 3. **Data Persistence**
- SharedPreferences for auth tokens, user IDs, phone/email
- Unique device ID stored locally
- Session management

### Backend Server (Go)

#### 1. **Phone Authentication Endpoints**
- **POST /api/auth/register/phone**
  - Request: `{"phone": "+1234567890", "display_name": "John"}`
  - Response: `{"status": "ok", "message": "OTP sent to phone"}`
  - Generates 6-digit OTP, stores with 5-minute expiry
  - OTP logged to server stdout for testing

- **POST /api/auth/verify/phone**
  - Request: `{"phone": "+1234567890", "otp": "123456", "display_name": "John"}`
  - Response: `{"status": "ok", "user_id": "user_xxx", "auth_token": "xxx", "phone": "+1234567890", "display_name": "John"}`
  - Validates OTP, creates user in SQLite, returns auth token
  - Marks OTP as verified

#### 2. **Email Authentication Endpoints**
- **POST /api/auth/register/email**
  - Request: `{"email": "user@example.com", "display_name": "Jane"}`
  - Response: `{"status": "ok", "message": "Verification link sent to email"}`
  - Generates 32-byte verification token, stores with 24-hour expiry
  - Token logged to server stdout for testing

- **POST /api/auth/verify/email**
  - Request: `{"email": "user@example.com", "token": "xxx", "display_name": "Jane"}`
  - Response: `{"status": "ok", "user_id": "user_yyy", "auth_token": "yyy", "email": "user@example.com", "display_name": "Jane"}`
  - Validates token, creates user, returns auth token
  - Marks token as verified

#### 3. **Contact Discovery Endpoint**
- **POST /api/contacts/discover**
  - Authorization: Bearer token (in Authorization header)
  - Request: `{"hashed_contacts": ["sha256_hash_of_phone_1", "sha256_hash_of_email_1", ...]}`
  - Response: `{"status": "ok", "contacts": [{"user_id": "user_1", "display_name": "Alice", "phone": "+1234567890", "email": null, "online": true, "last_seen": 1733280000}]}`
  - Hashes stored user phones/emails, compares with request hashes
  - Returns only contacts that match (privacy-preserving)
  - Requires valid auth token

#### 4. **Database Schema Updates**
New tables added:
- `otp_requests`: Stores phone → OTP mappings with expiry
- `email_verifications`: Stores email → verification token mappings with expiry
- Updated `users` table: Added `email`, `created_at`, `online`, `last_seen` fields

---

## How to Test

### Prerequisites
1. Start the backend server:
   ```bash
   cd server
   go run main.go
   ```
   Server runs on `http://localhost:8080`

2. Update Android app server URL:
   - For Android emulator: `http://10.0.2.2:8080` (default in code)
   - For physical device: Update `SERVER_URL` in `AuthRepository.kt` to your machine's IP (e.g., `http://192.168.1.100:8080`)

3. Build and run Android app:
   ```bash
   cd android
   ./gradlew clean assembleDebug
   ```

### Test Scenario 1: Phone Signup
1. Launch app → "Sign up with Phone"
2. Enter phone: `+12345678900`, display name: `TestUser1`
3. Tap "Continue"
4. Check server stdout for OTP (e.g., `[Auth] OTP for +12345678900: 123456`)
5. Enter OTP in app, tap "Verify Phone"
6. Expected: Success screen with user_id and auth_token displayed
7. Verify in database:
   ```bash
   sqlite3 server/server.db "SELECT id, phone, display_name FROM users;"
   ```

### Test Scenario 2: Email Signup
1. From app start, tap "Sign up with Email"
2. Enter email: `test@example.com`, display name: `TestUser2`
3. Tap "Continue"
4. Check server stdout for verification token (e.g., `[Auth] Email verification token for test@example.com: xxx`)
5. Enter token in app, tap "Verify Email"
6. Expected: Success screen with user_id and auth_token
7. Verify in database:
   ```bash
   sqlite3 server/server.db "SELECT id, email, display_name FROM users;"
   ```

### Test Scenario 3: Contact Discovery
1. Create 2-3 users via phone signup (as above)
2. On one user's device:
   - Go to Contacts screen
   - Grant device contacts permission
   - Tap "Refresh"
3. Expected: Any device contacts that match registered users appear with:
   - Green online indicator
   - Last seen timestamp
   - Display name from registration

### Test Scenario 4: Contact Matching (Privacy)
1. User A signs up with phone `+11234567890`
2. User B's device has User A in contacts as `+11234567890`
3. User B discovers contacts:
   - App hashes `+11234567890` with SHA-256
   - Sends hash to server (phone number is NOT sent)
   - Server compares hash with stored user hashes
   - User A appears in results
4. Verify privacy:
   - Check server logs - only hashed values should be visible
   - Raw phone numbers are never transmitted to server

---

## Expected Behavior

### Successful Phone Flow
```
AuthStartScreen
  ↓ (tap "Sign up with Phone")
PhoneSignupScreen (ENTER_PHONE)
  ↓ (enter +1234567890, "User1", tap Continue)
PhoneSignupScreen (VERIFY_OTP)
  ↓ (enter OTP from server logs, tap Verify)
Success! User created with auth token
  ↓
ContactListScreen (requires next step)
```

### Successful Email Flow
```
AuthStartScreen
  ↓ (tap "Sign up with Email")
EmailSignupScreen (ENTER_EMAIL)
  ↓ (enter user@example.com, "User2", tap Continue)
EmailSignupScreen (VERIFY_TOKEN)
  ↓ (enter token from server logs, tap Verify)
Success! User created with auth token
  ↓
ContactListScreen (requires next step)
```

### Contact Discovery Flow
```
ContactListScreen
  ↓ (tap Refresh)
App reads device contacts → ContactRepository
  ↓
ContactRepository hashes phones/emails
  ↓
POST /api/contacts/discover with auth token
  ↓
Server returns matching registered users
  ↓
Display available contacts with online status
```

---

## Server Testing (Using cURL)

### Phone Signup - Request OTP
```bash
curl -X POST http://localhost:8080/api/auth/register/phone \
  -H "Content-Type: application/json" \
  -d '{"phone":"+12345678900","display_name":"Test User"}'
```
Expected response:
```json
{"status":"ok","message":"OTP sent to phone (logged to server: 123456)"}
```

### Phone Signup - Verify OTP
```bash
curl -X POST http://localhost:8080/api/auth/verify/phone \
  -H "Content-Type: application/json" \
  -d '{"phone":"+12345678900","otp":"123456","display_name":"Test User"}'
```
Expected response:
```json
{"status":"ok","user_id":"user_1733280000000000000","auth_token":"xxx...","phone":"+12345678900","display_name":"Test User"}
```

### Email Signup - Request Token
```bash
curl -X POST http://localhost:8080/api/auth/register/email \
  -H "Content-Type: application/json" \
  -d '{"email":"test@example.com","display_name":"Test Email"}'
```
Expected response:
```json
{"status":"ok","message":"Verification link sent to email (token: xxx...)"}
```

### Email Signup - Verify Token
```bash
curl -X POST http://localhost:8080/api/auth/verify/email \
  -H "Content-Type: application/json" \
  -d '{"email":"test@example.com","token":"xxx...","display_name":"Test Email"}'
```
Expected response:
```json
{"status":"ok","user_id":"user_yyy...","auth_token":"yyy...","email":"test@example.com","display_name":"Test Email"}
```

### Contact Discovery
```bash
curl -X POST http://localhost:8080/api/contacts/discover \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_AUTH_TOKEN" \
  -d '{"hashed_contacts":["hash1","hash2"]}'
```
Expected response:
```json
{"status":"ok","contacts":[{"user_id":"user_1","display_name":"Alice","phone":"+12345678900","email":null,"online":true,"last_seen":1733280000}]}
```

---

## Known Limitations (MVP)

1. **No real SMS/Email delivery**: OTP and verification tokens are logged to server stdout, not actually sent
2. **No end-to-end encryption**: Messages not yet encrypted with Double Ratchet
3. **No real-time messaging**: WebSocket connected but message send/receive not wired to crypto layer
4. **No offline message queueing**: Message queue exists but not integrated with UI
5. **Contact discovery UI**: Shows contacts but "tap to chat" not yet connected to ChatScreen
6. **No user profile screen**: Can't edit profile after signup
7. **No logout**: Users stay logged in (SharedPreferences persists auth token)
8. **No network error handling**: Limited error recovery in API calls

---

## Next Steps (Post-MVP)

1. **Wire native crypto**: Integrate libsecurecomm with JNI
2. **Create ChatScreen integration**: Open chat when contact tapped
3. **Implement message send/receive**: WebSocket + encryption
4. **Add offline queueing**: Queue messages when no network
5. **SMS fallback**: Transport messages via SMS if WebSocket unavailable
6. **User profile**: Edit display name, phone, email
7. **Real OTP/Email delivery**: Integrate with SMS/Email provider (Twilio, SendGrid)
8. **Presence tracking**: Update online status in real-time
