#pragma once

#include <cstdint>

#ifdef _WINDOWS64
#include <Windows.h>
#include <bcrypt.h>
#endif

namespace ServerRuntime
{
	namespace Security
	{
		/**
		 * X25519 ECDH key exchange for the MC|CKey handshake.
		 *
		 * Replaces the old "send raw key in cleartext" handshake with an
		 * authenticated DH exchange:
		 *
		 * 1. Server generates an ephemeral X25519 keypair (privS, pubS).
		 * 2. Server sends pubS to the client in MC|CKey.
		 * 3. Client generates an ephemeral X25519 keypair (privC, pubC).
		 * 4. Client sends pubC to the server in MC|CAck.
		 * 5. Both sides compute shared_secret = X25519(privMine, pubTheirs).
		 * 6. Both sides derive the symmetric key via HKDF-SHA256(shared_secret).
		 *
		 * The symmetric key never travels on the wire. An on-path observer
		 * sees only the two ephemeral public keys and cannot recover the
		 * shared secret without one of the private keys.
		 *
		 * The handshake provides forward secrecy within a session (new
		 * ephemeral keypair per connection) but does NOT authenticate the
		 * server to the client - a MITM can still complete the exchange.
		 * For full authentication, pin the server's long-term public key
		 * in the client and have the server sign its ephemeral pubkey.
		 * Left as TODO; the current change closes the cleartext-key
		 * recovery vector (MCLE-01).
		 */
		class EcdhKeyExchange
		{
		public:
			// X25519 public key size: 32 bytes.
			static const int PUBLIC_KEY_SIZE = 32;
			// Derived symmetric key size: 32 bytes (matches StreamCipher::KEY_SIZE).
			static const int SHARED_KEY_SIZE = 32;

			EcdhKeyExchange();
			~EcdhKeyExchange();

			EcdhKeyExchange(const EcdhKeyExchange &) = delete;
			EcdhKeyExchange &operator=(const EcdhKeyExchange &) = delete;
			EcdhKeyExchange(EcdhKeyExchange &&) = delete;
			EcdhKeyExchange &operator=(EcdhKeyExchange &&) = delete;

			/**
			 * Generate a fresh ephemeral X25519 keypair.
			 * Returns true on success. After this call, GetPublicKey()
			 * returns the public key to send to the peer.
			 */
			bool GenerateKeypair();

			/**
			 * Copy the local public key into outPubKey (32 bytes).
			 * Must be called after GenerateKeypair().
			 */
			void GetPublicKey(uint8_t outPubKey[PUBLIC_KEY_SIZE]) const;

			/**
			 * Compute the shared secret from the peer's public key, then
			 * derive the symmetric key via HKDF-SHA256.
			 * outSharedKey is 32 bytes (StreamCipher::KEY_SIZE).
			 * Returns true on success.
			 */
			bool DeriveSharedKey(const uint8_t peerPubKey[PUBLIC_KEY_SIZE],
								 uint8_t outSharedKey[SHARED_KEY_SIZE]);

			/**
			 * Reset and securely wipe all key material.
			 */
			void Reset();

		private:
#ifdef _WINDOWS64
			BCRYPT_SECRET_HANDLE m_agreementSecret;
#endif
			uint8_t m_privKey[32];
			uint8_t m_pubKey[32];
			bool m_hasKeypair;
		};
	}
}
