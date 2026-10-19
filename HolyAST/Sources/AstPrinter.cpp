#include <HolyAST/AstPrinter.h>

#include <format>

namespace holy::ast {
	AstPrinter::AstPrinter(const core::StringInterner& interner) : m_interner(interner) {}

	std::string AstPrinter::Print(const TranslationUnit& unit) {
		m_out.clear();
		m_depth = 0;
		for (const Stmt* item : unit.items) {
			Print(item, "");
		}
		return m_out;
	}

	std::string AstPrinter::Print(const TypeNode& type) {
		m_out.clear();
		m_depth = 0;
		Print(&type, "");
		return m_out;
	}

	std::string AstPrinter::Print(const Expr& expr) {
		m_out.clear();
		m_depth = 0;
		Print(&expr, "");
		return m_out;
	}

	std::string AstPrinter::Print(const Stmt& stmt) {
		m_out.clear();
		m_depth = 0;
		Print(&stmt, "");
		return m_out;
	}

	std::string AstPrinter::Print(const Decl& decl) {
		m_out.clear();
		m_depth = 0;
		Print(&decl, "");
		return m_out;
	}

	void AstPrinter::Print(const TypeNode* node, std::string_view role) {
		BeginLine(role);
		if (node == nullptr) {
			m_out += "<none>";
			EndLine();
			return;
		}

		switch (node->kind) {
		case TypeKind::Builtin: {
			const auto* type = static_cast<const BuiltinTypeNode*>(node);
			m_out += "BuiltinType name=";
			m_out += BuiltinTypeName(type->type);
			EndLine();
			break;
		}
		case TypeKind::Named: {
			const auto* type = static_cast<const NamedTypeNode*>(node);
			m_out += "NamedType name=";
			WriteAtom(type->name);
			EndLine();
			break;
		}
		case TypeKind::Pointer: {
			const auto* type = static_cast<const PointerTypeNode*>(node);
			m_out += "PointerType";
			EndLine();
			++m_depth;
			Print(type->pointee, "pointee: ");
			--m_depth;
			break;
		}
		case TypeKind::Array: {
			const auto* type = static_cast<const ArrayTypeNode*>(node);
			m_out += "ArrayType";
			EndLine();
			++m_depth;
			Print(type->element, "element: ");
			Print(type->size, "size: ");
			--m_depth;
			break;
		}
		case TypeKind::Function: {
			const auto* type = static_cast<const FunctionTypeNode*>(node);
			m_out += "FunctionType variadic=";
			WriteBool(type->isVariadic);
			EndLine();
			++m_depth;
			Print(type->returnType, "returnType: ");
			for (size_t i = 0; i < type->paramTypes.Size(); ++i) {
				Print(type->paramTypes[i], IndexedRole("params", i) + ": ");
			}
			--m_depth;
			break;
		}
		default:
			m_out += "UnknownType";
			EndLine();
			break;
		}
	}

