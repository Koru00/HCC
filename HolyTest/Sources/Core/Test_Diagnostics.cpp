#include <HolyTest/Core/TestHarness.h>

#include <HolyCore/Diagnostics.h>

#include <string>
#include <string_view>

using namespace holy::core;

TEST(Diag_FreshState) {
	Arena a(1 << 20);
	SourceManager sm(a);
	DiagEngine diagEngine(a, sm);

	CHECK(diagEngine.Count() == 0);
	CHECK(diagEngine.ErrorCount() == 0);
	CHECK(diagEngine.WarningCount() == 0);
	CHECK(!diagEngine.HasErrors());
	CHECK(diagEngine.All().Empty());
	CHECK(diagEngine.FormatAll().empty());
}

TEST(Diag_CountsBySeverity) {
	Arena a(1 << 20);
	SourceManager sm(a);
	DiagEngine diagEngine(a, sm);

	diagEngine.Error(SourceLoc{}, "");
	diagEngine.Error(SourceLoc{}, "");

	diagEngine.Warning(SourceLoc{}, "");

	diagEngine.Note(SourceLoc{}, "");

	CHECK(diagEngine.ErrorCount() == 2);
	CHECK(diagEngine.WarningCount() == 1);
	CHECK(diagEngine.Count() == 4);
	CHECK(diagEngine.HasErrors());
}

TEST(Diag_StoresInOrderAndCopiesMessage) {
	Arena a(1 << 20);
	SourceManager sm(a);
	DiagEngine diagEngine(a, sm);
	FileId f = sm.AddBuffer("Order.HC", "x\n");
	SourceLoc loc = sm.LocFromOffset(f, 0);

	{
		std::string msg = "boom";
		diagEngine.Error(loc, msg);
		msg = "changed";
	}
	diagEngine.Warning(loc, "second");

	CHECK(diagEngine.All().Size() == 2);
	CHECK(diagEngine.All()[0].severity == Severity::Error);
	CHECK(diagEngine.All()[0].loc == loc);
	CHECK(diagEngine.All()[0].message == "boom");
	CHECK(diagEngine.All()[1].message == "second");
	CHECK(a.Owns(diagEngine.All()[0].message.data()));
}

TEST(Diag_FormatWithCaret) {
	Arena a(1 << 20);
	SourceManager sm(a);
	DiagEngine diagEngine(a, sm);
	FileId f = sm.AddBuffer("Bad.HC", "U0 Main( {\n bad x;\n");
	diagEngine.Error(sm.LocFromOffset(f, 17), "unexpected identifier");

	CHECK(diagEngine.Format(diagEngine.All()[0]) ==
		"Bad.HC:2:7: error: unexpected identifier\n bad x;\n      ^");
}

TEST(Diag_CaretKeepsTabs) {
	Arena a(1 << 20);
	SourceManager sm(a);
	DiagEngine diagEngine(a, sm);
	FileId f = sm.AddBuffer("T.HC", "\tx\n");
	diagEngine.Warning(sm.LocFromOffset(f, 1), "w");

	CHECK(diagEngine.Format(diagEngine.All()[0]) ==
		"T.HC:1:2: warning: w\n\tx\n\t^");
}

TEST(Diag_InvalidLocNoPosition) {
	Arena a(1 << 20);
	SourceManager sm(a);
	DiagEngine diagEngine(a, sm);
	diagEngine.Error(SourceLoc{}, "global problem");

	CHECK(diagEngine.Format(diagEngine.All()[0]) == "error: global problem");
}

TEST(Diag_EmptyLineOmitsSource) {
	Arena a(1 << 20);
	SourceManager sm(a);
	DiagEngine diagEngine(a, sm);
	FileId f = sm.AddBuffer("E.HC", "ab\n");
	diagEngine.Error(sm.LocFromOffset(f, 3), "unexpected end of file");

	CHECK(diagEngine.Format(diagEngine.All()[0]) ==
		"E.HC:2:1: error: unexpected end of file");
}

TEST(Diag_CrLfLine) {
	Arena a(1 << 20);
	SourceManager sm(a);
	DiagEngine diagEngine(a, sm);
	FileId f = sm.AddBuffer("C.HC", "x y\r\n");
	diagEngine.Error(sm.LocFromOffset(f, 2), "m");
	const std::string formatted = diagEngine.Format(diagEngine.All()[0]);

	CHECK(formatted == "C.HC:1:3: error: m\nx y\n ^");
	CHECK(formatted.find('\r') == std::string::npos);
}

TEST(Diag_FormatAllAndNotes) {
	Arena a(1 << 20);
	SourceManager sm(a);
	DiagEngine diagEngine(a, sm);
	FileId f = sm.AddBuffer("N.HC", "a b\n");
	diagEngine.Error(sm.LocFromOffset(f, 0), "first");
	diagEngine.Note(sm.LocFromOffset(f, 2), "see here");

	CHECK(diagEngine.FormatAll() ==
		"N.HC:1:1: error: first\na b\n^\n"
		"N.HC:1:3: note: see here\na b\n ^\n");
	CHECK(diagEngine.ErrorCount() == 1);
	CHECK(diagEngine.Count() == 2);
}

TEST(Diag_Clear) {
	Arena a(1 << 20);
	SourceManager sm(a);
	DiagEngine diagEngine(a, sm);
	diagEngine.Error(SourceLoc{}, "error");
	diagEngine.Warning(SourceLoc{}, "warning");
	diagEngine.Clear();

	CHECK(diagEngine.Count() == 0);
	CHECK(diagEngine.ErrorCount() == 0);
	CHECK(diagEngine.WarningCount() == 0);
	CHECK(!diagEngine.HasErrors());

	diagEngine.Error(SourceLoc{}, "again");
	CHECK(diagEngine.Count() == 1);
}