///
/// @file vzx.h
/// @author CodesBuilder (2602783536@qq.com)
/// @brief Definitions for Vizard Executable (VZX) format.
/// @version 0.1
/// @date 2022-09-18
///
/// @copyright Copyright (c) 2022 Vizard Contributors
///
#ifndef _VIZARD_VZRT_FMT_VZX_H_
#define _VIZARD_VZRT_FMT_VZX_H_

#include <cstdint>

namespace Vz {
	namespace Fmt {
		namespace Vzx {
			///
			/// @brief IMage Header (IMH)
			///
			struct ImgHeader final {
				// Magic number
				constexpr static std::uint8_t MAG0 = 'V', MAG1 = 'z', MAG2 = 'E', MAG3 = 'x';

				// Flags
				constexpr static std::uint8_t
					IMHFL_BE = 0x01,  // Big-endian
					IMHFL_E = 0x02;	  // Entry

				std::uint8_t magic[4];	   // Magic number
				std::uint8_t flags;		   // Flags
				std::uint8_t fmtVer;	   // Format version
				std::uint8_t reserved[2];  // Reserved
			};

			///
			/// @brief Section Attribute Descriptor (SAD)
			///
			struct SectionAttrDesc final {
				std::uint32_t size : 24;  // Size of section in 128 bytes
				std::uint8_t flags : 8;	  // Flags
			};

			///
			/// @brief Instruction Header (IH)
			///
			struct InsHeader final {
				std::uint8_t opcode : 6;	// Operation code
				std::uint8_t nOperand : 2;	// Number of operands
			};

			enum class ValueType : std::uint8_t {
				NUL = 0,  // null, also represents `void'
				I8,		  // i8
				I16,	  // i16
				I32,	  // i32
				I64,	  // i64
				U8,		  // u8
				U16,	  // u16
				U32,	  // u32
				U64,	  // u64
				STRING,	  // string
				ARRAY,	  // array
				MAP,	  // map
				UUID	  // UUID
			};

			///
			/// @brief Value Descriptor (VD)
			///
			struct ValueDesc final {
				ValueType type : 5;		 // Data Type
				std::uint8_t flags : 3;	 // Flags
			};

			///
			/// @brief Extra attribute for strings
			///
			struct StringExAttr final {
				std::uint32_t len;	// Length in bytes
			};

			///
			/// @brief Extra attributes for arrays
			///
			struct ArrayExAttr final {
				ValueType type : 5;		 // Element data type
				std::uint32_t len : 27;	 // Element count
			};

			///
			/// @brief Extra attributes for maps
			///
			struct MapExAttr final {
				ValueType keyType : 5;	  // Key data type
				ValueType valueType : 5;  // Value data type
				std::uint32_t len : 30;	  // Entry count
			};
		}
	}
}

#endif
