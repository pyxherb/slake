#ifndef _SLKBCC_TRAIT_H_
#define _SLKBCC_TRAIT_H_

#include "operand.h"
#include <unordered_map>

namespace slake {
	namespace bcc {
		class Scope;

		class Trait : public ILocated {
		private:
			location _loc;

		public:
			shared_ptr<Scope> scope;
			deque<shared_ptr<Ref>> parents;
			AccessModifier access;

			inline Trait(
				location loc,
				AccessModifier access,
				deque<shared_ptr<Ref>> parents,
				shared_ptr<Scope> scope = make_shared<Scope>())
				: _loc(loc), scope(scope), parents(parents) {}
			virtual ~Trait() = default;

			virtual inline location getLocation() const override { return _loc; }
		};
	}
}

#endif
