#pragma once

#include <HolyLex/Keywords.h>
#include <HolyLex/Lexer.h>
#include <HolyLex/Token.h>
#include <HolyCore/Arena.h>
#include <HolyCore/Diagnostics.h>
#include <HolyCore/Platform.h>
#include <HolyCore/SourceManager.h>
#include <HolyCore/StringInterner.h>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <format>

using namespace holy::core;
using namespace holy::lex;

namespace lextest {
	// raw text of an Unknown token and where it was
	struct UnknownHit {
		std::string text;
		uint32_t line = 0;
		uint32_t column = 0;
	};

	struct DumpResult {
		FileStatus status = FileStatus::Ok; // result of reading the file
		std::string text; // full dump; empty unless requested
		size_t bytes = 0; // size of the source
		size_t tokens = 0; // number of tokens, Eof excluded
		size_t unknownCount = 0; // Unknown tokens
		size_t errorCount = 0; // error diagnostics
		size_t warningCount = 0; // warning diagnostics
		std::string firstError; // Format() of the first error diagnostic, or empty
		std::vector<UnknownHit> unknowns; // one entry per Unknown token
	};

	inline std::string EscapeForDump(std::string_view s) {
		constexpr char hexDigits[] = "0123456789ABCDEF";
		std::string escaped;
		escaped.reserve(s.size());
		for (char ch : s) {
			const unsigned char c = static_cast<unsigned char>(ch);
			switch (c) {
			case '"':
				escaped += "\\\"";
				break;
			case '\\':
				escaped += "\\\\";
				break;
			case '\n':
				escaped += "\\n";
				break;
			case '\t':
				escaped += "\\t";
				break;
			case '\r':
				escaped += "\\r";
				break;
			default:
				if (c >= 0x20 && c <= 0x7E) {
					escaped += static_cast<char>(c);
				}
				else {
					escaped += "\\x";
					escaped += hexDigits[c >> 4];
					escaped += hexDigits[c & 0x0F];
				}
				break;
			}
		}
		return escaped;
	}

	inline std::string DumpToken(const SourceManager& sm, const Token& t) {
		PresumedLoc p = sm.Resolve(t.loc);
		std::string startLine = t.startLine ? "^" : "";
		std::string payload = "";
		switch (t.kind) {
		case TokenKind::Identifier:
		case TokenKind::Directive:
		case TokenKind::Unknown:
		case TokenKind::RawBlock:
			payload = " \"" + std::string(t.text) + "\"";
			break;
		case TokenKind::IntLiteral:
			payload = " " + std::to_string(t.intValue);
			break;
		case TokenKind::CharLiteral:
			payload = std::format(" 0x{:x}", t.intValue);
			break;
		case TokenKind::FloatLiteral:
			payload = std::format(" {}", t.floatValue);
			break;
		case TokenKind::StringLiteral:
			payload = std::format(" \"{:s}\"", t.stringValue);
			if (t.hasDolDoc) {
				payload += " [doldoc]";
			}
			break;
		}
		std::string dump = std::to_string(p.line) + ":" + std::to_string(p.column) + startLine + " " + TokenKindName(t.kind) + payload;
		return dump;
	}

	inline DumpResult LexAndDumpBuffer(std::string_view name, std::string_view contents, bool wantText,
		LexerOptions options = {}) {
		Arena arena;
		SourceManager sm(arena);
		DiagEngine diags(arena, sm);
		StringInterner interner(arena);
		KeywordTable keywords(arena, interner);
		const FileId f = sm.AddBuffer(name, contents);
		if (!f.IsValid()) {
			return DumpResult{ .status = FileStatus::TooLarge };
		}

		Lexer lx(arena, sm, diags, interner, keywords, f, options);
		DumpResult r;
		r.bytes = contents.size();
		TokenKind prev = TokenKind::Eof;
		auto emit = [&](const Token& token) {
			if (token.kind != TokenKind::Eof) {
				++r.tokens;
			}
			if (token.kind == TokenKind::Unknown) {
				const PresumedLoc p = sm.Resolve(token.loc);
				++r.unknownCount;
				r.unknowns.push_back(UnknownHit{ std::string(token.text), p.line, p.column });
			}
			if (wantText) {
				r.text += DumpToken(sm, token) + "\n";
			}
		};
		while (true) {
			Token t = lx.Next();

			emit(t);
			if (prev == TokenKind::KwAsm && t.kind == TokenKind::LBrace) {
				Token raw = lx.CaptureRawBlock(t);
				emit(raw);
				prev = TokenKind::RawBlock;
				continue;
			}

			prev = t.kind;
			if (t.kind == TokenKind::Eof) {
				break;
			}
		}

		r.errorCount = diags.ErrorCount();
		r.warningCount = diags.WarningCount();
		if (r.errorCount > 0) {
			for (const Diag& diag : diags.All()) {
				if (diag.severity == Severity::Error) {
					r.firstError = diags.Format(diag);
					break;
				}
			}
		}
		if (wantText && diags.Count() > 0) {
			r.text += "--- diagnostics ---\n";
			r.text += diags.FormatAll();
		}
		return r;
	}

	inline DumpResult LexAndDumpFile(const std::string& path, bool wantText, LexerOptions options = {}) {
		std::string bytes;
		const FileStatus st = ReadWholeFile(path, bytes);
		if (st != FileStatus::Ok) {
			return DumpResult{ .status = st };
		}

		return LexAndDumpBuffer(path, bytes, wantText, options);
	}
}