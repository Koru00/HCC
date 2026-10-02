#pragma once

#include <HolyCore/Arena.h>
#include <HolyCore/Platform.h>
#include <HolyCore/Vector.h>

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace holy::core {
	struct SourceLoc {
		uint32_t offset = 0;
		[[nodiscard]] constexpr bool IsValid() const { return offset != 0; }
		friend constexpr bool operator==(SourceLoc, SourceLoc) = default;
	};

	struct FileId {
		uint32_t index = 0;
		[[nodiscard]] constexpr bool IsValid() const { return index != 0; }
		friend constexpr bool operator==(FileId, FileId) = default;
	};

	struct PresumedLoc{
		std::string_view filename; // empty if invalid
		uint32_t line = 0; // 1-based; 0 = invalid
		uint32_t column = 0; // 1-based, in bytes
		[[nodiscard]] constexpr bool IsValid() const { return line != 0; }
	};

	class SourceManager {
	public:
		explicit SourceManager(Arena& arena);

		SourceManager(const SourceManager&) = delete;
		SourceManager& operator=(const SourceManager&) = delete;

		// Registering files
		[[nodiscard]] FileStatus LoadFile(std::string_view utf8Path, FileId& outId); // reads from disk via ReadWholeFile, then AddBuffer
		[[nodiscard]] FileId AddBuffer(std::string_view name, std::string_view contents); // FileId{} if it doesn't fit the offset space

		// File queries
		[[nodiscard]] size_t FileCount() const;
		[[nodiscard]] std::string_view FileName(FileId id) const; // assert(id valid)
		[[nodiscard]] std::string_view Contents(FileId id) const; // NUL-terminated just past .size()
		[[nodiscard]] SourceLoc StartOfFile(FileId id) const; // loc of byte 0
		[[nodiscard]] SourceLoc LocFromOffset(FileId id, uint32_t offsetInFile) const; // assert(offsetInFile <= size)

		// Location queries
		[[nodiscard]] FileId FileOf(SourceLoc loc) const;
		[[nodiscard]] PresumedLoc Resolve(SourceLoc loc) const;
		[[nodiscard]] std::string_view LineText(SourceLoc loc) const;
	private:
		struct FileEntry {
			std::string_view name; // arena copy
			std::string_view contents; // arena copy, NUL-terminated
			uint32_t baseOffset = 0; // global offset of byte 0
			Vector<uint32_t> lineStarts; // local offset of the first byte of each line; [0] is always 0
		};

		[[nodiscard]] const FileEntry& EntryOf(FileId id) const; // assert(id valid and in range)
		[[nodiscard]] const FileEntry* FindEntry(SourceLoc loc) const; // nullptr if loc isn't inside any file
		[[nodiscard]] static size_t LineIndex(const FileEntry& f, uint32_t localOffset); // 0-based line containing localOffset

		Arena& m_arena; // owns names, contents, and the vectors
		Vector<FileEntry> m_files; // in registration order, so baseOffset is ascending
		uint64_t m_nextOffset = 1; // base offset the next file will get (64-bit so overflow is detectable)
	};
}