#include <HolyTest/Core/TestHarness.h>
#include <HolyTest/Lexer/LexTestUtil.h>

using namespace holytest;
using namespace lextest;
using namespace holy::core;
using namespace holy::lex;

TEST(Literal_DecimalIntegers) {
	Fixture fx;
	std::vector<Token> tokens = LexAll(fx, "0 7 42 1234567890 18446744073709551615");
	CHECK(tokens.size() == 6);
	CHECK(tokens[0].kind == TokenKind::IntLiteral);
	CHECK(tokens[1].kind == TokenKind::IntLiteral);
	CHECK(tokens[2].kind == TokenKind::IntLiteral);
	CHECK(tokens[3].kind == TokenKind::IntLiteral);
	CHECK(tokens[4].kind == TokenKind::IntLiteral);
	CHECK(tokens[5].kind == TokenKind::Eof);
}

TEST(Literal_HexAndBinary) {
	Fixture fx;
	std::vector<Token> tokens = LexAll(fx, "0x0 0xFF 0xdeadBEEF 0X1f 0b0 0b1011 0xFFFFFFFFFFFFFFFF");
	CHECK(tokens[0].intValue == 0);
	CHECK(tokens[1].intValue == 255);
	CHECK(tokens[2].intValue == 0xdeadBEEF);
	CHECK(tokens[3].intValue == 0X1f);
	CHECK(tokens[4].intValue == 0b0);
	CHECK(tokens[5].intValue == 0b1011);
	CHECK(tokens[6].intValue == UINT64_MAX);
}

TEST(Literal_HolyCIntegerSuffix) {
	Fixture fx;
	std::vector<Token> tokens = LexAll(fx, "0x8000000000000000l 0xFFFFFFFFl");
	CHECK(tokens[0].kind == TokenKind::IntLiteral);
	CHECK(tokens[0].intValue == 0x8000000000000000ull);
	CHECK(tokens[1].kind == TokenKind::IntLiteral);
	CHECK(tokens[1].intValue == 0xFFFFFFFFull);
	CHECK(tokens[2].kind == TokenKind::Eof);
	CHECK(fx.diags.Count() == 0);
}

TEST(Literal_IntegerErrors) {
	Fixture fxCase1;
	std::vector<Token> tokens = LexAll(fxCase1, "18446744073709551616");
	CHECK(fxCase1.diags.Count() == 1);
	CHECK(fxCase1.diags.All()[0].message == "integer literal too large");
	CHECK(tokens[0].kind == TokenKind::IntLiteral);
	CHECK(tokens[0].intValue == 0);

	Fixture fxCase2;
	tokens = LexAll(fxCase2, "0x10000000000000000");
	CHECK(fxCase2.diags.Count() == 1);
	CHECK(fxCase2.diags.All()[0].message == "integer literal too large");
	CHECK(tokens[0].kind == TokenKind::IntLiteral);
	CHECK(tokens[0].intValue == 0);

	Fixture fxCase3;
	tokens = LexAll(fxCase3, "0x");
	CHECK(fxCase3.diags.Count() == 1);
	CHECK(fxCase3.diags.All()[0].message == "hexadecimal literal has no digits");
	CHECK(tokens[0].kind == TokenKind::IntLiteral);
	CHECK(tokens[0].text == "0x");

	Fixture fxCase4;
	tokens = LexAll(fxCase4, "0b");
	CHECK(fxCase4.diags.Count() == 1);
	CHECK(fxCase4.diags.All()[0].message == "binary literal has no digits");
	CHECK(tokens[0].kind == TokenKind::IntLiteral);

	Fixture fxCase5;
	tokens = LexAll(fxCase5, "0b102");
	CHECK(fxCase5.diags.Count() == 1);
	CHECK(fxCase5.diags.All()[0].message == "invalid digit '2' in binary literal");
	CHECK(tokens[0].kind == TokenKind::IntLiteral);
	CHECK(tokens[0].text == "0b102");

	Fixture fxCase6;
	tokens = LexAll(fxCase6, "0b10a");
	CHECK(fxCase6.diags.Count() == 1);
	CHECK(fxCase6.diags.All()[0].message == "invalid digit 'a' in binary literal");
	CHECK(tokens[0].kind == TokenKind::IntLiteral);
	CHECK(tokens[0].text == "0b10a");

	Fixture fxCase7;
	tokens = LexAll(fxCase7, "123abc");
	CHECK(fxCase7.diags.Count() == 1);
	CHECK(fxCase7.diags.All()[0].message == "invalid suffix 'abc' on numeric literal");
	CHECK(tokens[0].kind == TokenKind::IntLiteral);
	CHECK(tokens[0].intValue == 123);
	CHECK(tokens[0].text == "123abc");
	auto diagLoc = fxCase7.sm.Resolve(fxCase7.diags.All()[0].loc);
	CHECK(diagLoc.line == 1 && diagLoc.column == 4);
}

