#ifndef _SLKBC_INS_H_
#define _SLKBC_INS_H_

#include <slake/opcode.h>
#include <cstdint>

enum Operand : std::uint8_t {
	OPRD_VALUE,
	OPRD_INT,
	OPRD_FPNUM,
	OPRD_CODEREF,
	OPRD_DATAREF,
	OPRD_UUID,
	OPRD_END = 0xff
};

struct InsRegistry {
	const char* mnemonic;
	Slake::Opcode opcode;
	Operand operands[4];
};

extern InsRegistry insRegistries[];

#endif
