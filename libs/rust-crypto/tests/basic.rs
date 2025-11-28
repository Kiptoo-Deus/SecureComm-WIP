use securecomm_crypto::*;

#[test]
fn test_x25519_keygen() {
    let (_s, p) = x25519_generate();
    assert_eq!(p.as_bytes().len(), 32);
}

#[test]
fn test_ed25519_sign_verify() {
    let (sk, vk) = ed25519_generate();
    let msg = b"hello securecomm";
    let sig = sign(&sk, msg);
    assert!(verify(&vk, msg, &sig));
}

#[test]
fn test_aead_roundtrip() {
    let key = [7u8; 32];
    let nonce = [9u8; 12];

    let plaintext = b"secret-message";
    let aad = b"header";

    let cipher = aead_encrypt(&key, &nonce, plaintext, aad);
    let plain = aead_decrypt(&key, &nonce, &cipher, aad);

    assert_eq!(plaintext.to_vec(), plain);
}
