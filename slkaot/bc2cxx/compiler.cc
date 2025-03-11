#include "../bc2cxx.h"

using namespace slake;
using namespace slake::slkaot;
using namespace slake::slkaot::bc2cxx;

bool BC2CXX::CompileContext::allocRecycledReg(BC2CXX &bc2cxx, const opti::ProgramAnalyzedInfo &analyzedInfo, uint32_t reg, const Type &type) {
	VirtualRegInfo &targetVregInfo = vregInfo.at(reg);
	for (auto i : recycledRegs) {
		const opti::RegAnalyzedInfo &regInfo = analyzedInfo.analyzedRegInfo.at(i);

		switch (regInfo.type.typeId) {
			case TypeId::I8:
			case TypeId::I16:
			case TypeId::I32:
			case TypeId::I64:
			case TypeId::U8:
			case TypeId::U16:
			case TypeId::U32:
			case TypeId::U64:
			case TypeId::F32:
			case TypeId::F64:
			case TypeId::Bool:
				if (regInfo.type == type)
					goto succeeded;
				break;
			case TypeId::String:
			case TypeId::Instance:
			case TypeId::Array:
			case TypeId::FnDelegate:
				switch (type.typeId) {
					case TypeId::String:
					case TypeId::Instance:
					case TypeId::Array:
					case TypeId::FnDelegate:
						goto succeeded;
					default:;
				}
				break;
			case TypeId::Ref: {
				if (type.typeId == TypeId::Ref) {
					goto succeeded;
				}
				break;
			}
			default:
				std::terminate();
		}

		continue;
	succeeded:
		printf("Reused register #%u for #%u\n", i, reg);
		targetVregInfo.vregVarName = bc2cxx.mangleRegLocalVarName(i);
		targetVregInfo.nameBorrowedReg = i;
		recycledRegs.erase(i);
		return true;
	}
	return false;
}

void BC2CXX::recompileFnOverloading(CompileContext &compileContext, std::shared_ptr<cxxast::Fn> fnOverloading) {
	compileContext.resetForCompilation();

	FnOverloadingObject *fnOverloadingObject = fnOverloading->rtOverloading.get();

	switch (fnOverloadingObject->overloadingKind) {
		case FnOverloadingKind::Regular: {
			RegularFnOverloadingObject *fo = (RegularFnOverloadingObject *)fnOverloadingObject;

			if (fo->overloadingFlags & OL_GENERATOR) {
				recompileGeneratorFnOverloading(compileContext, fnOverloading);
			} else {
				recompileRegularFnOverloading(compileContext, fnOverloading);
			}
			break;
		}
	}
}
