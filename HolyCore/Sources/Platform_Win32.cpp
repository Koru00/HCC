#include <HolyCore/Platform.h>

#ifdef WIN32
#include <Windows.h>
#include <cassert>

namespace {
	const SYSTEM_INFO& SystemInfo() {
		static SYSTEM_INFO info = [] {
			SYSTEM_INFO tmp{};
			::GetNativeSystemInfo(&tmp);
			return tmp;
			}();

		return info;
	}

	DWORD ToWinProtect(holy::core::PageAccess pageAccess) {
		switch (pageAccess) {
		case holy::core::PageAccess::NoAccess:
			return PAGE_NOACCESS;
		case holy::core::PageAccess::ReadOnly:
			return PAGE_READONLY;
		case holy::core::PageAccess::ReadWrite:
			return PAGE_READWRITE;
		case holy::core::PageAccess::ReadExecute:
			return PAGE_EXECUTE_READ;
		case holy::core::PageAccess::ReadWriteExecute:
			return PAGE_EXECUTE_READWRITE;
		}

		assert(false && "Invalid PageAccess enum");
		return PAGE_NOACCESS;
	}

	bool Utf8ToWide(std::string_view input, std::wstring& output) {
		output.clear();

		if (input.empty()) return false;
		if ((input.size() > static_cast<size_t>(INT_MAX))) return false;
		
		int requiredLen = MultiByteToWideChar(
			CP_UTF8,
			MB_ERR_INVALID_CHARS,
			input.data(),
			static_cast<int>(input.size()),
			nullptr,
			0
		);

		if (requiredLen <= 0) return false; // Invalid UTF-8

		output.resize(requiredLen);

		int written = MultiByteToWideChar(
			CP_UTF8,
			MB_ERR_INVALID_CHARS,
			input.data(),
			static_cast<int>(input.size()),
			output.data(),
			requiredLen
		);

		if (written != requiredLen) return false;

		return true;
	}

	holy::core::FileStatus StatusFromWinError(DWORD err) {
		switch (err) {
		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return holy::core::FileStatus::NotFound;
		case ERROR_ACCESS_DENIED:
		case ERROR_SHARING_VIOLATION:
			return holy::core::FileStatus::AccessDenied;
		default:
			return holy::core::FileStatus::IoError;
		}
	}
}

namespace holy::core {
	size_t AllocationGranularity() {
		return SystemInfo().dwAllocationGranularity;
	}

	size_t PageSize() {
		return SystemInfo().dwPageSize;
	}

	void* ReservePages(size_t bytes) {
		if (bytes == 0) return nullptr;

		return VirtualAlloc(nullptr, bytes, MEM_RESERVE, PAGE_NOACCESS);
	}

	bool CommitPages(void* address, size_t bytes, PageAccess access) {
		if (address == nullptr || bytes == 0) return false;

		return VirtualAlloc(address, bytes, MEM_COMMIT, ToWinProtect(access)) != nullptr;
	}

	void DecommitPages(void* address, size_t bytes) {
		[[maybe_unused]] const BOOL ok = VirtualFree(address, bytes, MEM_DECOMMIT);
		assert(ok && "Failed to decommit pages");
	}

	bool ProtectPages(void* address, size_t bytes, PageAccess access) {
		DWORD oldProtect = 0;
		BOOL ok = ::VirtualProtect(address, bytes, ToWinProtect(access), &oldProtect);

		return ok != FALSE;
	}

	void ReleasePages(void* base) {
		assert(base != nullptr);
		[[maybe_unused]] BOOL ok = VirtualFree(base, 0, MEM_RELEASE);
		assert(ok && "Failed to release pages");
	}

	void* AllocPages(size_t bytes, PageAccess access) {
		if (bytes == 0) return nullptr;

		return VirtualAlloc(nullptr, bytes, MEM_RESERVE | MEM_COMMIT, ToWinProtect(access));
	}

	const char* FileStatusName(FileStatus status) {
		switch (status) {
		case holy::core::FileStatus::Ok:
			return "Ok";
		case holy::core::FileStatus::NotFound:
			return "NotFound";
		case holy::core::FileStatus::AccessDenied:
			return "AccessDenied";
		case holy::core::FileStatus::TooLarge:
			return "TooLarge";
		case holy::core::FileStatus::InvalidPath:
			return "InvalidPath";
		case holy::core::FileStatus::IoError:
			return "IoError";
		}

		assert(false && "Invalid FileStatus enum");
		return "IoError";
	}

	FileStatus ReadWholeFile(std::string_view utf8Path, std::string& outBytes) {
		outBytes.clear();
		FileStatus status = FileStatus::Ok;
		std::wstring wPath;

		if (!Utf8ToWide(utf8Path, wPath)) return FileStatus::InvalidPath;

		HANDLE handle = CreateFileW(wPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (handle == INVALID_HANDLE_VALUE) return StatusFromWinError(GetLastError());

		do {
			LARGE_INTEGER size{};
			if (!GetFileSizeEx(handle, &size)) {
				status = FileStatus::IoError;
				break;
			}

			if (size.QuadPart > kMaxFileBytes) {
				status = FileStatus::TooLarge;
				break;
			}

			if (size.QuadPart == 0) {
				status = FileStatus::Ok;
				break;
			}

			outBytes.resize(static_cast<size_t>(size.QuadPart));
			uint64_t total = 0;
			char* dst = outBytes.data();
			while (total < static_cast<uint64_t>(size.QuadPart)) {
				DWORD toRead = static_cast<DWORD>(std::min<uint64_t>(size.QuadPart - total, 64ull * 1024ull));

				DWORD read = 0;
				BOOL ok = ReadFile(handle, dst + total, toRead, &read, nullptr);

				if (!ok || read == 0) {
					status = FileStatus::IoError;
					break;
				}

				total += read;
			}

			if (status != FileStatus::Ok) break;

			if (total != static_cast<uint64_t>(size.QuadPart)) {
				status = FileStatus::IoError;
				break;
			}

			status = FileStatus::Ok;
		} while (false);

		CloseHandle(handle);

		if (status != FileStatus::Ok) {
			outBytes.clear();
		}

		return status;
	}
}
#endif // WIN32