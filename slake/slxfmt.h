///
/// @file slxfmt.h
/// @author CodesBuilder (codesbuilder@163.com)
/// @brief Definitions for Slake Executable (SLX) format.
///
/// @copyright Copyright (c) 2022-2023 Slake Contributors
///
#ifndef _SLAKE_SLXFMT_H_
#define _SLAKE_SLXFMT_H_

#include <cassert>
#include <cstdint>

#include "opcode.h"

#ifdef _MSC_VER
	#pragma pack(push)
	#pragma pack(1)
#endif

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
			uint8_t fmtVer;		// Format version
			uint16_t nImports;	// Number of imported modules
		};
		constexpr static uint8_t IMH_MAGIC[] = { 'S', 'L', 'A', 'X' };

		constexpr static uint8_t
			IH_OUTPUT = 0x01;

		///
		/// @brief Instruction Header (IH)
		///
		struct InsHeader final {
			uint32_t nOperands;	 // Number of operands
			Opcode opcode;		 // Operation code
			uint8_t flags;		 // Determines if the instruction has an output.
		};

		enum class TypeId : uint8_t {
			None = 0,		// None
			Any,			// Any
			I8,				// i8
			I16,			// i16
			I32,			// i32
			I64,			// i64
			U8,				// u8
			U16,			// u16
			U32,			// u32
			U64,			// u64
			F32,			// f32
			F64,			// f64
			String,			// String
			Bool,			// Boolean
			Array,			// Array
			Object,			// Object
			GenericArg,		// Generic argument
			Ref,			// Reference
			ParamTypeList,	// Parameter type list
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

		enum class ConstObjectType : uint8_t {
			IdRef = 0,
			String,
			Array
		};

		/// @brief Class Type Descriptor (CTD)
		struct ClassTypeDesc final {
			/// @brief Flags
			uint8_t flags;
			/// @brief Number of generic parameters
			uint8_t nGenericParams;
			/// @brief Length of class name
			uint8_t lenName;
			/// @brief Number of implemented interfaces (for classes), or number of parents (interfaces).
			uint8_t nImpls;
		};
		constexpr static uint8_t
			CTD_PUB = 0x01,		// Public
			CTD_FINAL = 0x02,	// Final
			CTD_DERIVED = 0x40	// Is a derived type
			;

		/// @brief Interface Type Descriptor (ITD)
		struct InterfaceTypeDesc final {
			uint8_t flags;
			uint8_t nGenericParams;
			uint8_t lenName;
			uint8_t nParents;
		};
		constexpr static uint8_t
			ITD_PUB = 0x01	// Public
			;

		/// @brief Function Descriptor (FND)
		struct FnDesc final {
			uint16_t flags;			   // Flags
			uint32_t lenBody;		   // Length of body
			uint32_t nSourceLocDescs;  // Number of SLDs
			uint32_t nRegisters;	   // Number of registers
			uint32_t nConstObjects;	   // Number of constant objects
			uint8_t nGenericParams;	   // Number of generic parameters
			uint8_t nParams;		   // Number of parameters
		};
		constexpr static uint16_t
			FND_PUB = 0x0001,		 // Public
			FND_FINAL = 0x0002,		 // Final
			FND_OVERRIDE = 0x0004,	 // Override
			FND_STATIC = 0x0008,	 // Static
			FND_NATIVE = 0x0010,	 // Native
			FND_DBG = 0x0040,		 // With debugging information
			FND_VARG = 0x0080,		 // Varidic arguments
			FND_GENERATOR = 0x0100,	 // Generator
			FND_VIRTUAL = 0x200		 // Virtual
			;

		struct SrcInfoEntry {
			uint32_t line;
		};

		struct LocalVarInfo {
			uint32_t validLineStart, validLineEnd;
		};

		/// @brief Variable Descriptonr (VAD)
		struct VarDesc final {
			uint8_t lenName;
			uint8_t flags;
		};
		constexpr static uint8_t
			VAD_PUB = 0x01,		// Public
			VAD_FINAL = 0x02,	// Final
			VAD_STATIC = 0x04,	// Static
			VAD_NATIVE = 0x08	// Native
			;

		struct IdRefHeader final {
			uint8_t nEntries;
			uint8_t hasVarArgs;
			uint16_t nParams;
		};
		/// @brief Reference Scope Descriptor (RSD)
		struct IdRefEntryDesc final {
			uint16_t lenName;
			uint8_t nGenericArgs;
			uint8_t reserved;
		};

		constexpr static uint8_t
			GPD_BASE = 0x01;

		/// @brief Generic Parameter Descriptor (GPD)
		struct GenericParamDesc final {
			uint8_t lenName;
			uint8_t flags;
			uint8_t nInterfaces;
		};

		// @brief Variable Debugging Descriptor (VDD)
		struct VarDebugDesc final {
			uint32_t line;
			uint32_t nLine;
		};

		// @brief Source Location Descriptor (SLD)
		struct SourceLocDesc final {
			uint32_t offIns;
			uint32_t nIns;
			uint32_t line;
			uint32_t column;
		};
	}
}

#ifdef _MSC_VER
	#pragma pack(pop)
#endif

#endif
