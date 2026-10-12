#include <HolyLex/Lexer.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <format>
#include <vector>

using namespace holy::core;
using namespace holy::lex::detail;

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

		struct Escape {
			char from;
			char to;
		};

		constexpr Escape kSimpleEscapes[] = {
			{'n', '\n'}, {'r', '\r'}, {'t', '\t'}, {'0', '\0'}, {'\\', '\\'}, {'\'', '\''}, {'"', '"'},
		};
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

		if (IsDigit(Peek()) || Peek() == '.' && IsDigit(Peek(1))) {
			return LexNumber();
		}

		if (Peek() == '"') {
			return LexString();
		}

		if (Peek() == '\'') {
			return LexChar();
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

	Token holy::lex::Lexer::LexString() {
		size_t start = m_pos;
		++m_pos;
		std::string decoded;
		while (true) {
			if (m_pos >= m_src.size()) {
				m_diags.Report(Severity::Error, LocAt(start), "unterminated string literal");
				return MakeToken(TokenKind::Unknown, start, m_pos);
			}

			char c = m_src[m_pos];
			if (c == '\"') {
				++m_pos;
				break;
			}

			if (c == '\\') {
				decoded.push_back(static_cast<char>(ReadEscape()));
			}
			else {
				decoded.push_back(c);
				++m_pos;
			}
		}

		Token t = MakeToken(TokenKind::StringLiteral, start, m_pos);
		t.stringValue = m_arena.CopyString(decoded);
		return t;
	}

	Token Lexer::LexChar() {
		size_t start = m_pos;
		++m_pos;
		uint64_t value = 0;
		size_t count = 0;
		bool tooLong = false;
		while (m_pos < m_src.size() && m_src[m_pos] != '\n') {
			if (Peek() == '\'') {
				++m_pos;
				if (count == 0) {
					m_diags.Report(Severity::Error, LocAt(start), "empty character constant");
					return MakeToken(TokenKind::CharLiteral, start, m_pos);
				}
				if (tooLong) {
					m_diags.Report(Severity::Error, LocAt(start), "character constant is too long");
					Token t = MakeToken(TokenKind::CharLiteral, start, m_pos);
					t.intValue = value;
					return t;
				}

				Token t = MakeToken(TokenKind::CharLiteral, start, m_pos);
				t.intValue = value;
				return t;
			}

			uint8_t byte;
			if (Peek() == '\\') {
				byte = ReadEscape();
			}
			else {
				byte = static_cast<uint8_t>(Peek());
				++m_pos;
			}

			if (count < 8) {
				value |= static_cast<uint64_t>(byte) << (8 * count);
			}
			else {
				tooLong = true;
			}
			++count;
		}

		m_diags.Report(Severity::Error, LocAt(start), "unterminated character constant");
		return MakeToken(TokenKind::Unknown, start, m_pos);
	}

	uint8_t Lexer::ReadEscape() {
		size_t start = m_pos;
		++m_pos;
		if (m_pos >= m_src.size()) {
			return '\\';
		}

		char c = m_src[m_pos];
		if (c == 'x') {
			++m_pos;
			if (IsHexDigit(Peek()) && IsHexDigit(Peek(1))) {
				uint8_t value = (DigitValue(Peek()) << 4) | DigitValue(Peek(1));
				m_pos += 2;
				return value;
			}
			else {
				m_diags.Report(Severity::Error, LocAt(start), "invalid hex escape sequence");
				return 0;
			}
		}
		else {
			for (const auto& esc : kSimpleEscapes) {
				if (esc.from == c) {
					++m_pos;
					return esc.to;
				}
			}
			if (c >= 0x20 && c <= 0x7E) {
				// If c is printable ASCII
				m_diags.Report(Severity::Error, LocAt(start), "unknown escape sequence '" + std::string(1, c) + "'");
			}
			else {
				m_diags.Report(Severity::Error, LocAt(start), "unknown escape sequence");
			}
			++m_pos;
			return c;
		}
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