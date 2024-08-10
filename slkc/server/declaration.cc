#include "server.h"
#include "../compiler/compiler.h"

using namespace slake;
using namespace slake::slkc;

Json::Value Document::extractTypeName(std::shared_ptr<TypeNameNode> typeName) {
	return std::to_string(typeName, compiler.get());
}

Json::Value Document::extractDeclaration(std::shared_ptr<VarNode> m) {
	Json::Value value;
	Json::Value &metadataValue = value["metadata"];

	value["declarationKind"] = m->isProperty ? (uint32_t)DeclarationKind::Property : (uint32_t)DeclarationKind::Var;

	std::string fullName = std::to_string(compiler->getFullName(m.get()), compiler.get());

	metadataValue["fullName"] = fullName;
	metadataValue["type"] = extractTypeName(m->type ? m->type : std::make_shared<AnyTypeNameNode>(SIZE_MAX));

	return value;
}

Json::Value Document::extractDeclaration(std::shared_ptr<ParamNode> m) {
	Json::Value value;
	Json::Value &metadataValue = value["metadata"];

	value["declarationKind"] = (uint32_t)DeclarationKind::Param;

	metadataValue["name"] = m->name;
	metadataValue["type"] = extractTypeName(m->type ? m->type : std::make_shared<AnyTypeNameNode>(SIZE_MAX));

	return value;
}

Json::Value Document::extractDeclaration(std::shared_ptr<LocalVarNode> m) {
	Json::Value value;
	Json::Value &metadataValue = value["metadata"];

	value["declarationKind"] = (uint32_t)DeclarationKind::LocalVar;

	metadataValue["name"] = m->name;
	metadataValue["type"] = extractTypeName(m->type ? m->type : std::make_shared<AnyTypeNameNode>(SIZE_MAX));

	return value;
}

Json::Value Document::extractDeclaration(std::shared_ptr<FnOverloadingNode> m) {
	Json::Value value;
	Json::Value &metadataValue = value["metadata"];

	value["declarationKind"] = (uint32_t)DeclarationKind::FnOverloading;

	metadataValue["fullName"] = std::to_string(compiler->getFullName(m.get()), compiler.get());
	metadataValue["returnType"] = extractTypeName((m->returnType ? m->returnType : std::make_shared<VoidTypeNameNode>(SIZE_MAX)));

	for (size_t i = 0; i < m->params.size(); ++i) {
		if (m->params[i]->name == "...") {
			metadataValue["hasVaridicParams"] = true;
		} else {
			metadataValue["paramDecls"].append(extractDeclaration(m->params[i]));
		}
	}

	return value;
}

Json::Value Document::extractDeclaration(std::shared_ptr<GenericParamNode> m) {
	Json::Value value;
	Json::Value &metadataValue = value["metadata"];

	value["declarationKind"] = (uint32_t)DeclarationKind::GenericParam;

	metadataValue["name"] = m->name;

	return value;
}

Json::Value Document::extractDeclaration(std::shared_ptr<ClassNode> m) {
	Json::Value value;
	Json::Value &metadataValue = value["metadata"];

	value["declarationKind"] = (uint32_t)DeclarationKind::Class;

	metadataValue["fullName"] = std::to_string(compiler->getFullName(m.get()), compiler.get());
	if (m->documentation.size()) {
		value["documentation"] = m->documentation;
	}

	return value;
}

Json::Value Document::extractDeclaration(std::shared_ptr<InterfaceNode> m) {
	Json::Value value;
	Json::Value &metadataValue = value["metadata"];

	value["declarationKind"] = (uint32_t)DeclarationKind::Interface;

	metadataValue["fullName"] = std::to_string(compiler->getFullName(m.get()), compiler.get());

	return value;
}

Json::Value Document::extractDeclaration(std::shared_ptr<ModuleNode> m) {
	Json::Value value;
	Json::Value &metadataValue = value["metadata"];

	value["declarationKind"] = (uint32_t)DeclarationKind::Module;

	metadataValue["fullName"] = std::to_string(compiler->getFullName(m.get()), compiler.get());

	return value;
}
