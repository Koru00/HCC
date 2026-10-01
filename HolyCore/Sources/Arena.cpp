#include <HolyCore/Arena.h>
#include <HolyCore/Platform.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <bit>

namespace {
	[[noreturn]] void Fatal(const char* msg) {
		std::fputs(msg, stderr);
		std::fputc('\n', stderr);
		std::abort();
	}

	size_t RoundUp(size_t value, size_t multipleOfPow2) {
		assert(
			multipleOfPow2 != 0 &&
			(multipleOfPow2 & (multipleOfPow2 - 1)) == 0
		);
		return (value + multipleOfPow2 - 1) & ~(multipleOfPow2 - 1);
	}
}

namespace holy::core {
	Arena::Arena(size_t reserveBytes) {
		assert(reserveBytes > 0);
		m_reserved = RoundUp(reserveBytes, kCommitChunkBytes);
		m_base = static_cast<uint8_t*>(ReservePages(m_reserved));
		if (m_base == nullptr) {
			Fatal("Arena: address space reservation failed");
		}
	}

	Arena::~Arena() {
		ReleasePages(m_base);
	}

	void* Arena::Allocate(size_t bytes, size_t alignment) {
		assert(alignment != 0 && (alignment & (alignment - 1)) == 0 && alignment <= kCommitChunkBytes);
		if (bytes == 0) bytes = 1;
		const size_t start = RoundUp(m_used, alignment);
		if (start > m_reserved || bytes > m_reserved - start) Fatal("Arena: out of reserved space");
		const size_t end = start + bytes;
		if (end > m_committed) {
			size_t newCommitted = RoundUp(end, kCommitChunkBytes);
			if (!CommitPages(m_base + m_committed, newCommitted - m_committed)) Fatal("Arena: commit failed");
			m_committed = newCommitted;
		}
		m_used = end;
		return m_base + start;
	}

	void Arena::Reset() {
#ifndef NDEBUG
		std::memset(m_base, 0xDD, m_used);
#endif // !NDEBUG
		m_used = 0;
	}

	std::string_view Arena::CopyString(std::string_view s) {
		size_t sizeBytes = s.size() + 1;
		char* p = static_cast<char*>(Allocate(sizeBytes, 1));
		std::memcpy(p, s.data(), s.size());
		p[s.size()] = '\0';
		return std::string_view(p, s.size());
	}

	bool Arena::Owns(const void* p) const {
		uintptr_t uintPtrP = (uintptr_t)p;
		uintptr_t uintPtrBase = (uintptr_t)m_base;
		return uintPtrP >= uintPtrBase && uintPtrP < uintPtrBase + m_used;
	}
}