#ifndef _SLKC_COMP_COMPILER_H_
#define _SLKC_COMP_COMPILER_H_

#include "../ast/parser.h"

namespace slkc {
	enum class CompilationErrorKind : int {
		OutOfMemory = 0,
		ExpectingLValueExpr,
		TargetIsNotCallable,
		NoSuchFnOverloading,
	};

	struct CompilationError {
		TokenRange tokenRange;
		CompilationErrorKind errorKind;
		std::variant<std::monostate> exData;

		SLAKE_FORCEINLINE CompilationError(
			const TokenRange &tokenRange,
			CompilationErrorKind errorKind)
			: tokenRange(tokenRange),
			  errorKind(errorKind) {
		}
	};
	enum class CompilationWarningKind : int {
		UnusedExprResult = 0,
	};

	SLAKE_FORCEINLINE CompilationError genOutOfMemoryCompError() {
		return CompilationError(TokenRange{ 0, 0 }, CompilationErrorKind::OutOfMemory);
	}

	struct CompilationWarning {
		TokenRange tokenRange;
		CompilationWarningKind warningKind;
		std::variant<std::monostate> exData;

		SLAKE_FORCEINLINE CompilationWarning(
			const TokenRange &tokenRange,
			CompilationWarningKind warningKind)
			: tokenRange(tokenRange),
			  warningKind(warningKind) {
		}
	};

	enum class ExprEvalPurpose {
		None,	 // None
		Stmt,	 // As a statement
		LValue,	 // As a lvalue
		RValue,	 // As a rvalue
		Call,	 // As target of a calling expression
	};

	struct StmtCompileContext {
	};

	struct BlockCompileContext {
	};

	struct FnCompileContext {
		peff::RcObjectPtr<peff::Alloc> allocator;
		peff::SharedPtr<Document> document;
		peff::DynArray<slake::Instruction> instructionsOut;
		peff::HashMap<peff::String, uint32_t> labels;
		peff::DynArray<CompilationError> errors;
		peff::DynArray<CompilationWarning> warnings;
		uint32_t nTotalRegs = 0;

		PEFF_FORCEINLINE FnCompileContext(peff::Alloc *allocator) : allocator(allocator), instructionsOut(allocator), labels(allocator), errors(allocator), warnings(allocator) {}

		PEFF_FORCEINLINE std::optional<CompilationError> pushIns(slake::Instruction &&ins) {
			if (!instructionsOut.pushBack(std::move(ins)))
				return genOutOfMemoryCompError();

			return {};
		}

		PEFF_FORCEINLINE std::optional<CompilationError> pushError(CompilationError &&error) {
			if (!errors.pushBack(std::move(error)))
				return genOutOfMemoryCompError();

			return {};
		}

		PEFF_FORCEINLINE std::optional<CompilationError> pushWarning(CompilationWarning &&warning) {
			if (!warnings.pushBack(std::move(warning)))
				return genOutOfMemoryCompError();

			return {};
		}

		PEFF_FORCEINLINE void defineReg(uint32_t index) {
			++nTotalRegs;
		}
	};

	SLAKE_FORCEINLINE slake::Instruction emitIns(slake::Opcode opcode, uint32_t outputRegIndex) {
		slake::Instruction ins;

		ins.opcode = slake::Opcode::MOV;
		ins.output = outputRegIndex;
		ins.nOperands = 0;
	}

	SLAKE_FORCEINLINE slake::Instruction emitIns(slake::Opcode opcode, uint32_t outputRegIndex, const std::initializer_list<slake::Value> &operands) {
		slake::Instruction ins;

		if (operands.size() > 3) {
			std::terminate();
		}

		ins.opcode = slake::Opcode::MOV;
		ins.output = outputRegIndex;
		ins.nOperands = operands.size();

		auto it = operands.begin();
		for (size_t i = 0; i < operands.size(); ++i) {
			ins.operands[i] = *it++;
		}

		return ins;
	}

	struct CustomTypeNameResolveContext {
		peff::Set<peff::SharedPtr<MemberNode>> resolvedMemberNodes;

		PEFF_FORCEINLINE CustomTypeNameResolveContext(peff::Alloc *allocator) : resolvedMemberNodes(allocator) {}
	};

