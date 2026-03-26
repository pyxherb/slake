///
/// @file slxfmt.h
/// @author CodesBuilder (codesbuilder@163.com)
/// @brief Definitions for Slake Executable (SLX) format.
///
/// @copyright Copyright (c) 2022 Slake Contributors
///
#ifndef _SLAKE_SLXFMT_H_
#define _SLAKE_SLXFMT_H_

#include <cassert>
#include <cstdint>

#include "opcode.h"

namespace slake {
	namespace slxfmt {
		constexpr static uint8_t GENERIC_PARAM_MAX = 16;

		constexpr static uint8_t
			IMH_MODNAME = 0x01,	 // With module name
			IMH_DBG = 0x02		 // With debugging information, e.g. source file name
			;

		///
		/// @brief Image Header (IMH)
		///
		struct ImgHeader final {
			uint8_t magic[4];	// Magic number
			uint8_t flags;		// Flags
			uint8_t fmt_ver;		// Format version
			uint16_t num_imports;	// Number of imported modules
		};
		constexpr static uint8_t IMH_MAGIC[] = { 'S', 'L', 'A', 'X' };

		constexpr static uint8_t
			IH_OUTPUT = 0x01;

		///
		/// @brief Instruction Header (IH)
		///
		struct InsHeader final {
			uint32_t num_operands;	 // Number of operands
			Opcode opcode;		 // Operation code
			uint8_t flags;		 // Determines if the instruction has an output.
		};

		enum class TypeId : uint8_t {
			Void = 0,			 // Void
			Any,				 // Any
			I8,					 // i8
			I16,				 // i16
			I32,				 // i32
			I64,				 // i64
			U8,					 // u8
			U16,				 // u16
			U32,				 // u32
			U64,				 // u64
			F32,				 // f32
			F64,				 // f64
			String,				 // String
			Bool,				 // Boolean
			Array,				 // Array
			Object,				 // Object
			Struct,				 // Structure
			ScopedEnum,			 // Scoped enumeration
			TypelessScopedEnum,	 // Typeless scoped enumeration
			UnionEnum,			 // Union enumeration
			UnionEnumItem,		 // Enumeration item
			GenericArg,			 // Generic argument
			Ref,				 // Reference
			ParamTypeList,		 // Parameter type list
			Unpacking,			 // Unpacking
		};

		enum class ValueType : uint8_t {
			None = 0,  // None
			I8,		   // i8
			I16,	   // i16
			I32,	   // i32
			I64,	   // i64
			U8,		   // u8
			U16,	   // u16
			U32,	   // u32
			U64,	   // u64
			F32,	   // f32
			F64,	   // f64
			Bool,	   // Boolean
			String,	   // String
			IdRef,	   // Identifier reference
			Array,	   // Array
			TypeName,  // Type name
			Reg,	   // Register
		};

		constexpr static uint8_t
			TYPE_FINAL = 0x01,
			TYPE_LOCAL = 0x02,
			TYPE_NULLABLE = 0x04;

		enum class ConstObjectType : uint8_t {
			IdRef = 0,
			String,
			Array
		};

		/// @brief Class Type Descriptor (CTD)
		struct ClassTypeDesc final {
			/// @brief Flags
			uint8_t flags;
			uint8_t reserved[3];
			/// @brief Number of generic parameters
			uint32_t num_generic_params;
			/// @brief Length of class name
			uint32_t len_name;
			/// @brief Number of implemented interfaces (for classes), or number of parents (interfaces).
			uint32_t num_impls;
		};
		constexpr static uint8_t
			CTD_FINAL = 0x01,	// Final
			CTD_DERIVED = 0x02	// Is a derived type
			;

		/// @brief Interface Type Descriptor (ITD)
		struct InterfaceTypeDesc final {
			uint8_t flags;
			uint8_t reserved[3];
			uint32_t num_generic_params;
			uint32_t len_name;
			uint32_t num_parents;
		};

		/// @brief Structure Type Descriptor (STD)
		struct StructTypeDesc final {
			uint8_t flags;
			uint8_t reserved[3];
			uint32_t num_generic_params;
			uint32_t len_name;
			uint32_t num_impls;
		};

		/// @brief Scoped Enumeration Type Descriptor (SETD)
		struct ScopedEnumTypeDesc final {
			uint8_t flags;
			uint8_t reserved[3];
			uint32_t len_name;
		};
		constexpr static uint8_t
			SETD_BASE = 0x01;

		/// @brief Enumeration Item Descriptor (EID)
		struct EnumItemDesc final {
			uint32_t len_name;
		};

		/// @brief Union Enumeration Type Descriptor (UETD)
		struct UnionEnumTypeDesc final {
			uint8_t flags;
			uint8_t reserved[3];
			uint32_t num_generic_params;
			uint32_t len_name;
		};

		/// @brief Function Descriptor (FND)
		struct FnDesc final {
			uint16_t flags;			   // Flags
			uint8_t num_generic_params;	   // Number of generic parameters
			uint8_t num_params;		   // Number of parameters
			uint32_t len_body;		   // Length of body
			uint32_t num_source_loc_descs;  // Number of SLDs
			uint32_t num_registers;	   // Number of registers
			uint32_t num_const_objects;	   // Number of constant objects
		};
		constexpr static uint16_t
			FND_FINAL = 0x0001,		 // Final
			FND_OVERRIDE = 0x0002,	 // Override
			FND_DBG = 0x0004,		 // With debugging information
			FND_VARG = 0x0008,		 // Varidic arguments
			FND_GENERATOR = 0x0010,	 // Generator
			FND_VIRTUAL = 0x0020	 // Virtual
			;

		struct SrcInfoEntry {
			uint32_t line;
		};

		struct LocalVarInfo {
			uint32_t valid_line_start, valid_line_end;
		};

		/// @brief Variable Descriptonr (VAD)
		struct VarDesc final {
			uint8_t len_name;
			uint8_t flags;
		};

		struct IdRefHeader final {
			uint8_t num_entries;
			uint8_t has_var_args;
			uint16_t num_params;
		};
		/// @brief Reference Entry Descriptor (RED)
		struct IdRefEntryDesc final {
			uint16_t len_name;
			uint8_t num_generic_args;
			uint8_t reserved;
		};

		constexpr static uint8_t
			GPD_BASE = 0x01;

		/// @brief Generic Parameter Descriptor (GPD)
		struct GenericParamDesc final {
			uint8_t len_name;
			uint8_t flags;
			uint8_t num_interfaces;
		};

		// @brief Variable Debugging Descriptor (VDD)
		struct VarDebugDesc final {
			uint32_t line;
			uint32_t num_line;
		};

		// @brief Source Location Descriptor (SLD)
		struct SourceLocDesc final {
			uint32_t line;
			uint32_t column;

			SLAKE_FORCEINLINE int compares_to(const SourceLocDesc &rhs) const noexcept {
				if (line < rhs.line)
					return -1;
				if (line > rhs.line)
					return 1;
				if (column < rhs.column)
					return -1;
				if (column > rhs.column)
					return 1;
				return 0;
			}

			SLAKE_FORCEINLINE bool operator==(const SourceLocDesc &rhs) const noexcept {
				return (line == rhs.line) && (column == rhs.column);
			}

			SLAKE_FORCEINLINE bool operator<(const SourceLocDesc &rhs) const noexcept {
				return compares_to(rhs) < 0;
			}

			SLAKE_FORCEINLINE bool operator>(const SourceLocDesc &rhs) const noexcept {
				return compares_to(rhs) > 0;
			}
		};
	}
}

#endif
