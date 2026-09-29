#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <string_view>
#include <type_traits>
#include <utility>

namespace holy::core {
	class Arena {
	public:
		static constexpr size_t kDefaultReserveBytes = size_t{ 256 } * 1024 * 1024; // 256 MiB of address space
		static constexpr size_t kCommitChunkBytes = size_t{ 64 } * 1024; // == allocation granularity

		explicit Arena(size_t reserveBytes = kDefaultReserveBytes);
		~Arena();

		Arena(const Arena&) = delete;
		Arena& operator=(const Arena&) = delete;
		Arena(Arena&&) = delete;
		Arena& operator=(Arena&&) = delete;

		// Raw allocation. Never returns null. alignment must be a power of two, <= kCommitChunkBytes.
		// bytes == 0 is allowed and still returns a valid non-null pointer.
		[[nodiscard]] void* Allocate(size_t bytes, size_t alignment = alignof(std::max_align_t));

		template <class T, class... Args>
		[[nodiscard]] T* New(Args&&... args);

		// Value-initialized (zeroed for scalars) array. count == 0 returns nullptr.
		template <class T>
		[[nodiscard]] T* NewArray(size_t count);

		// Copies the bytes, appends a NUL, returns a view of the copy (size == s.size(), NOT counting the NUL).
		[[nodiscard]] std::string_view CopyString(std::string_view s);

		// Forget everything allocated. Committed memory stays committed for reuse.
		void Reset();

		[[nodiscard]] bool Owns(const void* p) const; // is p inside the used part of this arena?
		[[nodiscard]] size_t BytesUsed() const { return m_used; }
		[[nodiscard]] size_t BytesCommitted() const { return m_committed; }
		[[nodiscard]] size_t BytesReserved() const { return m_reserved; }
	private:
		uint8_t* m_base = nullptr;
		size_t m_reserved = 0; // total reserved bytes (multiple of kCommitChunkBytes)
		size_t m_committed = 0; // bytes committed from m_base
		size_t m_used = 0; // bump offset from m_base
	};

	template <class T, class... Args>
	T* Arena::New(Args&&... args) {
		static_assert(std::is_trivially_destructible_v<T>, "Arena objects never get destructors");
		void* p = Allocate(sizeof(T), alignof(T));
		return ::new (p) T(std::forward<Args>(args)...);
	}

	template <class T>
	T* Arena::NewArray(size_t count) {
		static_assert(std::is_trivially_destructible_v<T>, "Arena objects never get destructors");
		if (count == 0) return nullptr;

		T* p;
		if (count > SIZE_MAX / sizeof(T)) {
			p = static_cast<T*>(Allocate(SIZE_MAX, alignof(T)));
		}
		else {
			p = static_cast<T*>(Allocate(count * sizeof(T), alignof(T)));
		}

		if (p) {
			for (size_t i = 0; i < count; i++) {
				::new (&p[i]) T();
			}
		}

		return p;
	}
}