#include <HolyTest/Core/TestHarness.h>

int main(int argc, char** argv) {
	const char* filter = (argc > 1 ? argv[1] : nullptr);

	int testsRun = 0;
	int failuresBefore = holytest::FailureCount();

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