#ifndef _SLAKE_OPCODE_H_
#define _SLAKE_OPCODE_H_

#include <cstdint>

namespace Slake {
	enum class Opcode : uint16_t {
		NOP = 0,  // No operation

		PUSH,  // Push an immediate value
		POP,   // Pop a value

		LOAD,	// Load a value onto stack
		RLOAD,	// Load a value onto stack by an existing reference
		STORE,	// Store a value from stack

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

		SARG,  // Store an argument
		LARG,  // Push an argument

		LTHIS,	// Load this
		STHIS,	// Store this

		CALL,	// Call and sets base for local addressing
		ACALL,	// Asynchronous Call
		RET,	// Return

		NEW,  // New

		LRET,  // Load return value

		THROW,	 // Throw an exception
		PUSHXH,	 // Push an exception handler

		ABORT,	// Abort

		CASTI8,	   // Cast to i8
		CASTI16,   // Cast to i16
		CASTI32,   // Cast to i32
		CASTI64,   // Cast to i64
		CASTU8,	   // Cast to u8
		CASTU16,   // Cast to u16
		CASTU32,   // Cast to u32
		CASTU64,   // Cast to u64
		CASTF32,   // Cast to f32
		CASTF64,   // Cast to f64
		CASTBOOL,  // Cast to boolean
		CASTOBJ,   // Cast to object

		TYPEOF,	 // Get type of an object

		OPCODE_MAX
	};
}

#endif
