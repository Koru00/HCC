#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <HolyCore/StringInterner.h>
#include <HolyAST/Decl.h>
#include <HolyAST/Expr.h>
#include <HolyAST/Type.h>
#include <HolyAST/Stmt.h>

namespace holy::ast {
	struct PrintOption {

	};

	class AstPrinter {
	public:
		explicit AstPrinter(const core::StringInterner& interner);
		~AstPrinter() = default;
		
		std::string Print(const TranslationUnit& unit);
		std::string Print(const TypeNode& type);
		std::string Print(const Expr& expr);
		std::string Print(const Stmt& stmt);
		std::string Print(const Decl& decl);
	private:
		void Print(const TypeNode* node, std::string_view role);
		void Print(const Expr* node, std::string_view role);
		void Print(const Stmt* node, std::string_view role);
		void Print(const Decl* node, std::string_view role);

		void BeginLine(std::string_view role);
		void EndLine();
		void WriteQuoted(std::string_view text);
		void WriteAtom(core::Atom atom);
		void WriteBool(bool boolean);
		void WriteFlags(DeclFlags flags);
		std::string IndexedRole(std::string_view name, size_t i);

		std::string m_out;
		uint32_t m_depth = 0;
		const core::StringInterner& m_interner;
	};
}