TEST(Literal_Floats) {
	Fixture fx;
	std::vector<Token> tokens = LexAll(fx, "1.5 0.25 3.0 1e3 1E3 2.5e-2 1.5e+2 .5");
	CHECK(tokens[0].kind == TokenKind::FloatLiteral);
	CHECK(tokens[0].floatValue == 1.5f);
	CHECK(tokens[1].kind == TokenKind::FloatLiteral);
	CHECK(tokens[1].floatValue == 0.25f);
	CHECK(tokens[2].kind == TokenKind::FloatLiteral);
	CHECK(tokens[2].floatValue == 3.0f);
	CHECK(tokens[3].kind == TokenKind::FloatLiteral);
	CHECK(tokens[3].floatValue == 1e3f);
	CHECK(tokens[4].kind == TokenKind::FloatLiteral);
	CHECK(tokens[4].floatValue == 1E3f);
	CHECK(tokens[5].kind == TokenKind::FloatLiteral);
	CHECK(tokens[5].floatValue == 2.5e-2);
	CHECK(tokens[6].kind == TokenKind::FloatLiteral);
	CHECK(tokens[6].floatValue == 1.5e+2f);
	CHECK(tokens[7].kind == TokenKind::FloatLiteral);
	CHECK(tokens[7].floatValue == .5f);
	CHECK(tokens[8].kind == TokenKind::Eof);
}

TEST(Literal_FloatExponentErrors) {
	for (const char* input : { "1e", "1e+", "1e-" }) {
		Fixture fx;
		std::vector<Token> tokens = LexAll(fx, input);
		CHECK(fx.diags.Count() == 1);
		CHECK(fx.diags.All()[0].message == "exponent has no digits");
		CHECK(tokens[0].kind == TokenKind::IntLiteral);
		CHECK(tokens[0].text == input);
		CHECK(tokens[1].kind == TokenKind::Eof);
	}
}

TEST(Literal_DotAndRangeInteractions) {
	Fixture fx;
	std::vector<Token> tokens = LexAll(fx, "1...5");
	CHECK(tokens[0].kind == TokenKind::IntLiteral);
	CHECK(tokens[1].kind == TokenKind::Ellipsis);
	CHECK(tokens[2].kind == TokenKind::IntLiteral);
	CHECK(tokens[3].kind == TokenKind::Eof);
	tokens = LexAll(fx, "1.foo");
	CHECK(tokens[0].kind == TokenKind::IntLiteral);
	CHECK(tokens[1].kind == TokenKind::Dot);
	CHECK(tokens[2].kind == TokenKind::Identifier);
	CHECK(tokens[3].kind == TokenKind::Eof);
	tokens = LexAll(fx, "a.b");
	CHECK(tokens[0].kind == TokenKind::Identifier);
	CHECK(tokens[1].kind == TokenKind::Dot);
	CHECK(tokens[2].kind == TokenKind::Identifier);
	CHECK(tokens[3].kind == TokenKind::Eof);
	tokens = LexAll(fx, "1 . 5");
	CHECK(tokens[0].kind == TokenKind::IntLiteral);
	CHECK(tokens[1].kind == TokenKind::Dot);
	CHECK(tokens[2].kind == TokenKind::IntLiteral);
	CHECK(tokens[3].kind == TokenKind::Eof);
}

