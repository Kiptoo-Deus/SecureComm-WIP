use rand_core::OsRng;
use x25519_dalek::{PublicKey as X25519Public, StaticSecret as X25519Secret};
use ed25519_dalek::{Signature, Signer, Verifier, SigningKey, VerifyingKey};
use chacha20poly1305::{
    aead::{Aead, Payload},
    ChaCha20Poly1305, Key, XNonce
};

/// Generate an X25519 static keypair
pub fn x25519_generate() -> (X25519Secret, X25519Public) {
    let secret = X25519Secret::new(OsRng);
    let public = X25519Public::from(&secret);
    (secret, public)
}

/// Generate an Ed25519 signing keypair
pub fn ed25519_generate() -> (SigningKey, VerifyingKey) {
    let signing = SigningKey::generate(&mut OsRng);
    let verify = signing.verifying_key();
    (signing, verify)
}

/// AEAD encrypt (ChaCha20-Poly1305)
pub fn aead_encrypt(
    key: &[u8; 32],
    nonce: &[u8; 12],
    plaintext: &[u8],
    aad: &[u8]
) -> Vec<u8> {
    let cipher = ChaCha20Poly1305::new(Key::from_slice(key));
    let nonce = XNonce::from_slice(nonce);
    cipher
        .encrypt(nonce, Payload { msg: plaintext, aad })
        .expect("encryption failure")
}

/// AEAD decrypt
pub fn aead_decrypt(
    key: &[u8; 32],
    nonce: &[u8; 12],
    ciphertext: &[u8],
    aad: &[u8]
) -> Vec<u8> {
    let cipher = ChaCha20Poly1305::new(Key::from_slice(key));
    let nonce = XNonce::from_slice(nonce);
    cipher
        .decrypt(nonce, Payload { msg: ciphertext, aad })
        .expect("decryption failure")
}

/// Sign a message (Ed25519)
pub fn sign(signing_key: &SigningKey, msg: &[u8]) -> Signature {
    signing_key.sign(msg)
}

/// Verify Ed25519 signature
pub fn verify(verify_key: &VerifyingKey, msg: &[u8], sig: &Signature) -> bool {
    verify_key.verify(msg, sig).is_ok()
}
