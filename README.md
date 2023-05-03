
# ICon6

Provides secure communication between two endpoints with UPD socket. Any client
can connect to any other client (calling them servers is purely semantical).
Networking is done with enet library and encryption is done with libsodium.

## Requirements

- cmake
- installed libsodium on in version 1.0.18

## Vulnerability

All application data is securely encrypted. Networking protocol of enet is not
encrypted, thus there is insecurity for find out if sent message is send by
reliable or sequenced channel. In case of connections not secured by
certificates (which are not implemented yet), attacker can perform Man in the
Middle attack. Attacker can also block/modify internal enet protocol messages to
perform a DoS attack.

Message size is limited by enet to 32 MiB, but encryption limits this size
further. It is recommended to not use bigger messages than 1 MiB in size.

There is no practical limit to number of sent messages (up to 2^64 messages sent
with encryption). Messages sent in different directions are encrypted using
different secret keys established during handshake. There is no rekeying
implemented.

## ICon6 certificates

### Structure of saved certificate key (CertKey)

- [4]  - opslimit argument for `crypto_pwhash`
- [4]  - memlimit argument for `crypto_pwhash`
- [16] - salt for key derivation (`crypto_pwhash`)
- [24] - nonce
- [32] - private key
- [64] - public key
- [16] - mac

(private key | public key) are encrypted with xchacha20-poly1305-ietf with
key derived from password with `crypto_pwhash` and given arguments

### Structure of certificate (Cert)

- [2]  - certificate length (including this field and parent signature)
- [32] - public key
- [32] - parent public key (the same as public key for self signed)
- [4]  - expiration date in format: number of days since 1900.01.01
- [4]  - issue date in format: number of days since 1900.01.01
- [2]  - N - number of bytes in certificate owner name
- [2]  - M - number of bytes in certificate owner url
- [N]  - certificate owner name (NULL terminated)
- [M]  - certificate owner url (NULL terminated)
- [64] - certificate signature by certificate owner
- [64] - certificate signature by parent

### Structure of certificate request (CertReq)

Everything except: certificate signature by parent



