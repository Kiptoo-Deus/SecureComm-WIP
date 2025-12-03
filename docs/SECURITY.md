# CarrierBridge — Security Overview & Threat Model

This file documents the security model, cryptographic choices, mitigations for common threats, and recommended operational practices.

## Security Goals
- Confidentiality: Messages and transactions must be unreadable by eavesdroppers
- Integrity: Detect any tampering or replay of messages or transactions
- Forward Secrecy: Compromise of current keys must not expose past messages
- Resilience: Maintain privacy and delivery in presence of network disruption and censorship
- Least Privilege: Minimize attack surface and limit secret access

## Cryptographic Primitives
- AEAD: ChaCha20-Poly1305 (libsodium) for authenticated encryption
- DH: X25519 (Curve25519) for ECDH
- KDF/HMAC: HMAC-SHA256 used for HKDF-like extract/expand and chain ratchets
- RNG: libsodium `randombytes_buf()` for nonces and ephemeral keys

## Protocol-Level Protections
- Double Ratchet (Signal-style): Provides message-level forward secrecy and break-in recovery
- Deterministic `session_id`: Prevents mismatched session routing between peers
- Skipped-key caching: Allows secure out-of-order delivery without reusing keys

## Threat Model (top-level)
- Network eavesdropper (active/passive)
- Malicious relay server (honest-but-curious or malicious)
- Local device compromise (partial)
- Censorship/Internet shutdown
- Traffic analysis / metadata leakage

## Threat Mitigations
- Eavesdropping/MITM:
  - End-to-end encryption prevents message plaintext exposure
  - HMAC verification and AEAD ensure tamper detection
- Malicious server:
  - Server can relay but not decrypt payloads (unless client keys leaked)
  - Use of deterministic session IDs and signed control messages prevents silent session hijacking
- Device compromise:
  - Do not persist long-term plain keys; use encrypted storage and recommend hardware-backed keystores
  - Use `sodium_memzero()` on sensitive memory
- Censorship:
  - Mesh fallback and VPN obfuscation reduce single-point censorability
  - Traffic shaping and stealth mode to resist DPI

## Key Management
- Master/root keys arise from X3DH or provisioning process
- Ratchet state (root_key, send/recv chain keys, dh keys) is serialized for transfer but must remain encrypted at rest
- Recommendations:
  - Use platform keystore (Keychain on macOS/iOS, Keystore on Android) for seed protection
  - For servers or high-security clients, use HSM or PKCS#11 module
  - Rotate keys on regular schedule for long-lived sessions

## Secure Storage
- Client-side persistent storage (SQLite) must be encrypted using per-device storage key (derived from keystore) and authenticated
- Do not store PINs, passphrases, or raw private keys in plaintext

## Operational Security
- TLS 1.3 with mutual authentication for control channels
- Short-lived certificates for VPN endpoints
- Robust logging policy: avoid sensitive data; redact or aggregate
- Monitoring and alerting for high failure rates (e.g., repeated decrypt failures)

## Auditing & Testing
- Static analysis, unit tests, and cryptographic test vectors
- Fuzz testing on serialization code (envelope parsing)
- Periodic third-party security audits and responsible disclosure program

## Known Risks & Limitations
- Metadata leakage: envelopes leak sender/receiver IDs and size; consider additional padding and traffic mixing
- Compromised device: full compromise can reveal state; recommend device attestation and remote wipe
- Mesh reliability: not guaranteed and may leak topology information

## Recommended Hardening Checklist (Pre-Production)
- [ ] Use platform keystore or HSM for root keys
- [ ] Enable disk encryption for devices and servers
- [ ] Disable verbose logging in production
- [ ] Enforce certificate pinning for first-run trust-on-first-use (TOFU) model where applicable
- [ ] Run automated unit and fuzz tests in CI

## Responsible Disclosure
If you discover a security vulnerability, follow the project's disclosure policy (see README_COMPREHENSIVE.md). Contact `security@carrierbridge.dev` with details and reproduction steps.

---

This file is a living document; it should be kept up to date as the project evolves and new components (e.g., MLS, payment gateways) are added.