TEST(Literal_FloatErrors) {
	Fixture fxCase1;
	std::vector<Token> tokens = LexAll(fxCase1, "1e999");
	CHECK(tokens[0].kind == TokenKind::FloatLiteral);
	CHECK(fxCase1.diags.All()[0].message == "floating-point literal out of range");
	Fixture fxCase2;
	tokens = LexAll(fxCase2, "1e");
	CHECK(tokens[0].kind == TokenKind::IntLiteral);
	CHECK(fxCase2.diags.All()[0].message == "exponent has no digits");
	Fixture fxCase3;
	tokens = LexAll(fxCase3, "1.5f");
	CHECK(tokens[0].kind == TokenKind::FloatLiteral);
	CHECK(fxCase3.diags.All()[0].message == "invalid suffix 'f' on numeric literal");
}

TEST(Literal_CharConstants) {
	Fixture fx;
	std::vector<Token> tokens = LexAll(fx, R"lit('A' 'ab' 'ABCD' '\n' '\'' '\\' '\x41' 'ABCDEFGH')lit");
	CHECK(tokens[0].kind == TokenKind::CharLiteral);
	CHECK(tokens[0].intValue == 65);
	CHECK(tokens[0].text == "'A'");
	CHECK(tokens[1].kind == TokenKind::CharLiteral);
	CHECK(tokens[1].intValue == 0x6261);
	CHECK(tokens[1].text == "'ab'");
	CHECK(tokens[2].kind == TokenKind::CharLiteral);
	CHECK(tokens[2].intValue == 0x44434241);
	CHECK(tokens[2].text == "'ABCD'");
	CHECK(tokens[3].kind == TokenKind::CharLiteral);
	CHECK(tokens[3].intValue == 10);
	CHECK(tokens[3].text == "'\\n'");
	CHECK(tokens[4].kind == TokenKind::CharLiteral);
	CHECK(tokens[4].intValue == 39);
	CHECK(tokens[4].text == "'\\''");
	CHECK(tokens[5].kind == TokenKind::CharLiteral);
	CHECK(tokens[5].intValue == 92);
	CHECK(tokens[5].text == "'\\\\'");
	CHECK(tokens[6].kind == TokenKind::CharLiteral);
	CHECK(tokens[6].intValue == 65);
	CHECK(tokens[6].text == "'\\x41'");
	CHECK(tokens[7].kind == TokenKind::CharLiteral);
	CHECK(tokens[7].intValue == 0x4847464544434241);
	CHECK(tokens[7].text == "'ABCDEFGH'");
	CHECK(tokens[8].kind == TokenKind::Eof);
	CHECK(fx.diags.Count() == 0);
}

TEST(Literal_CharErrors) {
	Fixture fxCase1;
	std::vector<Token> tokens = LexAll(fxCase1, "''");
	CHECK(tokens[0].kind == TokenKind::CharLiteral);
	CHECK(tokens[0].intValue == 0);
	CHECK(fxCase1.diags.Count() == 0);
	Fixture fxCase2;
	tokens = LexAll(fxCase2, "'ABCDEFGHI'");
	CHECK(tokens[0].kind == TokenKind::CharLiteral);
	CHECK(tokens[0].intValue == 0x4847464544434241);
	CHECK(fxCase2.diags.All()[0].message == "character constant is too long");
	Fixture fxCase3;
	tokens = LexAll(fxCase3, "'A\nx");
	CHECK(tokens[0].kind == TokenKind::Unknown);
	CHECK(tokens[0].text == "'A");
	CHECK(tokens[1].kind == TokenKind::Identifier);
	CHECK(fxCase3.diags.All()[0].message == "unterminated character constant");
	Fixture fxCase4;
	tokens = LexAll(fxCase4, "'A");
	CHECK(tokens[0].kind == TokenKind::Unknown);
	CHECK(tokens[1].kind == TokenKind::Eof);
}

