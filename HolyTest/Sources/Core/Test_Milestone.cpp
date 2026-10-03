#include <HolyTest/Core/TestHarness.h>

#include <HolyTest/Core/Golden.h>
#include <HolyCore/Diagnostics.h>
#include <HolyCore/Platform.h>
#include <HolyCore/SourceManager.h>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

using namespace holy::core;

namespace {
	SourceLoc LocAt(const SourceManager& sm, FileId f, std::string_view needle) {
		size_t pos = sm.Contents(f).find(needle);
		CHECK(pos != std::string_view::npos);
		if (pos == std::string_view::npos) {
			return SourceLoc{};
		}
		return sm.LocFromOffset(f, static_cast<uint32_t>(pos));
	}

	SourceLoc LocAfter(const SourceManager& sm, FileId f, std::string_view needle) {
		size_t pos = sm.Contents(f).find(needle);
		CHECK(pos != std::string_view::npos);
		if (pos == std::string_view::npos) {
			return SourceLoc{};
		}
		pos += needle.size();
		return sm.LocFromOffset(f, static_cast<uint32_t>(pos));
	}

	void ReportBadHc(const SourceManager& sm, FileId f, DiagEngine& de) {
		de.Error(LocAfter(sm, f, "I64 x = "), "expected expression before ';'");
		de.Error(LocAfter(sm, f, "Print(\"hi\\n\")"), "expected ';' after expression");
		de.Warning(LocAt(sm, f, "x = "), "unused variable 'x'");
		de.Error(LocAt(sm, f, "y = 5"), "undeclared identifier 'y'");
	}
}

TEST(Milestone_BadHC_Positions) {
	Arena a(1 << 20);
	SourceManager sm(a);
	FileId f;
	CHECK(sm.LoadFile("Sources/Core/Bad.HC", f) == FileStatus::Ok);
	if (!f.IsValid()) {
		return;
	}

	DiagEngine de(a, sm);
	ReportBadHc(sm, f, de);

	const SourceLoc loc1 = LocAfter(sm, f, "I64 x = ");
	const SourceLoc loc2 = LocAfter(sm, f, "Print(\"hi\\n\")");
	const SourceLoc loc3 = LocAt(sm, f, "x = ");
	const SourceLoc loc4 = LocAt(sm, f, "y = 5");

	const PresumedLoc presumed1 = sm.Resolve(loc1);
	const PresumedLoc presumed2 = sm.Resolve(loc2);
	const PresumedLoc presumed3 = sm.Resolve(loc3);
	const PresumedLoc presumed4 = sm.Resolve(loc4);
	CHECK(presumed1.filename == "Sources/Core/Bad.HC");
	CHECK(presumed1.line == 3u);
	CHECK(presumed1.column == 11u);
	CHECK(presumed2.filename == "Sources/Core/Bad.HC");
	CHECK(presumed2.line == 4u);
	CHECK(presumed2.column == 16u);
	CHECK(presumed3.filename == "Sources/Core/Bad.HC");
	CHECK(presumed3.line == 3u);
	CHECK(presumed3.column == 7u);
	CHECK(presumed4.filename == "Sources/Core/Bad.HC");
	CHECK(presumed4.line == 5u);
	CHECK(presumed4.column == 3u);
	CHECK(sm.LineText(loc1) == "  I64 x = ;");
}

TEST(Milestone_BadHC_Golden) {
	Arena a(1 << 20);
	SourceManager sm(a);
	FileId f;
	CHECK(sm.LoadFile("Sources/Core/Bad.HC", f) == FileStatus::Ok);
	if (!f.IsValid()) {
		return;
	}

	DiagEngine de(a, sm);
	ReportBadHc(sm, f, de);

	CHECK(de.Count() == 4);
	CHECK(de.ErrorCount() == 3);
	CHECK(de.WarningCount() == 1);
	CHECK(holytest::CheckGolden("Sources/Core/Bad.diag.golden", de.FormatAll()));
}

TEST(Milestone_GoldenFilesUseLf) {
	std::string badHC;
	FileStatus badHCStatus = ReadWholeFile("Sources/Core/Bad.HC", badHC);
	CHECK(badHCStatus == FileStatus::Ok);

	std::string badDiagHC;
	FileStatus badDiagStatus = ReadWholeFile("Sources/Core/Bad.diag.golden", badDiagHC);
	CHECK(badDiagStatus == FileStatus::Ok);

	for (size_t i = 0; i < badHC.size(); i++) {
		CHECK(badHC.at(i) != '\r');
		CHECK(badHC.at(i) != '\t');
	}

	for (size_t i = 0; i < badDiagHC.size(); i++) {
		CHECK(badDiagHC.at(i) != '\r');
		CHECK(badDiagHC.at(i) != '\t');
	}

	CHECK(badDiagHC.ends_with("^\n"));
}

TEST(Golden_SelfTest) {
	const std::string path = "holy_golden_selftest.tmp";
	const std::string actualPath = path + ".actual";
	std::remove(path.c_str());
	std::remove(actualPath.c_str());

	CHECK(!std::ifstream(path));
	CHECK(!holytest::CheckGolden(path, "abc\n"));

	std::string actualContents;
	{
		std::ifstream actualFile(actualPath);
		CHECK(actualFile);
		actualContents.assign(std::istreambuf_iterator<char>(actualFile), std::istreambuf_iterator<char>());
	}
	CHECK(actualContents == "abc\n");

	{
		std::ofstream goldenFile(path, std::ios::binary);
		CHECK(goldenFile);
		goldenFile << "abc\n";
	}

	CHECK(!holytest::CheckGolden(path, "abd\n"));
	actualContents.clear();
	{
		std::ifstream actualFile(actualPath);
		CHECK(actualFile);
		actualContents.assign(std::istreambuf_iterator<char>(actualFile), std::istreambuf_iterator<char>());
	}
	CHECK(actualContents == "abd\n");

	CHECK(holytest::CheckGolden(path, "abc\n"));
	{
		std::ifstream actualFile(actualPath);
		CHECK(!actualFile);
	}

	std::remove(path.c_str());
	std::remove(actualPath.c_str());
}
