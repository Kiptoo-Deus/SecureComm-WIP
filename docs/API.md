# CarrierBridge — API Reference

This document lists the primary developer-facing APIs: the C++ SDK surface, key interfaces in `libsecurecomm`, and high-level server endpoints for optional services (payment gateway, relay).

## C++ SDK (libsecurecomm) — Key Classes

### `securecomm::Ratchet`
Core double-ratchet class.

Key methods:
```cpp
// Initialize ratchet with a 32-byte root key and session id
void initialize(const std::vector<uint8_t>& root_key,
                const std::vector<uint8_t>& session_id);

// Encrypt raw plaintext into an Envelope
Envelope encrypt_envelope(const std::vector<uint8_t>& plaintext);

// Decrypt an Envelope and return plaintext if successful
std::optional<std::vector<uint8_t>> decrypt_envelope(const Envelope& env);

// Export/import state for persistence or migration
std::vector<uint8_t> export_state() const;
void import_state(const std::vector<uint8_t>& state);
```

Notes:
- `Envelope` contains `session_id`, `associated_data` (header), and `ciphertext`.
- `encrypt_envelope` auto-increments send counters and advances chain keys.

---

### `securecomm::Dispatcher`
High-level session manager and router.

Typical usage:
```cpp
Dispatcher d(transport_ptr, device_id);
d.register_device("alice");
d.register_device("bob");

d.set_on_inbound([](const std::string& sender, const std::vector<uint8_t>& payload){
    // handle inbound plaintext
});

auto session = d.create_session_with("bob", root_key);
d.send_message("bob", std::vector<uint8_t>{'H','i'});
```

Key methods (illustrative):
```cpp
void register_device(const std::string& device_id);
void set_on_inbound(std::function<void(const std::string&, const std::vector<uint8_t>&)> cb);
std::optional<Session> create_session_with(const std::string& remote_device, const std::vector<uint8_t>& root_key);
bool send_message(const std::string& recipient, const std::vector<uint8_t>& plaintext);
```

`Session` contains `session_id`, `peer_device_id`, and a `Ratchet` instance.

---

### `securecomm::Transport`
Abstract transport interface. Implementations provide network or local delivery.

Interface:
```cpp
class Transport {
public:
    virtual ~Transport() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void send(const std::vector<uint8_t>& bytes) = 0;
    // on_message callback is set by the Dispatcher
};
```

Provided implementations:
- `InMemoryTransport` — used by desktop demo and tests
- `WebsocketTransport` — planned (for server relay)
- `MeshTransport` — planned (Bluetooth/WiFi Direct)

---

## Envelope Format (Serialization)
- Header (4 bytes message index) + DH public key (32 bytes) = 36 bytes
- `session_id` (16 bytes) attached separately in `Envelope` structure
- Ciphertext: AEAD output with nonce and tag (implementation detail in `crypto.cpp`)

Serialized layout used by the demo:
```
[session_id(16)] [header(36)] [ciphertext(variable)]
```

## Server Endpoints (Optional)
These are recommended API contracts for a stateless message relay and payment gateway.

### Relay (HTTP(s))
- `POST /v1/envelope/send`
  - Body: `{ "to": "device_id", "from": "device_id", "envelope": "base64(...)" }`
  - Response: `200 OK` or `4xx/5xx`
- `GET /v1/health`
  - Response: `{ "status": "ok" }`

Security: TLS 1.3; optional client cert auth for trusted relays.

### Payment Gateway (M-Pesa bridge)
- `POST /v1/payments/submit`
  - Body: `{ "envelope": "base64(...)" }` (encrypted transaction payload)
  - Response: `{ "txn_id": "...", "status": "submitted" }`

- `POST /v1/payments/notify` (webhook from M-Pesa)
  - Body: `{ "txn_id": "...", "status": "success" }`

Gateway must verify MAC/signatures within the envelope payload.

---

## Example: Send Message Flow (HTTP Relay)
1. Client serializes envelope and base64 encodes it
2. Client `POST /v1/envelope/send` with recipient
3. Relay forwards envelope to destination device or stores for later delivery

Example request:
```json
POST /v1/envelope/send
{
  "to": "bob",
  "from": "alice",
  "envelope": "BASE64_STRING"
}
```

## SDK Example (C++)
```cpp
// Setup
auto transport = std::make_shared<InMemoryTransport>();
Dispatcher dispatcher(transport, "alice-device-id");

// Register and start
dispatcher.register_device("alice");
dispatcher.set_on_inbound([](const std::string& from, const std::vector<uint8_t>& pt){
    std::string s(pt.begin(), pt.end());
    std::cout << "Message from " << from << ": " << s << std::endl;
});

auto root_key = derive_root_key_somehow(); // 32 bytes
dispatcher.create_session_with("bob", root_key);
dispatcher.send_message("bob", std::vector<uint8_t>{'H','e','l','l','o'});
```

---

## Implementation Notes & Compatibility
- Keep envelopes small; large payloads should be chunked and combined with application-layer MICs
- Avoid logging or serializing private keys
- Use `export_state()`/`import_state()` to migrate sessions across devices

