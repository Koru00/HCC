#include <HolyTest/Core/TestHarness.h>
#include <HolyCore/Platform.h>
#include <cstdio>
#include <fstream>
#include <string>

using namespace holy::core;

TEST(Platform_PageSizes) {
	CHECK(PageSize() == 4096);
	CHECK(AllocationGranularity() == 65536);
}

TEST(Platform_ReserveCommitRelease) {
    const size_t pageSize = PageSize();
    const size_t pages16 = pageSize * 16;
    const size_t pages4 = pageSize * 4;

    void* base = ReservePages(pages16);
    CHECK(base != nullptr);
    CHECK(reinterpret_cast<uintptr_t>(base) % AllocationGranularity() == 0);

    // Commit first 4 pages
    CHECK(CommitPages(base, pages4));

    uint8_t* p = reinterpret_cast<uint8_t*>(base);

    // Check they are zero
    for (size_t i = 0; i < pages4; ++i)
        CHECK(p[i] == 0);

    // Write first and last byte
    p[0] = 0xAA;
    p[pages4 - 1] = 0xBB;

    CHECK(p[0] == 0xAA);
    CHECK(p[pages4 - 1] == 0xBB);

    // Commit next 4 pages (arena growth)
    CHECK(CommitPages(p + pages4, pages4));

    p[pages4] = 0xCC;
    p[pages4 + pages4 - 1] = 0xDD;

    CHECK(p[pages4] == 0xCC);
    CHECK(p[pages4 + pages4 - 1] == 0xDD);

    // Decommit first 4 pages
    DecommitPages(base, pages4);

    // Commit again → must be zeroed
    CHECK(CommitPages(base, pages4));
    CHECK(p[0] == 0);

    // Release
    ReleasePages(base);
}

TEST(Platform_AllocAndProtect) {
    const size_t pageSize = PageSize();

    void* ptr = AllocPages(pageSize);
    CHECK(ptr != nullptr);

    uint8_t* p = reinterpret_cast<uint8_t*>(ptr);
    p[0] = 0xAB;

    CHECK(ProtectPages(ptr, pageSize, PageAccess::ReadOnly));
    CHECK(ProtectPages(ptr, pageSize, PageAccess::ReadWrite));

    ReleasePages(ptr);
}

TEST(Platform_ExecutablePage) {
    const size_t pageSize = PageSize();

    void* ptr = AllocPages(pageSize, PageAccess::ReadWriteExecute);
    CHECK(ptr != nullptr);

    uint8_t* p = reinterpret_cast<uint8_t*>(ptr);

    // mov eax, 42; ret
    p[0] = 0xB8; // mov eax, imm32
    p[1] = 0x2A; // 42
    p[2] = 0x00;
    p[3] = 0x00;
    p[4] = 0x00;
    p[5] = 0xC3; // ret

    using Fn = int(*)();
    Fn fn = reinterpret_cast<Fn>(ptr);

    int result = fn();
    CHECK(result == 42);

    ReleasePages(ptr);
}

TEST(Platform_ReadWholeFile_Roundtrip) {
    const char* filename = "holy_platform_test.tmp";

    // Write file
    {
        std::ofstream ofs(filename, std::ios::binary);
        CHECK(ofs.good());

        // 0..255
        for (int i = 0; i < 256; ++i)
            ofs.put(static_cast<char>(i));

        ofs << "\r\nline2\n";
    }

    // Read back
    std::string bytes;
    FileStatus st = ReadWholeFile(filename, bytes);
    CHECK(st == FileStatus::Ok);

    // Build expected
    std::vector<uint8_t> expected;
    for (int i = 0; i < 256; ++i)
        expected.push_back(static_cast<uint8_t>(i));

    expected.push_back('\r');
    expected.push_back('\n');
    expected.push_back('l');
    expected.push_back('i');
    expected.push_back('n');
    expected.push_back('e');
    expected.push_back('2');
    expected.push_back('\n');

    CHECK(bytes.size() == expected.size());
    CHECK(std::memcmp(bytes.data(), expected.data(), expected.size()) == 0);

    std::remove(filename);
}

TEST(Platform_ReadWholeFile_Empty) {
    const char* filename = "holy_platform_empty.tmp";

    // Create empty file
    {
        std::ofstream ofs(filename, std::ios::binary);
        CHECK(ofs.good());
    }

    std::string out = { 'g','a','r','b','a','g','e' };

    FileStatus st = ReadWholeFile(filename, out);
    CHECK(st == FileStatus::Ok);
    CHECK(out.empty());

    std::remove(filename);
}

TEST(Platform_ReadWholeFile_Errors) {
    std::string out = { 'g','a','r','b','a','g','e' };

    // Missing file → NotFound
    FileStatus st1 = ReadWholeFile("missing_file_123.tmp", out);
    CHECK(st1 == FileStatus::NotFound);
    CHECK(out.empty());

    // Empty path → InvalidPath
    out = { 'g','a','r','b','a','g','e' };
    FileStatus st2 = ReadWholeFile("", out);
    CHECK(st2 == FileStatus::InvalidPath);
    CHECK(out.empty());
}