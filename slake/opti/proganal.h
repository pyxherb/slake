#ifndef _SLAKE_OPTI_REGANAL_H_
#define _SLAKE_OPTI_REGANAL_H_

#include "../runtime.h"
#include <variant>

namespace slake {
	namespace opti {
		struct RegLifetime {
			uint32_t offBeginIns;
			uint32_t offEndIns;
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

		struct RegAnalyzedInfo {
			RegLifetime lifetime;
			peff::Set<uint32_t> usePoints;
			TypeRef type;
			Value expectedValue = Value(ValueType::Undefined);
			union {
				FieldVarRegStorageInfo asFieldVar;
				LocalVarRegStorageInfo asLocalVar;
				ArgRefRegStorageInfo asArgRef;
			} storageInfo;
			RegStorageType storageType = RegStorageType::None;

			SLAKE_FORCEINLINE RegAnalyzedInfo(peff::Alloc *allocator) : usePoints(allocator) {}
		};

		struct LocalVarAnalyzedInfo {
			TypeRef type;
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
				: analyzedRegInfo(resourceAllocator),
				  analyzedFnCallInfo(resourceAllocator),
				  fnCallMap(resourceAllocator),
				  codeBlockBoundaries(resourceAllocator),
				  resourceAllocator(resourceAllocator) {
			}
		};

		struct ProgramAnalyzeContext {
			Runtime *runtime;
			peff::RcObjectPtr<peff::Alloc> resourceAllocator;
			RegularFnOverloadingObject *fnObject;
			ProgramAnalyzedInfo &analyzedInfoOut;
			HostRefHolder &hostRefHolder;
			uint32_t idxCurIns = 0;
			Object *lastCallTarget;
			TypeRef lastCallTargetType;
			peff::DynArray<uint32_t> argPushInsOffs;

			SLAKE_FORCEINLINE ProgramAnalyzeContext(
				Runtime *runtime,
				peff::Alloc *resourceAllocator,
				RegularFnOverloadingObject *fnObject,
				ProgramAnalyzedInfo &analyzedInfoOut,
				HostRefHolder &hostRefHolder)
				: runtime(runtime),
				  resourceAllocator(resourceAllocator),
				  fnObject(fnObject),
				  analyzedInfoOut(analyzedInfoOut),
				  hostRefHolder(hostRefHolder),
				  argPushInsOffs(resourceAllocator) {
			}
		};

		bool isInsHasSideEffect(Opcode opcode);
		bool isInsSimplifiable(Opcode opcode);

		void markRegAsForOutput(ProgramAnalyzeContext &analyzeContext, uint32_t i);
		InternalExceptionPointer wrapIntoHeapType(
			Runtime *runtime,
			TypeRef type,
			HostRefHolder &hostRefHolder,
			HeapTypeObject *&heapTypeOut);
		InternalExceptionPointer wrapIntoRefType(
			Runtime *runtime,
			TypeRef type,
			HostRefHolder &hostRefHolder,
			TypeRef &typeOut);
		InternalExceptionPointer wrapIntoArrayType(
			Runtime *runtime,
			TypeRef type,
			HostRefHolder &hostRefHolder,
			TypeRef &typeOut);
		InternalExceptionPointer evalObjectType(
			ProgramAnalyzeContext &analyzeContext,
			const Reference &entityRef,
			TypeRef &typeOut);
		InternalExceptionPointer evalValueType(
			ProgramAnalyzeContext &analyzeContext,
			const Value &value,
			TypeRef &typeOut);
		InternalExceptionPointer evalConstValue(
			ProgramAnalyzeContext &analyzeContext,
			const Value &value,
			Value &constValueOut);
		InternalExceptionPointer analyzeArithmeticIns(
			ProgramAnalyzeContext &analyzeContext,
			uint32_t regIndex) noexcept;
		InternalExceptionPointer analyzeCastIns(
			ProgramAnalyzeContext &analyzeContext,
			uint32_t regIndex);
		InternalExceptionPointer analyzeProgramInfoPass(
			Runtime *runtime,
			peff::Alloc *resourceAllocator,
			RegularFnOverloadingObject *fnObject,
			ProgramAnalyzedInfo &analyzedInfoOut,
			HostRefHolder &hostRefHolder);
	}
}

#endif
