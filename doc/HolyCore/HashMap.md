# HashMap

Header: `HolyCore/Include/HolyCore/HashMap.h` (implementation: `HashMap.ipp`)  
Namespace: `holy::core`

`HashMap<K,V,Hash,Eq>` is an arena-backed, open-addressed hash map using linear probing. Keys and values must be trivially copyable and trivially destructible. The default hash supports integral and enum keys, pointer keys, and `std::string_view`; custom key types need a custom hash functor.

## Construction and capacity

- `HashMap()` creates an empty map without an arena; associate an arena before insertion or reserve operations that allocate.
- `explicit HashMap(Arena&)` associates the map with an arena.
- `Reserve(expectedCount)` provisions capacity so the expected number of insertions can fit without rehashing, subject to the implementation's 75% maximum load factor.
- Initial capacity is at least 16 and capacities are powers of two. Rehashing allocates new slots from the arena; old buffers remain allocated until arena reset/destruction.

## API

- `Size()`, `Capacity()`, `Empty()` — inspect map state.
- `Insert(key, value)` — insert only if absent; returns false and leaves the stored value unchanged if the key already exists.
- `InsertOrAssign(key, value)` — insert or overwrite; returns a reference to the stored value.
- `Find(key)` — return a pointer to the mapped value or null; const and mutable overloads are available.
- `Contains(key)` — test key presence.
- `Clear()` — remove all entries while retaining capacity.
- `ForEach(fn)` — invoke `fn(const K&, V&)` for each occupied slot; the const overload provides const mapped values.

`DefaultHash<std::string_view>` hashes the view's bytes (FNV-1a followed by `Mix64`), so views used as keys must continue to refer to valid, unchanged text for the map's lifetime. `StringInterner` handles this requirement by storing keys in arena-owned copies.