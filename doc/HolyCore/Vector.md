# Vector

Header: `HolyCore/Include/HolyCore/Vector.h` (implementation: `Vector.ipp`)  
Namespace: `holy::core`

`Vector<T>` is a growable arena-backed sequence. `T` must be trivially copyable and trivially destructible because growth copies bytes and does not run element destructors.

## Construction and storage

- `Vector()` creates an empty vector with no arena. Assign an arena-backed vector or construct with `Vector(Arena&)` before an operation that needs storage.
- `explicit Vector(Arena&)` associates the vector with an arena.
- Growth allocates a larger buffer from the arena and copies existing elements. Previous buffers are not individually reclaimed. Capacity begins at a minimum of four and grows geometrically.

## API

- `Size()`, `Capacity()`, `Empty()` — inspect current size and capacity.
- `Data()`, `begin()`, `end()` — access contiguous storage and iterators.
- `operator[](index)`, `Front()`, `Back()` — access elements; indexing is assertion-checked in the implementation.
- `PushBack(value)` — append a copy.
- `EmplaceBack(args...)` — append a constructed value and return a reference to the new element.
- `PopBack()` — remove the last element from the logical size.
- `Clear()` — set size to zero and preserve capacity.
- `Reserve(minCapacity)` — ensure at least the requested capacity.
- `Resize(newSize)` — change logical size; newly exposed elements are value-initialized.

Do not use references or pointers into the vector across operations that may grow it. The vector's backing allocation remains owned by the arena.