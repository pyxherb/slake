#ifndef _SLKBCC_REF_H_
#define _SLKBCC_REF_H_

#include "typename.h"
#include <string>
#include <deque>

namespace slake {
	namespace bcc {
		class RefEntry : public ILocated {
		private:
			location _loc;

		public:
			string name;
			deque<shared_ptr<TypeName>> genericArgs;

			inline RefEntry(
				location loc,
				string name,
				deque<shared_ptr<TypeName>> genericArgs = {})
				: _loc(loc), name(name), genericArgs(genericArgs) {}
			virtual ~RefEntry() = default;

			virtual location getLocation() const override { return _loc; }
		};

		class Ref : public ILocated {
		public:
			deque<shared_ptr<RefEntry>> entries = {};

			inline Ref(deque<shared_ptr<RefEntry>> entries = {})
				: entries(entries) {}
			virtual ~Ref() = default;

			virtual location getLocation() const override { return entries[0]->getLocation(); }
		};
	}
}

#endif
