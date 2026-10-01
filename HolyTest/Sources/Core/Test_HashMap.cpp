#include <HolyTest/Core/TestHarness.h>

#include <HolyCore/HashMap.h>
#include <cstdint>
#include <string>
#include <string_view>

using namespace holy::core;

// worst case: every key collides
struct ConstHash {
	size_t operator()(const int&) const { return 0; }
};

// Test enum
enum class Color { Red, Green };

TEST(HashMap_FreshState) {
	Arena a(1 << 20);
	HashMap<int, int> map(a);
	CHECK(map.Size() == 0);
	CHECK(map.Capacity() == 0);
	CHECK(map.Empty());
	CHECK(map.Find(1) == nullptr);
	CHECK(!map.Contains(1));
	CHECK(a.BytesUsed() == 0);
}

TEST(HashMap_InsertFindBasic) {
	Arena a(1 << 20);
	HashMap<int, int> map(a);
	CHECK(map.Insert(1, 100));
	CHECK(!map.Insert(1, 999));
	CHECK(*map.Find(1) == 100);
	map.Insert(2, 200);
	CHECK(map.Size() == 2);
	CHECK(map.Find(3) == nullptr);
	CHECK(map.Capacity() == 16);
}

TEST(HashMap_InsertOrAssign) {
	Arena a(1 << 20);
	HashMap<int, int> map(a);
	map.InsertOrAssign(5, 1);
	map.InsertOrAssign(5, 2);
	CHECK(map.Size() == 1);
	CHECK(*map.Find(5) == 2);
	map.InsertOrAssign(5, 3) += 10;
	CHECK(*map.Find(5) == 13);
}

TEST(HashMap_GrowthAndCapacity) {
	Arena a(1 << 20);
	HashMap<int, int> map(a);
	map.Insert(1, 100);
	map.Insert(2, 200);
	map.Insert(3, 300);
	map.Insert(4, 400);

	map.Insert(5, 500);
	map.Insert(6, 600);
	map.Insert(7, 700);
	map.Insert(8, 800);

	map.Insert(9, 900);
	map.Insert(10, 1000);
	map.Insert(11, 1100);
	map.Insert(12, 1200);
	
	CHECK(map.Capacity() == 16);
	
	map.Insert(13, 1300);

	CHECK(map.Capacity() == 32);
	map.Clear();
	for (int k = 0; k < 10000; k++) {
		map.Insert(k * 7 + 1, k);
	}
	CHECK(map.Size() == 10000);
	CHECK(map.Size() * 4 <= map.Capacity() * 3);
}

TEST(HashMap_ReservePreventsRehash) {
	Arena a(1 << 20);
	HashMap<int, int> map(a);
	map.Reserve(100);
	CHECK(map.Capacity() == 256);
	map.Insert(0, 2);
	int* p = map.Find(0);
	for (int i = 1; i < 101; i++) {
		map.Insert(i, i);
	}
	CHECK(p == map.Find(0));
}

TEST(HashMap_ClearAndReuse) {
	Arena a(1 << 20);
	HashMap<int, int> map(a);
	for (int i = 0; i < 50; i++) {
		map.Insert(i, i);
	}
	size_t cap = map.Capacity();
	map.Clear();
	CHECK(map.Size() == 0);
	CHECK(cap == map.Capacity());
	CHECK(map.Find(3) == nullptr);
	for (int i = 0; i < 50; i++) {
		map.Insert(i, i);
	}
	CHECK(map.Find(3) != nullptr);
}

TEST(HashMap_StringViewKeys) {
	Arena a(1 << 20);
	HashMap<std::string_view, int> map(a);
	map.Insert("alpha", 1);
	map.Insert("beta", 2);
	map.Insert("", 3);

	CHECK(*map.Find("alpha") == 1);
	CHECK(*map.Find("beta") == 2);
	CHECK(*map.Find("") == 3);
	CHECK(map.Find("alph") == nullptr);
}

TEST(HashMap_HashFunctions) {
	CHECK(DefaultHash<int>()(1) != DefaultHash<int>()(2));
	CHECK(DefaultHash<int>()(7) == DefaultHash<int>()(7));
	CHECK(DefaultHash<Color>()(Color::Red) != DefaultHash<Color>()(Color::Green));
	const char* str1 = "a";
	const char* str1Cpy = "a";
	const char* str2 = "b";
	CHECK(DefaultHash<std::string_view>()(str1) == DefaultHash<std::string_view>()(str1Cpy));
	CHECK(DefaultHash<std::string_view>()(str1) != DefaultHash<std::string_view>()(str2));
}