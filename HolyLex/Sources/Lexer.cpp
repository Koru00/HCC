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
			{'n', '\n'}, {'r', '\r'}, {'t', '\t'}, {'0', '\0'}, {'\\', '\\'}, {'\'', '\''}, {'"', '"'}, {'`', '`'},
		};

		bool ScanDolDoc(std::string_view raw, size_t& unterminatedAt) {
			unterminatedAt = std::string_view::npos;
			bool found = false;
			size_t i = 0;
			while (i < raw.size()) {
				if (raw[i] == '\\') {
					i += i + 1 < raw.size() ? 2 : 1;
					continue;
				}

				if (raw[i] != '$') {
					++i;
					continue;
				}

				if (i + 1 < raw.size() && raw[i + 1] == '$') {
					i += 2;
					continue;
				}

				const size_t openingAt = i;
				size_t j = i + 1;
				while (j < raw.size()) {
					if (raw[j] == '\\') {
						j += j + 1 < raw.size() ? 2 : 1;
						continue;
					}

					if (raw[j] == '$') {
						found = true;
						i = j + 1;
						break;
					}

					++j;
				}

				if (j == raw.size()) {
					unterminatedAt = openingAt;
					break;
				}
			}

			return found;
		}
	}


	Lexer::Lexer(core::Arena& arena, const core::SourceManager& sources, core::DiagEngine& diags, 
		core::StringInterner& interner, const KeywordTable& keywords, core::FileId file, LexerOptions options) : m_arena(arena), m_sources(sources), m_diags(diags), 
		m_interner(interner), m_keywords(keywords), m_file(file) {
		
		assert(file.IsValid());
		m_src = m_sources.Contents(file);
		m_end = m_src.size();
		if (options.nulTerminatesSource) {
			const size_t nul = m_src.find('\0');
			if (nul != std::string_view::npos) {
				m_end = nul;
			}
		}
	}

	Token Lexer::Next() {
		SkipTrivia();
		const bool starts = m_newlineSeen;
		m_newlineSeen = false;

		Token t = LexToken();
		t.startLine = starts;
		return t;
	}

	Token Lexer::CaptureRawBlock(const Token& openBrace) {
		assert(openBrace.kind == TokenKind::LBrace);
		assert(openBrace.text.data() >= m_src.data());
		size_t begin = static_cast<size_t>(openBrace.text.data() - m_src.data()) + openBrace.text.size();
		assert(begin <= m_end);
		m_pos = begin;
		size_t depth = 1;
		bool closed = false;
		while (m_pos < m_end) {
			char c = Peek();
			switch (c) {
			case '{':
				++depth;
				++m_pos;
				break;
			case '}':
				--depth;
				if (depth == 0) {
					closed = true;
					break;
				}
				++m_pos;
				break;
			case '"':
				++m_pos;
				while (m_pos < m_end && Peek() != '"') {
					if (m_src[m_pos] == '\\') {
						m_pos += std::min<size_t>(2, m_end - m_pos);
					}
					else {
						++m_pos;
					}
				}

				if (Peek() == '"') {
					++m_pos;
				}
				break;
			case '\'':
				++m_pos;
				while (m_pos < m_end && Peek() != '\'' && Peek() != '\n') {
					if (m_src[m_pos] == '\\') {
						m_pos += std::min<size_t>(2, m_end - m_pos);
					}
					else {
						++m_pos;
					}
				}

				if (Peek() == '\'') {
					++m_pos;
				}
				break;
			case '/':
				if (Peek(1) == '/') {
					while (m_pos < m_end && Peek() != '\n') {
						++m_pos;
					}
				}
				else if (Peek(1) == '*') {
					SkipBlockComment();
				}
				else {
					++m_pos;
				}
				break;
			default:
				++m_pos;
				break;
			}
			if (closed) {
				break;
			}
		}

		if (!closed) {
			m_diags.Report(Severity::Error, openBrace.loc, "unterminated raw block, expected '}'");
		}

		Token t = MakeToken(TokenKind::RawBlock, begin, m_pos);
		t.text = m_src.substr(begin, m_pos - begin);
		t.loc = LocAt(begin); // First byte after '{'
		size_t i = m_pos;
		while (i > begin && (m_src[i - 1] == ' ' || m_src[i - 1] == '\t' || m_src[i - 1] == '\r')) {
			--i;
		}
		m_newlineSeen = i > begin && m_src[i - 1] == '\n';
		return t;
	}

	void Lexer::SkipTrivia() {
		while (true) {
			if (m_pos >= m_end) {
				return;
			}
			else if (IsSpace(Peek())) {
				if (Peek() == '\n') {
					m_newlineSeen = true;
				}
				m_pos++;
				continue;
			}
			else if (Peek() == '\\' && SkipLineSplice()) {
				continue;
			}
			else if (Peek() == '/' && Peek(1) == '/') {
				while (m_pos < m_end && Peek() != '\n') {
					m_pos++;
				}
				continue;
			}
			else if (Peek() == '/' && Peek(1) == '*') {
				SkipBlockComment();
				continue;
			}
			else {
				if (m_pos == 0) {
					// If is a the start of file set m_newlineSeen to true
					m_newlineSeen = true;
				}

				return;
			}
		}
	}

	bool Lexer::SkipLineSplice() {
		if (Peek() != '\\') {
			return false;
		}

		size_t newlineLength = 0;
		if (Peek(1) == '\n') {
			newlineLength = 1;
		}
		else if (Peek(1) == '\r') {
			newlineLength = Peek(2) == '\n' ? 2 : 1;
		}

		if (newlineLength == 0) {
			return false;
		}

		m_pos += 1 + newlineLength;
		return true;
	}

	void Lexer::SkipBlockComment() {
		size_t start = m_pos;
		m_pos += 2;
		size_t depth = 1;
		while (m_pos < m_end) {
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
		std::string_view rest = m_src.substr(m_pos, m_end - m_pos);
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

	Token Lexer::LexString() {
		size_t start = m_pos;
		++m_pos;
		std::string decoded;
		while (true) {
			if (m_pos >= m_end) {
				m_diags.Report(Severity::Error, LocAt(start), "unterminated string literal");
				return MakeToken(TokenKind::Unknown, start, m_pos);
			}

			char c = m_src[m_pos];
			if (c == '\"') {
				++m_pos;
				break;
			}

			if (c == '\\' && SkipLineSplice()) {
				continue;
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
		std::string_view raw = m_src.substr(start + 1, (m_pos - 1) - (start + 1));
		size_t bad;;
		t.hasDolDoc = ScanDolDoc(raw, bad);
		if (bad != std::string_view::npos) {
			m_diags.Report(Severity::Warning, LocAt(start + 1 + bad), "unterminated DolDoc code in string");
		}
		return t;
	}

	Token Lexer::LexChar() {
		size_t start = m_pos;
		++m_pos;
		uint64_t value = 0;
		size_t count = 0;
		bool tooLong = false;
		while (m_pos < m_end && m_src[m_pos] != '\n') {
			if (Peek() == '\'') {
				++m_pos;
				if (count == 0) {
					Token t = MakeToken(TokenKind::CharLiteral, start, m_pos);
					t.intValue = 0;
					return t;
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
			if (Peek() == '\\' && SkipLineSplice()) {
				continue;
			}

			if (Peek() == '\\') {
				byte = ReadEscape();
			}
			else {
				byte = static_cast<uint8_t>(Peek());
				++m_pos;
			}

			if (count < 16) {
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
		if (m_pos >= m_end) {
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

	Token Lexer::LexToken() {
		if (m_pos >= m_end) {
			return MakeToken(TokenKind::Eof, m_end, m_end);
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

		if (Peek() == '#') {
			return LexDirective();
		}

		Token t;
		if (TryLexPunctuator(t)) {
			return t;
		}

		return LexUnexpected();
	}

	Token Lexer::LexDirective() {
		size_t start = m_pos;
		++m_pos;
		if (!IsIdentStart(Peek())) {
			m_diags.Report(Severity::Error, LocAt(start), "expected directive name after '#'");
			return MakeToken(TokenKind::Unknown, start, m_pos);
		}

		size_t nameStart = m_pos;
		while (IsIdentChar(Peek())) {
			++m_pos;
		}

		Token t = MakeToken(TokenKind::Directive, start, m_pos);
		t.atom = m_interner.Intern(m_src.substr(nameStart, m_pos - nameStart));
		return t;
	}

	char Lexer::Peek(size_t ahead) const {
		return (m_pos < m_end && ahead < m_end - m_pos) ? m_src[m_pos + ahead] : '\0';
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