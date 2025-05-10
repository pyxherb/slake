#ifndef _SLAKE_OPTI_REGANAL_H_
#define _SLAKE_OPTI_REGANAL_H_

#include "../runtime.h"
#include <variant>

namespace slake {
	namespace opti {
		struct RegLifetime {
			size_t offBeginIns;
			size_t offEndIns;
		};

		enum class RegStorageType : uint8_t {
			None = 0,		   // Unrecognized, may from indirect access
			FieldVar,		   // Fields
			InstanceFieldVar,  // Instance fields
			LocalVar,		   // Local variables
			ArgRef,			   // Arguments
			ArrayElement,	   // Array element
		};

		struct FieldVarRegStorageInfo {
			bool isUsedForOutput;
		};

		struct LocalVarRegStorageInfo {
			bool isUsedForOutput;
			/// @brief Register where the local variable is defined and received
			uint32_t definitionReg;
		};

		struct ArgRefRegStorageInfo {
			bool isUsedForOutput;
			uint32_t idxArg;
		};

		using RegReferencedPoint = std::pair<uint32_t, uint32_t>;

		struct RegAnalyzedInfo {
			RegLifetime lifetime;
			Type type;
			Value expectedValue = Value(ValueType::Undefined);
			union {
				FieldVarRegStorageInfo asFieldVar;
				LocalVarRegStorageInfo asLocalVar;
				ArgRefRegStorageInfo asArgRef;
			} storageInfo;
			RegStorageType storageType = RegStorageType::None;
		};

		struct LocalVarAnalyzedInfo {
			Type type;
		};

		struct FnCallAnalyzedInfo {
			FnOverloadingObject *fnObject = nullptr;
			peff::DynArray<uint32_t> argPushInsOffs;

			SLAKE_FORCEINLINE FnCallAnalyzedInfo(peff::Alloc *selfAllocator)
				: argPushInsOffs(selfAllocator) {
			}
		};

		struct ProgramAnalyzedInfo {
			peff::RcObjectPtr<peff::Alloc> resourceAllocator;
			HostObjectRef<ContextObject> contextObject;
			peff::Map<uint32_t, RegAnalyzedInfo> analyzedRegInfo;
			peff::Map<uint32_t, FnCallAnalyzedInfo> analyzedFnCallInfo;
			peff::Map<FnOverloadingObject *, peff::DynArray<uint32_t>> fnCallMap;
			peff::Set<uint32_t> codeBlockBoundaries;

			SLAKE_FORCEINLINE ProgramAnalyzedInfo(Runtime *runtime, peff::Alloc *resourceAllocator)
				: analyzedRegInfo(&runtime->globalHeapPoolAlloc),
				  analyzedFnCallInfo(&runtime->globalHeapPoolAlloc),
				  fnCallMap(&runtime->globalHeapPoolAlloc),
				  codeBlockBoundaries(&runtime->globalHeapPoolAlloc),
				  resourceAllocator(resourceAllocator) {
			}
		};

		struct ProgramAnalyzeContext {
			Runtime *runtime;
			peff::RcObjectPtr<peff::Alloc> resourceAllocator;
			RegularFnOverloadingObject *fnObject;
			ProgramAnalyzedInfo &analyzedInfoOut;
			HostRefHolder &hostRefHolder;
			size_t idxCurIns = 0;
			Object *lastCallTarget;
			Type lastCallTargetType;
			peff::DynArray<uint32_t> argPushInsOffs;

			SLAKE_FORCEINLINE ProgramAnalyzeContext(
				Runtime *runtime,
				peff::Alloc *resourceAllocator,
				RegularFnOverloadingObject *fnObject,
				ProgramAnalyzedInfo &analyzedInfoOut,
				HostRefHolder &hostRefHolder)
				: runtime(runtime),
				  fnObject(fnObject),
				  analyzedInfoOut(analyzedInfoOut),
				  hostRefHolder(hostRefHolder),
				  argPushInsOffs(&runtime->globalHeapPoolAlloc) {
			}
		};

		void markRegAsForOutput(ProgramAnalyzeContext &analyzeContext, uint32_t i);
		InternalExceptionPointer wrapIntoRefType(
			Runtime *runtime,
			Type type,
			HostRefHolder &hostRefHolder,
			Type &typeOut);
		InternalExceptionPointer wrapIntoArrayType(
			Runtime *runtime,
			Type type,
			HostRefHolder &hostRefHolder,
			Type &typeOut);
		InternalExceptionPointer evalObjectType(
			ProgramAnalyzeContext &analyzeContext,
			const EntityRef &entityRef,
			Type &typeOut);
		InternalExceptionPointer evalValueType(
			ProgramAnalyzeContext &analyzeContext,
			const Value &value,
			Type &typeOut);
		InternalExceptionPointer evalConstValue(
			ProgramAnalyzeContext &analyzeContext,
			const Value &value,
			Value &constValueOut);
		InternalExceptionPointer analyzeArithmeticIns(
			ProgramAnalyzeContext &analyzeContext,
			uint32_t regIndex) noexcept;
		InternalExceptionPointer analyzeCastIns(
			ProgramAnalyzeContext &analyzeContext,
			size_t regIndex);
		InternalExceptionPointer analyzeProgramInfo(
			Runtime *runtime,
			peff::Alloc *resourceAllocator,
			RegularFnOverloadingObject *fnObject,
			ProgramAnalyzedInfo &analyzedInfoOut,
			HostRefHolder &hostRefHolder);
	}
}

#endif
