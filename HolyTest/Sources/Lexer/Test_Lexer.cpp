#include <HolyTest/Core/TestHarness.h>
#include <HolyTest/Lexer/LexTestUtil.h>

#include <HolyLex/Lexer.h>
#include <HolyCore/Diagnostics.h>
#include <string>
#include <string_view>
#include <vector>

using namespace holy::core;
using namespace holy::lex;
using namespace lextest;

TEST(Lexer_EmptyAndWhitespaceOnly) {
	Fixture fx;
	auto empty = LexAll(fx, "");
	CHECK(Kinds(empty) == (std::vector<TokenKind>{ TokenKind::Eof }));
	CHECK(empty[0].loc.IsValid());

	fx.file = fx.sm.AddBuffer("manual.HC", "");
	Lexer lx(fx.arena, fx.sm, fx.diags, fx.interner, fx.keywords, fx.file);
	CHECK(lx.Next().kind == TokenKind::Eof);
	CHECK(lx.Next().kind == TokenKind::Eof);
	CHECK(lx.Next().kind == TokenKind::Eof);

	Fixture whitespaceFx;
	auto whitespace = LexAll(whitespaceFx, " \t\r\n ");
	CHECK(Kinds(whitespace) == (std::vector<TokenKind>{ TokenKind::Eof }));
	auto eofLoc = whitespaceFx.sm.Resolve(whitespace[0].loc);
	CHECK(eofLoc.line == 2);
	CHECK(eofLoc.column == 2);
}

TEST(Lexer_IdentifiersAndKeywords) {
	Fixture fx;
	auto tokens = LexAll(fx, "if foo _bar x1 while I64 ifx");
	CHECK(Kinds(tokens) == (std::vector<TokenKind>{ TokenKind::KwIf, TokenKind::Identifier, TokenKind::Identifier,
		TokenKind::Identifier, TokenKind::KwWhile, TokenKind::Identifier, TokenKind::Identifier, TokenKind::Eof }));
	const std::vector<std::string_view> expected{ "if", "foo", "_bar", "x1", "while", "I64", "ifx", "" };
	for (size_t i = 0; i < expected.size(); ++i) {
		CHECK(tokens[i].text == expected[i]);
	}
	CHECK(tokens[0].atom.IsValid());
	CHECK(fx.interner.Text(tokens[0].atom) == "if");

	Fixture repeatedFx;
	auto repeated = LexAll(repeatedFx, "foo foo bar");
	CHECK(repeated[0].atom == repeated[1].atom);
	CHECK(repeated[0].atom != repeated[2].atom);
}

TEST(Lexer_EverySpellingLexesToItself) {
	const size_t punctuationBegin = static_cast<size_t>(TokenKind::StringLiteral) + 1;
	const size_t keywordMarkerBegin = static_cast<size_t>(TokenKind::KeywordMarkerBegin);
	const size_t keywordMarkerEnd = static_cast<size_t>(TokenKind::KeywordMarkerEnd);
	for (size_t i = punctuationBegin; i < keywordMarkerBegin; ++i) {
		TokenKind kind = static_cast<TokenKind>(i);
		Fixture fx;
		auto tokens = LexAll(fx, TokenKindSpelling(kind));
		CHECK(Kinds(tokens) == (std::vector<TokenKind>{ kind, TokenKind::Eof }));
		CHECK(tokens[0].text == TokenKindSpelling(kind));
	}
	for (size_t i = keywordMarkerBegin + 1; i < keywordMarkerEnd; ++i) {
		TokenKind kind = static_cast<TokenKind>(i);
		Fixture fx;
		auto tokens = LexAll(fx, TokenKindSpelling(kind));
		CHECK(Kinds(tokens) == (std::vector<TokenKind>{ kind, TokenKind::Eof }));
		CHECK(tokens[0].text == TokenKindSpelling(kind));
	}
}

