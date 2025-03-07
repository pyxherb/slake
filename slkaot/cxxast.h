#ifndef _SLKAOT_CXXAST_H_
#define _SLKAOT_CXXAST_H_

#include <slake/runtime.h>
#include <string>
#include <map>
#include <unordered_map>

namespace slake {
	namespace slkaot {
		namespace cxxast {
			class AbstractModule;

			enum class NodeKind : uint8_t {
				Directive,
				IncludeDirective,
				IfDirective,
				IfdefDirective,
				IfndefDirective,
				FnOverloading,
				Fn,
				Struct,
				Class,
				Namespace,
				Var,
				TypeName,
				Stmt,
				Expr,
			};

			class ASTNode {
			public:
				NodeKind nodeKind;
				std::vector<std::shared_ptr<ASTNode>> defPrecedingNodes;
				std::vector<std::shared_ptr<ASTNode>> defTrailingNodes;
				std::vector<std::shared_ptr<ASTNode>> declPrecedingNodes;
				std::vector<std::shared_ptr<ASTNode>> declTrailingNodes;

				ASTNode(NodeKind nodeKind);
				virtual ~ASTNode();
			};

			class AbstractMember : public ASTNode {
			public:
				std::weak_ptr<AbstractModule> parent;
				std::string name;

				AbstractMember(NodeKind nodeKind, std::string &&name);
				virtual ~AbstractMember();
			};

			struct GenericParam {
				std::string name;
			};
			using GenericParamList = std::vector<GenericParam>;

			class AbstractModule : public AbstractMember, public std::enable_shared_from_this<AbstractModule> {
			public:
				std::unordered_map<std::string_view, std::shared_ptr<AbstractMember>> publicMembers;
				std::unordered_map<std::string_view, std::shared_ptr<AbstractMember>> protectedMembers;

				AbstractModule(NodeKind nodeKind, std::string &&name);
				virtual ~AbstractModule();

				void addPublicMember(std::shared_ptr<AbstractMember> memberNode);
				void addProtectedMember(std::shared_ptr<AbstractMember> memberNode);
				std::shared_ptr<AbstractMember> getMember(const std::string_view &name);
				void removeMember(const std::string_view &name);
			};

			class TypeName;

			using ParamList = std::vector<std::shared_ptr<TypeName>>;
			using GenericArgList = std::vector<std::shared_ptr<TypeName>>;
			struct FnOverloadingSignature {
				bool isConst = false;
				ParamList paramTypes;
				GenericParamList genericParams;
			};

			struct FnOverloadingProperties {
				bool isPublic : 1;
				bool isStatic : 1;
				bool isOverriden : 1;
				bool isVirtual : 1;
				bool isNothrow : 1;
			};

			class Stmt;

			class Fn : public AbstractMember, public std::enable_shared_from_this<Fn> {
			public:
				std::shared_ptr<TypeName> returnType;
				FnOverloadingSignature signature;
				FnOverloadingProperties properties;
				std::vector<std::shared_ptr<Stmt>> body;
				HostObjectRef<FnOverloadingObject> rtOverloading;

				Fn(std::string &&name);
				virtual ~Fn();
			};

			class Struct : public AbstractModule {
			public:
				GenericParamList genericParams;

				Struct(std::string &&name);
				virtual ~Struct();
			};

			class Class : public AbstractModule {
			public:
				GenericParamList genericParams;

				Class(std::string &&name);
				virtual ~Class();
			};

			class Namespace : public AbstractModule {
			public:
				Namespace(std::string &&name);
				virtual ~Namespace();
			};

			enum class TypeNameKind : uint8_t {
				Void = 0,
				Int,
				Bool,
				Char,
				Float,
				Double,
				Pointer,
				FnPointer,
				Array,
				Ref,
				Rvalue,
				Custom
			};

			class TypeName : public ASTNode {
			public:
				bool isConst = false;
				bool isVolatile = false;
				TypeNameKind typeNameKind;

				TypeName(TypeNameKind typeNameKind);
				virtual ~TypeName();
			};

			class VoidTypeName : public TypeName {
			public:
				VoidTypeName();
				virtual ~VoidTypeName();
			};

			enum class SignKind : uint8_t {
				Unspecified = 0,
				Signed,
				Unsigned
			};

			enum class IntModifierKind : uint8_t {
				Unspecified = 0,
				Short,
				Long,
				LongLong,
			};

