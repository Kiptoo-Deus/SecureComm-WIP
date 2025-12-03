# CarrierBridge — Architecture (Technical Deep Dive)

This document provides a focused technical view of CarrierBridge's architecture and the rationale behind major design decisions. It is intended for engineers implementing, auditing, or deploying the system.

## High-Level Goals
- End-to-end confidentiality for messaging and transactions
- Resilience to network outages and censorship (mesh fallback)
- Interoperable client SDKs (C++ core, iOS, Android)
- Extensible transport and payment integrations (M-Pesa)

## Major Components
- `Client SDK` (C++ core library `libsecurecomm`)
  - Ratchet engine (Double Ratchet)
  - Dispatcher (session & routing)
  - Crypto primitives (libsodium wrapper)
  - Transport adapters (in-memory, websocket, mesh)
  - Payment module (M-Pesa bridge)
- `Server` (optional relay & coordination)
  - Relay endpoints for online message delivery
  - Optional VPN gateway and payment gateway interface
  - Can be horizontally scaled; stateless where possible
- `Mesh` overlay
  - Device-to-device routing (Bluetooth/WiFi Direct/Ad-hoc)
  - Local discovery, TTL-based forwarding, and queueing

## Data Flow (Two-party message)
1. Application calls `Dispatcher::send_message(recipient, plaintext)`
2. Dispatcher finds/creates a `Session` and calls `Ratchet::encrypt_envelope(plaintext)`
3. Ratchet produces an `Envelope` (header, session_id, ciphertext)
4. Dispatcher serializes envelope and sends via configured `Transport::send()`
5. Transport delivers bytes to remote device or server
6. Remote Dispatcher receives bytes, deserializes envelope, and calls `Ratchet::decrypt_envelope()`
7. Decryption uses message header (sender DH pubkey + msg number) to conditionally DH-ratchet and derive message key

## Double Ratchet Details (important decisions)
- Root key originates from X3DH or pre-shared symmetric root for demos
- `hkdf_root_chain()` derives BOTH send and receive chain keys deterministically from the new root PRK (avoids asymmetric swapping errors)
- First inbound message from a remote MUST NOT force a DH ratchet (the receiver uses the sender's pubkey for future ratchets only). This avoids key divergence for the first message.
- Skip-key map (skipped_message_keys_) caches message keys for out-of-order delivery
- `session_id` is deterministic (HMAC over sorted device IDs + root_key) to ensure both endpoints route to same session

## Transport Abstraction
- `Transport` interface provides `start()`, `stop()`, and `send(bytes)` callbacks and a message callback for inbound bytes
- Implementations:
  - `in_memory_transport` — for unit/integration tests and local demos
  - `websocket_transport` — for internet-relayed messages
  - `mesh_transport` — for D2D mesh relays (Bluetooth/WiFi-Direct)

Design notes:
- Keep transports simple and stateless where possible; Dispatcher maintains session state
- Use short-lived worker threads for packet handling with bounded queues

## Transaction Flow (Payments)
- Payment requests are serialized and encrypted using the same DR protocol (per-transaction/session)
- If online, payments are sent through the server or payment gateway (M-Pesa API) via TLS
- If offline, payments persist in local encrypted queue with retries and exponential backoff

## Storage & State
- Client-side: encrypted SQLite for queued messages, keys, and pending transactions
- Server-side: minimal transient queues; persistent storage only for audit/ledger (encrypted), if required
- State export/import supported for ratchet and session recovery

## Scaling & Horizontalization
- Server is stateless for message relay (if you use signed envelopes) — scale using load balancers
- For payments, use a separate payment microservice that handles gateway APIs and transaction reconciliation
- Mesh components scale via local peer discovery and do not require central coordination

## Security-sensitive Implementation Details
- All secrets are zeroed after use with `sodium_memzero`
- Keys generated with libsodium RNG (`randombytes_buf`)
- HKDF uses HMAC-SHA256 primitives (implemented and tested in `ratchet.cpp`)
- Avoid serializing private keys to logs; never log full keys or raw cryptographic material

## Sequence Diagram (Message send)

```
Client App -> Dispatcher: send_message(recipient, plaintext)
Dispatcher -> Ratchet: encrypt_envelope(plaintext)
Ratchet -> Dispatcher: Envelope(header, ciphertext)
Dispatcher -> Transport: send(bytes)
Transport -> RemoteTransport/Server: deliver bytes
RemoteTransport -> Dispatcher: on_message(bytes)
Dispatcher -> Ratchet: decrypt_envelope(Envelope)
Ratchet -> Dispatcher: plaintext
Dispatcher -> Client App: on_inbound(plaintext)
```

## Extensibility Points
- Add MLS manager module for group messaging
- Swap transport implementations (add QUIC/UDP/ICE)
- Add HSM-backed key operations for high-security deployments

## Observability
- Track metrics: message latency, decrypt failures, ratchet events, skipped key counts
- Expose health endpoints on servers and limited telemetry (opt-in, privacy-first)

---

