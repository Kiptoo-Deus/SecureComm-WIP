use chacha20poly1305::{ChaCha20Poly1305, Key, Nonce};
use chacha20poly1305::aead::{Aead, AeadCore};
use chacha20poly1305::KeyInit;

use x25519_dalek::{EphemeralSecret, PublicKey as X25519Public};

use rand_core::OsRng;

pub fn generate_x25519_keypair() -> (EphemeralSecret, X25519Public) {
    let secret = EphemeralSecret::new(&mut OsRng);
    let public = X25519Public::from(&secret);
    (secret, public)
}

pub fn encrypt(key: &[u8; 32], plaintext: &[u8]) -> Result<(Vec<u8>, [u8; 12]), String> {
    let cipher = ChaCha20Poly1305::new(Key::from_slice(key));
    let nonce = ChaCha20Poly1305::generate_nonce(&mut OsRng);
    let nonce_arr: [u8; 12] = nonce.clone().into();

    let ciphertext = cipher.encrypt(&nonce, plaintext)
        .map_err(|e| e.to_string())?;

    Ok((ciphertext, nonce_arr))
}

pub fn decrypt(key: &[u8; 32], nonce: &[u8; 12], ciphertext: &[u8]) -> Result<Vec<u8>, String> {
    let cipher = ChaCha20Poly1305::new(Key::from_slice(key));
    let nonce = Nonce::from_slice(nonce);
    cipher.decrypt(nonce, ciphertext).map_err(|e| e.to_string())
}
