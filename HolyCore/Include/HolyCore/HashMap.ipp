#include <HolyCore/Arena.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <new>
#include <string_view>
#include <type_traits>

template <class K, class V, class Hash, class Eq>
size_t holy::core::HashMap<K, V, Hash, Eq>::Probe(const K& key) const {
	assert(m_capacity != 0);
	size_t mask = m_capacity - 1;
	size_t i = Hash{}(key)&mask;

	size_t start = i;

	while (m_used[i] != 0) {
		if (Eq{}(m_slots[i].key, key))
			return i;

		i = (i + 1) & mask;

		if (i == start)
			break;
	}

	return i;
}

template <class K, class V, class Hash, class Eq>
void holy::core::HashMap<K, V, Hash, Eq>::Rehash(size_t newCapacity) {
	if (newCapacity <= m_capacity) return;
	assert(m_arena != nullptr);
	assert((newCapacity & (newCapacity - 1)) == 0);

	if (newCapacity > SIZE_MAX / sizeof(Slot)) {
		m_slots = static_cast<Slot*>(m_arena->Allocate(SIZE_MAX, alignof(Slot)));
	}
	
	Slot* newSlots = static_cast<Slot*>(m_arena->Allocate(newCapacity * sizeof(Slot), alignof(Slot)));
	uint8_t* newUsed = static_cast<uint8_t*>(m_arena->Allocate(newCapacity, 1));
	std::memset(newUsed, 0, newCapacity);

	Slot* oldSlots = m_slots;
	uint8_t* oldUsed = m_used;
	size_t oldCapacity = m_capacity;

	m_slots = newSlots;
	m_used = newUsed;
	m_capacity = newCapacity;

	for (size_t index = 0; index < oldCapacity; ++index) {
		if (oldUsed[index] != 0) {
			const Slot& old = oldSlots[index];

			size_t newIndex = Probe(old.key);

			::new (&m_slots[newIndex]) Slot{ old.key, old.value };
			m_used[newIndex] = 1;
		}
	}
}

template <class K, class V, class Hash, class Eq>
bool holy::core::HashMap<K, V, Hash, Eq>::Insert(const K& key, const V& value) {
	if (m_capacity == 0) {
		Rehash(kMinCapacity);
	}
	else if ((m_size + 1) * 4 > m_capacity * 3) {
		Rehash(m_capacity * 2);
	}

	size_t i = Probe(key);
	if (m_used[i]) {
		return false;
	}

	::new(&m_slots[i]) Slot { key, value };
	m_used[i] = 1;
	++m_size;
	return true;
}

template <class K, class V, class Hash, class Eq>
V& holy::core::HashMap<K, V, Hash, Eq>::InsertOrAssign(const K& key, const V& value) {
	if (m_capacity == 0) {
		Rehash(kMinCapacity);
	}
	else if ((m_size + 1) * 4 > m_capacity * 3) {
		Rehash(m_capacity * 2);
	}

	size_t i = Probe(key);
	if (m_used[i]) {
		m_slots[i].value = value;
		return m_slots[i].value;;
	}

	::new(&m_slots[i]) Slot{ key, value };
	m_used[i] = 1;
	++m_size;
	return m_slots[i].value;;
}

template <class K, class V, class Hash, class Eq>
V* holy::core::HashMap<K, V, Hash, Eq>::Find(const K& key) {
	if (m_capacity == 0) return nullptr;
	size_t i = Probe(key);
	return m_used[i] ? &m_slots[i].value : nullptr;
}

template <class K, class V, class Hash, class Eq>
const V* holy::core::HashMap<K, V, Hash, Eq>::Find(const K& key) const {
	return const_cast<HashMap*>(this)->Find(key);
}

template <class K, class V, class Hash, class Eq>
bool holy::core::HashMap<K, V, Hash, Eq>::Contains(const K& key) const {
	return Find(key) != nullptr;
}

template <class K, class V, class Hash, class Eq>
void holy::core::HashMap<K, V, Hash, Eq>::Reserve(size_t exprectedCount) {
	size_t newCapacity = kMinCapacity;
	while (newCapacity * 3 < exprectedCount * 4) {
		newCapacity *= 2;
	}

	Rehash(newCapacity);
}

template <class K, class V, class Hash, class Eq>
void holy::core::HashMap<K, V, Hash, Eq>::Clear() {
	if (m_capacity != 0) {
		std::memset(m_used, 0, m_capacity);
		m_size = 0;
	}
}

template <class K, class V, class Hash, class Eq>
template <class F>
void holy::core::HashMap<K, V, Hash, Eq>::ForEach(F&& fn) {
	for (size_t i = 0; i < m_capacity; i++) {
		if (m_used[i]) {
			fn(m_slots[i].key, m_slots[i].value);
		}
	}
}

template <class K, class V, class Hash, class Eq>
template <class F>
void holy::core::HashMap<K, V, Hash, Eq>::ForEach(F&& fn) const {
	for (size_t i = 0; i < m_capacity; i++) {
		if (m_used[i]) {
			fn(m_slots[i].key, m_slots[i].value);
		}
	}
}