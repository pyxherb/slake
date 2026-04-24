#ifndef _SLKC_DOCUMENT_H_
#define _SLKC_DOCUMENT_H_

#include <slkc/basedefs.h>
#include <peff/containers/dynarray.h>
#include <peff/containers/map.h>
#include <peff/advutils/shared_ptr.h>
#include "astnode.h"

namespace slkc {
#define SLKC_RETURN_IF_COMP_ERROR(...)                             \
	if (peff::Option<slkc::CompilationError> _ = (__VA_ARGS__); _) \
		return std::move(_);                                       \
	else
#define SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(lvar, ...) \
	if ((lvar = (__VA_ARGS__)))                        \
		return lvar;                                   \
	else

	enum class CompilationErrorKind : int {
		OutOfMemory = 0,
		StackOverflow,
		OutOfRuntimeMemory,
		ExpectingLValueExpr,
		ExpectingRValueExpr,
		TargetIsNotCallable,
		TargetIsNotUnpackable,
		NoSuchFnOverloading,
		IncompatibleOperand,
		OperatorNotFound,
		AmbiguousOperatorCall,
		MismatchedGenericArgNumber,
		ExpectingTypeName,
		ExpectingClassName,
		ExpectingInterfaceName,
		AbstractMethodNotImplemented,
		CyclicInheritedClass,
		CyclicInheritedInterface,
		RecursedValueType,
		ExpectingId,
		IdNotFound,
		ParamAlreadyDefined,
		GenericParamAlreadyDefined,
		InvalidInitializerListUsage,
		ErrorDeducingInitializerListType,
		ErrorDeducingSwitchConditionType,
		ErrorDeducingArgType,
		ErrorEvaluatingConstSwitchCaseCondition,
		MismatchedSwitchCaseConditionType,
		ErrorDeducingMatchConditionType,
		DuplicatedSwitchCaseBranch,
		ErrorDeducingMatchResultType,
		ErrorEvaluatingConstMatchCaseCondition,
		MismatchedMatchCaseConditionType,
		DuplicatedMatchCaseBranch,
		MissingDefaultMatchCaseBranch,
		InvalidThisUsage,
		NoMatchingFnOverloading,
		UnableToDetermineOverloading,
		ArgsMismatched,
		MemberAlreadyDefined,
		MissingBindingObject,
		RedundantWithObject,
		LocalVarAlreadyExists,
		InvalidBreakUsage,
		InvalidContinueUsage,
		InvalidCaseLabelUsage,
		TypeIsNotConstructible,
		InvalidCast,
		FunctionOverloadingDuplicated,
		RequiresInitialValue,
		ErrorDeducingExprType,
		ErrorDeducingVarType,
		TypeIsNotUnpackable,
		InvalidVarArgHintDuringInstantiation,
		CannotBeUnpackedInThisContext,
		TypeIsNotSubstitutable,
		RequiresCompTimeExpr,
		TypeArgTypeMismatched,
		InterfaceMethodsConflicted,
		TypeIsNotInitializable,
		MemberIsNotAccessible,
		InvalidEnumBaseType,
		EnumItemIsNotAssignable,
		IncompatibleInitialValueType,
		FunctionOverloadingDuplicatedDuringInstantiation,
		ReturnValueTypeDoesNotMatch,
		DereferencingNull,
		InstanceMemberVarNotInitialized,
		StaticMemberVarNotInitialized,
		ThisNotInitialized,

		ImportLimitExceeded,
		MalformedModuleName,
		ErrorParsingImportedModule,
		ModuleNotFound,
		RegLimitExceeded,

		InvalidMnemonic,

		ErrorWritingCompiledModule
	};

	class TypeNameNode;
	class ModuleNode;
	class FnOverloadingNode;
	class VarNode;

	struct IncompatibleOperandErrorExData {
		AstNodePtr<TypeNameNode> desired_type;
	};

	struct ErrorParsingImportedModuleErrorExData {
		peff::Option<LexicalError> lexical_error;
		AstNodePtr<ModuleNode> mod;

		SLAKE_FORCEINLINE ErrorParsingImportedModuleErrorExData(LexicalError &&lexical_error) : lexical_error(std::move(lexical_error)) {}
		SLAKE_FORCEINLINE ErrorParsingImportedModuleErrorExData(AstNodePtr<ModuleNode> mod) : mod(mod) {}
	};

	struct AbstractMethodNotImplementedErrorExData {
		AstNodePtr<FnOverloadingNode> overloading;
	};

	struct MemberVarNotInitializedErrorExData {
		AstNodePtr<VarNode> var;
	};

