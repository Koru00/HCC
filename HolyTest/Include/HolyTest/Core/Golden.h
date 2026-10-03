#pragma once

#include <HolyCore/Platform.h>
#include <cstdio>
#include <fstream>
#include <string>

namespace holytest {
	// function-local static bool, false by default; set by --update-golden
	inline bool& UpdateGolden() {
		static bool value = false;
		return value;
	}

	[[nodiscard]] inline bool CheckGolden(const std::string& goldenPath, const std::string& actual) {
		std::string actualPath = goldenPath + ".actual";
		if (UpdateGolden()) {
			std::ofstream file(goldenPath);
			if (!file) {
				return false;
			}
			file << actual;
			printf("\tgolden updated: %s\n", goldenPath.c_str());
			file.close();
		}

		std::string expected;
		holy::core::FileStatus status = holy::core::ReadWholeFile(goldenPath, expected);
		if (status != holy::core::FileStatus::Ok) {
			printf("\tgolden unreadable: %s (%s)\n", goldenPath.c_str(), holy::core::FileStatusName(status));
			std::ofstream file(actualPath);
			file << actual;
			file.close();
			return false;
		}

		if (expected == actual) {
			std::remove(actualPath.c_str());
			return true;
		}

		size_t lineNum = 1;
		size_t i = 0;
		const size_t minSize = std::min(actual.size(), expected.size());
		for (; i < minSize && actual[i] == expected[i]; ++i) {
			if (expected[i] == '\n') {
				++lineNum;
			}
		}

		printf("\tgolden mismatch: %s (first difference at line %zu)\n", goldenPath.c_str(), lineNum);
		std::ofstream file(actualPath);
		file << actual;
		file.close();
		printf("\twrote %s\n", actualPath.c_str());

		return false;
	}
}