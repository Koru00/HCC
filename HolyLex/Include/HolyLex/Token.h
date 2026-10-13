#pragma once

#include <HolyCore/SourceManager.h>
#include <HolyCore/StringInterner.h>
#include <cstdint>
#include <string_view>
#include <type_traits>

#define HOLY_PUNCTUATORS(X) \
	/* Brackets and separators */ \
	X(LParen, "(") \
	X(RParen, ")") \
	X(LBracket, "[") \
	X(RBracket, "]") \
	X(LBrace, "{") \
	X(RBrace, "}") \
	X(Semi, ";") \
	X(Comma, ",") \
	X(Dot, ".") \
	X(Colon, ":") \
	X(Ellipsis, "...") \
	X(Arrow, "->") \
	/* Single-character operators */ \
	X(Plus, "+") \
	X(Minus, "-") \
	X(Star, "*") \
	X(Slash, "/") \
	X(Percent, "%") \
	X(Amp, "&") \
	X(Pipe, "|") \
	X(Greater, ">") \
	X(Less, "<") \
	X(Bang, "!") \
	X(Power, "`") \
	X(Caret, "^") \
	X(Tilde, "~") \
    X(Question, "?") \
    X(Equal, "=") \
	/* Two-character operators */ \
	X(PlusPlus, "++") \
	X(MinusMinus, "--") \
	X(LessLess, "<<") \
	X(GreaterGreater, ">>") \
	X(LessEqual, "<=") \
	X(GreaterEqual, ">=") \
	X(EqualEqual, "==") \
	X(AmpAmp, "&&") \
	X(CaretCaret, "^^") \
	X(PipePipe, "||") \
	X(ColonColon, "::") \
	/* Compound assignment */ \
	X(PlusEqual, "+=") \
	X(MinusEqual, "-=") \
	X(StarEqual, "*=") \
	X(SlashEqual, "/=") \
	X(PercentEqual, "%=") \
	X(AmpEqual, "&=") \
	X(XorEqual, "^=") \
	X(PipeEqual, "|=") \
	X(CaretEqual, "`=") \
	X(LessLessEqual, "<<=") \
	X(GreaterGreaterEqual, ">>=")

#define HOLY_KEYWORDS(X) \
	/* Base keywords */ \
	X(KwIf, "if") \
	X(KwElse, "else") \
	X(KwWhile, "while") \
	X(KwFor, "for") \
	X(KwDo, "do") \
	X(KwSwitch, "switch") \
	X(KwCase, "case") \
	X(KwDefault, "default") \
	X(KwBreak, "break") \
	X(KwGoto, "goto") \
	X(KwReturn, "return") \
	X(KwTry, "try") \
	X(KwCatch, "catch") \
	X(KwClass, "class") \
	X(KwUnion, "union") \
	X(KwExtern, "extern") \
	X(KwStatic, "static") \
	X(KwAsm, "asm") \
	X(KwSizeof, "sizeof") \
	/* Additional reference keywords */ \
	X(KwThrow, "throw") \
	X(KwContinue, "continue") \
	X(KwLock, "lock") \
	X(KwPublic, "public") \
	X(KwInterrupt, "interrupt") \
	X(KwHasErrorCode, "haserrorcode") \
	X(KwArgPop, "argpop") \
	X(KwNoArgPop, "noargpop") \
	X(KwReg, "reg") \
	X(KwNoReg, "noreg") \
	X(KwNoWarn, "no_warn") \
	X(KwImport, "import") \
	X(Kw_Import, "_import") \
	X(Kw_Extern, "_extern") \
	X(KwStart, "start") \
	X(KwEnd, "end") \
	X(KwOffset, "offset") \
	X(KwLastclass, "lastclass") \
	X(KwDefined, "defined")


namespace holy::lex {
	enum class TokenKind {
		Eof, Unknown, Identifier, Directive, RawBlock, IntLiteral, FloatLiteral, CharLiteral, StringLiteral,
#define X(name, spelling) name,
		HOLY_PUNCTUATORS(X)
#undef X
		KeywordMarkerBegin,
#define X(name, spelling) name,
		HOLY_KEYWORDS(X)
#undef X
		KeywordMarkerEnd,
		Count,
	};

	[[nodiscard]] const char* TokenKindName(TokenKind kind);
	[[nodiscard]] const char* TokenKindSpelling(TokenKind kind);
	[[nodiscard]] constexpr bool IsKeyword(TokenKind kind) {
		return kind > TokenKind::KeywordMarkerBegin && kind < TokenKind::KeywordMarkerEnd;
	}

	struct Token {
		TokenKind kind = TokenKind::Unknown;
		core::SourceLoc loc;	// first byte of the token
		std::string_view text;	// raw spelling, a view into the source buffer
		core::Atom atom; // Identifier and keywords: interned spelling; otherwise invalid
		uint64_t intValue = 0; // IntLiteral and CharLiteral
		double floatValue = 0.0; // FloatLiteral
		std::string_view stringValue; // StringLiteral: decoded bytes (arena copy)
		bool startLine = false;
		bool hasDolDoc = false;

		[[nodiscard]] bool Is(TokenKind k) const { return kind == k; }
		[[nodiscard]] bool IsNot(TokenKind k) const { return kind != k; }
		[[nodiscard]] bool IsKeyword() const { return holy::lex::IsKeyword(kind); }
	};

	static_assert(std::is_trivially_copyable_v<Token>&& std::is_trivially_destructible_v<Token>, "Token must be arena/Vector friendly");
}