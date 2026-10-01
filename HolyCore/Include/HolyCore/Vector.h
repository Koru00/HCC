#pragma once

#include <HolyCore/Arena.h>
#include <type_traits>

namespace holy::core {
	template <class T>
	class Vector {
		static_assert(std::is_trivially_copyable_v<T>&& std::is_trivially_destructible_v<T>,
			"Vector<T>: T must be trivially copyable and destructible (arena storage)");
	public:
		static constexpr size_t kMinCapacity = 4;

		Vector() = default; // no arena; must be assigned before growing
		explicit Vector(Arena& arena) : m_arena(&arena) {}

		[[nodiscard]] size_t Size() const { return m_size; }
		[[nodiscard]] size_t Capacity() const { return m_capacity; }
		[[nodiscard]] bool Empty() const { return m_size == 0; }

		[[nodiscard]] T* Data() { return m_data; }
		[[nodiscard]] const T* Data() const { return m_data; }

		[[nodiscard]] T& operator[](size_t index);
		[[nodiscard]] const T& operator[](size_t index) const;
		[[nodiscard]] T& Front();
		[[nodiscard]] const T& Front() const;
		[[nodiscard]] T& Back();
		[[nodiscard]] const T& Back() const;

		T* begin() { return m_data; }
		T* end() { return m_data + m_size; };
		const T* begin() const { return m_data; }
		const T* end() const { return m_data + m_size; };

		void PushBack(const T& value);
		template <class... Args> T& EmplaceBack(Args&&... args); // returns the new element
		void PopBack();
		void Clear();	// size = 0, capacity kept
		void Reserve(size_t minCapacity);
		void Resize(size_t newSize); // new elements are value-initialized
	private:
		void* Grow(size_t minCapacity);

		Arena* m_arena = nullptr;
		T* m_data = nullptr;
		size_t m_size = 0;
		size_t m_capacity = 0;
	};
}

#include <HolyCore/Vector.ipp>