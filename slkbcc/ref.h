#ifndef _SLKBCC_REF_H_
#define _SLKBCC_REF_H_

#include "typename.h"
#include <string>
#include <deque>

namespace Slake {
	namespace Assembler {
		class RefScope : public ILocated {
		private:
			location _loc;

		public:
			string name;
			deque<shared_ptr<TypeName>> genericArgs;

			inline RefScope(
				location loc,
				string name,
				deque<shared_ptr<TypeName>> genericArgs = {})
				: _loc(loc), name(name), genericArgs(genericArgs) {}
			virtual ~RefScope() = default;

			virtual location getLocation() const override { return _loc; }
		};

		class Ref : public ILocated {
		public:
			deque<shared_ptr<RefScope>> scopes = {};

			inline Ref(deque<shared_ptr<RefScope>> scopes = {})
				: scopes(scopes) {}
			virtual ~Ref() = default;

			virtual location getLocation() const override { return scopes[0]->getLocation(); }
		};
	}
}

#endif