	void AstPrinter::Print(const Expr* node, std::string_view role) {
		BeginLine(role);
		if (node == nullptr) {
			m_out += "<none>";
			EndLine();
			return;
		}

		switch (node->kind) {
		case ExprKind::IntLiteral: {
			const auto* expr = static_cast<const IntLiteralExpr*>(node);
			m_out += std::format("IntLiteral value={} char=", expr->value);
			WriteBool(expr->isChar);
			EndLine();
			break;
		}
		case ExprKind::FloatLiteral: {
			const auto* expr = static_cast<const FloatLiteralExpr*>(node);
			m_out += std::format("FloatLiteral value={}", expr->value);
			EndLine();
			break;
		}
		case ExprKind::StringLiteral: {
			const auto* expr = static_cast<const StringLiteralExpr*>(node);
			m_out += "StringLiteral text=";
			WriteQuoted(m_interner.Text(expr->text));
			EndLine();
			break;
		}
		case ExprKind::Name: {
			const auto* expr = static_cast<const NameExpr*>(node);
			m_out += "NameExpr name=";
			WriteAtom(expr->name);
			EndLine();
			break;
		}
		case ExprKind::Unary: {
			const auto* expr = static_cast<const UnaryExpr*>(node);
			m_out += "UnaryExpr op=";
			const char* spelling = unaryOpSpelling(expr->op);
			m_out += spelling != nullptr ? spelling : "<unknown>";
			m_out += " postfix=";
			WriteBool(expr->op == UnaryOp::PostInc || expr->op == UnaryOp::PostDec);
			EndLine();
			++m_depth;
			Print(expr->operand, "operand: ");
			--m_depth;
			break;
		}
		case ExprKind::Binary: {
			const auto* expr = static_cast<const BinaryExpr*>(node);
			m_out += "BinaryExpr op=";
			const char* spelling = binaryOpSpelling(expr->op);
			m_out += spelling != nullptr ? spelling : "<unknown>";
			EndLine();
			++m_depth;
			Print(expr->lhs, "lhs: ");
			Print(expr->rhs, "rhs: ");
			--m_depth;
			break;
		}
		case ExprKind::Assign: {
			const auto* expr = static_cast<const AssignExpr*>(node);
			m_out += "AssignExpr op=";
			const char* spelling = assignOpSpelling(expr->op);
			m_out += spelling != nullptr ? spelling : "<unknown>";
			EndLine();
			++m_depth;
			Print(expr->target, "target: ");
			Print(expr->value, "value: ");
			--m_depth;
			break;
		}
		case ExprKind::Call: {
			const auto* expr = static_cast<const CallExpr*>(node);
			m_out += "CallExpr hasParens=";
			WriteBool(expr->hasParens);
			EndLine();
			++m_depth;
			Print(expr->calle, "callee: ");
			for (size_t i = 0; i < expr->args.Size(); ++i) {
				Print(expr->args[i], IndexedRole("args", i) + ": ");
			}
			--m_depth;
			break;
		}
		case ExprKind::Index: {
			const auto* expr = static_cast<const IndexExpr*>(node);
			m_out += "IndexExpr";
			EndLine();
			++m_depth;
			Print(expr->base, "base: ");
			Print(expr->index, "index: ");
			--m_depth;
			break;
		}
		case ExprKind::Member: {
			const auto* expr = static_cast<const MemberExpr*>(node);
			m_out += "MemberExpr member=";
			WriteAtom(expr->member);
			m_out += " arrow=";
			WriteBool(expr->viaArrow);
			EndLine();
			++m_depth;
			Print(expr->base, "base: ");
			--m_depth;
			break;
		}
		case ExprKind::Cast: {
			const auto* expr = static_cast<const CastExpr*>(node);
			m_out += "CastExpr";
			EndLine();
			++m_depth;
			Print(expr->operand, "operand: ");
			Print(expr->type, "type: ");
			--m_depth;
			break;
		}
		case ExprKind::Sizeof: {
			const auto* expr = static_cast<const SizeofExpr*>(node);
			m_out += "SizeofExpr";
			EndLine();
			++m_depth;
			Print(expr->type, "type: ");
			Print(expr->operand, "operand: ");
			--m_depth;
			break;
		}
		case ExprKind::Offsetof: {
			const auto* expr = static_cast<const OffsetofExpr*>(node);
			m_out += "OffsetofExpr member=";
			WriteAtom(expr->member);
			EndLine();
			++m_depth;
			Print(expr->type, "type: ");
			--m_depth;
			break;
		}
		default:
			m_out += "UnknownExpr";
			EndLine();
			break;
		}
	}

