#include <HolyLex/Lexer.h>

#include <cassert>
#include <charconv>
#include <cstdint>
#include <string>
#include <system_error>

using namespace holy::core;
using namespace holy::lex::detail;

namespace holy::lex {
	Token Lexer::LexNumber() {
		size_t start = m_pos;
		size_t numEnd;
		size_t digitStart;
		uint64_t value = 0;
		bool isFloat = false;
		bool overflow = false;
		bool reported = false;

		// Hex number
		if (Peek() == '0' && (Peek(1) == 'x' || Peek(1) == 'X')) {
			m_pos += 2;
			digitStart = m_pos;
			while (IsHexDigit(Peek())) {
				if (value >> 60 != 0) {
					overflow = true;
				}
				else {
					value = (value << 4) | DigitValue(Peek());
				}
				m_pos++;
			}

			// If digitStart and m_pos are equal that means that no digit were read
			if (digitStart == m_pos) {
				m_diags.Report(Severity::Error, LocAt(start), "hexadecimal literal has no digits");
				while (IsIdentChar(Peek())) {
					m_pos++;
				}
				reported = true;
			}
		}
		// Binary number
		else if (Peek() == '0' && (Peek(1) == 'b' || Peek(1) == 'B')) {
			m_pos += 2;
			digitStart = m_pos;
			while (IsDigit(Peek())) {
				if (Peek() != '0' && Peek() != '1') {
					break;
				}

				unsigned bit = Peek() == '1' ? 1 : 0;
				if (value >> 63 != 0) {
					overflow = true;
				}
				else {
					value = (value << 1) | bit;
				}
				
				++m_pos;
			}

			if (digitStart == m_pos) {
				m_diags.Report(Severity::Error, LocAt(start), "binary literal has no digits");
				while (IsIdentChar(Peek())) {
					m_pos++;
				}
				reported = true;
			}
			else if ((Peek() >= '2' && Peek() <= '9') || IsIdentStart(Peek())) {
				m_diags.Report(Severity::Error, LocAt(m_pos), "invalid digit '" + std::string(1, Peek()) + "' in binary literal");
				while (IsIdentChar(Peek())) {
					m_pos++;
				}
				reported = true;
			}
		}
		// Decimal
		else {
			if (Peek() == '.') {
				isFloat = true;
				++m_pos;
				while (IsDigit(Peek())) {
					++m_pos;
				}
			}
			else {
				while (IsDigit(Peek())) {
					int d = DigitValue(Peek());
					if (value > (UINT64_MAX - d) / 10) {
						overflow = true;
					}
					else {
						value = value * 10 + d;
					}
					++m_pos;
				}

				if (Peek() == '.' && IsDigit(Peek(1))) {
					isFloat = true;
					++m_pos;
					while (IsDigit(Peek())) {
						++m_pos;
					}
				}
			}

			if (Peek() == 'e' || Peek() == 'E') {
				size_t exponentStart = m_pos;
				size_t exponentDigitsOffset = 1;
				if (Peek(exponentDigitsOffset) == '+' || Peek(exponentDigitsOffset) == '-') {
					++exponentDigitsOffset;
				}

				if (!IsDigit(Peek(exponentDigitsOffset))) {
					m_diags.Report(Severity::Error, LocAt(exponentStart), "exponent has no digits");
					reported = true;
					m_pos += exponentDigitsOffset;
					while (IsIdentChar(Peek())) {
						++m_pos;
					}
				}
				else {
					isFloat = true;
					m_pos += exponentDigitsOffset;
					while (IsDigit(Peek())) {
						++m_pos;
					}
				}
			}
		}

		numEnd = m_pos;
		// Suffix check
		if (!reported) {
			if (IsIdentStart(Peek())) {
				size_t suffixStart = m_pos;
				while (IsIdentChar(Peek())) {
					m_pos++;
				}

				m_diags.Report(Severity::Error, LocAt(suffixStart), "invalid suffix '" + std::string(m_src.substr(suffixStart, m_pos - suffixStart)) + "' on numeric literal");
			}
		}

		Token t = MakeToken(isFloat ? TokenKind::FloatLiteral : TokenKind::IntLiteral, start, m_pos);
		if (isFloat) {
			double d = 0.0;
			if (!reported) {
				auto r = std::from_chars(m_src.data() + start, m_src.data() + numEnd, d);
				if (r.ec != std::errc() || r.ptr != m_src.data() + numEnd) {
					m_diags.Report(Severity::Error, LocAt(start), "floating-point literal out of range");
					d = 0.0;
				}
			}
			t.floatValue = d;
		}
		else if (overflow) {
			m_diags.Report(Severity::Error, LocAt(start), "integer literal too large");
			value = 0;
		}
		t.intValue = value;
		return t;
	}
}