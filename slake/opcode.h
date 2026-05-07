#ifndef _SLAKE_OPCODE_H_
#define _SLAKE_OPCODE_H_

#include <cstdint>
#include <map>
#include <string>
#include "basedefs.h"

namespace slake {
	enum class Opcode : uint16_t {
		INVALID = 0,  // Invalid

		LVALUE,	 // Load value of a variable
		STORE,	 // Store a value into a variable.

		JMP,  // Jump
		BR,	  // Branch

		PHI,  // Phi

		LARGV,	// Load value of an argument

		// Add
		ADDI8,
		ADDI16,
		ADDI32,
		ADDI64,
		ADDISIZE,
		ADDU8,
		ADDU16,
		ADDU32,
		ADDU64,
		ADDUSIZE,
		ADDF32,
		ADDF64,

		// Subtract
		SUBI8,
		SUBI16,
		SUBI32,
		SUBI64,
		SUBISIZE,
		SUBU8,
		SUBU16,
		SUBU32,
		SUBU64,
		SUBUSIZE,
		SUBF32,
		SUBF64,

		// Multiply
		MULI8,
		MULI16,
		MULI32,
		MULI64,
		MULISIZE,
		MULU8,
		MULU16,
		MULU32,
		MULU64,
		MULUSIZE,
		MULF32,
		MULF64,

		// Divide
		DIVI8,
		DIVI16,
		DIVI32,
		DIVI64,
		DIVISIZE,
		DIVU8,
		DIVU16,
		DIVU32,
		DIVU64,
		DIVUSIZE,
		DIVF32,
		DIVF64,

		// Modulo
		MODI8,
		MODI16,
		MODI32,
		MODI64,
		MODISIZE,
		MODU8,
		MODU16,
		MODU32,
		MODU64,
		MODUSIZE,
		MODF32,
		MODF64,

		// AND
		ANDI8,
		ANDI16,
		ANDI32,
		ANDI64,
		ANDISIZE,
		ANDU8,
		ANDU16,
		ANDU32,
		ANDU64,
		ANDUSIZE,
		ANDBOOL,

		// OR
		ORI8,
		ORI16,
		ORI32,
		ORI64,
		ORISIZE,
		ORU8,
		ORU16,
		ORU32,
		ORU64,
		ORUSIZE,
		ORBOOL,

		// XOR
		XORI8,
		XORI16,
		XORI32,
		XORI64,
		XORISIZE,
		XORU8,
		XORU16,
		XORU32,
		XORU64,
		XORUSIZE,

		// Equal
		EQI8,
		EQI16,
		EQI32,
		EQI64,
		EQISIZE,
		EQU8,
		EQU16,
		EQU32,
		EQU64,
		EQUSIZE,
		EQF32,
		EQF64,
		EQBOOL,
		EQREF,
		EQTYPE,

		// Not Equal
		NEQI8,
		NEQI16,
		NEQI32,
		NEQI64,
		NEQISIZE,
		NEQU8,
		NEQU16,
		NEQU32,
		NEQU64,
		NEQUSIZE,
		NEQF32,
		NEQF64,
		NEQBOOL,
		NEQSTR,
		NEQREF,
		NEQTYPE,

		// Less Than
		LTI8,
		LTI16,
		LTI32,
		LTI64,
		LTISIZE,
		LTU8,
		LTU16,
		LTU32,
		LTU64,
		LTUSIZE,
		LTF32,
		LTF64,

		// Greater Than
		GTI8,
		GTI16,
		GTI32,
		GTI64,
		GTISIZE,
		GTU8,
		GTU16,
		GTU32,
		GTU64,
		GTUSIZE,
		GTF32,
		GTF64,

		// Less Than or Equal
		LTEQI8,
		LTEQI16,
		LTEQI32,
		LTEQI64,
		LTEQISIZE,
		LTEQU8,
		LTEQU16,
		LTEQU32,
		LTEQU64,
		LTEQUSIZE,
		LTEQF32,
		LTEQF64,

		// Greater Than or Equal
		GTEQI8,
		GTEQI16,
		GTEQI32,
		GTEQI64,
		GTEQISIZE,
		GTEQU8,
		GTEQU16,
		GTEQU32,
		GTEQU64,
		GTEQUSIZE,
		GTEQF32,
		GTEQF64,

		// Left Shift
		SHLI8,
		SHLI16,
		SHLI32,
		SHLI64,
		SHLISIZE,
		SHLU8,
		SHLU16,
		SHLU32,
		SHLU64,
		SHLUSIZE,

		// Right Shift
		SHRI8,
		SHRI16,
		SHRI32,
		SHRI64,
		SHRISIZE,
		SHRU8,
		SHRU16,
		SHRU32,
		SHRU64,
		SHRUSIZE,

		// Compare
		CMPI8,
		CMPI16,
		CMPI32,
		CMPI64,
		CMPISIZE,
		CMPU8,
		CMPU16,
		CMPU32,
		CMPU64,
		CMPUSIZE,
		CMPF32,
		CMPF64,

		NOT,   // Bitwise NOT
		LNOT,  // Logical NOT
		NEG,   // Negate

		AT,	 // Subscript

		LOAD,	// Load value of a variable.
		RLOAD,	// Access and load corresponding member with an existing value.

		LCURFN,	 // Load current function.

		COPYI8,		// Fast copy for i8 literals
		COPYI16,	// Fast copy for i16 literals
		COPYI32,	// Fast copy for i32 literals
		COPYI64,	// Fast copy for i64 literals
		COPYISIZE,	// Fast copy for isize literals
		COPYU8,		// Fast copy for u8 literals
		COPYU16,	// Fast copy for u16 literals
		COPYU32,	// Fast copy for u32 literals
		COPYU64,	// Fast copy for u64 literals
		COPYUSIZE,	// Fast copy for usize literals
		COPYF32,	// Fast copy for f32 literals
		COPYF64,	// Fast copy for f64 literals
		COPYBOOL,	// Fast copy for bool literals
		COPYNULL,	// Fast copy for null literals

		COPY,  // Copy values between registers

		LARG,	 // Load an argument.
		LAPARG,	 // Load an argument pack.

		LVAR,	 // Create a new local variable
		ALLOCA,	 // Allocate spaces from the stack

		ENTER,	// Enter frame
		LEAVE,	// Leave frame

		PUSHARG,  // Push an value into the argument stack
		PUSHAP,	  // Push an argument pack.

		CALL,	   // Call
		MCALL,	   // Method Call
		CTORCALL,  // Constructor Call
		RET,	   // Return

		COCALL,	  // Coroutine call
		COMCALL,  // Coroutine method call
		YIELD,	  // Yield
		RESUME,	  // Resume
		CODONE,	  // Is coroutine done?

		LTHIS,	// Load this register

		NEW,	 // New
		ARRNEW,	 // Array new

		THROW,	  // Throw an exception
		PUSHEH,	  // Push an exception handler
		LEXCEPT,  // Load current exception

		CAST,	   // Cast
		NULLCAST,  // Nullable cast

		APTOTUPLE,	// Cast an argument pack to a tuple

		DCMT,  // Downcast method table

		TYPEOF,	 // Get type of an object

		CONSTSW,  // Constant switch

		OPCODE_MAX
	};
}

#endif