TEST(Literal_Strings) {
	Fixture fx;
	std::vector<Token> tokens = LexAll(fx, R"lit("" "abc" "a\tb\n" "q\"q" "\x41\x4a" "nul\0end")lit");
	CHECK(tokens[0].kind == TokenKind::StringLiteral);
	CHECK(tokens[0].stringValue.compare("") == 0);
	CHECK(tokens[0].stringValue.data()[tokens[0].stringValue.size()] == '\0');
	CHECK(fx.arena.Owns(tokens[0].stringValue.data()));
	CHECK(tokens[1].kind == TokenKind::StringLiteral);
	CHECK(tokens[1].stringValue.compare("abc") == 0);
	CHECK(tokens[1].stringValue.data()[tokens[1].stringValue.size()] == '\0');
	CHECK(fx.arena.Owns(tokens[1].stringValue.data()));
	CHECK(tokens[2].kind == TokenKind::StringLiteral);
	CHECK(tokens[2].stringValue.compare("a\tb\n") == 0);
	CHECK(tokens[2].stringValue.data()[tokens[2].stringValue.size()] == '\0');
	CHECK(fx.arena.Owns(tokens[2].stringValue.data()));
	CHECK(tokens[3].kind == TokenKind::StringLiteral);
	CHECK(tokens[3].stringValue.compare("q\"q") == 0);
	CHECK(tokens[3].stringValue.data()[tokens[3].stringValue.size()] == '\0');
	CHECK(fx.arena.Owns(tokens[3].stringValue.data()));
	CHECK(tokens[4].kind == TokenKind::StringLiteral);
	CHECK(tokens[4].stringValue.compare("\x41\x4a") == 0);
	CHECK(tokens[4].stringValue.data()[tokens[4].stringValue.size()] == '\0');
	CHECK(fx.arena.Owns(tokens[4].stringValue.data()));
	CHECK(tokens[5].kind == TokenKind::StringLiteral);
	CHECK(tokens[5].stringValue.compare(std::string_view("nul\0end", 7)) == 0);
	CHECK(tokens[5].stringValue.data()[tokens[5].stringValue.size()] == '\0');
	CHECK(fx.arena.Owns(tokens[5].stringValue.data()));
}

TEST(Literal_StringSpansLines) {
	Fixture fx;
	std::vector<Token> tokens = LexAll(fx, "\"line1\n\line2\"");
	CHECK(tokens[0].kind == TokenKind::StringLiteral);
	CHECK(tokens[0].stringValue.compare("line1\n\line2") == 0);
	CHECK(tokens[1].kind == TokenKind::Eof);
}

TEST(Literal_StringErrors) {
	Fixture fx;
	std::vector<Token> tokens = LexAll(fx, R"lit("abc)lit");
	CHECK(tokens[0].kind == TokenKind::Unknown);
	CHECK(tokens[0].text.compare("\"abc") == 0);
	CHECK(tokens[1].kind == TokenKind::Eof);
}

TEST(Literal_Mixed) {
	Fixture fx;
	std::vector<Token> tokens = LexAll(fx, "U8 *s = \"hi\"; I64 n = 0x10 + 3;");
	CHECK(tokens[0].kind == TokenKind::Identifier);
	CHECK(tokens[1].kind == TokenKind::Star);
	CHECK(tokens[2].kind == TokenKind::Identifier);
	CHECK(tokens[3].kind == TokenKind::Equal);
	CHECK(tokens[4].kind == TokenKind::StringLiteral);
	CHECK(tokens[4].stringValue.compare("hi") == 0);
	CHECK(tokens[4].text.compare("\"hi\"") == 0);
	CHECK(tokens[5].kind == TokenKind::Semi);
	CHECK(tokens[6].kind == TokenKind::Identifier);
	CHECK(tokens[7].kind == TokenKind::Identifier);
	CHECK(tokens[8].kind == TokenKind::Equal);
	CHECK(tokens[9].kind == TokenKind::IntLiteral);
	CHECK(tokens[9].intValue == 16);
	CHECK(tokens[10].kind == TokenKind::Plus);
	CHECK(tokens[11].kind == TokenKind::IntLiteral);
	CHECK(tokens[11].intValue == 3);
	CHECK(tokens[12].kind == TokenKind::Semi);
	CHECK(tokens[13].kind == TokenKind::Eof);
}