	void AstPrinter::Print(const Stmt* node, std::string_view role) {
		BeginLine(role);
		if (node == nullptr) {
			m_out += "<none>";
			EndLine();
			return;
		}

		switch (node->kind) {
		case StmtKind::Empty:
			m_out += "EmptyStmt";
			EndLine();
			break;
		case StmtKind::Compound: {
			const auto* stmt = static_cast<const CompoundStmt*>(node);
			m_out += "CompoundStmt";
			EndLine();
			++m_depth;
			for (const Stmt* child : stmt->stmts) {
				Print(child, "");
			}
			--m_depth;
			break;
		}
		case StmtKind::Expr: {
			const auto* stmt = static_cast<const ExprStmt*>(node);
			m_out += "ExprStmt";
			EndLine();
			++m_depth;
			Print(stmt->expr, "expr: ");
			--m_depth;
			break;
		}
		case StmtKind::Decl: {
			const auto* stmt = static_cast<const DeclStmt*>(node);
			m_out += "DeclStmt";
			EndLine();
			++m_depth;
			for (const Decl* decl : stmt->decls) {
				Print(decl, "");
			}
			--m_depth;
			break;
		}
		case StmtKind::Print: {
			const auto* stmt = static_cast<const PrintStmt*>(node);
			m_out += "PrintStmt";
			EndLine();
			++m_depth;
			Print(stmt->format, "format: ");
			for (size_t i = 0; i < stmt->args.Size(); ++i) {
				Print(stmt->args[i], IndexedRole("args", i) + ": ");
			}
			--m_depth;
			break;
		}
		case StmtKind::If: {
			const auto* stmt = static_cast<const IfStmt*>(node);
			m_out += "IfStmt";
			EndLine();
			++m_depth;
			Print(stmt->cond, "cond: ");
			Print(stmt->thenBranch, "then: ");
			Print(stmt->elseBranch, "else: ");
			--m_depth;
			break;
		}
		case StmtKind::While: {
			const auto* stmt = static_cast<const WhileStmt*>(node);
			m_out += "WhileStmt";
			EndLine();
			++m_depth;
			Print(stmt->cond, "cond: ");
			Print(stmt->body, "body: ");
			--m_depth;
			break;
		}
		case StmtKind::DoWhile: {
			const auto* stmt = static_cast<const DoWhileStmt*>(node);
			m_out += "DoWhileStmt";
			EndLine();
			++m_depth;
			Print(stmt->body, "body: ");
			Print(stmt->cond, "cond: ");
			--m_depth;
			break;
		}
		case StmtKind::For: {
			const auto* stmt = static_cast<const ForStmt*>(node);
			m_out += "ForStmt";
			EndLine();
			++m_depth;
			Print(stmt->init, "init: ");
			Print(stmt->cond, "cond: ");
			Print(stmt->step, "step: ");
			Print(stmt->body, "body: ");
			--m_depth;
			break;
		}
		case StmtKind::Switch: {
			const auto* stmt = static_cast<const SwitchStmt*>(node);
			m_out += "SwitchStmt noBounds=";
			WriteBool(stmt->isNoBounds);
			EndLine();
			++m_depth;
			Print(stmt->subject, "subject: ");
			Print(stmt->body, "body: ");
			--m_depth;
			break;
		}
		case StmtKind::Case: {
			const auto* stmt = static_cast<const CaseStmt*>(node);
			m_out += "CaseStmt default=";
			WriteBool(stmt->isDefault);
			m_out += " auto=";
			WriteBool(stmt->isAutoValue);
			EndLine();
			++m_depth;
			Print(stmt->lo, "lo: ");
			Print(stmt->hi, "hi: ");
			--m_depth;
			break;
		}
		case StmtKind::SubSwitch: {
			const auto* stmt = static_cast<const SubSwitchStmt*>(node);
			m_out += "SubSwitchStmt";
			EndLine();
			++m_depth;
			Print(stmt->body, "body: ");
			--m_depth;
			break;
		}
		case StmtKind::Break: {
			const auto* stmt = static_cast<const BreakStmt*>(node);
			m_out += "BreakStmt";
			EndLine();
			++m_depth;
			Print(stmt->levels, "levels: ");
			--m_depth;
			break;
		}
		case StmtKind::Continue: {
			const auto* stmt = static_cast<const ContinueStmt*>(node);
			m_out += "ContinueStmt";
			EndLine();
			++m_depth;
			Print(stmt->levels, "levels: ");
			--m_depth;
			break;
		}
		case StmtKind::Return: {
			const auto* stmt = static_cast<const ReturnStmt*>(node);
			m_out += "ReturnStmt";
			EndLine();
			++m_depth;
			Print(stmt->value, "value: ");
			--m_depth;
			break;
		}
		case StmtKind::Goto: {
			const auto* stmt = static_cast<const GotoStmt*>(node);
			m_out += "GotoStmt label=";
			WriteAtom(stmt->label);
			EndLine();
			break;
		}
		case StmtKind::Label: {
			const auto* stmt = static_cast<const LabelStmt*>(node);
			m_out += "LabelStmt name=";
			WriteAtom(stmt->name);
			EndLine();
			break;
		}
		case StmtKind::Try: {
			const auto* stmt = static_cast<const TryStmt*>(node);
			m_out += "TryStmt";
			EndLine();
			++m_depth;
			Print(stmt->body, "body: ");
			Print(stmt->catchBody, "catch: ");
			--m_depth;
			break;
		}
		case StmtKind::Throw: {
			const auto* stmt = static_cast<const ThrowStmt*>(node);
			m_out += "ThrowStmt";
			EndLine();
			++m_depth;
			Print(stmt->value, "value: ");
			--m_depth;
			break;
		}
		case StmtKind::Asm: {
			const auto* stmt = static_cast<const AsmStmt*>(node);
			m_out += "AsmStmt text=";
			WriteQuoted(stmt->text);
			EndLine();
			break;
		}
		case StmtKind::Lock: {
			const auto* stmt = static_cast<const LockStmt*>(node);
			m_out += "LockStmt";
			EndLine();
			++m_depth;
			Print(stmt->body, "body: ");
			--m_depth;
			break;
		}
		case StmtKind::NoWarn: {
			const auto* stmt = static_cast<const NoWarnStmt*>(node);
			m_out += "NoWarnStmt names=[";
			for (size_t i = 0; i < stmt->names.Size(); ++i) {
				if (i != 0) {
					m_out += ",";
				}
				WriteAtom(stmt->names[i]);
			}
			m_out += "]";
			EndLine();
			break;
		}
		default:
			m_out += "UnknownStmt";
			EndLine();
			break;
		}
	}

