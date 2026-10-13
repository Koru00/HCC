#include <HolyLex/Keywords.h>

#include <cassert>
#include <cstddef>
#include <iterator>

namespace {
	const char* kKindNames[] = {
		"Eof", "Unknown", "Identifier", "Directive", "RawBlock", "IntLiteral", "FloatLiteral", "CharLiteral", "StringLiteral",
#define X(name, spelling) #name,
		HOLY_PUNCTUATORS(X)
#undef X
		"KeywordMarkerBegin",
#define X(name, spelling) #name,
		HOLY_KEYWORDS(X)
#undef X
		"KeywordMarkerEnd"
	};

	const char* kSpelling[] = {
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
#define X(name, spelling) spelling,
		HOLY_PUNCTUATORS(X)
#undef X
		nullptr,
#define X(name, spelling) spelling,
		HOLY_KEYWORDS(X)
#undef X
		nullptr
	};

	static_assert(std::size(kKindNames) == static_cast<size_t>(holy::lex::TokenKind::Count));
	static_assert(std::size(kSpelling) == static_cast<size_t>(holy::lex::TokenKind::Count));
}

namespace holy::lex {
	const char* TokenKindName(TokenKind kind) {
		const size_t i = static_cast<size_t>(kind);
		assert(i < static_cast<size_t>(TokenKind::Count));
		return kKindNames[i];
	}

	const char* TokenKindSpelling(TokenKind kind) {
		const size_t i = static_cast<size_t>(kind);
		assert(i < static_cast<size_t>(TokenKind::Count));
		return kSpelling[i];
	}

	KeywordTable::KeywordTable(core::Arena& arena, core::StringInterner& interner) : m_map(arena) {
		for (size_t i = static_cast<size_t>(TokenKind::KeywordMarkerBegin) + 1; i < static_cast<size_t>(TokenKind::KeywordMarkerEnd); i++) {
			m_map.Insert(interner.Intern(kSpelling[i]), static_cast<TokenKind>(i));
		}
	}

	TokenKind KeywordTable::LookUp(core::Atom atom) const {
		const TokenKind* kind = m_map.Find(atom);
		return kind ? *kind : TokenKind::Identifier;
	}
}