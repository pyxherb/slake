#ifndef _SLKC_DECOMP_DECOMPILER_H_
#define _SLKC_DECOMP_DECOMPILER_H_

#include "../comp/compiler.h"

namespace slkc {
	class DumpWriter {
	public:
		SLKC_API virtual ~DumpWriter();
		[[nodiscard]] virtual bool write(const char *data, size_t len) = 0;

		SLAKE_FORCEINLINE bool write(const std::string_view& s) {
			return write(s.data(), s.size());
		}
	};

	struct DecompileEnv {
		size_t indentLevel = 0;
	};

	SLKC_API const char *getMnemonicName(slake::Opcode opcode);

	[[nodiscard]] SLKC_API bool decompileGenericParam(peff::Alloc *allocator, DumpWriter *writer, const slake::GenericParam &genericParam);
	[[nodiscard]] SLKC_API bool decompileTypeName(peff::Alloc *allocator, DumpWriter *writer, const slake::TypeRef &type);
	[[nodiscard]] SLKC_API bool decompileValue(peff::Alloc *allocator, DumpWriter *writer, const slake::Value &value);
	[[nodiscard]] SLKC_API bool decompileIdRefEntries(peff::Alloc *allocator, DumpWriter *writer, const peff::DynArray<slake::IdRefEntry> &idRefIn);
	[[nodiscard]] SLKC_API bool decompileIdRef(peff::Alloc *allocator, DumpWriter *writer, slake::IdRefObject *idRefIn);
	[[nodiscard]] SLKC_API bool decompileModuleMembers(peff::Alloc *allocator, DumpWriter *writer, slake::ModuleObject *moduleObject, size_t indentLevel = 0);
	[[nodiscard]] SLKC_API bool decompileModule(peff::Alloc *allocator, DumpWriter *writer, slake::ModuleObject *moduleObject);
}

#endif
