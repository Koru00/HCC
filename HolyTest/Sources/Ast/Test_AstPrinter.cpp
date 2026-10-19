#include <HolyTest/Core/TestHarness.h>

#include <HolyAST/AstPrinter.h>
#include <HolyAST/Decl.h>
#include <HolyAST/Stmt.h>
#include <HolyAST/Expr.h>
#include <HolyAST/Type.h>

#include <string>

using namespace holy::core;
using namespace holy::ast;

TEST(ASTPrinter) {
	Arena arena(1 << 20);
	StringInterner interner(arena);
	const SourceLoc loc{};

	auto* returnType = arena.New<BuiltinTypeNode>(BuiltinType::U0, loc);
	auto* variableType = arena.New<BuiltinTypeNode>(BuiltinType::I64, loc);
	Atom mainName = interner.Intern("Main");
	Atom variableName = interner.Intern("i");

	Vector<Decl*> variables(arena);
	variables.PushBack(arena.New<VarDecl>(variableType, variableName, nullptr, DeclFlags::None, Atom{}, loc));
	auto* variableStatement = arena.New<DeclStmt>(variables, loc);

	auto* initTarget = arena.New<NameExpr>(variableName, loc);
	auto* initValue = arena.New<IntLiteralExpr>(0, false, loc);
	auto* initAssignment = arena.New<AssignExpr>(AssignOp::Assign, initTarget, initValue, loc);
	auto* initStatement = arena.New<ExprStmt>(initAssignment, loc);

	auto* conditionLhs = arena.New<NameExpr>(variableName, loc);
	auto* conditionRhs = arena.New<IntLiteralExpr>(3, false, loc);
	auto* condition = arena.New<BinaryExpr>(BinaryOp::Lt, conditionLhs, conditionRhs, loc);

	auto* stepOperand = arena.New<NameExpr>(variableName, loc);
	auto* step = arena.New<UnaryExpr>(UnaryOp::PostInc, stepOperand, loc);

	auto* format = arena.New<StringLiteralExpr>(interner.Intern("%d\n"), loc);
	Vector<Expr*> args(arena);
	args.PushBack(arena.New<NameExpr>(variableName, loc));
	auto* print = arena.New<PrintStmt>(format, args, loc);

	auto* loop = arena.New<ForStmt>(initStatement, condition, step, print, loc);
	Vector<Stmt*> functionBodyStatements(arena);
	functionBodyStatements.PushBack(variableStatement);
	functionBodyStatements.PushBack(loop);
	auto* functionBody = arena.New<CompoundStmt>(functionBodyStatements, loc);

	Vector<ParamDecl*> params(arena);
	auto* function = arena.New<FuncDecl>(returnType, mainName, params, false, functionBody, DeclFlags::None, Atom{}, loc);
	Vector<Decl*> declarations(arena);
	declarations.PushBack(function);
	auto* topLevel = arena.New<DeclStmt>(declarations, loc);

	TranslationUnit unit;
	unit.items = Vector<Stmt*>(arena);
	unit.items.PushBack(topLevel);

	AstPrinter printer(interner);
	const std::string expected =
		"DeclStmt\n"
		"  FuncDecl name=Main variadic=false flags=none asm=\n"
		"    returnType: BuiltinType name=U0\n"
		"    body: CompoundStmt\n"
		"      DeclStmt\n"
		"        VarDecl name=i flags=none asm=\n"
		"          type: BuiltinType name=I64\n"
		"          init: <none>\n"
		"      ForStmt\n"
		"        init: ExprStmt\n"
		"          expr: AssignExpr op==\n"
		"            target: NameExpr name=i\n"
		"            value: IntLiteral value=0 char=false\n"
		"        cond: BinaryExpr op=<\n"
		"          lhs: NameExpr name=i\n"
		"          rhs: IntLiteral value=3 char=false\n"
		"        step: UnaryExpr op=++ postfix=true\n"
		"          operand: NameExpr name=i\n"
		"        body: PrintStmt\n"
		"          format: StringLiteral text=\"%d\\n\"\n"
		"          args[0]: NameExpr name=i\n";
	CHECK(printer.Print(unit) == expected);
}