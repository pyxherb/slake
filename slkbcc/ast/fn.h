#ifndef _SLKBCC_FN_H_
#define _SLKBCC_FN_H_

#include "operand.h"
#include <slake/opcode.h>
#include <unordered_map>

namespace slake {
	namespace bcc {
		class Instruction final : public ILocated {
		protected:
			location _loc;

		public:
			Opcode opcode;
			deque<shared_ptr<Operand>> operands;

			inline Instruction(location loc, Opcode opcode, deque<shared_ptr<Operand>> operands)
				: _loc(loc), opcode(opcode), operands(operands) {}
			virtual ~Instruction() = default;

			virtual inline location getLocation() const override { return _loc; }
		};

		class ParamDeclList final : public deque<shared_ptr<TypeName>> {
		public:
			bool isVariadic = false;
			ParamDeclList() = default;
			virtual ~ParamDeclList() = default;
		};

		class Fn : public ILocated {
		private:
			location _loc;

		public:
			shared_ptr<TypeName> returnType;
			ParamDeclList params;
			deque<shared_ptr<Instruction>> body;
			AccessModifier access = 0;
			unordered_map<string, uint32_t> labels;

			inline Fn(
				location loc,
				AccessModifier access,
				shared_ptr<TypeName> returnType,
				ParamDeclList params = {},
				deque<shared_ptr<Instruction>> body = {},
				unordered_map<string, uint32_t> labels = {})
				: _loc(loc),
				  returnType(returnType),
				  params(params),
				  body(body),
				  labels(labels),
				  access(access) {}
			virtual ~Fn() = default;

			virtual inline location getLocation() const override { return _loc; }
		};
	}
}

#endif
