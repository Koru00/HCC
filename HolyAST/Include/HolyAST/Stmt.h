#pragma once

#include <string_view>
#include <HolyCore/SourceManager.h>
#include <HolyCore/Vector.h>
#include <HolyCore/StringInterner.h>

namespace holy::ast {
	enum class StmtKind {
		Empty,
		Compound,
		Expr,
		Decl,
		Print,
		If,
		While,
		DoWhile,
		For,
		Switch,
		Case,
		SubSwitch,
		Break,
		Continue,
		Return,
		Goto,
		Label,
		Try,
		Throw,
		Asm,
		Lock,
		NoWarn
	};

	// Forward declaration
	struct Expr;
	struct Decl;

	struct Stmt {
		StmtKind kind;
		core::SourceLoc loc;

		Stmt(StmtKind k, core::SourceLoc l) : kind(k), loc(l) {}
	};

	template<class T>
	inline T* As(Stmt* node) {
		assert(node != nullptr);
		assert(node->kind == T::StaticKind);
		return static_cast<T*>(node);
	}

	template<class T>
	inline const T* As(const Stmt* node) {
		assert(node != nullptr);
		assert(node->kind == T::StaticKind);
		return static_cast<T*>(node);
	}

	struct EmptyStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::Empty;

		EmptyStmt(core::SourceLoc loc)
			: Stmt(StaticKind, loc) {}
	};
	static_assert(std::is_trivially_destructible_v<EmptyStmt>);

	struct CompoundStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::Compound;
		core::Vector<Stmt*> stmts;

		CompoundStmt(core::Vector<Stmt*> stmts, core::SourceLoc loc)
			: Stmt(StaticKind, loc), stmts(stmts) {}
	};
	static_assert(std::is_trivially_destructible_v<CompoundStmt>);

	struct ExprStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::Expr;
		Expr* expr;

		ExprStmt(Expr* expr, core::SourceLoc loc)
			: Stmt(StaticKind, loc), expr(expr) {}
	};
	static_assert(std::is_trivially_destructible_v<ExprStmt>);

	struct DeclStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::Decl;
		core::Vector<Decl*> decls;

		DeclStmt(core::Vector<Decl*> decls, core::SourceLoc loc)
			: Stmt(StaticKind, loc), decls(decls) {}
	};
	static_assert(std::is_trivially_destructible_v<DeclStmt>);

	struct PrintStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::Print;
		Expr* format;
		core::Vector<Expr*> args;

		PrintStmt(Expr* format, core::Vector<Expr*> args, core::SourceLoc loc)
			: Stmt(StaticKind, loc), format(format), args(args) {}
	};
	static_assert(std::is_trivially_destructible_v<PrintStmt>);

	struct IfStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::If;
		Expr* cond;
		Stmt* thenBranch;
		Stmt* elseBranch;

		IfStmt(Expr* cond, Stmt* thenBranch, Stmt* elseBranch, core::SourceLoc loc)
			: Stmt(StaticKind, loc), cond(cond), thenBranch(thenBranch), elseBranch(elseBranch) {}
	};
	static_assert(std::is_trivially_destructible_v<IfStmt>);
	
	struct WhileStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::While;
		Expr* cond;
		Stmt* body;

		WhileStmt(Expr* cond, Stmt* body, core::SourceLoc loc)
			: Stmt(StaticKind, loc), cond(cond), body(body) {}
	};
	static_assert(std::is_trivially_destructible_v<WhileStmt>);

	struct DoWhileStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::DoWhile;
		Stmt* body;
		Expr* cond;

		DoWhileStmt(Stmt* body, Expr* cond, core::SourceLoc loc)
			: Stmt(StaticKind, loc), body(body), cond(cond) {}
	};
	static_assert(std::is_trivially_destructible_v<DoWhileStmt>);

	struct ForStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::For;
		Stmt* init;
		Expr* cond;
		Expr* step;
		Stmt* body;

		ForStmt(Stmt* init, Expr* cond, Expr* step, Stmt* body, core::SourceLoc loc)
			: Stmt(StaticKind, loc), init(init), cond(cond), step(step), body(body) {}
	};
	static_assert(std::is_trivially_destructible_v<ForStmt>);

	struct SwitchStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::Switch;
		Expr* subject;
		Stmt* body;
		bool isNoBounds;

		SwitchStmt(Expr* subject, Stmt* body, bool isNoBounds, core::SourceLoc loc)
			: Stmt(StaticKind, loc), subject(subject), body(body), isNoBounds(isNoBounds) {}
	};
	static_assert(std::is_trivially_destructible_v<SwitchStmt>);

	struct CaseStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::Case;
		Expr* lo;
		Expr* hi;
		bool isDefault;
		bool isAutoValue;

		CaseStmt(Expr* lo, Expr* hi, bool isDefault, bool isAutoValue, core::SourceLoc loc)
			: Stmt(StaticKind, loc), lo(lo), hi(hi), isDefault(isDefault), isAutoValue(isAutoValue) {}
	};
	static_assert(std::is_trivially_destructible_v<CaseStmt>);

	struct SubSwitchStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::SubSwitch;
		Stmt* body;

		SubSwitchStmt(Stmt* body, core::SourceLoc loc)
			: Stmt(StaticKind, loc), body(body) {}
	};
	static_assert(std::is_trivially_destructible_v<SubSwitchStmt>);

	struct BreakStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::Break;
		Expr* levels;

		BreakStmt(Expr* levels, core::SourceLoc loc)
			: Stmt(StaticKind, loc), levels(levels) {}
	};
	static_assert(std::is_trivially_destructible_v<BreakStmt>);
	
	struct ContinueStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::Continue;
		Expr* levels;

		ContinueStmt(Expr* levels, core::SourceLoc loc)
			: Stmt(StaticKind, loc), levels(levels) {}
	};
	static_assert(std::is_trivially_destructible_v<ContinueStmt>);

	struct ReturnStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::Return;
		Expr* value;

		ReturnStmt(Expr* value, core::SourceLoc loc)
			: Stmt(StaticKind, loc), value(value) {}
	};
	static_assert(std::is_trivially_destructible_v<ReturnStmt>);

	struct GotoStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::Goto;
		core::Atom label;

		GotoStmt(core::Atom label, core::SourceLoc loc)
			: Stmt(StaticKind, loc), label(label) {}
	};
	static_assert(std::is_trivially_destructible_v<GotoStmt>);

	struct LabelStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::Label;
		core::Atom name;

		LabelStmt(core::Atom name, core::SourceLoc loc)
			: Stmt(StaticKind, loc), name(name) {}
	};
	static_assert(std::is_trivially_destructible_v<LabelStmt>);

	struct TryStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::Try;
		Stmt* body;
		Stmt* catchBody;

		TryStmt(Stmt* body, Stmt* catchBody, core::SourceLoc loc)
			: Stmt(StaticKind, loc), body(body), catchBody(catchBody) {}
	};
	static_assert(std::is_trivially_destructible_v<TryStmt>);

	struct ThrowStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::Throw;
		Expr* value;

		ThrowStmt(Expr* value, core::SourceLoc loc)
			: Stmt(StaticKind, loc), value(value) {}
	};
	static_assert(std::is_trivially_destructible_v<ThrowStmt>);

	struct AsmStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::Asm;
		std::string_view text;

		AsmStmt(std::string_view text, core::SourceLoc loc)
			: Stmt(StaticKind, loc), text(text) {}
	};
	static_assert(std::is_trivially_destructible_v<AsmStmt>);

	struct LockStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::Lock;
		Stmt* body;

		LockStmt(Stmt* body, core::SourceLoc loc)
			: Stmt(StaticKind, loc), body(body) {}
	};
	static_assert(std::is_trivially_destructible_v<LockStmt>);

	struct NoWarnStmt : public Stmt {
		static constexpr StmtKind StaticKind = StmtKind::NoWarn;
		core::Vector<core::Atom> names;

		NoWarnStmt(core::Vector<core::Atom> names, core::SourceLoc loc)
			: Stmt(StaticKind, loc), names(names) {}
	};
	static_assert(std::is_trivially_destructible_v<NoWarnStmt>);
}