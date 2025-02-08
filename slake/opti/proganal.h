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
			None = 0,	// Unrecognized
			FieldVar,	// Fields
			LocalVar,	// Local variables
			ArgRef,		// Arguments
		};

		struct FieldVarRegStorageInfo {
			bool isUsedForOutput;
		};

		struct LocalVarRegStorageInfo {
			bool isUsedForOutput;
		};

		struct ArgRefRegStorageInfo {
			bool isUsedForOutput;
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

		struct StackFrameState {
			peff::DynArray<size_t> stackBases;

			SLAKE_FORCEINLINE StackFrameState(peff::Alloc *selfAllocator)
				: stackBases(selfAllocator) {
			}
		};

		struct FnCallAnalyzedInfo {
			FnOverloadingObject *fnObject = nullptr;
			peff::DynArray<uint32_t> argPushInsOffs;

			SLAKE_FORCEINLINE FnCallAnalyzedInfo(peff::Alloc *selfAllocator)
				: argPushInsOffs(selfAllocator) {
			}
		};

		struct ProgramAnalyzedInfo {
			HostObjectRef<ContextObject> contextObject;
			peff::Map<uint32_t, RegAnalyzedInfo> analyzedRegInfo;
			peff::Map<uint32_t, FnCallAnalyzedInfo> analyzedFnCallInfo;
			peff::Map<FnOverloadingObject *, peff::DynArray<uint32_t>> fnCallMap;
			peff::Set<uint32_t> codeBlockBoundaries;

			SLAKE_FORCEINLINE ProgramAnalyzedInfo(Runtime *runtime)
				: analyzedRegInfo(&runtime->globalHeapPoolAlloc),
				  analyzedFnCallInfo(&runtime->globalHeapPoolAlloc),
				  fnCallMap(&runtime->globalHeapPoolAlloc),
				  codeBlockBoundaries(&runtime->globalHeapPoolAlloc) {
			}
		};

		struct ProgramAnalyzeContext {
			Runtime *runtime;
			RegularFnOverloadingObject *fnObject;
			ProgramAnalyzedInfo &analyzedInfoOut;
			HostRefHolder &hostRefHolder;
			size_t idxCurIns = 0;
			StackFrameState stackFrameState;
			Object *lastCallTarget;
			Type lastCallTargetType;
			peff::DynArray<uint32_t> argPushInsOffs;

			SLAKE_FORCEINLINE ProgramAnalyzeContext(
				Runtime *runtime,
				RegularFnOverloadingObject *fnObject,
				ProgramAnalyzedInfo &analyzedInfoOut,
				HostRefHolder &hostRefHolder)
				: runtime(runtime),
				  fnObject(fnObject),
				  analyzedInfoOut(analyzedInfoOut),
				  hostRefHolder(hostRefHolder),
				  stackFrameState(&runtime->globalHeapPoolAlloc),
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
			const ObjectRef &objectRef,
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
			RegularFnOverloadingObject *fnObject,
			ProgramAnalyzedInfo &analyzedInfoOut,
			HostRefHolder &hostRefHolder);
	}
}

#endif
