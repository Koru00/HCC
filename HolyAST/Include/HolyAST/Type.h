#pragma once

#include <HolyCore/StringInterner.h>
#include <HolyCore/Vector.h>
#include <HolyCore/SourceManager.h>
#include <cassert>

namespace holy::ast {
	enum class TypeKind {
		Builtin,
		Named,
		Pointer,
		Array,
		Function
	};

	enum class BuiltinType {
		U0,
		I8, U8,
		I16, U16,
		I32, U32,
		I64, U64,
		F64,
		Bool
	};

	// Forward declaration
	struct Expr;

	// Base class
	struct TypeNode {
		TypeKind kind;
		core::SourceLoc loc;

		TypeNode(TypeKind k, core::SourceLoc loc) : kind(k), loc(loc) {}
	};

	template<class T>
	inline T* As(TypeNode* node) {
		assert(node != nullptr);
		assert(node->kind == T::StaticKind);
		return static_cast<T*>(node);
	}

	template<class T>
	inline const T* As(const TypeNode* node) {
		assert(node != nullptr);
		assert(node->kind == T::StaticKind);
		return static_cast<const T*>(node);
	}

	struct BuiltinTypeNode : public TypeNode {
		static constexpr TypeKind StaticKind = TypeKind::Builtin;
		BuiltinType type;

		BuiltinTypeNode(BuiltinType t, core::SourceLoc loc)
			: TypeNode(StaticKind, loc), type(t) {}
	};
	static_assert(std::is_trivially_destructible_v<BuiltinTypeNode>);

	struct NamedTypeNode : public TypeNode {
		static constexpr TypeKind StaticKind = TypeKind::Named;
		core::Atom name;

		NamedTypeNode(core::Atom n, core::SourceLoc loc)
			: TypeNode(StaticKind, loc), name(n) {}
	};
	static_assert(std::is_trivially_destructible_v<NamedTypeNode>);

	struct PointerTypeNode : public TypeNode {
		static constexpr TypeKind StaticKind = TypeKind::Pointer;
		TypeNode* pointee;

		PointerTypeNode(TypeNode* p, core::SourceLoc loc)
			: TypeNode(StaticKind, loc), pointee(p) {}
	};
	static_assert(std::is_trivially_destructible_v<PointerTypeNode>);

	struct ArrayTypeNode : public TypeNode {
		static constexpr TypeKind StaticKind = TypeKind::Array;
		TypeNode* element;
		Expr* size;

		ArrayTypeNode(TypeNode* el, Expr* sz, core::SourceLoc loc)
			: TypeNode(StaticKind, loc), element(el), size(sz) {}
	};
	static_assert(std::is_trivially_destructible_v<ArrayTypeNode>);

	struct FunctionTypeNode : public TypeNode {
		static constexpr TypeKind StaticKind = TypeKind::Function;
		TypeNode* returnType;
		core::Vector<TypeNode*> paramTypes;
		bool isVariadic;

		FunctionTypeNode(TypeNode* ret, core::Vector<TypeNode*> params, bool variadic, core::SourceLoc loc)
			: TypeNode(StaticKind, loc), returnType(ret), paramTypes(std::move(params)), isVariadic(variadic) {}
	};
	static_assert(std::is_trivially_destructible_v<FunctionTypeNode>);

	inline const char* BuiltinTypeName(BuiltinType type) {
		switch (type) {
		case holy::ast::BuiltinType::U0:
			return "U0";
			break;
		case holy::ast::BuiltinType::I8:
			return "I8";
			break;
		case holy::ast::BuiltinType::U8:
			return "U8";
			break;
		case holy::ast::BuiltinType::I16:
			return "I16";
			break;
		case holy::ast::BuiltinType::U16:
			return "U16";
			break;
		case holy::ast::BuiltinType::I32:
			return "I32";
			break;
		case holy::ast::BuiltinType::U32:
			return "U32";
			break;
		case holy::ast::BuiltinType::I64:
			return "I64";
			break;
		case holy::ast::BuiltinType::U64:
			return "U64";
			break;
		case holy::ast::BuiltinType::F64:
			return "F64";
			break;
		case holy::ast::BuiltinType::Bool:
			return "Bool";
			break;
		default:
			return "";
			break;
		}
	}
}