			class IntTypeName : public TypeName {
			public:
				SignKind signKind;
				IntModifierKind modifierKind;

				IntTypeName(SignKind signKind, IntModifierKind modifierKind);
				virtual ~IntTypeName();
			};

			class BoolTypeName : public TypeName {
			public:
				BoolTypeName();
				virtual ~BoolTypeName();
			};

			class CharTypeName : public TypeName {
			public:
				SignKind signKind;

				CharTypeName(SignKind signKind);
				virtual ~CharTypeName();
			};

			class FloatTypeName : public TypeName {
			public:
				FloatTypeName();
				virtual ~FloatTypeName();
			};

			class DoubleTypeName : public TypeName {
			public:
				DoubleTypeName();
				virtual ~DoubleTypeName();
			};

			class ArrayTypeName : public TypeName {
			public:
				std::shared_ptr<TypeName> elementType;

				ArrayTypeName(std::shared_ptr<TypeName> elementType);
				virtual ~ArrayTypeName();
			};

			class PointerTypeName : public TypeName {
			public:
				std::shared_ptr<TypeName> pointedType;

				PointerTypeName(std::shared_ptr<TypeName> pointedType);
				virtual ~PointerTypeName();
			};

			class FnPointerTypeName : public TypeName {
			public:
				std::shared_ptr<TypeName> returnType;
				std::vector<std::shared_ptr<TypeName>> paramTypes;
				bool hasVarArgs;

				FnPointerTypeName(std::shared_ptr<TypeName> returnType,
					std::vector<std::shared_ptr<TypeName>> &&paramTypes,
					bool hasVarArgs);
				virtual ~FnPointerTypeName();
			};

			class RefTypeName : public TypeName {
			public:
				std::shared_ptr<TypeName> refType;

				RefTypeName(std::shared_ptr<TypeName> refType);
				virtual ~RefTypeName();
			};

			class RvalueTypeName : public TypeName {
			public:
				std::shared_ptr<TypeName> refType;

				RvalueTypeName(std::shared_ptr<TypeName> refType);
				virtual ~RvalueTypeName();
			};

			class Expr;

			class CustomTypeName : public TypeName {
			public:
				bool hasTypeNamePrefix;
				std::shared_ptr<Expr> refTarget;

				CustomTypeName(bool hasTypeNamePrefix, std::shared_ptr<Expr> refTarget);
				virtual ~CustomTypeName();
			};

			enum class StmtKind : uint8_t {
				Expr = 0,
				LocalVarDef,
				If,
				For,
				While,
				Break,
				Continue,
				Return,
				Block,
				Label,
				Goto
			};

			class Stmt : public ASTNode {
			public:
				StmtKind stmtKind;

				Stmt(StmtKind stmtKind);
				virtual ~Stmt();
			};

			class ExprStmt : public Stmt {
			public:
				std::shared_ptr<Expr> expr;

				ExprStmt(std::shared_ptr<Expr> expr);
				virtual ~ExprStmt();
			};

			struct VarDefPair {
				std::string name;
				std::shared_ptr<Expr> initialValue;
				size_t arrayLength = SIZE_MAX;
			};

			class LocalVarDefStmt : public Stmt {
			public:
				std::shared_ptr<TypeName> type;
				std::vector<VarDefPair> varDefPairs;

				LocalVarDefStmt(std::shared_ptr<TypeName> type,
					std::vector<VarDefPair> &&varDefPairs);
				virtual ~LocalVarDefStmt();
			};

			class IfStmt : public Stmt {
			public:
				std::shared_ptr<Expr> condition;
				std::shared_ptr<Stmt> trueBranch, elseBranch;

				IfStmt(
					std::shared_ptr<Expr> condition,
					std::shared_ptr<Stmt> trueBranch,
					std::shared_ptr<Stmt> elseBranch);
				virtual ~IfStmt();
			};

			class ForStmt : public Stmt {
			public:
				std::shared_ptr<LocalVarDefStmt> varDefs;
				std::shared_ptr<Expr> condition, endExpr;
				std::shared_ptr<Stmt> body;

				ForStmt(
					std::shared_ptr<LocalVarDefStmt> varDefs,
					std::shared_ptr<Expr> condition,
					std::shared_ptr<Expr> endExpr,
					std::shared_ptr<Stmt> body);
				virtual ~ForStmt();
			};

			class WhileStmt : public Stmt {
			public:
				std::shared_ptr<Expr> condition;
				std::shared_ptr<Stmt> body;

