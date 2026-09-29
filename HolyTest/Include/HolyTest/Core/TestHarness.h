#pragma once

#include <vector>
#include <cstdio>

#define TEST(name) \
    static void name(); \
    static ::holytest::Registrar name##_registrar(#name, &name); \
    static void name()

#define CHECK(cond) \
    do { if (!(cond)) { std::printf("    CHECK failed: %s (%s:%d)\n", #cond, __FILE__, __LINE__); \
                        ++::holytest::FailureCount(); } } while (0)

namespace holytest {
	struct TestCase {
		const char* name;
		void (*fn)();
	};

	inline std::vector<TestCase>& Registry() {
		static std::vector<TestCase> reg;
		return reg;
	}

	inline int& FailureCount() {
		static int count = 0;
		return count;
	}

	struct Registrar {
		Registrar(const char* name, void (*fn)()) {
			Registry().push_back(TestCase{ .name = name, .fn = fn });
		}
	};
}