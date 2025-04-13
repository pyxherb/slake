#include "../compiler.h"

using namespace slkc;

SLKC_API Writer::~Writer() {
}

SLKC_API std::optional<CompilationError> slkc::dumpModule(
	peff::Alloc *allocator,
	Writer *writer,
	const peff::SharedPtr<ModuleNode> &mod) {
	peff::DynArray<peff::SharedPtr<ImportNode>> collectedImports(allocator);
	peff::DynArray<peff::SharedPtr<ClassNode>> collectedClasses(allocator);
	peff::DynArray<peff::SharedPtr<InterfaceNode>> collectedInterfaces(allocator);
	peff::DynArray<peff::SharedPtr<FnSlotNode>> collectedFnSlots(allocator);

	for (auto &[k, v] : mod->memberIndices) {
		peff::SharedPtr<MemberNode> m = mod->members.at(v);

		switch (m->astNodeType) {
			case AstNodeType::Import: {
				peff::SharedPtr<ImportNode> importNode = m.castTo<ImportNode>();

				if (!collectedImports.pushBack(std::move(importNode))) {
					return genOutOfMemoryCompError();
				}
				break;
			}
			case AstNodeType::Class: {
				peff::SharedPtr<ClassNode> classNode = m.castTo<ClassNode>();

				if (!collectedClasses.pushBack(std::move(classNode))) {
					return genOutOfMemoryCompError();
				}
				break;
			}
			case AstNodeType::Interface: {
				peff::SharedPtr<InterfaceNode> interfaceNode = m.castTo<InterfaceNode>();

				if (!collectedInterfaces.pushBack(std::move(interfaceNode))) {
					return genOutOfMemoryCompError();
				}
				break;
			}
			case AstNodeType::FnSlot: {
				peff::SharedPtr<FnSlotNode> fnSlotNode = m.castTo<FnSlotNode>();

				if (!collectedFnSlots.pushBack(std::move(fnSlotNode))) {
					return genOutOfMemoryCompError();
				}
				break;
			}
		}
	}

	for (auto i : mod->anonymousImports) {
		if (!collectedImports.pushBack(peff::SharedPtr<ImportNode>(i))) {
			return genOutOfMemoryCompError();
		}
	}

	slake::slxfmt::ImgHeader ih = {};

	memcpy(ih.magic, slake::slxfmt::IMH_MAGIC, sizeof(ih.magic));

	ih.fmtVer = 0x02;

	if (collectedImports.size() > UINT16_MAX) {
		return CompilationError(collectedImports.at(UINT16_MAX)->tokenRange, CompilationErrorKind::ImportLimitExceeded);
	}
	ih.nImports = collectedImports.size();
}
