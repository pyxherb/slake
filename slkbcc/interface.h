#ifndef _SLKBCC_INTERFACE_H_
#define _SLKBCC_INTERFACE_H_

#include "operand.h"
#include <unordered_map>

namespace Slake {
	namespace Assembler {
		class Scope;

		class Interface : public ILocated {
		private:
			location _loc;

		public:
			shared_ptr<Scope> scope;
			shared_ptr<Ref> parent;
			AccessModifier access;

			inline Interface(
				location loc,
				shared_ptr<Ref> parent,
				AccessModifier access,
				shared_ptr<Scope> scope)
				: _loc(loc), scope(scope), parent(parent) {}
			virtual ~Interface() = default;

			virtual inline location getLocation() const override { return _loc; }
		};
	}
}

#endif