TEST(Lexer_MaximalMunch) {
	Fixture fx;
	CHECK(Kinds(LexAll(fx, ">>= >> > <<= << <")) == (std::vector<TokenKind>{ TokenKind::GreaterGreaterEqual,
		TokenKind::GreaterGreater, TokenKind::Greater, TokenKind::LessLessEqual, TokenKind::LessLess, TokenKind::Less, TokenKind::Eof }));
	CHECK(Kinds(LexAll(fx, "a->b")) == (std::vector<TokenKind>{ TokenKind::Identifier, TokenKind::Arrow, TokenKind::Identifier, TokenKind::Eof }));
	CHECK(Kinds(LexAll(fx, "x+++y")) == (std::vector<TokenKind>{ TokenKind::Identifier, TokenKind::PlusPlus, TokenKind::Plus, TokenKind::Identifier, TokenKind::Eof }));
	CHECK(Kinds(LexAll(fx, "....")) == (std::vector<TokenKind>{ TokenKind::Ellipsis, TokenKind::Dot, TokenKind::Eof }));
	CHECK(Kinds(LexAll(fx, "&&&")) == (std::vector<TokenKind>{ TokenKind::AmpAmp, TokenKind::Amp, TokenKind::Eof }));
	CHECK(Kinds(LexAll(fx, "a-->b")) == (std::vector<TokenKind>{ TokenKind::Identifier, TokenKind::MinusMinus, TokenKind::Greater, TokenKind::Identifier, TokenKind::Eof }));
}

TEST(Lexer_Comments) {
	Fixture fx;
	auto tokens = LexAll(fx, "a // c1\nb /* x\ny */ c\n");
	CHECK(Kinds(tokens) == (std::vector<TokenKind>{ TokenKind::Identifier, TokenKind::Identifier, TokenKind::Identifier, TokenKind::Eof }));
	CHECK(tokens[0].text == "a");
	CHECK(tokens[1].text == "b");
	CHECK(tokens[2].text == "c");
	auto aLoc = fx.sm.Resolve(tokens[0].loc);
	auto bLoc = fx.sm.Resolve(tokens[1].loc);
	auto cLoc = fx.sm.Resolve(tokens[2].loc);
	auto eofLoc = fx.sm.Resolve(tokens[3].loc);
	CHECK(aLoc.line == 1 && aLoc.column == 1);
	CHECK(bLoc.line == 2 && bLoc.column == 1);
	CHECK(cLoc.line == 3 && cLoc.column == 6);
	CHECK(eofLoc.line == 4 && eofLoc.column == 1);
	CHECK(fx.diags.Count() == 0);

	Fixture lineCommentFx;
	CHECK(Kinds(LexAll(lineCommentFx, "a // x")) == (std::vector<TokenKind>{ TokenKind::Identifier, TokenKind::Eof }));
	CHECK(lineCommentFx.diags.Count() == 0);
	Fixture emptyBlockFx;
	CHECK(Kinds(LexAll(emptyBlockFx, "/**/")) == (std::vector<TokenKind>{ TokenKind::Eof }));
	CHECK(emptyBlockFx.diags.Count() == 0);
	Fixture slashInsideFx;
	auto slashInside = LexAll(slashInsideFx, "/*/ x */ y");
	CHECK(Kinds(slashInside) == (std::vector<TokenKind>{ TokenKind::Identifier, TokenKind::Eof }));
	CHECK(slashInside[0].text == "y");
	CHECK(slashInsideFx.diags.Count() == 0);
	Fixture lineInsideFx;
	auto lineInside = LexAll(lineInsideFx, "/* // */ a");
	CHECK(Kinds(lineInside) == (std::vector<TokenKind>{ TokenKind::Identifier, TokenKind::Eof }));
	CHECK(lineInside[0].text == "a");
	CHECK(lineInsideFx.diags.Count() == 0);
	Fixture lineCommentInsideFx;
	auto lineCommentInside = LexAll(lineCommentInsideFx, "// /* \n a");
	CHECK(Kinds(lineCommentInside) == (std::vector<TokenKind>{ TokenKind::Identifier, TokenKind::Eof }));
	CHECK(lineCommentInside[0].text == "a");
	CHECK(lineCommentInsideFx.diags.Count() == 0);

	Fixture nestedFx;
	auto nested = LexAll(nestedFx, "/* a /* b */ c */ d");
	if (Lexer::kBlockCommentsNest) {
		CHECK(Kinds(nested) == (std::vector<TokenKind>{ TokenKind::Identifier, TokenKind::Eof }));
		CHECK(nested[0].text == "d");
	}
	else {
		CHECK(Kinds(nested) == (std::vector<TokenKind>{ TokenKind::Identifier, TokenKind::Star, TokenKind::Slash,
			TokenKind::Identifier, TokenKind::Eof }));
		CHECK(nested[0].text == "c");
		CHECK(nested[3].text == "d");
	}
	CHECK(nestedFx.diags.Count() == 0);
}