				WhileStmt(
					std::shared_ptr<Expr> condition,
					std::shared_ptr<Stmt> body);
				virtual ~WhileStmt();
			};

			class BreakStmt : public Stmt {
			public:
				BreakStmt();
				virtual ~BreakStmt();
			};

			class ContinueStmt : public Stmt {
			public:
				ContinueStmt();
				virtual ~ContinueStmt();
			};

			class ReturnStmt : public Stmt {
			public:
				std::shared_ptr<Expr> value;

				ReturnStmt(std::shared_ptr<Expr> value);
				virtual ~ReturnStmt();
			};

			class BlockStmt : public Stmt {
			public:
				std::vector<std::shared_ptr<Stmt>> body;

				BlockStmt(std::vector<std::shared_ptr<Stmt>> &&body = {});
				virtual ~BlockStmt();
			};

			class LabelStmt : public Stmt {
			public:
				std::string name;

				LabelStmt(std::string &&name);
				virtual ~LabelStmt();
			};

			class GotoStmt : public Stmt {
			public:
				std::string name;

				GotoStmt(std::string &&name);
				virtual ~GotoStmt();
			};

			enum class ExprKind : uint8_t {
				IntLiteral = 0,
				LongLiteral,
				UIntLiteral,
				ULongLiteral,
				CharLiteral,
				StringLiteral,
				FloatLiteral,
				DoubleLiteral,
				BoolLiteral,
				NullptrLiteral,
				Id,
				InitializerList,

				Unary,
				Binary,
				Call,
				Cast,
				New,
				Conditional,
				TypeSizeof,
				ExprSizeof,
				This
			};

			class Expr : public ASTNode {
			public:
				ExprKind exprKind;

				Expr(ExprKind exprKind);
				virtual ~Expr();
			};

			template <typename T, ExprKind EK>
			class LiteralExpr : public Expr {
			public:
				T data;

				SLAKE_FORCEINLINE LiteralExpr(T &&data) : Expr(EK), data(data) {}
				virtual inline ~LiteralExpr() {}
			};

			using IntLiteralExpr = LiteralExpr<int, ExprKind::IntLiteral>;
			using LongLiteralExpr = LiteralExpr<long long, ExprKind::LongLiteral>;
			using UIntLiteralExpr = LiteralExpr<unsigned int, ExprKind::UIntLiteral>;
			using ULongLiteralExpr = LiteralExpr<unsigned long long, ExprKind::ULongLiteral>;
			using CharLiteralExpr = LiteralExpr<char, ExprKind::CharLiteral>;
			using StringLiteralExpr = LiteralExpr<std::string, ExprKind::StringLiteral>;
			using FloatLiteralExpr = LiteralExpr<float, ExprKind::FloatLiteral>;
			using DoubleLiteralExpr = LiteralExpr<double, ExprKind::DoubleLiteral>;
			using BoolLiteralExpr = LiteralExpr<bool, ExprKind::BoolLiteral>;

			class NullptrLiteralExpr : public Expr {
			public:
				NullptrLiteralExpr();
				virtual ~NullptrLiteralExpr();
			};

			class IdExpr : public Expr {
			public:
				std::string name;
				GenericArgList genericArgs;

				IdExpr(std::string &&name, GenericArgList &&genericArgs = {});
				virtual ~IdExpr();
			};

			class InitializerListExpr : public Expr {
			public:
				std::shared_ptr<TypeName> type;
				std::vector<std::shared_ptr<Expr>> args;

				InitializerListExpr(
					std::shared_ptr<TypeName> type,
					std::vector<std::shared_ptr<Expr>> &&args);
				virtual ~InitializerListExpr();
			};

			enum class UnaryOp {
				IncForward = 0,
				IncBackward,
				DecForward,
				DecBackward,
				Plus,
				Negate,
				Not,
				LNot,
				Dereference,
				AddressOf,
			};

			class UnaryExpr : public Expr {
			public:
				UnaryOp op;
				std::shared_ptr<Expr> operand;

				UnaryExpr(UnaryOp op, std::shared_ptr<Expr> operand);
				virtual ~UnaryExpr();
			};

			enum class BinaryOp {
				Add = 0,
				Sub,
				Mul,
				Div,
				Mod,
				And,
				Or,
				Xor,
				LAnd,
				LOr,
				Shl,
				Shr,
				Eq,
				Neq,
				Lt,
				Gt,
				LtEq,
				GtEq,
				Cmp,
				Subscript,

