#ifndef _SLKC_COMPILER_FN_HH
#define _SLKC_COMPILER_FN_HH

#include "localvar.hh"
#include <slake/opcode.h>

namespace Slake {
	namespace Compiler {
		struct Ins {
			Opcode opcode;
			std::vector<std::shared_ptr<Expr>> operands;

			inline Ins(Opcode opcode, std::vector<std::shared_ptr<Expr>> operands) : opcode(opcode), operands(operands) {}
		};

		struct Fn {
			std::vector<Ins> body;
			std::unordered_map<std::string, std::uint32_t> labels;
			std::unordered_map<std::string, LocalVar> lvars;

			inline void insertLabel(std::string name) { labels[name] = body.size(); };
			inline std::uint32_t getLabel(std::string name) { return labels.at(name); }

			inline void insertIns(const Ins& ins) {
				body.push_back(ins);
			}
			template <typename... Args>
			inline void insertIns(const Ins& ins, Args... args) {
				insertIns(ins);
				insertIns(args...);
			}
		};
	}
}

#endif
