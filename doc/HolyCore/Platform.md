# Platform

Header: `HolyCore/Include/HolyCore/Platform.h`  
Namespace: `holy::core`

This API provides page-allocation operations and whole-file reads behind a small platform abstraction. The current project includes a Win32 implementation in `HolyCore/Sources/Platform_Win32.cpp`; other operating-system implementations are not present.

## Virtual-memory API

- `AllocationGranularity()` and `PageSize()` — query native allocation granularity and page size.
- `ReservePages(bytes)` — reserve address space; returns a base pointer or null on failure.
- `CommitPages(address, bytes, access = ReadWrite)` — commit reserved pages with the requested access; returns success status.
- `DecommitPages(address, bytes)` — decommit pages.
- `ProtectPages(address, bytes, access)` — change access on committed pages; returns success status.
- `ReleasePages(base)` — release a reserved region.
- `AllocPages(bytes, access = ReadWrite)` — reserve and commit in one operation.

`PageAccess` supports `NoAccess`, `ReadOnly`, `ReadWrite`, `ReadExecute`, and `ReadWriteExecute`. The header notes that executable-writable protection is intended only for the JIT.

## File API

`ReadWholeFile(std::string_view utf8Path, std::string& outBytes)` reads a whole file into `outBytes` and returns a `FileStatus`. The Win32 implementation converts UTF-8 paths to UTF-16, rejects invalid paths and files larger than `kMaxFileBytes`, and clears the output on errors.

`FileStatus` values are `Ok`, `NotFound`, `AccessDenied`, `TooLarge`, `InvalidPath`, and `IoError`. Use `FileStatusName(status)` for a display name. The current maximum file size is `0x7FFFFFFF` bytes, reflecting the source-location offset limit.