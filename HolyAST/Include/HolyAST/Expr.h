#pragma once

#include <HolyCore/SourceManager.h>
#include <HolyCore/StringInterner.h>

namespace holy::ast {
	enum class ExprKind {
		IntLiteral,
		FloatLiteral,
		StringLiteral,
		Name,
		Unary,
		Binary,
		Assign,
		Call,
		Index,
		Member,
		Cast,
		Sizeof,
		Offsetof
	};

	enum class UnaryOp {
		Plus,
		Minus,
		BitNot,
		LogicalNot,
		AddressOf,
		Deref,
		PreInc,
		PreDec,
		PostInc,
		PostDec,
	};

	enum class BinaryOp {
		Add,
		Sub,
		Mul,
		Div,
		Mod,
		Shl,
		Shr,
		BitAnd,
		BitOr,
		BitXor,
		LogicalAnd,
		LogicalOr,
		LogicalXor,
		Pow,
		Eq,
		Ne,
		Lt,
		Le,
		Gt,
		Ge
	};

	enum class AssignOp {
		Assign,
		AddAssign,
		SubAssign,
		MulAssign,
		DivAssign,
		ModAssign,
		ShlAssign,
		ShrAssign,
		AndAssign,
		OrAssign,
		XorAssign
	};

	// Forward declaration
	struct TypeNode;

	struct Expr {
		ExprKind kind;
		core::SourceLoc loc;

		Expr(ExprKind k, core::SourceLoc l) : kind(k), loc(l) {}
	};

	template<class T>
	inline T* As(Expr* node) {
		assert(node != nullptr);
		assert(node->kind == T::StaticKind);
		return static_cast<T*>(node);
	}

	template<class T>
	inline const T* As(const Expr* node) {
		assert(node != nullptr);
		assert(node->kind == T::StaticKind);
		return static_cast<T*>(node);
	}

	struct IntLiteralExpr : public Expr {
		static constexpr ExprKind StaticKind = ExprKind::IntLiteral;
		uint64_t value;
		bool isChar;

		IntLiteralExpr(uint64_t value, bool isChar, core::SourceLoc loc)
			: Expr(StaticKind, loc), value(value), isChar(isChar) {}
	};
	static_assert(std::is_trivially_destructible_v<IntLiteralExpr>);

	struct FloatLiteralExpr : public Expr {
		static constexpr ExprKind staticKind = ExprKind::FloatLiteral;
		double value;

		FloatLiteralExpr(double value, core::SourceLoc loc)
			: Expr(staticKind, loc), value(value) {}
	};
	static_assert(std::is_trivially_destructible_v<FloatLiteralExpr>);

	struct StringLiteralExpr : public Expr {
		static constexpr ExprKind staticKind = ExprKind::StringLiteral;
		core::Atom text;

		StringLiteralExpr(core::Atom text, core::SourceLoc loc)
			: Expr(staticKind, loc), text(text) {}
	};
	static_assert(std::is_trivially_destructible_v<StringLiteralExpr>);

	struct NameExpr : public Expr {
		static constexpr ExprKind staticKind = ExprKind::Name;
		core::Atom name;

		NameExpr(core::Atom name, core::SourceLoc loc)
			: Expr(staticKind, loc), name(name) {}
	};
	static_assert(std::is_trivially_destructible_v<NameExpr>);

	struct UnaryExpr : public Expr {
		static constexpr ExprKind staticKind = ExprKind::Unary;
		UnaryOp op;
		Expr* operand;

		UnaryExpr(UnaryOp op, Expr* operand, core::SourceLoc loc)
			: Expr(staticKind, loc), op(op), operand(operand) {}
	};
	static_assert(std::is_trivially_destructible_v<UnaryExpr>);

	struct BinaryExpr : public Expr {
		static constexpr ExprKind staticKind = ExprKind::Binary;
		BinaryOp op;
		Expr* lhs;
		Expr* rhs;

		BinaryExpr(BinaryOp op, Expr* lhs, Expr* rhs, core::SourceLoc loc)
			: Expr(staticKind, loc), op(op), lhs(lhs), rhs(rhs) {}
	};
	static_assert(std::is_trivially_destructible_v<BinaryExpr>);

	struct AssignExpr : public Expr {
		static constexpr ExprKind staticKind = ExprKind::Assign;
		AssignOp op;
		Expr* target;
		Expr* value;

		AssignExpr(AssignOp op, Expr* target, Expr* value, core::SourceLoc loc)
			: Expr(staticKind, loc), op(op), target(target), value(value) {}
	};
	static_assert(std::is_trivially_destructible_v<AssignExpr>);

	struct CallExpr : public Expr {
		static constexpr ExprKind staticKind = ExprKind::Call;
		Expr* calle;
		core::Vector<Expr*> args;
		bool hasParens;

		CallExpr(Expr* calle, core::Vector<Expr*> args, bool hasParens, core::SourceLoc loc)
			: Expr(staticKind, loc), calle(calle), args(args), hasParens(hasParens) {}
	};
	static_assert(std::is_trivially_destructible_v<CallExpr>);

	struct IndexExpr : public Expr {
		static constexpr ExprKind staticKind = ExprKind::Index;
		Expr* base;
		Expr* index;

