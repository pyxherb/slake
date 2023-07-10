#ifndef _SLKBCC_VAR_H_
#define _SLKBCC_VAR_H_

#include "operand.h"

namespace slake {
	namespace bcc {
		class Var : public ILocated {
		private:
			location _loc;

		public:
			shared_ptr<TypeName> type;
			shared_ptr<Operand> initValue;
			AccessModifier access;

			inline Var(location loc, AccessModifier access, shared_ptr<TypeName> type, shared_ptr<Operand> initValue = {})
				: _loc(loc), type(type), initValue(initValue) {}
			virtual ~Var() = default;

			virtual inline location getLocation() const override { return _loc; }
		};
	}
}

#endif
