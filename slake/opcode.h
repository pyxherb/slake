#ifndef _SLAKE_OPCODE_H_
#define _SLAKE_OPCODE_H_

#include <cstdint>
#include <map>
#include <string>

namespace slake {
	enum class Opcode : uint16_t {
		NOP = 0,  // No operation

		PUSH,  // Push an element into the stack.
		POP,   // Pop an element from the stack.

		LOAD,	// Load value of a variable.
		RLOAD,	// Access and load corresponding member with an existing value.
		STORE,	// Store a value into a variable.

		LVAR,  // Create a new local variable
		REG,   // Create a new register

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
		SEQ,   // Strictly Equal
		SNEQ,  // Strictly Not Equal
		LT,	   // Less than
		GT,	   // Greater than
		LTEQ,  // Less than or equal
		GTEQ,  // Greater than or equal
		LSH,   // Left shift
		RSH,   // Right shift
		SWAP,  // Swap

		NOT,   // Bitwise NOT
		LNOT,  // Logical NOT
		INCF,  // Forward Increase
		DECF,  // Forward Decrease
		INCB,  // Backward Increase
		DECB,  // Backward Decrease
		NEG,   // Negate

		AT,	 // Subscript

		JMP,  // Jump
		JT,	  // Jump if true
		JF,	  // Jump if false

		PUSHARG,  // Push an value into the argument stack

		CALL,	// Call
		MCALL,	// Method Call
		RET,	// Return

		LRET,  // Load return value

		ACALL,	 // Asynchronous Call
		AMCALL,	 // Asynchronous Method Call
		YIELD,	 // Yield
		AWAIT,	 // Await

		LTHIS,	// Load this register

		NEW,	 // New
		ARRNEW,	 // Array new

		THROW,	  // Throw an exception
		PUSHXH,	  // Push an exception handler
		LEXCEPT,  // Load current exception

		ABORT,	// Abort

		CAST,  // Cast

		TYPEOF,	 // Get type of an object

		CONSTSW,  // Constant switch

		OPCODE_MAX
	};

	extern const std::map<Opcode, std::string> OPCODE_MNEMONIC_MAP;
	extern const std::map<std::string, Opcode> MNEMONIC_OPCODE_MAP;
}

#endif
