#include <HolyTest/Core/TestHarness.h>

#include <HolyTest/Core/Golden.h>
#include <HolyTest/Lexer/TokenDump.h>
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

using namespace lextest;

TEST(Dump_Escape) {
	CHECK(EscapeForDump(std::string_view("a\"b\\c\n\t\x01\xFF", 9)) == R"(a\"b\\c\n\t\x01\xFF)");
	CHECK(EscapeForDump("") == "");
}

TEST(Dump_Format) {
	DumpResult result = LexAndDumpBuffer("t.HC", R"lit(x 42 'A' 1.5 "a\nb")lit", true);
	CHECK(result.text == "1:1^ Identifier \"x\"\n1:3 IntLiteral 42\n1:6 CharLiteral 0x41\n1:10 FloatLiteral 1.5\n1:14 StringLiteral \"a\nb\"\n1:20 Eof\n");
}

TEST(Dump_AsmHook) {
	DumpResult result = LexAndDumpBuffer("t.HC", "asm { RET } x", true);
	CHECK(result.text == "1:1^ KwAsm\n1:5 LBrace\n1:6 RawBlock \" RET \"\n1:11 RBrace\n1:13 Identifier \"x\"\n1:14 Eof\n");
}

TEST(Lexer_Goldens) {
	const std::filesystem::path lexerDir = "Sources/Lexer";
	std::vector<std::filesystem::path> files;
	for (const auto& entry : std::filesystem::directory_iterator(lexerDir)) {
		if (entry.is_regular_file() && entry.path().extension() == ".HC") {
			files.push_back(entry.path());
		}
	}
	std::sort(files.begin(), files.end());
	CHECK(files.size() >= 6);

	for (const auto& path : files) {
		DumpResult result = LexAndDumpFile(path.generic_string(), true);
		CHECK(result.status == FileStatus::Ok);
		std::filesystem::path goldenPath = path;
		goldenPath.replace_extension(".tokens.golden");
		CHECK(holytest::CheckGolden(goldenPath.generic_string(), result.text));
	}
}