#pragma once

#include <HolyLex/Keywords.h>
#include <HolyLex/Token.h>
#include <HolyCore/Arena.h>
#include <HolyCore/Diagnostics.h>
#include <HolyCore/SourceManager.h>
#include <HolyCore/StringInterner.h>
#include <HolyCore/Vector.h>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace holy::lex {
	class Lexer {
	public:
		static constexpr bool kBlockCommentsNest = false;

		Lexer(core::Arena& arena, const core::SourceManager& sources, core::DiagEngine& diags,
			core::StringInterner& interner, const KeywordTable& keywords, core::FileId file);

		Lexer(const Lexer&) = delete;
		Lexer& operator=(const Lexer&) = delete;

		[[nodiscard]] Token Next();
		[[nodiscard]] core::FileId File() { return m_file; }

	private:
		void SkipTrivia(); // whitespace and comments
		void SkipBlockComment(); // m_pos is at "/*"; reports "unterminated block comment" if needed
		Token LexIdentifierOrKeyword(); // m_pos is at an identifier start
		bool TryLexPunctuator(Token& out); // longest match; false (and no movement) if none
		Token LexUnexpected(); // consumes one byte, reports, returns Unknown

		[[nodiscard]] char Peek(size_t ahead = 0) const; // byte at m_pos + ahead, or '\0' past the end
		[[nodiscard]] core::SourceLoc LocAt(size_t offset) const; // offset must be <= file size
		[[nodiscard]] Token MakeToken(TokenKind kind, size_t start, size_t end); // kind, loc, and text only

		core::Arena& m_arena;
		const core::SourceManager& m_sources;
		core::DiagEngine& m_diags;
		core::StringInterner& m_interner;
		const KeywordTable& m_keywords;
		core::FileId m_file;
		std::string_view m_src;
		size_t m_pos = 0;
	};
}