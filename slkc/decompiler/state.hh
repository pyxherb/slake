#ifndef _SLKC_DECOMPILER_STATE_HH
#define _SLKC_DECOMPILER_STATE_HH

#include <slake/slxfmt.h>
#include <unordered_map>
#include <fstream>
#include <stdexcept>
#include <compiler/compiler.hh>

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
			std::unordered_map<std::uint32_t, std::string> labelNames;
		};

		std::shared_ptr<Compiler::Expr> readValue(std::fstream& fs);
		void decompileScope(std::fstream& fs);
		void decompile(std::fstream& fs);
	}
}

#endif
