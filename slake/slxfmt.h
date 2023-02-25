///
/// @file slx.h
/// @author CodesBuilder (2602783536@qq.com)
/// @brief Definitions for Slake Executable (SLX) format.
/// @version 0.1
/// @date 2022-09-18
///
/// @copyright Copyright (c) 2022 Slake Contributors
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

namespace Slake {
	namespace SlxFmt {
		///
		/// @brief IMage Header (IMH)
		///
		struct ImgHeader final {
			std::uint8_t magic[4];	   // Magic number
			std::uint8_t flags;		   // Flags
			std::uint8_t fmtVer;	   // Format version
			std::uint8_t reserved[2];  // Reserved
		};
		constexpr static std::uint8_t IMH_MAGIC[4] = { 'S', 'L', 'A', 'X' };

		///
		/// @brief Instruction Header (IH)
		///
		struct InsHeader final {
			Opcode opcode : 6;			 // Operation code
			std::uint8_t nOperands : 2;	 // Number of operands

			inline InsHeader() {}
			inline InsHeader(Opcode opcode, std::uint8_t nOperands) {
				assert((std::uint8_t)opcode < (1 << 6));
				assert(nOperands < 4);
				this->opcode = opcode;
				this->nOperands = nOperands;
			}
		};

		enum class ValueType : std::uint8_t {
			NONE = 0,	 // None
			ANY = NONE,	 // Note that the compiler recongizes them as different types, but the runtime does not.
			I8,			 // i8
			I16,		 // i16
			I32,		 // i32
			I64,		 // i64
			U8,			 // u8
			U16,		 // u16
			U32,		 // u32
			U64,		 // u64
			FLOAT,		 // Float
			DOUBLE,		 // Double
			STRING,		 // String
			UUID,		 // UUID
			BOOL,		 // Boolean
			ARRAY,		 // Array
			OBJECT,		 // Object
			REF			 // Reference
		};

		/// @brief Value Descriptor (VD)
		struct ValueDesc final {
			ValueType type : 5;		 // Data Type
			std::uint8_t flags : 3;	 // Flags
		};

		/// @brief Extra attribute for strings
		struct StringExAttr final {
			std::uint32_t len;	// Length in bytes
		};

		/// @brief Class Type Descriptor (CTD)
		struct ClassTypeDesc final {
			std::uint8_t flags;			  // Flags
			std::uint8_t nGenericParams;  // Number of generic parameters
			std::uint8_t lenName;		  // Length of name
			std::uint8_t lenImpls;		  // Length of implemented interface list
		};
		constexpr static std::uint8_t
			CTD_PUB = 0x01,	  // Public
			CTD_FINAL = 0x02  // Final
			;

		/// @brief Structure Type Descriptor (STD)
		struct StructTypeDesc final {
			std::uint8_t flags;		 // Flags
			std::uint8_t lenName;	 // Length of name
			std::uint32_t nMembers;	 // Number of members
		};

		/// @brief Function Descriptor (FND)
		struct FnDesc final {
			std::uint8_t nGenericParams;  // Number of generic parameters
			std::uint8_t flags;			  // Flags
			std::uint8_t lenName;		  // Length of name
			std::uint8_t reserved;
			std::uint32_t lenBody;	// Length of body
		};
		constexpr static std::uint8_t
			FND_PUB = 0x01,		  // Public
			FND_FINAL = 0x02,	  // Final
			FND_OVERRIDE = 0x04,  // Override
			FND_STATIC = 0x08,	  // Static
			FND_NATIVE = 0x10	  // Native
			;

		/// @brief Variable Descriptonr (VAD)
		struct VarDesc final {
			std::uint8_t lenName;
			std::uint8_t flags;
		};
		constexpr static std::uint8_t
			VAD_PUB = 0x01,		// Public
			VAD_FINAL = 0x02,	// Final
			VAD_STATIC = 0x04,	// Static
			VAD_NATIVE = 0x08,	// Native
			VAD_INIT = 0x80		// Initialized
			;

		enum class ScopeRefType : std::uint8_t {
			MEMBER = 0,
			TYPE,
			MODULE
		};

		/// @brief Scope Reference Descriptor (SRD)
		struct ScopeRefDesc final {
			std::uint8_t lenName : 8;
			ScopeRefType type : 4;
			bool hasNext : 1;
			std::uint8_t reserved : 3;
		};
	}
}

#ifdef _MSC_VER
	#pragma pack(pop)
#endif

#endif
