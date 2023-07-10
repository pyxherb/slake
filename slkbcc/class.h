#ifndef _SLKBCC_CLASS_H_
#define _SLKBCC_CLASS_H_

#include "operand.h"

namespace slake {
	namespace bcc {
		class Scope;

		class Class : public ILocated {
		private:
			location _loc;

		public:
			shared_ptr<Ref> parent;
			shared_ptr<Scope> scope;
			AccessModifier access;
			deque<shared_ptr<Ref>> impls;

			inline Class(
				location loc,
				AccessModifier access,
				shared_ptr<Ref> parent,
				shared_ptr<Scope> scope = make_shared<Scope>())
				: _loc(loc), parent(parent), scope(scope) {}
			virtual ~Class() = default;

			virtual inline location getLocation() const override { return _loc; }
		};
	}
}

#endif
