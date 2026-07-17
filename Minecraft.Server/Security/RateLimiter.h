#pragma once

#include <string>
#include <deque>
#include <unordered_map>

#ifdef _WINDOWS64
#include <Windows.h>
#endif

namespace ServerRuntime
{
	namespace Security
	{
		class RateLimiter
		{
		public:
			RateLimiter();
			~RateLimiter();

			RateLimiter(const RateLimiter &) = delete;
			RateLimiter &operator=(const RateLimiter &) = delete;
			RateLimiter(RateLimiter &&) = delete;
			RateLimiter &operator=(RateLimiter &&) = delete;

			/**
			 * Returns true if the connection from this IP should be allowed.
			 * Returns false if the IP has exceeded maxPerWindow connections within windowMs milliseconds.
			 */
			bool AllowConnection(const std::string &ip, int maxPerWindow, int windowMs);

			/**
			 * Removes stale entries older than evictionAgeMs from the tracking map.
			 */
			void EvictStale(int evictionAgeMs = 300000);

			// security: hard cap on the tracking map. EvictStale is purely
			// time-based; if evictionAgeMs is set very high or rate-limiting
			// is disabled, an attacker generating one connection per unique
			// IP causes unbounded map growth (memory DoS). (MCLE-05)
			static constexpr size_t kMaxTrackedIps = 65536;

		private:
			CRITICAL_SECTION m_lock;
			std::unordered_map<std::string, std::deque<ULONGLONG>> m_connectionTimes;
		};

		/**
		 * Global rate limiter instance for the dedicated server accept loop.
		 */
		RateLimiter &GetGlobalRateLimiter();
	}
}
