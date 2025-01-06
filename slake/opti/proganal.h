#ifndef _SLAKE_OPTI_REGANAL_H_
#define _SLAKE_OPTI_REGANAL_H_

#include "../runtime.h"

namespace slake {
	namespace opti {
		struct RegLifetime {
			size_t offBeginIns;
			size_t offEndIns;
		};

		enum class RegPurpose : uint8_t {
			Regular = 0,
			LocalVar,
			ArgRef,
		};

		struct LocalVarRegPurposeInfo {
			uint32_t off;
		};

		struct ArgRefRegPurposeInfo {
			uint32_t off;
		};

		struct RegAnalyzedInfo {
			RegLifetime lifetime;
			Type type;
			Value expectedValue = Value(ValueType::Undefined);
			union {
				LocalVarRegPurposeInfo asLocalVar;
				ArgRefRegPurposeInfo asArgRef;
			} regPurposeExData;
			RegPurpose purpose = RegPurpose::Regular;
		};

		struct LocalVarAnalyzedInfo {
			Type type;
		};

		struct StackFrameState {
			std::pmr::vector<LocalVarAnalyzedInfo> analyzedLocalVarInfo;
			std::pmr::vector<size_t> stackBases;
		};

		struct ProgramAnalyzedInfo {
			std::pmr::map<uint32_t, RegAnalyzedInfo> analyzedRegInfo;
		};

		struct ProgramAnalyzeContext {
			Runtime *runtime;
			RegularFnOverloadingObject *fnObject;
			ProgramAnalyzedInfo &analyzedInfoOut;
			HostRefHolder &hostRefHolder;
			size_t idxCurIns = 0;
			StackFrameState stackFrameState = {};
			Object *lastCallTarget;
			Type lastCallTargetType;
		};

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
			const VarRefContext &varRefContext,
			Object *object,
			Type &typeOut);
		InternalExceptionPointer evalValueType(
			ProgramAnalyzeContext &analyzeContext,
			const Value &value,
			Type &typeOut);
		InternalExceptionPointer evalConstValue(
			ProgramAnalyzeContext &analyzeContext,
			const Value &value,
			Value &constValueOut);
		InternalExceptionPointer analyzeExprIns(
			ProgramAnalyzeContext &analyzeContext,
			uint32_t regIndex);
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
