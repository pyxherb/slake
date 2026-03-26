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
		size_t indent_level = 0;
	};

	SLKC_API const char *get_mnemonic_name(slake::Opcode opcode);

	class Decompiler final {
	public:
		bool dump_cfg = false;

		SLKC_API Decompiler();
		SLKC_API ~Decompiler();

		[[nodiscard]] SLKC_API bool decompile_generic_param(peff::Alloc *allocator, DumpWriter *writer, const slake::GenericParam &generic_param);
		[[nodiscard]] SLKC_API bool decompile_type_name(peff::Alloc *allocator, DumpWriter *writer, const slake::TypeRef &type);
		[[nodiscard]] SLKC_API bool decompile_value(peff::Alloc *allocator, DumpWriter *writer, const slake::Value &value);
		[[nodiscard]] SLKC_API bool decompile_id_ref_entries(peff::Alloc *allocator, DumpWriter *writer, const peff::DynArray<slake::IdRefEntry> &id_ref_in);
		[[nodiscard]] SLKC_API bool decompile_id_ref(peff::Alloc *allocator, DumpWriter *writer, slake::IdRefObject *id_ref_in);
		[[nodiscard]] SLKC_API bool decompile_module_members(peff::Alloc *allocator, DumpWriter *writer, slake::BasicModuleObject *module_object, size_t indent_level = 0);
		[[nodiscard]] SLKC_API bool decompile_module(peff::Alloc *allocator, DumpWriter *writer, slake::ModuleObject *module_object);
	};
}

#endif
