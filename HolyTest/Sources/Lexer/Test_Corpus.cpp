#include <HolyTest/Core/TestHarness.h>
#include <HolyTest/Lexer/TokenDump.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace lextest;

TEST(Corpus_TempleOS_LexesCleanly) {
	std::string root = "../reference/TempleOS";
	if (!std::filesystem::is_directory(root)) {
		fprintf(stderr, "\tcorpus not found: %s", root.c_str());
		CHECK(false);
		return;
	}

	std::fstream file("Sources/Lexer/corpus_skip.txt");
	std::set<std::string> pathToSkip;

	if (file.is_open()) {
		std::string line;
		while (std::getline(file, line)) {
			std::string path;
			for (size_t i = 0; i < line.size(); i++) {
				char c = line[i];
				switch (c) {
				case ' ':
				case '\t':
				case '\n':
				case '\r':
					continue;
				case '#':
					goto exit_loop;
					break;
				case ',':
					pathToSkip.insert(path);
					path = "";
					break;
				default:
					path.append(1, c);
					break;
				}
				continue;
			exit_loop:
				break;
			}
			if (pathToSkip.count(path) == 0 && !path.empty()) {
				pathToSkip.insert(path);
			}
		}

		file.close();
	}

	const std::set<std::string> extensions = { ".HC", ".HH", ".CPP", ".HPP" };
	std::vector<std::pair<std::string, std::filesystem::path>> files;
	size_t skipped = 0;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
		if (!entry.is_regular_file()) {
			continue;
		}

		std::string extension = entry.path().extension().string();
		for (char& c : extension) {
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
		}
		if (extensions.count(extension) == 0) {
			continue;
		}

		std::string rel = std::filesystem::relative(entry.path(), root).generic_string();
		if (pathToSkip.count(rel) != 0) {
			++skipped;
			continue;
		}
		files.emplace_back(std::move(rel), entry.path());
	}

	std::sort(files.begin(), files.end(), [](const auto& left, const auto& right) {
		return left.first < right.first;
	});

	struct FailingFile {
		std::string rel;
		size_t errorCount;
		std::string firstError;
	};
	struct Stat {
		size_t count = 0;
		std::string firstExample;
	};

	size_t filesLexed = 0;
	size_t readFailures = 0;
	size_t totalBytes = 0;
	size_t totalTokens = 0;
	size_t totalUnknowns = 0;
	size_t totalErrors = 0;
	size_t totalWarnings = 0;
	std::vector<FailingFile> failingFiles;
	std::map<std::string, Stat> unknownStats;

	for (const auto& corpusFile : files) {
		DumpResult result = LexAndDumpFile(corpusFile.second.generic_string(), false, { .nulTerminatesSource = true });
		if (result.status != FileStatus::Ok) {
			++readFailures;
			std::printf("    Read failure: %s\n", corpusFile.first.c_str());
			continue;
		}

		++filesLexed;
		totalBytes += result.bytes;
		totalTokens += result.tokens;
		totalUnknowns += result.unknownCount;
		totalErrors += result.errorCount;
		totalWarnings += result.warningCount;

		if (result.errorCount > 0 || result.unknownCount > 0) {
			failingFiles.push_back({ corpusFile.first, result.errorCount, result.firstError });
		}

		for (const UnknownHit& hit : result.unknowns) {
			Stat& stat = unknownStats[EscapeForDump(hit.text)];
			if (stat.count == 0) {
				stat.firstExample = corpusFile.first + ":" + std::to_string(hit.line) + ":" + std::to_string(hit.column);
			}
			++stat.count;
		}
	}

	std::printf("    Summary: files lexed=%zu, files skipped=%zu, bytes=%zu, tokens=%zu, errors=%zu, warnings=%zu, unknown tokens=%zu\n",
		filesLexed, skipped, totalBytes, totalTokens, totalErrors, totalWarnings, totalUnknowns);

	std::printf("    Failing files:\n");
	for (size_t i = 0; i < failingFiles.size() && i < 25; ++i) {
		const FailingFile& failingFile = failingFiles[i];
		std::string firstLine = failingFile.firstError;
		const size_t lineEnd = firstLine.find_first_of("\r\n");
		if (lineEnd != std::string::npos) {
			firstLine.resize(lineEnd);
		}
		std::printf("    %s: %zu error(s): %s\n", failingFile.rel.c_str(), failingFile.errorCount, firstLine.c_str());
	}

	std::vector<std::pair<std::string, Stat>> frequentUnknowns(unknownStats.begin(), unknownStats.end());
	std::sort(frequentUnknowns.begin(), frequentUnknowns.end(), [](const auto& left, const auto& right) {
		if (left.second.count != right.second.count) {
			return left.second.count > right.second.count;
		}
		return left.first < right.first;
	});
	std::printf("    Most frequent unexpected bytes:\n");
	for (size_t i = 0; i < frequentUnknowns.size() && i < 15; ++i) {
		const auto& [escaped, stat] = frequentUnknowns[i];
		std::printf("    %zux \"%s\" first at %s\n", stat.count, escaped.c_str(), stat.firstExample.c_str());
	}

	CHECK(filesLexed > 0);
	CHECK(readFailures == 0);
	CHECK(totalUnknowns == 0);
	CHECK(totalErrors == 0);
}