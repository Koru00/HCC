#include <HolyCore/StringInterner.h>
#include <cassert>

namespace holy::core {
	StringInterner::StringInterner(Arena& arena) : m_arena(arena), m_map(arena), m_text(m_arena) {
		m_text.PushBack(std::string_view{}); // Placeholder for id 0
	}

	Atom StringInterner::Intern(std::string_view text) {
		if (const uint32_t* found = m_map.Find(text)) {
			return Atom{ *found };
		}

		const std::string_view copy = m_arena.CopyString(text);
		assert(m_text.Size() < UINT32_MAX);
		const uint32_t id = static_cast<uint32_t>(m_text.Size());
		m_text.PushBack(copy);
		m_map.Insert(copy, id); // Insert's result is ignored because it can only be true here
		return Atom{ id };
	}

	Atom StringInterner::Find(std::string_view text) const {
		if (const uint32_t* found = m_map.Find(text)) {
			return Atom{ *found };
		}
		return Atom{};
	}

	std::string_view StringInterner::Text(Atom atom) const {
		assert(atom.IsValid() && atom.id < m_text.Size());
		return m_text[atom.id];
	}

	size_t StringInterner::Count() const {
		return m_text.Size() - 1;
	}


}