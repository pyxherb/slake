#ifndef _SWAMPEAK_OPCODE_H_
#define _SWAMPEAK_OPCODE_H_

#include <cstdint>

namespace Swampeak {
	enum class Opcode : std::uint8_t {
		NOP = 0,  // No operation

		PUSH,	 // Push an immediate value
		POP,	 // Pop a value
		LOAD,	 // Load a value onto stack
		STORE,	 // Store a value from stack
		LLOAD,	 // Load a value from local variable area
		LSTORE,	 // Store a value into local variable area

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
		NOT,   // NOT
		LNOT,  // Logical NOT

		JMP,  // Jump
		JT,	  // Jump if true
		JF,	  // Jump if false

		CALL,  // Call
		RET,   // Return

		SYSCALL	 // System call
	};
}

#endif
