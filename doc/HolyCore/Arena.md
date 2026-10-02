# Arena

Header: `HolyCore/Include/HolyCore/Arena.h`  
Namespace: `holy::core`

`Arena` provides a non-copyable and non-movable bump allocator backed by reserved virtual memory. Its default reservation is 256 MiB, and it commits storage in 64 KiB chunks as required.

## API

- `explicit Arena(size_t reserveBytes = kDefaultReserveBytes)` — reserves address space. The size is rounded/managed in commit chunks by the implementation.
- `void* Allocate(size_t bytes, size_t alignment = alignof(std::max_align_t))` — aligned raw allocation. Alignment must be a power of two no greater than `kCommitChunkBytes`; a zero-byte request is permitted. Allocation failure is fatal rather than reported as null.
- `T* New< T >(Args&&... args)` — constructs one trivially destructible object in arena storage using placement construction.
- `T* NewArray< T >(size_t count)` — value-initializes an array of trivially destructible objects; a zero count returns null.
- `std::string_view CopyString(std::string_view s)` — copies bytes, appends a NUL byte, and returns a view whose size excludes that terminator.
- `void Reset()` — resets the used offset while retaining committed memory for reuse. Existing allocations must be treated as invalid after reset.
- `bool Owns(const void* p) const` — reports whether `p` lies in the used portion of the arena.
- `BytesUsed()`, `BytesCommitted()`, `BytesReserved()` — report current allocator counters.

## Lifetime and constraints

Arena allocations are not individually freed and object destructors are not run. Do not store types requiring destruction in the arena. Every object/view referencing arena storage must stop being used before reset or destruction. The implementation uses the platform page-allocation APIs, currently provided by Win32 in this project.