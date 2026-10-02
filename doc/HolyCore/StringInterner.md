# StringInterner

Header: `HolyCore/Include/HolyCore/StringInterner.h`  
Namespace: `holy::core`

`StringInterner` deduplicates strings and represents each unique value by an `Atom`, a small integer ID. Text is copied into a caller-provided `Arena` and remains valid until that arena is reset or destroyed.

## Types and API

### `Atom`

A wrapper around a 32-bit ID. ID zero is invalid; valid IDs start at one. Equality compares the ID. `DefaultHash<Atom>` hashes that ID.

### `StringInterner(Arena& arena)`

Constructs an interner whose text copies and internal container storage belong to `arena`. Copy construction and assignment are disabled.

- `Atom Intern(std::string_view text)` — return the existing atom if the exact text has already been interned; otherwise copy the text and return a new atom.
- `Atom Find(std::string_view text) const` — look up existing text without inserting; return an invalid atom when absent.
- `std::string_view Text(Atom atom) const` — retrieve interned text; asserts that the atom is valid and belongs to this interner.
- `size_t Count() const` — number of distinct strings interned.

The empty string is a valid string to intern and receives a valid atom. Treat returned views and atoms as scoped to the lifetime of this interner and its arena.