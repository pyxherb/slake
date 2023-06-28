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

		LVAR,	   // Create a new local variable
		LVARI8,	   // Create a new i8 local variable
		LVARI16,   // Create a new i16 local variable
		LVARI32,   // Create a new i32 local variable
		LVARI64,   // Create a new i64 local variable
		LVARU8,	   // Create a new u8 local variable
		LVARU16,   // Create a new u16 local variable
		LVARU32,   // Create a new u32 local variable
		LVARU64,   // Create a new u64 local variable
		LVARF32,   // Create a new f32 local variable
		LVARF64,   // Create a new f64 local variable
		LVARBOOL,  // Create a new boolean local variable
		LVAROBJ,   // Create a new object local variable

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

		CALL,	 // Call
		MCALL,	 // Method Call
		ACALL,	 // Asynchronous Call
		AMCALL,	 // Asynchronous Method Call
		RET,	 // Return

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
