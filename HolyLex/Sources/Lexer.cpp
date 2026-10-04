#include <HolyLex/Lexer.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <format>
#include <vector>

using namespace holy::core;

namespace holy::lex {
	namespace {
		bool IsSpace(char c) {
			switch (c) {
			case ' ':
			case '\t':
			case '\n':
			case '\r':
			case '\f':
			case '\v':
				return true;
			default:
				return false;
			}
		}

		bool IsIdentStart(char c) {
			if (c >= 'a' && c <= 'z') {
				return true;
			}
			else if (c >= 'A' && c <= 'Z') {
				return true;
			}
			else if (c == '_') {
				return true;
			}

			return false;
		}

		bool IsIdentChar(char c) {
			return IsIdentStart(c) || (c >= '0' && c <= '9');
		}

		struct PunctEntry {
			const char* spelling;
			size_t length;
			TokenKind kind;
		};

		static const std::vector<PunctEntry>& Punctuators() {
			static const std::vector<PunctEntry> punctEntries = [] {
				std::vector<PunctEntry> entries;
				for (size_t i = static_cast<size_t>(TokenKind::StringLiteral) + 1;
					i < static_cast<size_t>(TokenKind::KeywordMarkerBegin); ++i) {
					TokenKind kind = static_cast<TokenKind>(i);
					const char* spelling = TokenKindSpelling(kind);
					entries.push_back(PunctEntry{ .spelling = spelling, .length = std::strlen(spelling), .kind = kind });
				}
				std::stable_sort(entries.begin(), entries.end(), [](const PunctEntry& a, const PunctEntry& b) {
					return a.length > b.length;
				});
				return entries;
			}();
			return punctEntries;
		}

		std::string DescribeByte(char c) {
			if (c >= 0x20 && c <= 0x7E) {
				return "unexpected character '" + std::string(1, c) + "'";
			}
			else {
				return std::format("unexpected byte 0x{:02X}", static_cast<unsigned char>(c));
			}
		}
	}


	Lexer::Lexer(core::Arena& arena, const core::SourceManager& sources, core::DiagEngine& diags, 
		core::StringInterner& interner, const KeywordTable& keywords, core::FileId file) : m_arena(arena), m_sources(sources), m_diags(diags), 
		m_interner(interner), m_keywords(keywords), m_file(file) {
		
		assert(file.IsValid());
		m_src = m_sources.Contents(file);
	}

	Token Lexer::Next() {
		SkipTrivia();
		if (m_pos >= m_src.size()) {
			return MakeToken(TokenKind::Eof, m_src.size(), m_src.size());
		}
		
		if (IsIdentStart(Peek())) {
			return LexIdentifierOrKeyword();
		}
		
		Token t;
		if (TryLexPunctuator(t)) {
			return t;
		}

		return LexUnexpected();
	}

	void Lexer::SkipTrivia() {
		while (true) {
			if (m_pos >= m_src.size()) {
				return;
			}
			else if (IsSpace(Peek())) {
				m_pos++;
				continue;
			}
			else if (Peek() == '/' && Peek(1) == '/') {
				while (m_pos < m_src.size() && Peek() != '\n') {
					m_pos++;
				}
				continue;
			}
			else if (Peek() == '/' && Peek(1) == '*') {
				SkipBlockComment();
				continue;
			}
			else {
				return;
			}
		}
	}

	void Lexer::SkipBlockComment() {
		size_t start = m_pos;
		m_pos += 2;
		size_t depth = 1;
		while (m_pos < m_src.size()) {
			if (Peek() == '*' && Peek(1) == '/') {
				m_pos += 2;
				if (--depth == 0) {
					return;
				}
			}
			else if (kBlockCommentsNest && Peek() == '/' && Peek(1) == '*') {
				m_pos += 2;
				depth++;
			}
			else {
				m_pos++;
			}
		}

		m_diags.Error(LocAt(start), "unterminated block comment");
	}

	Token Lexer::LexIdentifierOrKeyword() {
		size_t start = m_pos;
		while (IsIdentChar(Peek())) {
			m_pos++;
		}
		Token t = MakeToken(TokenKind::Identifier, start, m_pos);
		t.atom = m_interner.Intern(t.text);
		t.kind = m_keywords.LookUp(t.atom);
		return t;
	}

	bool Lexer::TryLexPunctuator(Token& out) {
		std::string_view rest = m_src.substr(m_pos);
		for (size_t i = 0; i < Punctuators().size(); i++) {
			PunctEntry entry = Punctuators()[i];
			if (rest.size() >= entry.length && std::memcmp(rest.data(), entry.spelling, entry.length) == 0) {
				size_t start = m_pos;
				m_pos += entry.length;
				out = MakeToken(entry.kind, start, m_pos);
				return true;
			}
		}

		return false;
	}

	Token Lexer::LexUnexpected() {
		size_t start = m_pos;
		++m_pos;
		Token t = MakeToken(TokenKind::Unknown, start, m_pos);
		m_diags.Error(t.loc, DescribeByte(m_src[start]));
		return t;
	}

	char Lexer::Peek(size_t ahead) const {
		return (m_pos + ahead < m_src.size()) ? m_src[m_pos + ahead] : '\0';
	}

	core::SourceLoc Lexer::LocAt(size_t offset) const {
		assert(offset <= m_src.size());
		return m_sources.LocFromOffset(m_file, static_cast<uint32_t>(offset));
	}

	Token Lexer::MakeToken(TokenKind kind, size_t start, size_t end) {
		Token t;
		t.kind = kind;
		t.loc = LocAt(start);
		t.text = m_src.substr(start, end - start);
		return t;
	}
}