				Assign,

				Scope,
				MemberAccess,
				PtrAccess,
			};

			class BinaryExpr : public Expr {
			public:
				BinaryOp op;
				std::shared_ptr<Expr> lhs, rhs;

				BinaryExpr(BinaryOp op, std::shared_ptr<Expr> lhs, std::shared_ptr<Expr> rhs);
				virtual ~BinaryExpr();
			};

			class CallExpr : public Expr {
			public:
				std::shared_ptr<Expr> callee;
				std::vector<std::shared_ptr<Expr>> args;

				CallExpr(std::shared_ptr<Expr> callee, std::vector<std::shared_ptr<Expr>> &&args = {});
				virtual ~CallExpr();
			};

			class CastExpr : public Expr {
			public:
				std::shared_ptr<TypeName> destType;
				std::shared_ptr<Expr> source;

				CastExpr(std::shared_ptr<TypeName> destType, std::shared_ptr<Expr> source);
				virtual ~CastExpr();
			};

			class NewExpr : public Expr {
			public:
				std::shared_ptr<TypeName> type;
				std::vector<std::shared_ptr<Expr>> args;

				NewExpr(std::shared_ptr<TypeName> type, std::vector<std::shared_ptr<Expr>> &&args = {});
				virtual ~NewExpr();
			};

			class ConditionalExpr : public Expr {
			public:
				std::shared_ptr<Expr> condition, trueExpr, falseExpr;

				ConditionalExpr(std::shared_ptr<Expr> condition, std::shared_ptr<Expr> trueExpr, std::shared_ptr<Expr> falseExpr);
				virtual ~ConditionalExpr();
			};

			class TypeSizeofExpr : public Expr {
			public:
				std::shared_ptr<TypeName> target;

				TypeSizeofExpr(std::shared_ptr<TypeName> target);
				virtual ~TypeSizeofExpr();
			};

			class ExprSizeofExpr : public Expr {
			public:
				std::shared_ptr<Expr> target;

				ExprSizeofExpr(std::shared_ptr<Expr> target);
				virtual ~ExprSizeofExpr();
			};

			class ThisExpr : public Expr {
			public:
				ThisExpr();
				virtual ~ThisExpr();
			};

			class Directive : public ASTNode {
			public:
				std::string name;
				std::vector<std::shared_ptr<Expr>> args;

				Directive(std::string &&name, std::vector<std::shared_ptr<Expr>> &&args);
				virtual ~Directive();
			};

			class IncludeDirective : public ASTNode {
			public:
				std::string name;
				bool isSystem;

				IncludeDirective(
					std::string &&name,
					bool isSystem);
				virtual ~IncludeDirective();
			};

			class IfDirective : public ASTNode {
			public:
				std::shared_ptr<Expr> condition;
				std::shared_ptr<ASTNode> trueBranch, elseBranch;

				IfDirective(std::shared_ptr<Expr> condition,
					std::shared_ptr<ASTNode> trueBranch,
					std::shared_ptr<ASTNode> elseBranch);
				virtual ~IfDirective();
			};

			class IfdefDirective : public ASTNode {
			public:
				std::string name;
				std::shared_ptr<ASTNode> trueBranch, elseBranch;

				IfdefDirective(
					std::string &&name,
					std::shared_ptr<ASTNode> trueBranch,
					std::shared_ptr<ASTNode> elseBranch);
				virtual ~IfdefDirective();
			};

			class IfndefDirective : public ASTNode {
			public:
				std::string name;
				std::vector<std::shared_ptr<ASTNode>> trueBranch, elseBranch;

				IfndefDirective(
					std::string &&name,
					std::vector<std::shared_ptr<ASTNode>> &&trueBranch,
					std::vector<std::shared_ptr<ASTNode>> &&elseBranch);
				virtual ~IfndefDirective();
			};

			enum class StorageClass {
				Unspecified = 0,
				Static,
				Extern,
				Mutable
			};

			class Var : public AbstractMember {
			public:
				StorageClass storageClass;
				std::shared_ptr<TypeName> type;
				std::shared_ptr<Expr> initialValue;

				Var(
					std::string &&name,
					StorageClass storageClass,
					std::shared_ptr<TypeName> type,
					std::shared_ptr<Expr> initialValue);
				virtual ~Var();
			};

			std::shared_ptr<Expr> getFullRefOf(std::shared_ptr<AbstractMember> member);
		}
	}
}

#endif
