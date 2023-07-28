#ifndef _SLAKE_OPCODE_H_
#define _SLAKE_OPCODE_H_

#include <cstdint>

namespace slake {
	enum class Opcode : uint16_t {
		NOP = 0,  // No operation

		PUSH,  // Push an immediate value
		POP,   // Pop a value

		LOAD,	// Load a value onto stack
		RLOAD,	// Load a value onto stack by an existing reference
		STORE,	// Store a value from stack

		LVAR,  // Create a new local variable

		LLOAD,	 // Load a value from local variable area
		LSTORE,	 // Store a value into local variable area

		LVALUE,	 // Load value of a variable

		EXPAND,	 // Expand stack pointer
		SHRINK,	 // Shrink stack pointer

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

		REV,   // Bitwise NOT
		NOT,   // Logical NOT
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
		LARG,	  // Load an argument

		LTHIS,	// Load this

		CALL,	 // Call
		MCALL,	 // Method Call
		ACALL,	 // Asynchronous Call
		AMCALL,	 // Asynchronous Method Call
		RET,	 // Return

		NEW,  // New

		LRET,  // Load return value

		THROW,	  // Throw an exception
		PUSHXH,	  // Push an exception handler
		LEXCEPT,  // Load current exception

		ABORT,	// Abort

		CAST,  // Cast

		TYPEOF,	 // Get type of an object

		OPCODE_MAX
	};
}

#endif
