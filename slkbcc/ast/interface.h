#ifndef _SLKBCC_INTERFACE_H_
#define _SLKBCC_INTERFACE_H_

#include "operand.h"
#include <unordered_map>

namespace slake {
	namespace bcc {
		class Scope;

		class Interface : public ILocated {
		private:
			location _loc;

		public:
			shared_ptr<Scope> scope;
			deque<shared_ptr<Ref>> parents;
			AccessModifier access;

			inline Interface(
				location loc,
				AccessModifier access,
				deque<shared_ptr<Ref>> parents,
				shared_ptr<Scope> scope = make_shared<Scope>())
				: _loc(loc), scope(scope), parents(parents) {}
			virtual ~Interface() = default;

			virtual inline location getLocation() const override { return _loc; }
		};
	}
}

#endif
