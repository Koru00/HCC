#pragma once

#include <HolyLex/Token.h>
#include <HolyCore/Arena.h>
#include <HolyCore/HashMap.h>
#include <HolyCore/StringInterner.h>

namespace holy::lex {
	class KeywordTable {
	public:
		KeywordTable(core::Arena& arena, core::StringInterner& interner);	// interns every keyword spelling and maps its Atom to its kind

		KeywordTable(const KeywordTable&) = delete;
		KeywordTable& operator=(const KeywordTable&) = delete;

		[[nodiscard]] TokenKind LookUp(core::Atom atom) const;	// keyword kind, or TokenKind::Identifier if it isn't one
		[[nodiscard]] size_t Count() const { return m_map.Size(); }
	private:
		core::HashMap<core::Atom, TokenKind> m_map;
	};
}