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
				FnOverloading,
				Fn,
				Struct,
				Class,
				Namespace,
				TypeName,
				Stmt,
				Expr
			};

			class ASTNode : std::enable_shared_from_this<ASTNode> {
			public:
				NodeKind nodeKind;

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

			class AbstractModule : public AbstractMember {
			public:
				std::string name;
				std::unordered_map<std::string, std::shared_ptr<AbstractMember>> publicMembers;
				std::unordered_map<std::string, std::shared_ptr<AbstractMember>> protectedMembers;

				AbstractModule(NodeKind nodeKind, std::string &&name);

				void addPublicMember(std::string &&name, std::shared_ptr<AbstractMember> memberNode);
				void addProtectedMember(std::string &&name, std::shared_ptr<AbstractMember> memberNode);
				std::shared_ptr<AbstractMember> getMember();
				void removeMember(const std::string &name);
			};

			using ParamList = std::vector<Type>;
			struct FnOverloadingSignature {
				bool isConst;
				ParamList paramTypes;
			};

			struct FnOverloadingSignatureComparator {
				bool operator()(const FnOverloadingSignature &lhs, const FnOverloadingSignature &rhs);
			};

			struct FnOverloadingProperties {
				bool isStatic : 1;
				bool isOverriden : 1;
				bool isVirtual : 1;
			};

			class Stmt;

			class FnOverloading : public ASTNode {
			public:
				Type returnType;
				ParamList paramTypes;
				FnOverloadingProperties properties;
				std::vector<std::shared_ptr<Stmt>> body;

				FnOverloading();
				virtual ~FnOverloading();
			};

			class Fn : public AbstractMember {
			public:
				std::map<FnOverloadingSignature, std::shared_ptr<FnOverloading>, FnOverloadingSignatureComparator> overloadings;

				Fn(std::string &&name);
				virtual ~Fn();
			};

			class Struct : public AbstractModule {
			public:
				Struct(std::string &&name);
				virtual ~Struct();
			};

			class Class : public AbstractModule {
			public:
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
				Array,
				Custom
			};

			class TypeName : public ASTNode {
			public:
				bool isConst = false;
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
				size_t rank;

				ArrayTypeName(std::shared_ptr<TypeName> elementType, size_t rank);
				virtual ~ArrayTypeName();
			};

			class PointerTypeName : public TypeName {
			public:
				std::shared_ptr<TypeName> pointedType;

				PointerTypeName(std::shared_ptr<TypeName> pointedType);
				virtual ~PointerTypeName();
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
				Return
			};

			class Stmt : public ASTNode {
			public:
				StmtKind stmtKind;

				Stmt(StmtKind stmtKind);
				virtual ~Stmt();
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
				NullptrLiteral,
				Id,

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

			class NullptrLiteralExpr : public Expr {
			public:
				NullptrLiteralExpr();
				virtual ~NullptrLiteralExpr();
			};

			class IdExpr : public Expr {
			public:
				std::string name;

				IdExpr(std::string &&name);
				virtual ~IdExpr();
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

				Scope,
				MemberAccess,
				PtrAccess,
				TypedMemberAccess,
				TypedPtrAccess
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
		}
	}
}

#endif
