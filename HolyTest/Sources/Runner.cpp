#include <HolyTest/Core/TestHarness.h>
#include <HolyTest/Core/Golden.h>
#include <cstdio>
#include <string_view>
#include <string>

bool has_flag(std::string_view flag, int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (flag == argv[i]) {
            return true;
        }
    }
    return false;
}

std::string_view get_option(std::string_view option, int argc, char** argv) {
    for (int i = 1; i < argc - 1; ++i) {
        if (option == argv[i]) {
            return argv[i + 1];
        }
    }
    return {};
}

int main(int argc, const char* argv[]) {
    bool list = false;
	const char* filter = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--list") {
            list = true;
        }
        else if (std::string_view(argv[i]) == "--update-golden") {
            holytest::UpdateGolden() = true;
        }
        else {
            std::string_view arg = argv[i];
            if (arg.starts_with("--")) {
                printf("unknown option: %s\n", argv[i]);
                return 2;
            }
            else {
                filter = argv[i];
            }
        }
    }

	int testsRun = 0;
	int failuresBefore = holytest::FailureCount();

    if (!list) {
        for (const auto& tc : holytest::Registry()) {

            if (filter && !std::strstr(tc.name, filter))
                continue;

            ++testsRun;

            int before = holytest::FailureCount();
            std::printf("[ RUN ] %s\n", tc.name);

            tc.fn();

            int after = holytest::FailureCount();
            if (after == before)
                std::printf("[ OK ] %s\n", tc.name);
            else
                std::printf("[ FAIL ] %s\n", tc.name);
        }

        int totalFailures = holytest::FailureCount() - failuresBefore;

        std::printf("%d test(s) run, %d check failure(s)\n",
            testsRun, totalFailures);

        return totalFailures == 0 ? 0 : 1;
    }
    else {
        printf("List %zu tests\n", holytest::Registry().size());
        size_t testNum = 1;
        for (const auto& tc : holytest::Registry()) {

            if (filter && !std::strstr(tc.name, filter))
                continue;

            printf("[%zu]\t%s", testNum++, tc.name);
        }
    }
    
    return 0;
}