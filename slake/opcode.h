#ifndef _SLAKE_OPCODE_H_
#define _SLAKE_OPCODE_H_

#include <cstdint>
#include <map>
#include <string>
#include "basedefs.h"

namespace slake {
	enum class Opcode : uint8_t {
		INVALID = 0,  // Invalid

		LOAD,	// Load value of a variable.
		RLOAD,	// Access and load corresponding member with an existing value.
		STORE,	// Store a value into a variable.

		MOV,  // Move values between registers

		LARG,	 // Load an argument.
		LAPARG,	 // Load an argument pack.

		LVAR,	 // Create a new local variable
		ALLOCA,	 // Allocate spaces from the stack

		LVALUE,	 // Load value of a variable

		ENTER,	// Enter frame
		LEAVE,	// Leave frame

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

		JMP,  // Jump
		JT,	  // Jump if true
		JF,	  // Jump if false

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
		PUSHXH,	  // Push an exception handler
		LEXCEPT,  // Load current exception

		CAST,  // Cast

		APTOTUPLE,	// Cast an argument pack to a tuple

		TYPEOF,	 // Get type of an object

		CONSTSW,  // Constant switch

		PHI,  // Phi

		OPCODE_MAX
	};
}

#endif
