#include <HolyCore/SourceManager.h>

#include <algorithm>
#include <cassert>
#include <string>

namespace holy::core {
	SourceManager::SourceManager(Arena& arena) : m_arena(arena), m_files(arena) {}

	// Registering files
	FileStatus SourceManager::LoadFile(std::string_view utf8Path, FileId& outId) {
		outId = FileId{};
		std::string bytes;
		const FileStatus status = ReadWholeFile(utf8Path, bytes);
		if (status != FileStatus::Ok) {
			return status;
		}

		outId = AddBuffer(utf8Path, bytes);
		if (!outId.IsValid()) {
			return FileStatus::TooLarge;
		}

		return FileStatus::Ok;
	}

	FileId SourceManager::AddBuffer(std::string_view name, std::string_view contents) {
		if (contents.size() > kMaxFileBytes) {
			return FileId{};
		}

		if (m_nextOffset + contents.size() + 1 > UINT32_MAX) {
			return FileId{};
		}

		FileEntry fileEntry{};
		fileEntry.name = m_arena.CopyString(name);
		fileEntry.contents = m_arena.CopyString(contents);
		fileEntry.baseOffset = static_cast<uint32_t>(m_nextOffset);
		fileEntry.lineStarts = Vector<uint32_t>(m_arena);
		fileEntry.lineStarts.PushBack(0);
		for (size_t i = 0; i < fileEntry.contents.size(); i++) {
			if (fileEntry.contents[i] == '\n') {
				fileEntry.lineStarts.PushBack(static_cast<uint32_t>(i + 1));
			}
		}

		m_nextOffset += contents.size() + 1;
		m_files.PushBack(fileEntry);
		return FileId{ static_cast<uint32_t>(m_files.Size()) };
	}

	// File queries
	size_t SourceManager::FileCount() const {
		return m_files.Size();
	}

	std::string_view SourceManager::FileName(FileId id) const {
		return EntryOf(id).name;
	}

	std::string_view SourceManager::Contents(FileId id) const {
		return EntryOf(id).contents;
	}

	SourceLoc SourceManager::StartOfFile(FileId id) const {
		return SourceLoc{ EntryOf(id).baseOffset };
	}

	SourceLoc SourceManager::LocFromOffset(FileId id, uint32_t offsetInFile) const {
		const FileEntry& entry = EntryOf(id);
		assert(offsetInFile <= entry.contents.size());
		return SourceLoc{ entry.baseOffset + offsetInFile };
	}

	// Location queries
	FileId SourceManager::FileOf(SourceLoc loc) const {
		const FileEntry* entry = FindEntry(loc);
		if (entry == nullptr) {
			return FileId{};
		}

		return FileId{ static_cast<uint32_t>(entry - m_files.Data()) + 1 };
	}

	PresumedLoc SourceManager::Resolve(SourceLoc loc) const {
		const FileEntry* entry = FindEntry(loc);
		if (entry == nullptr) {
			return PresumedLoc{};
		}

		uint32_t local = loc.offset - entry->baseOffset;
		uint32_t idx = LineIndex(*entry, local);
		return PresumedLoc{ .filename = entry->name, .line = idx + 1, .column = local - entry->lineStarts[idx] + 1 };
	}

	std::string_view SourceManager::LineText(SourceLoc loc) const {
		const FileEntry* entry = FindEntry(loc);
		if (entry == nullptr) {
			return {};
		}

		uint32_t local = loc.offset - entry->baseOffset;
		uint32_t idx = LineIndex(*entry, local);
		uint32_t start = entry->lineStarts[idx];
		uint32_t end;
		if (idx + 1 < entry->lineStarts.Size()) {
			end = entry->lineStarts[idx + 1] - 1;
		}
		else {
			end = entry->contents.size();
		}

		if (end > start && entry->contents[end - 1] == '\r') {
			--end;
		}

		return entry->contents.substr(start, end - start);
	}


	const SourceManager::FileEntry& SourceManager::EntryOf(FileId id) const {
		assert(id.IsValid() && id.index <= m_files.Size());
		return m_files[id.index - 1];
	}

	const SourceManager::FileEntry* SourceManager::FindEntry(SourceLoc loc) const {
		if (!loc.IsValid()) {
			return nullptr;
		}

		const FileEntry* it = std::upper_bound(m_files.begin(), m_files.end(), loc.offset, [](uint32_t off, const FileEntry& e) { return off < e.baseOffset; });
		if (it == m_files.begin()) {
			return nullptr;
		}

		--it;
		if (loc.offset - it->baseOffset > it->contents.size()) {
			return nullptr;
		}

		return &*it;
	}

	size_t SourceManager::LineIndex(const FileEntry& f, uint32_t localOffset) {
		const uint32_t* it = std::upper_bound(f.lineStarts.begin(), f.lineStarts.end(), localOffset);
		return static_cast<size_t>(it - f.lineStarts.begin()) - 1;
	}
}