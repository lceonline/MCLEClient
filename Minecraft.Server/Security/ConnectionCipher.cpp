#include "stdafx.h"
#include "ConnectionCipher.h"

#include <cstring>

namespace ServerRuntime
{
	namespace Security
	{
		ConnectionCipherRegistry::ConnectionCipherRegistry()
		{
			InitializeCriticalSection(&m_lock);
			memset(m_pending, 0, sizeof(m_pending));
		}

		ConnectionCipherRegistry::~ConnectionCipherRegistry()
		{
			DeleteCriticalSection(&m_lock);
		}

		bool ConnectionCipherRegistry::PrepareEcdhKey(unsigned char smallId,
													 uint8_t outPubKey[EcdhKeyExchange::PUBLIC_KEY_SIZE])
		{
			if (!m_ecdh[smallId].GenerateKeypair())
			{
				return false;
			}

			EnterCriticalSection(&m_lock);
			m_ecdh[smallId].GetPublicKey(outPubKey);
			m_pending[smallId] = true;
			LeaveCriticalSection(&m_lock);
			return true;
		}

		bool ConnectionCipherRegistry::CommitCipherWithPeerPub(unsigned char smallId,
															  const uint8_t peerPubKey[EcdhKeyExchange::PUBLIC_KEY_SIZE])
		{
			uint8_t derivedKey[StreamCipher::KEY_SIZE];
			if (!m_ecdh[smallId].DeriveSharedKey(peerPubKey, derivedKey))
			{
				return false;
			}

			EnterCriticalSection(&m_lock);
			if (!m_pending[smallId])
			{
				SecureZeroMemory(derivedKey, sizeof(derivedKey));
				LeaveCriticalSection(&m_lock);
				return false;
			}

			// StreamCipher wants 32 bytes: 16-byte AES key + 16-byte IV.
			// We derive 32 bytes via HKDF; split into key (first 16) and IV (last 16).
			uint8_t aesKey[16];
			uint8_t iv[16];
			memcpy(aesKey, derivedKey, 16);
			memcpy(iv, derivedKey + 16, 16);

			uint8_t keyMaterial[32];
			memcpy(keyMaterial, aesKey, 16);
			memcpy(keyMaterial + 16, iv, 16);

			m_ciphers[smallId].Initialize(keyMaterial);
			SecureZeroMemory(keyMaterial, sizeof(keyMaterial));
			SecureZeroMemory(derivedKey, sizeof(derivedKey));
			m_ecdh[smallId].Reset();
			m_pending[smallId] = false;
			LeaveCriticalSection(&m_lock);
			return true;
		}

		void ConnectionCipherRegistry::CancelPending(unsigned char smallId)
		{
			EnterCriticalSection(&m_lock);
			m_ecdh[smallId].Reset();
			m_pending[smallId] = false;
			LeaveCriticalSection(&m_lock);
		}

		bool ConnectionCipherRegistry::HasPendingKey(unsigned char smallId) const
		{
			EnterCriticalSection(&m_lock);
			bool pending = m_pending[smallId];
			LeaveCriticalSection(&m_lock);
			return pending;
		}

		void ConnectionCipherRegistry::DeactivateCipher(unsigned char smallId)
		{
			EnterCriticalSection(&m_lock);
			m_ciphers[smallId].Reset();
			m_ecdh[smallId].Reset();
			m_pending[smallId] = false;
			LeaveCriticalSection(&m_lock);
		}

		bool ConnectionCipherRegistry::TryEncryptOutgoing(unsigned char smallId, uint8_t *data, int length)
		{
			EnterCriticalSection(&m_lock);
			bool active = m_ciphers[smallId].IsActive();
			if (active)
			{
				m_ciphers[smallId].Encrypt(data, length);
			}
			LeaveCriticalSection(&m_lock);
			return active;
		}

		bool ConnectionCipherRegistry::IsCipherActive(unsigned char smallId) const
		{
			EnterCriticalSection(&m_lock);
			bool active = m_ciphers[smallId].IsActive();
			LeaveCriticalSection(&m_lock);
			return active;
		}

		void ConnectionCipherRegistry::DecryptIncoming(unsigned char smallId, uint8_t *data, int length)
		{
			EnterCriticalSection(&m_lock);
			m_ciphers[smallId].Decrypt(data, length);
			LeaveCriticalSection(&m_lock);
		}

		ConnectionCipherRegistry &GetCipherRegistry()
		{
			static ConnectionCipherRegistry s_instance;
			return s_instance;
		}
	}
}