TEST(Lexer_UnterminatedBlockComment) {
	Fixture fx;
	auto tokens = LexAll(fx, "a /* never closed");
	CHECK(Kinds(tokens) == (std::vector<TokenKind>{ TokenKind::Identifier, TokenKind::Eof }));
	CHECK(fx.diags.ErrorCount() == 1);
	CHECK(fx.diags.All()[0].message == "unterminated block comment");
	auto diagLoc = fx.sm.Resolve(fx.diags.All()[0].loc);
	CHECK(diagLoc.line == 1 && diagLoc.column == 3);
	CHECK(tokens[1].loc == fx.sm.LocFromOffset(fx.file, static_cast<uint32_t>(fx.sm.Contents(fx.file).size())));
}

TEST(Lexer_UnexpectedCharacters) {
	Fixture backslashFx;
	auto backslash = LexAll(backslashFx, "a \\ b");
	CHECK(Kinds(backslash) == (std::vector<TokenKind>{ TokenKind::Identifier, TokenKind::Unknown, TokenKind::Identifier, TokenKind::Eof }));
	CHECK(backslash[1].text == "\\");
	CHECK(backslashFx.diags.ErrorCount() == 1);
	CHECK(backslashFx.diags.All()[0].message == "unexpected character '\\'");

	Fixture controlFx;
	auto control = LexAll(controlFx, std::string_view("\x01", 1));
	CHECK(Kinds(control) == (std::vector<TokenKind>{ TokenKind::Unknown, TokenKind::Eof }));
	CHECK(controlFx.diags.All()[0].message == "unexpected byte 0x01");

	Fixture highByteFx;
	LexAll(highByteFx, std::string_view("\xFF", 1));
	CHECK(highByteFx.diags.All()[0].message == "unexpected byte 0xFF");

	Fixture nulFx;
	auto nul = LexAll(nulFx, std::string_view("a\0b", 3));
	CHECK(Kinds(nul) == (std::vector<TokenKind>{ TokenKind::Identifier, TokenKind::Unknown, TokenKind::Identifier, TokenKind::Eof }));
	CHECK(nulFx.diags.All()[0].message == "unexpected byte 0x00");
	CHECK(nulFx.diags.Count() == 1);
}

TEST(Lexer_LocationsAndText) {
	Fixture fx;
	auto tokens = LexAll(fx, "I64 x;\n foo->bar\n");
	CHECK(Kinds(tokens) == (std::vector<TokenKind>{ TokenKind::Identifier, TokenKind::Identifier, TokenKind::Semi,
		TokenKind::Identifier, TokenKind::Arrow, TokenKind::Identifier, TokenKind::Eof }));
	const uint32_t expectedLines[] = { 1, 1, 1, 2, 2, 2, 3 };
	const uint32_t expectedColumns[] = { 1, 5, 6, 2, 5, 7, 1 };
	const std::string_view expectedTexts[] = { "I64", "x", ";", "foo", "->", "bar", "" };
	auto contents = fx.sm.Contents(fx.file);
	for (size_t i = 0; i < tokens.size(); ++i) {
		auto loc = fx.sm.Resolve(tokens[i].loc);
		CHECK(loc.line == expectedLines[i] && loc.column == expectedColumns[i]);
		CHECK(tokens[i].text.size() == expectedTexts[i].size());
		if (tokens[i].kind != TokenKind::Eof) {
			CHECK(tokens[i].text.data() >= contents.data());
			CHECK(tokens[i].text.data() < contents.data() + contents.size());
		}
	}
}

TEST(Lexer_RealisticSnippet) {
	Fixture fx;
	const std::string_view source = "U0 Main()\n{\n I64 x;\n if (x) x++;\n else x--;\n}\n";
	auto tokens = LexAll(fx, source);
	CHECK(Kinds(tokens) == (std::vector<TokenKind>{ TokenKind::Identifier, TokenKind::Identifier, TokenKind::LParen,
		TokenKind::RParen, TokenKind::LBrace, TokenKind::Identifier, TokenKind::Identifier, TokenKind::Semi,
		TokenKind::KwIf, TokenKind::LParen, TokenKind::Identifier, TokenKind::RParen, TokenKind::Identifier,
		TokenKind::PlusPlus, TokenKind::Semi, TokenKind::KwElse, TokenKind::Identifier, TokenKind::MinusMinus,
		TokenKind::Semi, TokenKind::RBrace, TokenKind::Eof }));
	CHECK(fx.diags.Count() == 0);
}