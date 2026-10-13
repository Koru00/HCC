#include <HolyTest/Core/TestHarness.h>
#include <HolyTest/Lexer/LexTestUtil.h>
#include <HolyLex/Lexer.h>
#include <string_view>
#include <vector>

using namespace holy::core;
using namespace holy::lex;
using namespace lextest;

TEST(Directive_Basics) {
	Fixture fx;
	std::vector<Token> tokens = LexAll(fx, "#include \"foo.HC\"\n#define MAX 10\nx");
	CHECK(tokens[0].kind == TokenKind::Directive);
	CHECK(tokens[0].startLine == true);
	CHECK(fx.interner.Text(tokens[0].atom) == "include");
	CHECK(tokens[1].kind == TokenKind::StringLiteral);
	CHECK(tokens[1].startLine == false);
	CHECK(tokens[2].kind == TokenKind::Directive);
	CHECK(fx.interner.Text(tokens[2].atom) == "define");
	CHECK(tokens[2].startLine == true);
	CHECK(tokens[3].kind == TokenKind::Identifier);
	CHECK(tokens[3].startLine == false);
	CHECK(tokens[4].kind == TokenKind::IntLiteral);
	CHECK(tokens[4].startLine == false);
	CHECK(tokens[5].kind == TokenKind::Identifier);
	CHECK(tokens[5].startLine == true);
	CHECK(tokens[6].kind == TokenKind::Eof);
}

TEST(Directive_NotAtLineStart) {
	Fixture fx;
	std::vector<Token> tokens = LexAll(fx, "I64 x; #exe {}");
	CHECK(tokens[0].kind == TokenKind::Identifier);
	CHECK(tokens[1].kind == TokenKind::Identifier);
	CHECK(tokens[2].kind == TokenKind::Semi);
	CHECK(tokens[3].kind == TokenKind::Directive);
	CHECK(tokens[3].startLine == false);
	CHECK(fx.interner.Text(tokens[3].atom) == "exe");
	CHECK(tokens[4].kind == TokenKind::LBrace);
	CHECK(tokens[5].kind == TokenKind::RBrace);
	CHECK(tokens[6].kind == TokenKind::Eof);
}

TEST(Directive_Errors) {
	Fixture fxCase1;
	std::vector<Token> tokens = LexAll(fxCase1, "# include");
	CHECK(tokens[0].kind == TokenKind::Unknown);
	CHECK(tokens[0].text == "#");
	CHECK(tokens[1].kind == TokenKind::Identifier);
	CHECK(tokens[2].kind == TokenKind::Eof);
	CHECK(fxCase1.diags.All()[0].message == "expected directive name after '#'");
	Fixture fxCase2;
	tokens = LexAll(fxCase2, "#");
	CHECK(tokens[0].kind == TokenKind::Unknown);
	CHECK(tokens[1].kind == TokenKind::Eof);
	Fixture fxCase3;
	tokens = LexAll(fxCase3, "#1");
	CHECK(tokens[0].kind == TokenKind::Unknown);
	CHECK(tokens[1].kind == TokenKind::IntLiteral);
	CHECK(tokens[2].kind == TokenKind::Eof);
}

TEST(StartsLine_Flags) {
	Fixture fx;
	std::vector<Token> tokens = LexAll(fx, "a b\n c\n/* x\n y */ d // z\n\n e");
	CHECK(tokens[0].startLine == true);
	CHECK(tokens[1].startLine == false);
	CHECK(tokens[2].startLine == true);
	CHECK(tokens[3].startLine == true);
	CHECK(tokens[4].startLine == true);
	
	tokens = LexAll(fx, "x /*\n*/ y");
	CHECK(tokens[0].startLine == true);
	CHECK(tokens[1].startLine == false);
	
	tokens = LexAll(fx, "\"a\nb\" c");
	CHECK(tokens[0].startLine == true);
	CHECK(tokens[1].startLine == false);
}

TEST(DolDoc_Recognition) {
	Fixture fxCase1;
	std::vector<Token> tokens = LexAll(fxCase1, "\"plain\"");
	CHECK(tokens[0].hasDolDoc == false);
	
	Fixture fxCase2;
	tokens = LexAll(fxCase2, "\"$FG,4$red$FG$\"");
	CHECK(tokens[0].hasDolDoc == true);

	Fixture fxCase3;
	tokens = LexAll(fxCase3, "\"cost: $$5\"");
	CHECK(tokens[0].hasDolDoc == false);
	CHECK(tokens[0].stringValue == "cost: $$5");

	Fixture fxCase4;
	tokens = LexAll(fxCase4, "\"$$$FG,2$x$$\"");
	CHECK(tokens[0].hasDolDoc == true);

	Fixture fxCase5;
	tokens = LexAll(fxCase5, R"lit("$TX, \"hi\", A = \"x\"$")lit");
	CHECK(tokens[0].hasDolDoc == true);

	CHECK(tokens[0].stringValue == "$TX, \"hi\", A = \"x\"$");

	Fixture fxCase6;
	tokens = LexAll(fxCase6, "\"a $FG,4 b\"");
	CHECK(tokens[0].hasDolDoc == false);
	CHECK(fxCase6.diags.WarningCount() == 1);
	CHECK(fxCase6.diags.ErrorCount() == 0);
	CHECK(fxCase6.diags.All()[0].message == "unterminated DolDoc code in string");
}