	struct ImplementationDetectionContext {
		peff::Set<peff::SharedPtr<InterfaceNode>> walkedInterfaces;

		PEFF_FORCEINLINE ImplementationDetectionContext(peff::Alloc *allocator) : walkedInterfaces(allocator) {}
	};

	class Compiler {
	public:
		static peff::SharedPtr<MemberNode> resolveStaticMember(
			FnCompileContext &compileContext,
			const peff::SharedPtr<MemberNode> &memberNode,
			const IdRefEntry &name);
		static [[nodiscard]]
		std::optional<CompilationError> resolveInstanceMember(
			FnCompileContext &compileContext,
			peff::SharedPtr<MemberNode> memberNode,
			const IdRefEntry &name,
			peff::SharedPtr<MemberNode> &memberOut);
		static [[nodiscard]]
		std::optional<CompilationError> resolveIdRef(
			FnCompileContext &compileContext,
			const peff::SharedPtr<MemberNode> &resolveRoot,
			IdRef *idRef,
			peff::SharedPtr<MemberNode> &memberOut,
			bool isStatic = true);
		/// @brief Resolve a custom type name.
		/// @param compileContext The compile context.
		/// @param resolveContext Previous resolve context.
		/// @param typeName Type name to be resolved.
		/// @param memberNodeOut Where the resolved member node will be stored.
		/// @return Indicates if fatal error does not occur.
		static [[nodiscard]]
		std::optional<CompilationError> resolveCustomTypeName(
			FnCompileContext &compileContext,
			CustomTypeNameResolveContext &resolveContext,
			const peff::SharedPtr<CustomTypeNameNode> &typeName,
			peff::SharedPtr<MemberNode> &memberNodeOut);

		static std::optional<CompilationError> collectInvolvedInterfaces(
			FnCompileContext &compileContext,
			const peff::SharedPtr<InterfaceNode> &derived,
			peff::Set<peff::SharedPtr<InterfaceNode>> &walkedInterfaces,
			bool insertSelf);
		static std::optional<CompilationError> isImplementedByInterface(
			FnCompileContext &compileContext,
			const peff::SharedPtr<InterfaceNode> &base,
			const peff::SharedPtr<InterfaceNode> &derived,
			bool &whetherOut);
		static std::optional<CompilationError> isImplementedByClass(
			FnCompileContext &compileContext,
			const peff::SharedPtr<InterfaceNode> &base,
			const peff::SharedPtr<ClassNode> &derived,
			bool &whetherOut);
		static std::optional<CompilationError> isBaseOf(
			FnCompileContext &compileContext,
			const peff::SharedPtr<ClassNode> &base,
			const peff::SharedPtr<ClassNode> &derived,
			bool &whetherOut);

		static std::optional<CompilationError> removeRefOfTypeName(
			FnCompileContext &compileContext,
			peff::SharedPtr<TypeNameNode> src,
			peff::SharedPtr<TypeNameNode> &typeNameOut);
		static std::optional<CompilationError> isTypeNamesSame(
			FnCompileContext &compileContext,
			const peff::SharedPtr<TypeNameNode> &lhs,
			const peff::SharedPtr<TypeNameNode> &rhs,
			bool &whetherOut);
		static std::optional<CompilationError> isTypeNamesConvertible(
			FnCompileContext &compileContext,
			const peff::SharedPtr<TypeNameNode> &src,
			const peff::SharedPtr<TypeNameNode> &dest,
			bool &whetherOut);
		static std::optional<CompilationError> compileUnaryExpr(
			FnCompileContext &compileContext,
			peff::SharedPtr<UnaryExprNode> expr,
			ExprEvalPurpose evalPurpose,
			uint32_t resultRegOut,
			peff::SharedPtr<TypeNameNode> &evaluatedTypeOut,
			bool evalTypeOnly = false);
		static std::optional<CompilationError> compileExpr(
			FnCompileContext &compileContext,
			const peff::SharedPtr<ExprNode> &expr,
			ExprEvalPurpose evalPurpose,
			uint32_t resultRegOut,
			peff::SharedPtr<TypeNameNode> &evaluatedTypeOut,
			bool evalTypeOnly = false);
	};
}

#define SLKC_RETURN_IF_COMP_ERROR(e) \
	if (std::optional<slkc::CompilationError> _ = (e); _) return _

#endif
