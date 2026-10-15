#pragma once

#include <HolyCore/Arena.h>
#include <HolyCore/SourceManager.h>
#include <HolyCore/Diagnostics.h>
#include <HolyLex/Lexer.h>

using namespace holy::core;
using namespace holy::lex;

namespace lextest {
	struct Fixture {
		Arena arena{ 1 << 20 };
		SourceManager sm{ arena };
		DiagEngine diags{ arena, sm };
		StringInterner interner{ arena };
		KeywordTable keywords{ arena, interner };
		FileId file;
	};

	inline std::vector<Token> LexAll(Fixture& fx, std::string_view src, LexerOptions options = {}) {
		fx.file = fx.sm.AddBuffer("t.HC", src);
		Lexer lx(fx.arena, fx.sm, fx.diags, fx.interner, fx.keywords, fx.file, options);
		std::vector<Token> tokens;
		Token t;
		while (t.kind != TokenKind::Eof) {
			t = lx.Next();
			tokens.push_back(t);
		}

		return tokens;
	}

	inline std::vector<TokenKind> Kinds(const std::vector<Token>& tokens) {
		std::vector<TokenKind> kinds;
		for (size_t i = 0; i < tokens.size(); i++) {
			kinds.push_back(tokens[i].kind);
		}

		return kinds;
	}
}