	struct CompilationError {
		TokenRange token_range;
		CompilationErrorKind error_kind;
		std::variant<std::monostate,
			IncompatibleOperandErrorExData,
			ErrorParsingImportedModuleErrorExData,
			AbstractMethodNotImplementedErrorExData,
			MemberVarNotInitializedErrorExData>
			ex_data;

		SLAKE_FORCEINLINE CompilationError(
			const TokenRange &token_range,
			CompilationErrorKind error_kind)
			: token_range(token_range),
			  error_kind(error_kind) {
			assert(token_range);
		}

		SLAKE_FORCEINLINE CompilationError(
			const TokenRange &token_range,
			IncompatibleOperandErrorExData &&ex_data)
			: token_range(token_range),
			  error_kind(CompilationErrorKind::IncompatibleOperand),
			  ex_data(ex_data) {
			assert(token_range);
		}

		SLAKE_FORCEINLINE CompilationError(
			const TokenRange &token_range,
			ErrorParsingImportedModuleErrorExData &&ex_data)
			: token_range(token_range),
			  error_kind(CompilationErrorKind::ErrorParsingImportedModule),
			  ex_data(std::move(ex_data)) {
			assert(token_range);
		}

		SLAKE_FORCEINLINE CompilationError(
			const TokenRange &token_range,
			AbstractMethodNotImplementedErrorExData &&ex_data)
			: token_range(token_range),
			  error_kind(CompilationErrorKind::AbstractMethodNotImplemented),
			  ex_data(ex_data) {
			assert(token_range);
		}

		SLAKE_FORCEINLINE CompilationError(
			const TokenRange &token_range,
			CompilationErrorKind kind,
			MemberVarNotInitializedErrorExData &&ex_data)
			: token_range(token_range),
			  error_kind(kind),
			  ex_data(ex_data) {
			assert(token_range);
		}

		SLAKE_FORCEINLINE bool operator<(const CompilationError &rhs) const noexcept {
			return token_range < rhs.token_range;
		}

		SLAKE_FORCEINLINE bool operator>(const CompilationError &rhs) const noexcept {
			return token_range > rhs.token_range;
		}
	};

	SLAKE_FORCEINLINE CompilationError gen_oom_comp_error() {
		return CompilationError(TokenRange{ 0, 0 }, CompilationErrorKind::OutOfMemory);
	}

	SLAKE_FORCEINLINE CompilationError gen_stack_overflow() {
		return CompilationError(TokenRange{ 0, 0 }, CompilationErrorKind::StackOverflow);
	}

	SLAKE_FORCEINLINE CompilationError gen_out_of_runtime_memory_comp_error() {
		return CompilationError(TokenRange{ 0, 0 }, CompilationErrorKind::OutOfRuntimeMemory);
	}

	enum class CompilationWarningKind : int {
		UnusedExprResult = 0,
	};

	struct CompilationWarning {
		TokenRange token_range;
		CompilationWarningKind warning_kind;
		std::variant<std::monostate> ex_data;

		SLAKE_FORCEINLINE CompilationWarning(
			const TokenRange &token_range,
			CompilationWarningKind warning_kind)
			: token_range(token_range),
			  warning_kind(warning_kind) {
		}
	};

	struct CompileEnv;
	struct GenericArgListCmp {
		Document *document;
		slake::Runtime *runtime;
		mutable peff::Option<slkc::CompilationError> stored_error;

		SLAKE_API GenericArgListCmp(Document *document, slake::Runtime *runtime);
		SLAKE_API GenericArgListCmp(const GenericArgListCmp &r);
		SLAKE_API ~GenericArgListCmp();

		SLAKE_API peff::Option<int> operator()(const peff::DynArray<AstNodePtr<TypeNameNode>> &lhs, const peff::DynArray<AstNodePtr<TypeNameNode>> &rhs) const noexcept;
	};

	class MemberNode;

	using GenericCacheTable =
		peff::FallibleMap<
			peff::DynArray<AstNodePtr<TypeNameNode>>,
			AstNodePtr<MemberNode>,
			GenericArgListCmp,
			true>;

	struct GenericInstantiationContext;

	struct MemberGenericInstantiationTask {
		peff::SharedPtr<GenericInstantiationContext> context;
		AstNodePtr<MemberNode> member;
	};

	struct TypeSlotGenericInstantiationTask {
		peff::SharedPtr<GenericInstantiationContext> context;
		AstNodePtr<TypeNameNode> &type_name;
	};

	struct AstNodeGenericInstantiationTask {
		peff::SharedPtr<GenericInstantiationContext> context;
		AstNodePtr<TypeNameNode> &node;
	};

