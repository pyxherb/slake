#include "../compiler.h"

using namespace slake;
using namespace slake::slkc;

void Compiler::registerBuiltinTypedefs() {
	_i32Class = std::make_shared<ClassNode>(this, "i32");
	_i32Class->implInterfaces.push_back(
		std::make_shared<CustomTypeNameNode>(
			std::make_shared<IdRefNode>(
				IdRefEntries{ { "core" },
					{ "traits" },
					{ "IComparable", { std::make_shared<I32TypeNameNode>(SIZE_MAX) } } }),
			this,
			_rootScope.get()));
}
