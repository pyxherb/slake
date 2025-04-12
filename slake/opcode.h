#ifndef _SLAKE_OPCODE_H_
#define _SLAKE_OPCODE_H_

#include <cstdint>
#include <map>
#include <string>
#include "basedefs.h"

namespace slake {
	enum class Opcode : uint8_t {
		NOP = 0,  // No operation

		LOAD,	// Load value of a variable.
		RLOAD,	// Access and load corresponding member with an existing value.
		STORE,	// Store a value into a variable.

		MOV,  // Move values between registers

		LARG,  // Load an argument.

		LVAR,  // Create a new local variable

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

		CALL,	   // Call
		MCALL,	   // Method Call
		CTORCALL,  // Constructor Call
		RET,	   // Return

		YIELD,	// Yield

		LTHIS,	// Load this register

		NEW,	 // New
		ARRNEW,	 // Array new

		THROW,	  // Throw an exception
		PUSHXH,	  // Push an exception handler
		LEXCEPT,  // Load current exception

		CAST,  // Cast

		TYPEOF,	 // Get type of an object

		CONSTSW,  // Constant switch

		PHI,  // Phi

		OPCODE_MAX
	};

	SLAKE_API extern const std::map<Opcode, std::string> OPCODE_MNEMONIC_MAP;
	SLAKE_API extern const std::map<std::string, Opcode> MNEMONIC_OPCODE_MAP;
}

#endif
