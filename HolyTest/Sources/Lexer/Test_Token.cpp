#include <HolyTest/Core/TestHarness.h>

#include <HolyLex/Keywords.h>
#include <HolyLex/Token.h>
#include <HolyCore/Arena.h>
#include <HolyCore/StringInterner.h>
#include <cstddef>
#include <string_view>

using namespace holy::core;
using namespace holy::lex;

namespace {
	bool IsAsciiLetter(char c) {
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
	}

	bool IsAsciiDigit(char c) {
		return c >= '0' && c <= '9';
	}

	bool IsNoSpellingKind(TokenKind kind) {
		return kind == TokenKind::Eof
			|| kind == TokenKind::Unknown
			|| kind == TokenKind::Identifier
			|| kind == TokenKind::IntLiteral
			|| kind == TokenKind::FloatLiteral
			|| kind == TokenKind::CharLiteral
			|| kind == TokenKind::StringLiteral
			|| kind == TokenKind::KeywordMarkerBegin
			|| kind == TokenKind::KeywordMarkerEnd;
	}
}

TEST(Token_DefaultIsUnknown) {
	Token t;
	CHECK(t.kind == TokenKind::Unknown);
	CHECK(!t.loc.IsValid());
	CHECK(t.text.empty());
	CHECK(!t.atom.IsValid());
	CHECK(t.intValue == 0);
	CHECK(t.floatValue == 0.0);
	CHECK(t.stringValue.empty());
	CHECK(t.Is(TokenKind::Unknown));
	CHECK(t.IsNot(TokenKind::Eof));
}

TEST(Token_KindNamesAndSpellings) {
	CHECK(std::string_view(TokenKindName(TokenKind::Eof)) == "Eof");
	CHECK(std::string_view(TokenKindName(TokenKind::LParen)) == "LParen");
	CHECK(std::string_view(TokenKindName(TokenKind::KwIf)) == "KwIf");
	CHECK(std::string_view(TokenKindName(TokenKind::Identifier)) == "Identifier");

	CHECK(std::string_view(TokenKindSpelling(TokenKind::LParen)) == "(");
	CHECK(std::string_view(TokenKindSpelling(TokenKind::KwIf)) == "if");
	CHECK(std::string_view(TokenKindSpelling(TokenKind::Arrow)) == "->");
	CHECK(std::string_view(TokenKindSpelling(TokenKind::LessLessEqual)) == "<<=");
	CHECK(std::string_view(TokenKindSpelling(TokenKind::Ellipsis)) == "...");
	CHECK(TokenKindSpelling(TokenKind::Eof) == nullptr);
	CHECK(TokenKindSpelling(TokenKind::Identifier) == nullptr);
	CHECK(TokenKindSpelling(TokenKind::IntLiteral) == nullptr);
}

TEST(Token_KindTableInvariants) {
	const size_t kindCount = static_cast<size_t>(TokenKind::Count);
	for (size_t i = 0; i < kindCount; ++i) {
		const TokenKind kind = static_cast<TokenKind>(i);
		const char* name = TokenKindName(kind);
		const char* spelling = TokenKindSpelling(kind);

		CHECK(name != nullptr);
		CHECK(name != nullptr && name[0] != '\0');
		if (spelling == nullptr) {
			CHECK(IsNoSpellingKind(kind));
			continue;
		}

		CHECK(spelling[0] != '\0');
		for (const char* c = spelling; *c != '\0'; ++c) {
			if (IsKeyword(kind)) {
				CHECK(IsAsciiLetter(*c) || *c == '_');
			} else {
				CHECK(!IsAsciiLetter(*c) && !IsAsciiDigit(*c) && *c != '_');
			}
		}

		for (size_t j = i + 1; j < kindCount; ++j) {
			const char* otherSpelling = TokenKindSpelling(static_cast<TokenKind>(j));
			CHECK(otherSpelling == nullptr || std::string_view(spelling) != otherSpelling);
		}
	}
}

TEST(Token_IsKeywordRange) {
	CHECK(IsKeyword(TokenKind::KwIf));
	CHECK(IsKeyword(TokenKind::KwReturn));
	CHECK(!IsKeyword(TokenKind::Identifier));
	CHECK(!IsKeyword(TokenKind::LParen));
	CHECK(!IsKeyword(TokenKind::Eof));
	CHECK(!IsKeyword(TokenKind::KeywordMarkerBegin));
	CHECK(!IsKeyword(TokenKind::KeywordMarkerEnd));

	Token t;
	t.kind = TokenKind::KwIf;
	CHECK(t.IsKeyword());
}

TEST(Keywords_Lookup) {
	Arena a(1 << 20);
	StringInterner in(a);
	KeywordTable kt(a, in);

	CHECK(kt.LookUp(in.Intern("if")) == TokenKind::KwIf);
	CHECK(kt.LookUp(in.Intern("while")) == TokenKind::KwWhile);
	CHECK(kt.LookUp(in.Intern("class")) == TokenKind::KwClass);
	CHECK(kt.LookUp(in.Intern("return")) == TokenKind::KwReturn);
	CHECK(kt.LookUp(in.Intern("I64")) == TokenKind::Identifier);
	CHECK(kt.LookUp(in.Intern("U0")) == TokenKind::Identifier);
	CHECK(kt.LookUp(in.Intern("Print")) == TokenKind::Identifier);
	CHECK(kt.LookUp(in.Intern("IF")) == TokenKind::Identifier);
	CHECK(kt.LookUp(in.Intern("ifx")) == TokenKind::Identifier);
	CHECK(kt.LookUp(in.Intern("")) == TokenKind::Identifier);
}

TEST(Keywords_AllSpellingsRoundTrip) {
	Arena a(1 << 20);
	StringInterner in(a);
	KeywordTable kt(a, in);

	size_t visited = 0;
	for (size_t i = static_cast<size_t>(TokenKind::KeywordMarkerBegin) + 1;
		i < static_cast<size_t>(TokenKind::KeywordMarkerEnd); ++i) {
		const TokenKind kind = static_cast<TokenKind>(i);
		CHECK(kt.LookUp(in.Intern(TokenKindSpelling(kind))) == kind);
		++visited;
	}

	CHECK(visited == kt.Count());
	CHECK(visited > 0);
}

TEST(Keywords_InternedBeforeTable) {
	Arena a(1 << 20);
	StringInterner in(a);
	const Atom fooAtom = in.Intern("foo");
	const Atom ifAtom = in.Intern("if");
	KeywordTable kt(a, in);

	CHECK(kt.LookUp(ifAtom) == TokenKind::KwIf);
	CHECK(kt.LookUp(fooAtom) == TokenKind::Identifier);
}
