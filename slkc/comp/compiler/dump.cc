#include "../compiler.h"

using namespace slkc;

SLKC_API Writer::~Writer() {
}

SLKC_API std::optional<CompilationError> slkc::dumpModule(
	peff::Alloc *allocator,
	Writer *writer,
	slake::ModuleObject *mod) {
	peff::DynArray<slake::ClassObject *> collectedClasses(allocator);
	peff::DynArray<slake::InterfaceObject *> collectedInterfaces(allocator);
	peff::DynArray<slake::FnObject *> collectedFns(allocator);

	for (auto [k, v] : mod->scope->members) {
		switch (v->getKind()) {
			case slake::ObjectKind::Class: {
				if (!collectedClasses.pushBack((slake::ClassObject *)v)) {
					return genOutOfMemoryCompError();
				}
				break;
			}
			case slake::ObjectKind::Interface: {
				if (!collectedInterfaces.pushBack((slake::InterfaceObject *)v)) {
					return genOutOfMemoryCompError();
				}
				break;
			}
			case slake::ObjectKind::Fn: {
				if (!collectedFns.pushBack((slake::FnObject *)v)) {
					return genOutOfMemoryCompError();
				}
				break;
			}
			default:
				break;
		}
	}

	slake::slxfmt::ImgHeader ih = {};

	memcpy(ih.magic, slake::slxfmt::IMH_MAGIC, sizeof(ih.magic));

	ih.fmtVer = 0x02;

	return {};
}