	class FnOverloadingNode;
	class FnNode;

	struct GenericInstantiationDispatcher {
		peff::RcObjectPtr<peff::Alloc> allocator;
		peff::List<MemberGenericInstantiationTask> member_tasks;
		peff::List<TypeSlotGenericInstantiationTask> type_tasks;
		peff::List<AstNodeGenericInstantiationTask> ast_node_tasks;
		peff::Set<AstNodePtr<FnOverloadingNode>> collected_fn_overloadings;
		peff::Set<AstNodePtr<FnNode>> collected_fns;

		SLAKE_FORCEINLINE GenericInstantiationDispatcher(peff::Alloc *allocator) : allocator(allocator), member_tasks(allocator), ast_node_tasks(allocator), type_tasks(allocator), collected_fn_overloadings(allocator), collected_fns(allocator) {}

		[[nodiscard]] SLAKE_FORCEINLINE peff::Option<CompilationError> push_member_task(MemberGenericInstantiationTask &&task) noexcept {
			return member_tasks.push_back(std::move(task)) ? peff::Option<CompilationError>{} : gen_oom_comp_error();
		}

		[[nodiscard]] SLAKE_FORCEINLINE peff::Option<CompilationError> push_type_slot_task(TypeSlotGenericInstantiationTask &&task) noexcept {
			return type_tasks.push_back(std::move(task)) ? peff::Option<CompilationError>{} : gen_oom_comp_error();
		}

		[[nodiscard]] SLAKE_FORCEINLINE peff::Option<CompilationError> push_ast_node_task(AstNodeGenericInstantiationTask &&task) noexcept {
			return ast_node_tasks.push_back(std::move(task)) ? peff::Option<CompilationError>{} : gen_oom_comp_error();
		}
	};

	struct GenericInstantiationContext : public peff::SharedFromThis<GenericInstantiationContext> {
		peff::RcObjectPtr<peff::Alloc> allocator;
		const peff::DynArray<AstNodePtr<TypeNameNode>> *generic_args;
		peff::HashMap<std::string_view, AstNodePtr<TypeNameNode>> mapped_generic_args;
		AstNodePtr<MemberNode> mapped_node;
		GenericInstantiationDispatcher *dispatcher = nullptr;

		SLAKE_FORCEINLINE GenericInstantiationContext(
			peff::Alloc *allocator,
			const peff::DynArray<AstNodePtr<TypeNameNode>> *generic_args,
			GenericInstantiationDispatcher *dispatcher)
			: allocator(allocator),
			  generic_args(generic_args),
			  mapped_generic_args(allocator),
			  dispatcher(dispatcher) {
		}
	};

	class ExternalModuleProvider;

	class Document : public peff::SharedFromThis<Document> {
	private:
		SLKC_API void _do_clear_deferred_destructible_ast_nodes();

	public:
		peff::RcObjectPtr<peff::Alloc> allocator;
		AstNodePtr<ModuleNode> root_module;
		ModuleNode *main_module = nullptr;
		peff::DynArray<peff::SharedPtr<ExternalModuleProvider>> external_module_providers;
		peff::Map<
			MemberNode *,
			GenericCacheTable>
			generic_cache_dir;

		AstNode *destructible_ast_node_list = nullptr;

		SLKC_API Document(peff::Alloc *allocator);
		SLKC_API virtual ~Document();

		SLKC_API peff::Option<CompilationError> lookup_generic_cache_table(AstNodePtr<MemberNode> original_object, GenericCacheTable *&table_out);

		SLKC_API peff::Option<CompilationError> lookup_generic_cache_table(
			AstNodePtr<MemberNode> original_object,
			const GenericCacheTable *&table_out) const {
			return const_cast<Document *>(this)->lookup_generic_cache_table(original_object, const_cast<GenericCacheTable *&>(table_out));
		}

		SLKC_API peff::Option<CompilationError> lookup_generic_cache(
			AstNodePtr<MemberNode> original_object,
			const peff::DynArray<AstNodePtr<TypeNameNode>> &generic_args,
			AstNodePtr<MemberNode> &member_out) const;

		SLKC_API peff::Option<CompilationError> instantiate_generic_object(
			AstNodePtr<MemberNode> original_object,
			size_t idx_name_token,
			const peff::DynArray<AstNodePtr<TypeNameNode>> &generic_args,
			AstNodePtr<MemberNode> &member_out);

		SLAKE_FORCEINLINE void clear_deferred_destructible_ast_nodes() {
			if (destructible_ast_node_list) {
				_do_clear_deferred_destructible_ast_nodes();
			}
		}
	};
}

#endif
