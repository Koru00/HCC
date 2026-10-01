#pragma once

#include <HolyCore/Arena.h>
#include <HolyCore/HashMap.h>
#include <HolyCore/Vector.h>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace holy::core {
	struct Atom {
		uint32_t id = 0;

		[[nodiscard]] constexpr bool IsValid() const { return id != 0; }
		friend constexpr bool operator==(Atom, Atom) = default; // C++20: compares id
	};

	template <>
	struct DefaultHash<Atom> {
		size_t operator()(Atom atom) const { return DefaultHash<uint32_t>{}(atom.id); }
	};

	class StringInterner {
	public:
		explicit StringInterner(Arena& arena);

		StringInterner(const StringInterner&) = delete;
		StringInterner& operator=(const StringInterner&) = delete;

		[[nodiscard]] Atom Intern(std::string_view text); // existing atom, or a new one (copies text into the arena)
		[[nodiscard]] Atom Find(std::string_view text) const; // existing atom, or Atom{} if never interned; never inserts
		[[nodiscard]] std::string_view Text(Atom atom) const; // the interned text; assert(atom is valid and from this interner)
		[[nodiscard]] size_t Count() const; // number of distinct strings interned
	
	private:
		Arena& m_arena; // owns every byte we use
		HashMap<std::string_view, uint32_t> m_map; // text -> id (keys are the arena copies)
		Vector<std::string_view> m_text; // id -> text; slot 0 is a placeholder so ids start at 1
	};
}