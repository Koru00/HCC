#include <HolyTest/Core/TestHarness.h>
#include <HolyCore/SourceManager.h>
#include <cstdio>
#include <fstream>
#include <string>
#include <string_view>

using namespace holy::core;

TEST(Source_AddBufferBasics) {
    Arena arena;
    SourceManager sm(arena);

    FileId f = sm.AddBuffer("a.HC", "ab\ncd\n");

    CHECK(f.IsValid());
    CHECK(sm.FileName(f) == "a.HC");
    CHECK(sm.Contents(f) == "ab\ncd\n");
    CHECK(sm.Contents(f).data()[sm.Contents(f).size()] == '\0');
    CHECK(sm.FileCount() == 1);
}

TEST(Source_ResolveLines) {
    Arena arena;
    SourceManager sm(arena);

    FileId f = sm.AddBuffer("a.HC", "ab\ncd\n");

    auto c0 = sm.Resolve(sm.LocFromOffset(f, 0));
    CHECK(c0.IsValid());
    CHECK(c0.filename == "a.HC");
    CHECK(c0.line == 1);
    CHECK(c0.column == 1);

    auto c1 = sm.Resolve(sm.LocFromOffset(f, 1));
    CHECK(c1.IsValid());
    CHECK(c1.filename == "a.HC");
    CHECK(c1.line == 1);
    CHECK(c1.column == 2);

    auto c2 = sm.Resolve(sm.LocFromOffset(f, 2));
    CHECK(c2.IsValid());
    CHECK(c2.filename == "a.HC");
    CHECK(c2.line == 1);
    CHECK(c2.column == 3);

    auto c3 = sm.Resolve(sm.LocFromOffset(f, 3));
    CHECK(c3.IsValid());
    CHECK(c3.filename == "a.HC");
    CHECK(c3.line == 2);
    CHECK(c3.column == 1);

    auto c4 = sm.Resolve(sm.LocFromOffset(f, 4));
    CHECK(c4.IsValid());
    CHECK(c4.filename == "a.HC");
    CHECK(c4.line == 2);
    CHECK(c4.column == 2);

    auto c6 = sm.Resolve(sm.LocFromOffset(f, 6));
    CHECK(c6.IsValid());
    CHECK(c6.filename == "a.HC");
    CHECK(c6.line == 3);
    CHECK(c6.column == 1);
}

TEST(Source_LineText) {
    Arena arena;
    SourceManager sm(arena);

    FileId f = sm.AddBuffer("a.HC", "ab\ncd\n");

    CHECK(sm.LineText(sm.LocFromOffset(f, 0)) == "ab");
    CHECK(sm.LineText(sm.LocFromOffset(f, 3)) == "cd");
    CHECK(sm.LineText(sm.LocFromOffset(f, 6)) == "");
}

TEST(Source_CrLf) {
    Arena arena;
    SourceManager sm(arena);

    FileId f = sm.AddBuffer("crlf.HC", "x\r\ny");

    CHECK(sm.LineText(sm.LocFromOffset(f, 0)) == "x");
    CHECK(sm.LineText(sm.LocFromOffset(f, 3)) == "y");

    auto p3 = sm.Resolve(sm.LocFromOffset(f, 3));
    CHECK(p3.line == 2);
    CHECK(p3.column == 1);

    auto p1 = sm.Resolve(sm.LocFromOffset(f, 1));
    CHECK(p1.line == 1);
    CHECK(p1.column == 2);
}

TEST(Source_NoTrailingNewlineAndEmpty) {
    Arena arena;
    SourceManager sm(arena);

    FileId f1 = sm.AddBuffer("a.HC", "abc");

    auto eof1 = sm.Resolve(sm.LocFromOffset(f1, 3));
    CHECK(eof1.IsValid());
    CHECK(eof1.line == 1);
    CHECK(eof1.column == 4);

    CHECK(sm.LineText(sm.LocFromOffset(f1, 3)) == "abc");

    FileId f2 = sm.AddBuffer("empty.HC", "");

    auto emptyLoc = sm.Resolve(sm.LocFromOffset(f2, 0));
    CHECK(emptyLoc.IsValid());
    CHECK(emptyLoc.line == 1);
    CHECK(emptyLoc.column == 1);

    CHECK(sm.LineText(sm.LocFromOffset(f2, 0)) == "");
    CHECK(sm.Contents(f2).empty());
}

TEST(Source_MultiFile) {
    Arena arena;
    SourceManager sm(arena);

    FileId fa = sm.AddBuffer("a.HC", "aa\n");
    FileId fb = sm.AddBuffer("b.HC", "bbbb\nb");

    CHECK(
        sm.StartOfFile(fb).offset ==
        sm.LocFromOffset(fa, 3).offset + 1);

    CHECK(
        sm.FileOf(sm.LocFromOffset(fa, 3)) == fa);

    CHECK(
        sm.FileOf(sm.StartOfFile(fb)) == fb);

    auto b = sm.Resolve(sm.LocFromOffset(fb, 5));
    CHECK(b.IsValid());
    CHECK(b.filename == "b.HC");
    CHECK(b.line == 2);
    CHECK(b.column == 1);

    auto a = sm.Resolve(sm.LocFromOffset(fa, 0));
    CHECK(a.IsValid());
    CHECK(a.filename == "a.HC");
    CHECK(a.line == 1);
    CHECK(a.column == 1);
}

TEST(Source_InvalidLocs) {
    Arena arena;
    SourceManager sm(arena);

    CHECK(!SourceLoc{}.IsValid());

    auto r0 = sm.Resolve(SourceLoc{});
    CHECK(!r0.IsValid());
    CHECK(r0.filename.empty());

    CHECK(!sm.FileOf(SourceLoc{}).IsValid());
    CHECK(sm.LineText(SourceLoc{}).empty());

    sm.AddBuffer("a.HC", "abc");

    SourceLoc bogus{ 9999 };

    CHECK(!sm.FileOf(bogus).IsValid());
    CHECK(!sm.Resolve(bogus).IsValid());
}

TEST(Source_RawBytes) {
    Arena arena;
    SourceManager sm(arena);

    std::string_view bytes("a\0b\n\xFF", 5);

    FileId f = sm.AddBuffer("raw.HC", bytes);

    CHECK(sm.Contents(f).size() == 5);

    auto b = sm.Resolve(sm.LocFromOffset(f, 2));
    CHECK(b.line == 1);
    CHECK(b.column == 3);

    auto ff = sm.Resolve(sm.LocFromOffset(f, 4));
    CHECK(ff.line == 2);
    CHECK(ff.column == 1);

    CHECK(sm.LineText(sm.LocFromOffset(f, 4)).size() == 1);
}

TEST(Source_LoadFile) {
    Arena arena;
    SourceManager sm(arena);

    {
        std::ofstream out("holy_sm_test.tmp", std::ios::binary);
        out << "int x;\nbad\n";
    }

    FileId id;
    CHECK(sm.LoadFile("holy_sm_test.tmp", id) == FileStatus::Ok);
    CHECK(id.IsValid());

    auto p = sm.Resolve(sm.LocFromOffset(id, 7));
    CHECK(p.IsValid());
    CHECK(p.line == 2);
    CHECK(p.column == 1);

    CHECK(sm.LineText(sm.LocFromOffset(id, 7)) == "bad");

    std::remove("holy_sm_test.tmp");

    FileId missing;
    CHECK(
        sm.LoadFile("holy_no_such_file.HC", missing) ==
        FileStatus::NotFound);

    CHECK(!missing.IsValid());
}