#pragma once

#include <HolyCore/Arena.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <new>
#include <string_view>
#include <type_traits>

namespace holy::core {
	template <class> inline constexpr bool kAlwaysFalse = false; // for static_assert in discarded branches

	// murmur3 fmix64 finalizer
	inline uint64_t Mix64(uint64_t x) {
		x ^= x >> 33;
		x *= 0xff51afd7ed558ccdULL;
		x ^= x >> 33;
		x *= 0xc4ceb9fe1a85ec53ULL;
		x ^= x >> 33;
		return x;
	}

	// FNV-1a, then Mix64
	inline uint64_t HashBytes(const void* data, size_t size) {
		uint64_t h = 14695981039346656037ULL;
		for (size_t i = 0; i < size; i++) {
			h ^= static_cast<const unsigned char*>(data)[i];
			h *= 1099511628211ULL;
		}

		return Mix64(h);
	}

	template<class K>
	struct DefaultHash {
		size_t operator()(const K& key) const {
			if constexpr (std::is_integral_v<K> || std::is_enum_v<K>) {
				return static_cast<size_t>(
					Mix64(static_cast<uint64_t>(key))
					);
			}
			else if constexpr (std::is_pointer_v<K>) {
				return static_cast<size_t>(
					reinterpret_cast<uintptr_t>(key)
					);
			}
			else {
				static_assert(
					kAlwaysFalse<K>,
					"DefaultHash: unsupported key type"
					);
			}
		}
	};

	template <>
	struct DefaultHash<std::string_view> {
		size_t operator()(std::string_view key) const {
			return static_cast<size_t>(HashBytes(key.data(), key.size()));
		}
	};

	template <class K, class V, class Hash = DefaultHash<K>, class Eq = std::equal_to<K>>
	class HashMap {
		static_assert(std::is_trivially_copyable_v<K>&& std::is_trivially_destructible_v<K>);
		static_assert(std::is_trivially_copyable_v<V>&& std::is_trivially_destructible_v<V>);
	public:
		static constexpr size_t kMinCapacity = 16;

		HashMap() = default;
		explicit HashMap(Arena& arena) : m_arena(&arena) {}

		[[nodiscard]] size_t Size() const { return m_size; };
		[[nodiscard]] size_t Capacity() const { return m_capacity; };
		[[nodiscard]] bool Empty() const { return m_size == 0; };

		void Reserve(size_t exprectedCount); // so expectedCount inserts never rehash

		bool Insert(const K& key, const V& value); // false if key exists (value untouched)
		V& InsertOrAssign(const K& key, const V& value); // returns the stored value

		[[nodiscard]] V* Find(const K& key); // nullptr if absent
		[[nodiscard]] const V* Find(const K& key) const;
		[[nodiscard]] bool Contains(const K& key) const;

		void Clear(); // size = 0, capacity kept

		template <class F> void ForEach(F&& fn); // fn(const K&, V&)
		template <class F> void ForEach(F&& fn) const;

	private:
		struct Slot {
			K key;
			V value;
		};

		[[nodiscard]] size_t Probe(const K& key) const;
		void Rehash(size_t newCapacity);

		Arena* m_arena = nullptr;
		Slot* m_slots = nullptr;
		uint8_t* m_used = nullptr; // one byte per slot: 0 = empty, 1 = full
		size_t m_capacity = 0;	// power of two, or 0
		size_t m_size = 0;
	};
}

#include <HolyCore/HashMap.ipp>