		IndexExpr(Expr* base, Expr* index, core::SourceLoc loc)
			: Expr(staticKind, loc), base(base), index(index) {}
	};
	static_assert(std::is_trivially_destructible_v<IndexExpr>);

	struct MemberExpr : public Expr {
		static constexpr ExprKind staticKind = ExprKind::Member;
		Expr* base;
		core::Atom member;
		bool viaArrow;

		MemberExpr(Expr* base, core::Atom member, bool viaArrow, core::SourceLoc loc)
			: Expr(staticKind, loc), base(base), member(member), viaArrow(viaArrow) {}
	};
	static_assert(std::is_trivially_destructible_v<MemberExpr>);

	struct CastExpr : public Expr {
		static constexpr ExprKind staticKind = ExprKind::Cast;
		TypeNode* type;
		Expr* operand;

		CastExpr(Expr* operand, TypeNode* type, core::SourceLoc loc)
			: Expr(staticKind, loc), operand(operand), type(type) {}
	};
	static_assert(std::is_trivially_destructible_v<CastExpr>);

	struct SizeofExpr : public Expr {
		static constexpr ExprKind staticKind = ExprKind::Sizeof;
		TypeNode* type;
		Expr* operand;

		SizeofExpr(TypeNode* type, Expr* operand, core::SourceLoc loc)
			: Expr(staticKind, loc), type(type), operand(operand) {}
	};
	static_assert(std::is_trivially_destructible_v<SizeofExpr>);

	struct OffsetofExpr : public Expr {
		static constexpr ExprKind staticKind = ExprKind::Offsetof;
		TypeNode* type;
		core::Atom member;

		OffsetofExpr(TypeNode* type, core::Atom member, core::SourceLoc loc)
			: Expr(staticKind, loc), type(type), member(member) {}
	};
	static_assert(std::is_trivially_destructible_v<OffsetofExpr>);

	inline const char* unaryOpSpelling(holy::ast::UnaryOp op) {
		switch (op) {
		case holy::ast::UnaryOp::Plus:
			return "+";
		case holy::ast::UnaryOp::Minus:
			return "-";
		case holy::ast::UnaryOp::BitNot:
			return "~";
		case holy::ast::UnaryOp::LogicalNot:
			return "!";
		case holy::ast::UnaryOp::AddressOf:
			return "&";
		case holy::ast::UnaryOp::Deref:
			return "*";
		case holy::ast::UnaryOp::PreInc:
		case holy::ast::UnaryOp::PostInc:
			return "++";
		case holy::ast::UnaryOp::PreDec:
		case holy::ast::UnaryOp::PostDec:
			return "--";
		default:
			return nullptr;
		}
	}

	inline const char* binaryOpSpelling(holy::ast::BinaryOp op) {
		switch (op) {
		case holy::ast::BinaryOp::Add:
			return "+";
		case holy::ast::BinaryOp::Sub:
			return "-";
		case holy::ast::BinaryOp::Mul:
			return "*";
		case holy::ast::BinaryOp::Div:
			return "/";
		case holy::ast::BinaryOp::Mod:
			return "%";
		case holy::ast::BinaryOp::Shl:
			return "<<";
		case holy::ast::BinaryOp::Shr:
			return ">>";
		case holy::ast::BinaryOp::BitAnd:
			return "&";
		case holy::ast::BinaryOp::BitOr:
			return "|";
		case holy::ast::BinaryOp::BitXor:
			return "^";
		case holy::ast::BinaryOp::LogicalAnd:
			return "&&";
		case holy::ast::BinaryOp::LogicalOr:
			return "||";
		case holy::ast::BinaryOp::LogicalXor:
			return "^^";
		case holy::ast::BinaryOp::Pow:
			return "**";
		case holy::ast::BinaryOp::Eq:
			return "==";
		case holy::ast::BinaryOp::Ne:
			return "!=";
		case holy::ast::BinaryOp::Lt:
			return "<";
		case holy::ast::BinaryOp::Le:
			return "<=";
		case holy::ast::BinaryOp::Gt:
			return ">";
		case holy::ast::BinaryOp::Ge:
			return ">=";
		default:
			return nullptr;
		}
	}

	inline const char* assignOpSpelling(holy::ast::AssignOp op) {
		switch (op) {
		case holy::ast::AssignOp::Assign:
			return "=";
		case holy::ast::AssignOp::AddAssign:
			return "+=";
		case holy::ast::AssignOp::SubAssign:
			return "-=";
		case holy::ast::AssignOp::MulAssign:
			return "*=";
		case holy::ast::AssignOp::DivAssign:
			return "/=";
		case holy::ast::AssignOp::ModAssign:
			return "%=";
		case holy::ast::AssignOp::ShlAssign:
			return "<<=";
		case holy::ast::AssignOp::ShrAssign:
			return ">>=";
		case holy::ast::AssignOp::AndAssign:
			return "&=";
		case holy::ast::AssignOp::OrAssign:
			return "|=";
		case holy::ast::AssignOp::XorAssign:
			return "^=";
		default:
			return nullptr;
		}
	}
}