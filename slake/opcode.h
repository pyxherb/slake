#ifndef _SLAKE_OPCODE_H_
#define _SLAKE_OPCODE_H_

#include <cstdint>
#include <map>
#include <string>
#include "basedefs.h"

namespace slake {
	enum class Opcode : uint8_t {
		INVALID = 0,  // Invalid

		LVALUE,	 // Load value of a variable
		STORE,	 // Store a value into a variable.

		JMP,  // Jump
		BR,	  // Branch

		PHI,  // Phi

		LARGV,	// Load value of an argument

		ADD,   // Add
		SUB,   // Subtract
		MUL,   // Multiply
		DIV,   // Divide
		MOD,   // Modulo
		AND,   // AND
		OR,	   // OR
		XOR,   // XOR
		LAND,  // Logical AND
		LOR,   // Logical OR
		EQ,	   // Equal
		NEQ,   // Not Equal
		LT,	   // Less than
		GT,	   // Greater than
		LTEQ,  // Less than or equal
		GTEQ,  // Greater than or equal
		LSH,   // Left shift
		RSH,   // Right shift
		CMP,   // Compare

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

		CAST,  // Cast

		APTOTUPLE,	// Cast an argument pack to a tuple

		DCMT,  // Downcast method table

		TYPEOF,	 // Get type of an object

		CONSTSW,  // Constant switch

		OPCODE_MAX
	};
}

#endif
