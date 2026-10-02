#include <HolyCore/Diagnostics.h>
#include <cassert>

namespace holy::core {
	const char* SeverityName(Severity severity) {
		switch (severity) {
		case holy::core::Severity::Note:
			return "note";
		case holy::core::Severity::Warning:
			return "warning";
		case holy::core::Severity::Error:
			return "error";
		}

		assert(false);
		return "unknown";
	}
	
	DiagEngine::DiagEngine(Arena& arena, const SourceManager& source) : m_arena(arena), m_sources(source), m_diags(arena) {}

	void DiagEngine::Report(Severity severity, SourceLoc loc, std::string_view message) {
		Diag d{ .severity = severity, .loc = loc, .message = m_arena.CopyString(message) };
		m_diags.PushBack(d);
		switch (severity) {
		case holy::core::Severity::Note:
			break;
		case holy::core::Severity::Warning:
			++m_warningCount;
			break;
		case holy::core::Severity::Error:
			++m_errorCount;
		}
	}

	void DiagEngine::Clear() {
		m_diags.Clear();
		m_errorCount = 0;
		m_warningCount = 0;
	}

	void DiagEngine::Error(SourceLoc loc, std::string_view message) {
		Report(Severity::Error, loc, message);
	}

	void DiagEngine::Warning(SourceLoc loc, std::string_view message) {
		Report(Severity::Warning, loc, message);
	}

	void DiagEngine::Note(SourceLoc loc, std::string_view message) {
		Report(Severity::Note, loc, message);
	}
	
	std::string DiagEngine::Format(const Diag& diag) const {
		const PresumedLoc presumedLoc = m_sources.Resolve(diag.loc);
		std::string out;
		if (presumedLoc.IsValid()) {
			out.append(presumedLoc.filename)
				.append(":")
				.append(std::to_string(presumedLoc.line))
				.append(":")
				.append(std::to_string(presumedLoc.column))
				.append(": ");
		}
		out.append(SeverityName(diag.severity))
			.append(": ")
			.append(diag.message);
		if (!presumedLoc.IsValid()) {
			return out;
		}

		const std::string_view text = m_sources.LineText(diag.loc);
		if (text.empty()) {
			return out;
		}

		out.append("\n").append(text).append("\n");

		const bool followsSpace = presumedLoc.column > 1 &&
			presumedLoc.column - 2 < text.size() &&
			text[presumedLoc.column - 2] == ' ';
		const uint32_t caretColumn = presumedLoc.column - (followsSpace ? 1 : 0);
		for (uint32_t i = 0; i + 1 < caretColumn; ++i) {
			const bool isTab = i < text.size() && text[i] == '\t';
			out.append(isTab ? "\t" : " ");
		}

		out.append("^");
		return out;
	}
	
	std::string DiagEngine::FormatAll() const {
		std::string out;
		for (Diag diag : m_diags) {
			out += Format(diag);
			out += '\n';
		}

		return out;
	}
}