	void AstPrinter::Print(const Decl* node, std::string_view role) {
		BeginLine(role);
		if (node == nullptr) {
			m_out += "<none>";
			EndLine();
			return;
		}

		switch (node->kind) {
		case DeclKind::Var: {
			const auto* decl = static_cast<const VarDecl*>(node);
			m_out += "VarDecl name=";
			WriteAtom(decl->name);
			m_out += " flags=";
			WriteFlags(decl->flags);
			m_out += " asm=";
			WriteAtom(decl->asmName);
			EndLine();
			++m_depth;
			Print(decl->type, "type: ");
			Print(decl->init, "init: ");
			--m_depth;
			break;
		}
		case DeclKind::Param: {
			const auto* decl = static_cast<const ParamDecl*>(node);
			m_out += "ParamDecl name=";
			WriteAtom(decl->name);
			EndLine();
			++m_depth;
			Print(decl->type, "type: ");
			Print(decl->defaultValue, "default: ");
			--m_depth;
			break;
		}
		case DeclKind::Func: {
			const auto* decl = static_cast<const FuncDecl*>(node);
			m_out += "FuncDecl name=";
			WriteAtom(decl->name);
			m_out += " variadic=";
			WriteBool(decl->isVariadic);
			m_out += " flags=";
			WriteFlags(decl->flags);
			m_out += " asm=";
			WriteAtom(decl->asmName);
			EndLine();
			++m_depth;
			Print(decl->returnType, "returnType: ");
			for (size_t i = 0; i < decl->params.Size(); ++i) {
				Print(static_cast<const Decl*>(decl->params[i]), IndexedRole("params", i) + ": ");
			}
			Print(decl->body, "body: ");
			--m_depth;
			break;
		}
		case DeclKind::Class: {
			const auto* decl = static_cast<const ClassDecl*>(node);
			m_out += "ClassDecl union=";
			WriteBool(decl->isUnion);
			m_out += " name=";
			WriteAtom(decl->name);
			m_out += " base=";
			WriteAtom(decl->baseName);
			m_out += " hasBody=";
			WriteBool(decl->hasBody);
			m_out += " flags=";
			WriteFlags(decl->flags);
			EndLine();
			++m_depth;
			for (const VarDecl* member : decl->members) {
				Print(static_cast<const Decl*>(member), "");
			}
			--m_depth;
			break;
		}
		default:
			m_out += "UnknownDecl";
			EndLine();
			break;
		}
	}

	void AstPrinter::BeginLine(std::string_view role) {
		for (uint32_t i = 0; i < m_depth; ++i) {
			m_out += "  ";
		}
		m_out += role;
	}

	void AstPrinter::EndLine() {
		m_out += '\n';
	}

	void AstPrinter::WriteQuoted(std::string_view text) {
		m_out += '"';
		for (char ch : text) {
			switch (ch) {
			case '\\': m_out += "\\\\"; break;
			case '"': m_out += "\\\""; break;
			case '\n': m_out += "\\n"; break;
			case '\r': m_out += "\\r"; break;
			case '\t': m_out += "\\t"; break;
			default: m_out += ch; break;
			}
		}
		m_out += '"';
	}

	void AstPrinter::WriteAtom(core::Atom atom) {
		if (atom.IsValid()) {
			m_out += m_interner.Text(atom);
		}
	}

	void AstPrinter::WriteBool(bool boolean) {
		m_out += boolean ? "true" : "false";
	}

	void AstPrinter::WriteFlags(DeclFlags flags) {
		if (flags == DeclFlags::None) {
			m_out += "none";
			return;
		}

		bool first = true;
		const auto append = [this, &first, flags](DeclFlags flag, std::string_view name) {
			if (HasFlag(flags, flag)) {
				if (!first) {
					m_out += '|';
				}
				m_out += name;
				first = false;
			}
		};
		append(DeclFlags::Public, "public");
		append(DeclFlags::Static, "static");
		append(DeclFlags::Extern, "extern");
		append(DeclFlags::Import, "import");
		append(DeclFlags::Reg, "reg");
		append(DeclFlags::NoReg, "noReg");
		append(DeclFlags::Interrupt, "interrupt");
		append(DeclFlags::HasErrCode, "hasErrCode");
		append(DeclFlags::ArgPop, "argPop");
		append(DeclFlags::NoArgPop, "noArgPop");
	}

	std::string AstPrinter::IndexedRole(std::string_view name, size_t i) {
		return std::format("{}[{}]", name, i);
	}
}