# HolyCore API Reference

`HolyCore` provides arena-backed data structures, source-file tracking, string interning, and platform/file primitives in namespace `holy::core`.

## Components

- [Arena](Arena.md) — virtual-memory-backed bump allocator and string copying.
- [Vector](Vector.md) — arena-backed dynamic array for trivially copyable types.
- [HashMap](HashMap.md) — arena-backed open-addressing hash map.
- [StringInterner](StringInterner.md) — maps text to stable integer atoms.
- [SourceManager](SourceManager.md) — owns source buffers and resolves byte offsets to line and column.
- [Platform](Platform.md) — virtual memory and whole-file platform services.

## Ownership model

These APIs are designed around `Arena`. Keep the arena alive and do not reset it while any container, source view, or interned view referring to its allocations is in use. Container growth allocates replacement storage from the arena; old storage remains allocated until the arena is reset or destroyed.

See the public headers under `HolyCore/Include/HolyCore/` for the definitive declarations.