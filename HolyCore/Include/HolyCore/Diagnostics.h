#pragma once

#include <HolyCore/Arena.h>
#include <HolyCore/SourceManager.h>
#include <HolyCore/Vector.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace holy::core {
	enum class Severity {
		Note,
		Warning,
		Error
	};

	const char* SeverityName(Severity severity);

	struct Diag {
		Severity severity = Severity::Error;
		SourceLoc loc;
		std::string_view message;
	};

	class DiagEngine {
	public:
		DiagEngine(Arena& arena, const SourceManager& source);

		DiagEngine(const DiagEngine&) = delete;
		DiagEngine& operator=(const DiagEngine&) = delete;

		// Reporting
		void Report(Severity severity, SourceLoc loc, std::string_view message); // copies message into the arena, updates counters
		void Error(SourceLoc loc, std::string_view message);
		void Warning(SourceLoc loc, std::string_view message);
		void Note(SourceLoc loc, std::string_view message);

		// Queries
		[[nodiscard]] size_t Count() const { return m_diags.Size(); }
		[[nodiscard]] size_t ErrorCount() const { return m_errorCount; }
		[[nodiscard]] size_t WarningCount() const { return m_warningCount; }
		[[nodiscard]] bool HasErrors() const { return m_errorCount != 0; }
		[[nodiscard]] const Vector<Diag>& All() const { return m_diags; }

		void Clear();

		// Formatting (layout described above)
		[[nodiscard]] std::string Format(const Diag& diag) const; // one diagnostic, no trailing newline
		[[nodiscard]] std::string FormatAll() const; // every diagnostic, each followed by '\n'
	private:
		Arena& m_arena; // owns the message copies and the vector
		const SourceManager& m_sources; // resolves locations and line text
		Vector<Diag> m_diags; // all diagnostics, in report order
		size_t m_errorCount = 0;
		size_t m_warningCount = 0;
	};
}