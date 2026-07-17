#pragma once

#include "StreamCipher.h"
#include "EcdhKeyExchange.h"

#ifdef _WINDOWS64
#include <Windows.h>
#endif

namespace ServerRuntime
{
	namespace Security
	{
		/**
		 * Per-connection cipher registry for the dedicated server.
		 *
		 * Handshake protocol (4-message, via CustomPayloadPacket):
		 * 1. Server calls PrepareEcdhKey(smallId) -> sends MC|CKey with server
		 *    ephemeral X25519 public key (NOT the symmetric key) to client.
		 * 2. Client stores server pubkey, generates its own keypair, sends
		 *    MC|CAck with its ephemeral X25519 public key, derives shared
		 *    secret via X25519(serverPub, clientPriv), derives symmetric
		 *    key via HKDF-SHA256, activates send cipher.
		 * 3. Server recv thread detects MC|CAck -> calls CommitCipherWithPeerPub
		 *    which derives the same shared secret + symmetric key, sends
		 *    MC|COn plaintext, then activates cipher.
		 * 4. Client recv thread detects MC|COn -> activates recv cipher.
		 *
		 * The symmetric key never travels on the wire. An on-path observer
		 * sees only the two ephemeral public keys and cannot recover the
		 * shared secret without one of the private keys. (MCLE-01)
		 *
		 * Backwards compatible: old clients ignore MC|CKey, server never gets ack,
		 * cipher stays inactive. Old servers never send MC|CKey, client stays plaintext.
		 */
		class ConnectionCipherRegistry
		{
		public:
			ConnectionCipherRegistry();
			~ConnectionCipherRegistry();

			ConnectionCipherRegistry(const ConnectionCipherRegistry &) = delete;
			ConnectionCipherRegistry &operator=(const ConnectionCipherRegistry &) = delete;
			ConnectionCipherRegistry(ConnectionCipherRegistry &&) = delete;
			ConnectionCipherRegistry &operator=(ConnectionCipherRegistry &&) = delete;

			/**
			 * Generate an ephemeral X25519 keypair and store it as pending
			 * for the given smallId. The server's public key is returned
			 * via outPubKey (32 bytes) for inclusion in MC|CKey.
			 * Does NOT activate the cipher. Call CommitCipherWithPeerPub()
			 * after the client sends back its public key in MC|CAck.
			 */
			bool PrepareEcdhKey(unsigned char smallId,
								uint8_t outPubKey[EcdhKeyExchange::PUBLIC_KEY_SIZE]);

			/**
			 * Activate the cipher by deriving the shared key from the
			 * peer's public key (sent in MC|CAck). Called from the recv
			 * thread when the client's MC|CAck is detected.
			 * Returns false if no keypair was pending for this smallId
			 * or if derivation failed.
			 */
			bool CommitCipherWithPeerPub(unsigned char smallId,
										 const uint8_t peerPubKey[EcdhKeyExchange::PUBLIC_KEY_SIZE]);

			/**
			 * Cancel a pending keypair (e.g., client disconnected before ack).
			 */
			void CancelPending(unsigned char smallId);

			/**
			 * Check if a keypair is pending for the given smallId (no side effects).
			 */
			bool HasPendingKey(unsigned char smallId) const;

			/**
			 * Deactivate the cipher and cancel any pending keypair for a disconnected connection.
			 */
			void DeactivateCipher(unsigned char smallId);

			/**
			 * Atomically check if cipher is active and encrypt outgoing data.
			 * Returns true if data was encrypted, false if cipher is inactive (data untouched).
			 */
			bool TryEncryptOutgoing(unsigned char smallId, uint8_t *data, int length);

			/**
			 * Check if the cipher is active (handshake completed) for a given smallId.
			 * Thread-safe, read-only query.
			 */
			bool IsCipherActive(unsigned char smallId) const;

			/**
			 * Decrypt incoming data from a specific connection.
			 * No-op if the cipher is not active for this connection.
			 */
			void DecryptIncoming(unsigned char smallId, uint8_t *data, int length);

		private:
			static const int MAX_CONNECTIONS = 256;
			StreamCipher m_ciphers[MAX_CONNECTIONS];
			EcdhKeyExchange m_ecdh[MAX_CONNECTIONS];
			bool m_pending[MAX_CONNECTIONS];
			mutable CRITICAL_SECTION m_lock;
		};

		/**
		 * Global cipher registry singleton.
		 */
		ConnectionCipherRegistry &GetCipherRegistry();
	}
}
