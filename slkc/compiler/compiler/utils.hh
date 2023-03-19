#ifndef _SLKC_COMPILER_UTILS_HH
#define _SLKC_COMPILER_UTILS_HH

#include "state.hh"
#include <slake/slxfmt.h>
#include <fstream>

namespace Slake {
	namespace Compiler {
		template <typename T>
		inline void _writeValue(const T &value, std::fstream &fs) {
			fs.write((char *)&value, sizeof(value));
		}
		template <typename T>
		inline void _writeValue(const T &&value, std::fstream &fs) {
			T v = value;
			_writeValue(v, fs);
		}
		template <typename T>
		inline void _writeValue(const T &value, std::streamsize size, std::fstream &fs) {
			fs.write((char *)&value, size);
		}
		void writeValueDesc(std::shared_ptr<State> s, std::shared_ptr<Expr> src, std::fstream &fs);
		void writeIns(std::shared_ptr<State> s, Opcode opcode, std::fstream &fs, std::initializer_list<std::shared_ptr<Expr>> operands = {});

		extern const std::unordered_map<LiteralType, SlxFmt::ValueType> _lt2vtMap;
		extern const std::unordered_map<LiteralType, TypeNameKind> _lt2tnKindMap;
		extern const std::unordered_map<TypeNameKind, SlxFmt::ValueType> _tnKind2vtMap;
		extern const std::unordered_map<UnaryOp, Opcode> _unaryOp2opcodeMap;
		extern const std::unordered_map<BinaryOp, Opcode> _binaryOp2opcodeMap;
	}
}

#endif
