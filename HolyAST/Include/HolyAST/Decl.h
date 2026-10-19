#pragma once

#include <cstdint>
#include <utility>
#include <HolyCore/SourceManager.h>
#include <HolyCore/StringInterner.h>

namespace holy::ast {
	enum class DeclKind {
		Var,
		Param,
		Func,
		Class
	};

	enum class DeclFlags : uint32_t {
		None = 0,
		Public = 1 << 0,
		Static = 1 << 1,
		Extern = 1 << 2,
		Import = 1 << 3,
		Reg = 1 << 4,
		NoReg = 1 << 5,
		Interrupt = 1 << 6,
		HasErrCode = 1 << 7,
		ArgPop = 1 << 8,
		NoArgPop = 1 << 9
	};

	constexpr DeclFlags operator|(DeclFlags a, DeclFlags b) {
		return static_cast<DeclFlags>(static_cast<std::underlying_type_t<DeclFlags>>(a) | static_cast<std::underlying_type_t<DeclFlags>>(b));
	}

	constexpr DeclFlags operator&(DeclFlags a, DeclFlags b) {
		return static_cast<DeclFlags>(static_cast<std::underlying_type_t<DeclFlags>>(a) & static_cast<std::underlying_type_t<DeclFlags>>(b));
	}

	inline bool HasFlag(DeclFlags set, DeclFlags flag) {
		return static_cast<uint32_t>(set & flag) != 0;
	}

	// Forward declaration
	struct Expr;
	struct TypeNode;
	struct Stmt;

	struct Decl {
		DeclKind kind;
		core::SourceLoc loc;

		Decl(DeclKind kind, core::SourceLoc loc)
			: kind(kind), loc(loc) {}
	};

	template<class T>
	inline T* As(Decl* node) {
		assert(node != nullptr);
		assert(node->kind == T::StaticKind);
		return static_cast<T*>(node);
	}

	template<class T>
	inline const T* As(const Decl* node) {
		assert(node != nullptr);
		assert(node->kind == T::StaticKind);
		return static_cast<T*>(node);
	}

	struct VarDecl : public Decl {
		static constexpr DeclKind StaticKind = DeclKind::Var;
		TypeNode* type;
		core::Atom name;
		Expr* init;
		DeclFlags flags;
		core::Atom asmName;

		VarDecl(TypeNode* type, core::Atom name, Expr* init, DeclFlags flags, core::Atom asmName, core::SourceLoc loc)
			: Decl(StaticKind, loc), type(type), name(name), init(init), flags(flags), asmName(asmName) {}
	};
	static_assert(std::is_trivially_destructible_v<VarDecl>);

	struct ParamDecl : public Decl {
		static constexpr DeclKind StaticKind = DeclKind::Param;
		TypeNode* type;
		core::Atom name;
		Expr* defaultValue;

		ParamDecl(TypeNode* type, core::Atom name, Expr* defaultValue, core::SourceLoc loc)
			: Decl(StaticKind, loc), type(type), name(name), defaultValue(defaultValue) {}
	};
	static_assert(std::is_trivially_destructible_v<ParamDecl>);

	struct FuncDecl : public Decl {
		static constexpr DeclKind StaticKind = DeclKind::Func;
		TypeNode* returnType;
		core::Atom name;
		core::Vector<ParamDecl*> params;
		bool isVariadic;
		Stmt* body;
		DeclFlags flags;
		core::Atom asmName;

		FuncDecl(TypeNode* returnType, core::Atom name, core::Vector<ParamDecl*> params, bool isVariadic, Stmt* body, DeclFlags flags, core::Atom asmName, core::SourceLoc loc)
			: Decl(StaticKind, loc), returnType(returnType), name(name), params(params), isVariadic(isVariadic), body(body), flags(flags), asmName(asmName) {}
	};
	static_assert(std::is_trivially_destructible_v<FuncDecl>);

	struct ClassDecl : public Decl {
		static constexpr DeclKind StaticKind = DeclKind::Class;
		bool isUnion;
		core::Atom name;
		core::Atom baseName;
		core::Vector<VarDecl*> members;
		bool hasBody;
		DeclFlags flags;

		ClassDecl(bool isUnion, core::Atom name, core::Vector<VarDecl*> members, bool hasBody, DeclFlags flags, core::SourceLoc loc)
			: Decl(StaticKind, loc), isUnion(isUnion), name(name), members(members), hasBody(hasBody), flags(flags) {}
	};
	static_assert(std::is_trivially_destructible_v<ClassDecl>);

	struct TranslationUnit {
		core::Vector<Stmt*> items;
	};
	static_assert(std::is_trivially_destructible_v<TranslationUnit>);
}