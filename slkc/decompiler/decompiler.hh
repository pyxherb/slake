#ifndef _SLKC_DECOMPILER_DECOMPILER_HH
#define _SLKC_DECOMPILER_DECOMPILER_HH

#include <slake/slxfmt.h>

#include <compiler/compiler.hh>
#include <fstream>
#include <stdexcept>
#include <unordered_map>

namespace Slake {
	namespace Decompiler {
		class DecompileError : public std::runtime_error {
		public:
			inline DecompileError(std::string msg) : runtime_error(msg) {}
			virtual inline ~DecompileError() {}
		};

		struct Ins {
			Opcode opcode;
			std::vector<std::shared_ptr<Compiler::Expr>> operands;

			inline Ins(Opcode opcode, std::initializer_list<std::shared_ptr<Compiler::Expr>> operands) {
				this->opcode = opcode;
				this->operands = operands;
			}
		};

		struct State {
			std::unordered_map<uint32_t, std::string> labelNames;
		};

		std::shared_ptr<Compiler::Expr> readValue(std::fstream& fs);
		void decompileScope(std::fstream& fs, uint8_t indentLevel = 0);
		void decompile(std::fstream& fs);
	}
}

#endif
