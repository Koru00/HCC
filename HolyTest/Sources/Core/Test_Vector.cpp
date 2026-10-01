#include <HolyTest/Core/TestHarness.h>

#include <HolyCore/Vector.h>
#include <cstdint>

using namespace holy::core;

struct Pt {
	int x, y;
	Pt(int x_, int y_) : x(x_), y(y_) {}
};

struct alignas(32) Wide {
	float f[8];
};

TEST(Vector_FreshState) {
	Arena a(1 << 20);
	Vector<int> v(a);
	CHECK(v.Size() == 0);
	CHECK(v.Capacity() == 0);
	CHECK(v.Empty());
	CHECK(v.Data() == nullptr);
	CHECK(a.BytesUsed() == 0);
}

TEST(Vector_PushBackGrowth) {
	Arena a(1 << 20);
	Vector<int> v(a);

	for (int i = 0; i < 1000; i++) {
		v.PushBack(i);
		if (i == 0) {
			CHECK(v.Capacity() == 4);
		}
		else if (i == 4) {
			CHECK(v.Capacity() == 8);
		}
	}

	CHECK(v.Size() == 1000);
	CHECK(v.Capacity() == 1024);
	CHECK(v.Front() == 0);
	CHECK(v.Back() == 999);

	for (int i = 0; i < 1000; i++) {
		CHECK(v[i] == i);
	}
	CHECK(a.Owns(v.Data()));
}

#include <iostream>

TEST(Vector_EmplaceBack) {
	Arena a(1 << 20);
	Vector<Pt> v(a);
	v.EmplaceBack(1, 2);
	CHECK(v[0].x == 1);
	CHECK(v[0].y == 2);
}

TEST(Vector_PushOwnElement) {
	Arena a(1 << 20);
	Vector<int> v(a);
	v.PushBack(10);
	v.PushBack(20);
	v.PushBack(30);
	v.PushBack(40);
	v.PushBack(v[2]);
	CHECK(v[4] == 30);
	CHECK(v.Capacity() == 8);
	CHECK(v[0] == 10);
	CHECK(v[1] == 20);
	CHECK(v[2] == 30);
}

TEST(Vector_ReserveResizeClear) {
	Arena a(1 << 20);
	Vector<int> v(a);
	v.Reserve(100);
	CHECK(v.Capacity() == 100);
	CHECK(v.Size() == 0);
	v.Reserve(50);
	CHECK(v.Capacity() == 100);

	Arena newA(1 << 20);
	Vector<int> newV(newA);
	newV.PushBack(-1);
	newV.PushBack(-1);
	newV.PushBack(-1);
	newV.PushBack(-1);
	newV.PushBack(-1);
	newV.Clear();
	CHECK(newV.Size() == 0);
	newV.Resize(5);
	CHECK(newV[0] == 0);
	CHECK(newV[1] == 0);
	CHECK(newV[2] == 0);
	CHECK(newV[3] == 0);
	CHECK(newV[4] == 0);
	newV.Resize(2);
	CHECK(newV.Size() == 2);
	newV.PopBack();
	CHECK(newV.Size() == 1);
}

TEST(Vector_RangeFor) {
	Arena a(1 << 20);
	Vector<int> v(a);
	for (int i = 0; i < 11; i++) {
		v.PushBack(i);
	}

	int sum = 0;
	for (int i : v) {
		sum += i;
	}
	CHECK(sum == 55);
}

TEST(Vector_OfVectors) {
	Arena a(1 << 20);
	Vector<Vector<int>> outer(a);
	outer.Resize(3);
	for (int i = 0; i < 3; i++) {
		outer[i] = Vector<int>(a);
		outer[i].PushBack(i + 1);
	}

	outer.Resize(10);
	for (int i = 0; i < 3; i++) {
		CHECK(outer[i][0] == i + 1);
	}

	for (int i = 3; i < 10; i++) {
		CHECK(outer[i].Size() == 0);
	}
}

TEST(Vector_Alignment) {
	Arena a(1 << 20);
	Vector<double> v(a);
	v.PushBack(2.0f);
	CHECK(reinterpret_cast<std::uintptr_t>(v.Data()) % alignof(double) == 0);
	Vector<Wide> wide(a);
	wide.PushBack(Wide{});
	wide.PushBack(Wide{});
	CHECK(reinterpret_cast<std::uintptr_t>(wide.Data()) % 32 == 0);
}