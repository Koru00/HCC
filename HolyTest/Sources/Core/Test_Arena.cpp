#include <HolyTest/Core/TestHarness.h>
#include <HolyCore/Arena.h>
#include <cstdint>
#include <cstring>
#include <string_view>

using namespace holy::core;

struct Node {
	int a;
	double b;
	Node* next;
	Node(int a_, double b_, Node* n) : a(a_), b(b_), next(n) {}
};

TEST(Arena_FreshState) {
	Arena a(4 * 1024 * 1024);
	CHECK(a.BytesUsed() == 0);
	CHECK(a.BytesCommitted() == 0);
	CHECK(a.BytesReserved() == 4 * 1024 * 1024);
}

TEST(Arena_AlignmentAndOrder) {
	Arena a(1 << 20);

	void* p1 = a.Allocate(1);
	CHECK(p1 != nullptr);
	CHECK(reinterpret_cast<uintptr_t>(p1) % alignof(std::max_align_t) == 0);
	void* p2 = a.Allocate(1, 16);
	CHECK(p2 != nullptr);
	CHECK(reinterpret_cast<uintptr_t>(p2) % 16 == 0);
	void* p3 = a.Allocate(1, 64);
	CHECK(reinterpret_cast<uintptr_t>(p3) % 64 == 0);
	CHECK(p3 != nullptr);
	void* p4 = a.Allocate(1, 4096);
	CHECK(reinterpret_cast<uintptr_t>(p4) % 4096 == 0);
	CHECK(p4 != nullptr);
}

TEST(Arena_ContiguousSmallAllocs) {
	Arena a(1 << 20);
	a.Allocate(8, 8);
	a.Allocate(8, 8);
	a.Allocate(8, 8);
	CHECK(a.BytesUsed() == 24);
}

TEST(Arena_GrowsAcrossChunks) {
	Arena a(4 << 20);

	constexpr size_t blockSize = 200ull * 1024ull; // 200 KiB
	constexpr int blockCount = 10;

	std::vector<uint8_t*> blocks;
	blocks.reserve(blockCount);

	for (int i = 0; i < blockCount; ++i) {
		void* p = a.Allocate(blockSize, 16);
		CHECK(p != nullptr);

		uint8_t* b = reinterpret_cast<uint8_t*>(p);
		blocks.push_back(b);

		std::memset(b, uint8_t(i + 1), blockSize);
	}

	for (int i = 0; i < blockCount; ++i) {
		uint8_t* b = blocks[i];
		uint8_t expected = uint8_t(i + 1);

		CHECK(b[0] == expected);
		CHECK(b[blockSize / 2] == expected);
		CHECK(b[blockSize - 1] == expected);
	}

	CHECK(a.BytesUsed() == 2'048'000); // 200 KiB * 10

	CHECK(a.BytesCommitted() == 2'097'152); // RoundUp(2,048,000, 64 KiB)

	CHECK(a.BytesCommitted() <= a.BytesReserved());
}

TEST(Arena_NewTyped) {
	Arena a(4 << 20);

	Node* n = a.New<Node>(7, 2.5, nullptr);
	CHECK(n->a == 7);
	CHECK(n->b == 2.5);
	CHECK(n->next == nullptr);

	int* i = a.New<int>(42);
	CHECK(i[0] == 42);
}

TEST(Arena_NewArrayZeroInit) {
	Arena a(4 << 20);

	uint32_t* array = a.NewArray<uint32_t>(100);
	for (int i = 0; i < 100; i++) {
		array[i] = 0xFFFFFFFF;
	}

	a.Reset();

	uint32_t* newArray = a.NewArray<uint32_t>(100);
	for (int i = 0; i < 100; i++) {
		CHECK(array[i] == 0);
	}

	CHECK(a.NewArray<int>(0) == nullptr);
}

TEST(Arena_Reset) {
	Arena a(4 << 20);

	void* p = a.Allocate(1000);
	size_t byteCommitted = a.BytesCommitted();
	a.Reset();
	CHECK(a.BytesCommitted() == byteCommitted);
	CHECK(a.BytesUsed() == 0);
	void* newP = a.Allocate(1000);
	CHECK(p == newP);
}

TEST(Arena_CopyStringAndOwns) {
	Arena a(4 << 20);
	std::string_view str = "hello";
	std::string_view copyStr;

	copyStr = a.CopyString(str);

	CHECK(copyStr.compare("hello") == 0);
	CHECK(copyStr.data() != str.data());
	CHECK(copyStr.data()[5] == '\0');

	std::string_view strEmpty;
	std::string_view copyStrEmpty = a.CopyString(strEmpty);
	CHECK(copyStrEmpty.empty());
	CHECK(a.Owns(copyStr.data()));
	int integer = 2;
	CHECK(!a.Owns(&integer));
	a.Reset();
	CHECK(!a.Owns(copyStr.data()));
}