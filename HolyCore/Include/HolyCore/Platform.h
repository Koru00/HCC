#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace holy::core {
	enum class PageAccess : uint8_t {
		NoAccess,
		ReadOnly,
		ReadWrite,
		ReadExecute,
		ReadWriteExecute,	// only the JIT (Level 3) should ask for this
	};

	size_t AllocationGranularity();
	size_t PageSize();

	[[nodiscard]] void* ReservePages(size_t bytes);
	[[nodiscard]] bool CommitPages(void* address, size_t bytes, PageAccess access = PageAccess::ReadWrite);
	void DecommitPages(void* address, size_t bytes);
	[[nodiscard]] bool ProtectPages(void* address, size_t bytes, PageAccess access);
	void ReleasePages(void* base);
	[[nodiscard]] void* AllocPages(size_t bytes, PageAccess access = PageAccess::ReadWrite); // reserve + commit

	enum class FileStatus : uint8_t {
		Ok,
		NotFound,
		AccessDenied,
		TooLarge,
		InvalidPath,
		IoError,
	};

	inline constexpr uint64_t kMaxFileBytes = 0x7FFFFFFFull;   // SourceLoc offsets will be 32-bit
	
	const char* FileStatusName(FileStatus status);
	[[nodiscard]] FileStatus ReadWholeFile(std::string_view utf8Path, std::string& outBytes);
}