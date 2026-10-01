#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>

template<class T>
T& holy::core::Vector<T>::operator[](size_t index) {
	assert(index < m_size);
	return m_data[index];
}

template<class T>
const T& holy::core::Vector<T>::operator[](size_t index) const {
	assert(index < m_size);
	return m_data[index];
}

template<class T>
T& holy::core::Vector<T>::Front() {
	assert(!Empty());
	return m_data[0];
}

template<class T>
const T& holy::core::Vector<T>::Front() const {
	assert(!Empty());
	return m_data[0];
}

template<class T>
T& holy::core::Vector<T>::Back() {
	assert(!Empty());
	return m_data[m_size - 1];
}

template<class T>
const T& holy::core::Vector<T>::Back() const {
	assert(!Empty());
	return m_data[m_size - 1];
}

template<class T>
void holy::core::Vector<T>::PushBack(const T& value) {
	if (m_size == m_capacity) {
		Grow(m_size + 1);
	}

	m_data[m_size++] = value;
}

template <class T>
template <class... Args>
T& holy::core::Vector<T>::EmplaceBack(Args&&... args) {
	if (m_size == m_capacity) {
		Grow(m_size + 1);
	}
	T* newElement = new (&m_data[m_size++]) T(std::forward<Args>(args)...);
	return *newElement;
}

template<class T>
void holy::core::Vector<T>::PopBack() {
	assert(!Empty());
	--m_size;
}

template<class T>
void holy::core::Vector<T>::Clear() {
	m_size = 0;
}

template<class T>
void holy::core::Vector<T>::Reserve(size_t minCapacity) {
	if (minCapacity > m_capacity) {
		Grow(minCapacity);
	}
}

template<class T>
void holy::core::Vector<T>::Resize(size_t newSize) {
	if (newSize > m_capacity) {
		Grow(newSize);
	}

	if (newSize > m_size) {
		for (size_t i = m_size; i < newSize; ++i) {
			new (&m_data[i]) T(); // value-initialize new elements
		}
	}

	m_size = newSize;
}

template<class T>
void* holy::core::Vector<T>::Grow(size_t minCapacity) {
	assert(m_arena != nullptr);
	size_t newCapacity = std::max<size_t>(m_capacity * 2, std::max<size_t>(kMinCapacity, minCapacity));
	if (newCapacity > SIZE_MAX / sizeof(T)) {
		// This hits the arena's fatal path
		m_arena->Allocate(SIZE_MAX, alignof(T));
	}

	T* p = static_cast<T*>(m_arena->Allocate(newCapacity * sizeof(T), alignof(T)));

	if (m_size != 0) {
		std::memcpy(p, m_data, m_size * sizeof(T));
	}

	m_data = p;
	m_capacity = newCapacity;
	return p;
}