# SourceManager

Header: `HolyCore/Include/HolyCore/SourceManager.h`  
Namespace: `holy::core`

`SourceManager` registers source buffers, owns copies of their names and bytes, and translates global source locations into file, line, column, and line-text information. It depends on an `Arena` supplied by the caller.

## Types

### `SourceLoc`

A 32-bit global byte offset. Offset zero is invalid; valid files begin at nonzero offsets. Equality is defined.

### `FileId`

A one-based index into the manager's registration order. Index zero is invalid. Equality is defined.

### `PresumedLoc`

A resolved file name and one-based line and byte-column. A line value of zero indicates an invalid location; an invalid result has an empty filename.

## Construction and registration

### `explicit SourceManager(Arena& arena)`

Constructs a manager whose file entries, copied names, copied contents, and line-start arrays are owned by `arena`. The arena must outlive the manager and any views returned by it. Copy construction and assignment are disabled.

### `FileStatus LoadFile(std::string_view utf8Path, FileId& outId)`

Reads a file using `ReadWholeFile`, then registers its contents. `outId` is reset to invalid before loading and is set only on success. A read error is returned as-is; if the buffer cannot fit the source manager's offset space, the result is `FileStatus::TooLarge`.

### `FileId AddBuffer(std::string_view name, std::string_view contents)`

Copies and registers the provided name and bytes. Returns an invalid `FileId` if the buffer exceeds `kMaxFileBytes` or the manager's remaining 32-bit global-offset space. Contents are treated as bytes; embedded NUL bytes are supported. The stored view has an additional NUL terminator just beyond its reported size.

## File queries

| Method | Contract |
| --- | --- |
| `size_t FileCount() const` | Number of registered files. |
| `std::string_view FileName(FileId id) const` | Name for a valid registered ID. Asserts on invalid/out-of-range ID. |
| `std::string_view Contents(FileId id) const` | Registered bytes; returned view excludes the trailing NUL. Asserts on invalid/out-of-range ID. |
| `SourceLoc StartOfFile(FileId id) const` | Global location of byte zero in the file. Asserts on invalid/out-of-range ID. |
| `SourceLoc LocFromOffset(FileId id, uint32_t offsetInFile) const` | Converts a file-local byte offset to a global location. Accepts offsets through and including EOF; asserts if greater than the file size. |

## Location queries

| Method | Contract |
| --- | --- |
| `FileId FileOf(SourceLoc loc) const` | Returns the containing file ID, or an invalid ID when the location is invalid or outside registered contents. EOF is considered a valid location in that file. |
| `PresumedLoc Resolve(SourceLoc loc) const` | Returns filename and one-based line/byte-column, or an invalid result for an unknown location. |
| `std::string_view LineText(SourceLoc loc) const` | Returns the complete line containing the location, without its line terminator; returns an empty view for an invalid location. An empty line is also a valid result. |

Line starts are recorded after each LF byte. CRLF line text has both terminator bytes removed. Columns count bytes, not Unicode characters. An offset at EOF after a trailing newline resolves to the trailing empty line.

## Example

```cpp
holy::core::Arena arena;
holy::core::SourceManager sources(arena);

holy::core::FileId file = sources.AddBuffer("example.hc", "first\nsecond\n");
if (file.IsValid()) {
	holy::core::SourceLoc location = sources.LocFromOffset(file, 6);
	holy::core::PresumedLoc presumed = sources.Resolve(location);
	// presumed.line == 2; presumed.column == 1
}
```

## Implementation notes

File IDs follow registration order. Global offsets are ascending, with one offset reserved between successive files so file-end locations remain distinguishable from the next file's start. The implementation searches file bases and line-start offsets with binary search.