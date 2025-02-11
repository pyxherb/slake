#include "bc2cxx.h"

using namespace slake;
using namespace slake::slkaot;
using namespace slake::slkaot::bc2cxx;

bool BC2CXX::_isSimpleIdExpr(std::shared_ptr<cxxast::Expr> expr) {
	switch (expr->exprKind) {
		case cxxast::ExprKind::Id:
			return true;
		case cxxast::ExprKind::Binary: {
			std::shared_ptr<cxxast::BinaryExpr> e = std::static_pointer_cast<cxxast::BinaryExpr>(expr);

			switch (e->op) {
				case cxxast::BinaryOp::Scope:
				case cxxast::BinaryOp::MemberAccess:
				case cxxast::BinaryOp::PtrAccess:
					return _isSimpleIdExpr(e->lhs) && _isSimpleIdExpr(e->rhs);
				default:
					return false;
			}
			break;
		}
	}

	return false;
}

std::shared_ptr<cxxast::Expr> BC2CXX::_getAbsRef(std::shared_ptr<cxxast::AbstractMember> m) {
	std::shared_ptr<cxxast::Expr> e = std::make_shared<cxxast::IdExpr>(std::string(m->name));
	std::weak_ptr<cxxast::AbstractModule> mod = m->parent;

	while (!mod.expired()) {
		std::shared_ptr<cxxast::AbstractModule> modPtr = mod.lock();

		e = std::make_shared<cxxast::BinaryExpr>(
			cxxast::BinaryOp::Scope,
			std::make_shared<cxxast::IdExpr>(std::string(modPtr->name)),
			e);

		mod = modPtr->parent;
	}

	return e;
}

std::shared_ptr<cxxast::Expr> BC2CXX::_getLastExpr(std::shared_ptr<cxxast::Expr> e) {
	switch (e->exprKind) {
		case cxxast::ExprKind::Unary:
			return _getLastExpr(std::static_pointer_cast<cxxast::UnaryExpr>(e)->operand);
		case cxxast::ExprKind::Binary:
			return _getLastExpr(std::static_pointer_cast<cxxast::BinaryExpr>(e)->rhs);
		default:
			return e;
	}
}

void BC2CXX::_applyGenericArgs(std::shared_ptr<cxxast::Expr> expr, cxxast::GenericArgList &&args) {
	std::shared_ptr<cxxast::Expr> e = _getLastExpr(expr);
	assert(e->exprKind == cxxast::ExprKind::Id);
	std::static_pointer_cast<cxxast::IdExpr>(e)->genericArgs = std::move(args);
}

std::string BC2CXX::mangleRefForTypeName(const peff::DynArray<IdRefEntry> &entries) {
	std::string name;

	for (size_t i = 0; i < entries.size(); ++i) {
		const IdRefEntry &idRefEntry = entries.at(i);

		if (i)
			name += "_";

		for (size_t j = 0; j < idRefEntry.name.size(); ++j) {
			char c[3];

			c[0] = (idRefEntry.name.at(j) & 0xf) + 'A';
			c[1] = (idRefEntry.name.at(j) >> 4) + 'A';
			c[2] = '\0';

			name += c;
		}

		name += "0";

		for (size_t j = 0; j < idRefEntry.genericArgs.size(); ++j) {
			if (j)
				name += "2";
			name += mangleTypeName(idRefEntry.genericArgs.at(j));
		}

		name += "1";
	}

	return name;
}

std::string BC2CXX::mangleTypeName(const Type &type) {
	switch (type.typeId) {
		case TypeId::None:
			return "void";
		case TypeId::Value: {
			switch (type.getValueTypeExData()) {
				case ValueType::I8:
					return "i8";
				case ValueType::I16:
					return "i16";
				case ValueType::I32:
					return "i32";
				case ValueType::I64:
					return "i64";
				case ValueType::U8:
					return "u8";
				case ValueType::U16:
					return "u16";
				case ValueType::U32:
					return "u32";
				case ValueType::U64:
					return "u64";
				case ValueType::F32:
					return "f32";
				case ValueType::F64:
					return "f64";
				case ValueType::Bool:
					return "bool";
				default:
					// Invalid type name, terminate.
					std::terminate();
			}
			break;
		}
		case TypeId::String:
			return "string";
		case TypeId::Instance: {
			if (type.isLoadingDeferred()) {
				HostObjectRef<IdRefObject> id = (IdRefObject *)type.getCustomTypeExData();

				return "obj" + mangleRefForTypeName(id->entries);
			} else {
				HostObjectRef<MemberObject> id = (MemberObject *)type.getCustomTypeExData();

				peff::DynArray<IdRefEntry> entries;
				if (!id->associatedRuntime->getFullRef(peff::getDefaultAlloc(), id.get(), entries))
					throw std::bad_alloc();

				return "obj" + mangleRefForTypeName(entries);
			}
		}
		case TypeId::Array: {
			return "arr" + mangleTypeName(type.getArrayExData());
		}
		case TypeId::FnDelegate: {
			std::string name = "fn";

			FnTypeDefObject *typeDef = (FnTypeDefObject *)type.exData.typeDef;

			name += mangleTypeName(typeDef->returnType);

			name += "0";

			for (size_t i = 0; i < typeDef->paramTypes.size(); ++i) {
				if (i)
					name += "2";
				name += mangleTypeName(typeDef->paramTypes.at(i));
			}

			name += "1";

			return name;
		}
		case TypeId::Ref:
			return "ref" + mangleTypeName(type.getRefExData());
		case TypeId::Any:
			return "any";
		default:
			std::terminate();
	}
}

std::string BC2CXX::mangleClassName(const std::string &className, const GenericArgList &genericArgs) {
	return "_SLKAOT_" + className;
}

std::string BC2CXX::mangleFnName(const std::string_view &fnName) {
	if (!fnName.compare(0, sizeof("operator") - 1, "operator")) {
		const std::string_view operatorName = fnName.substr(sizeof("operator") - 1);
		return mangleOperatorName(operatorName);
	}

	std::string mangledName = "_slkaot_";

	for (size_t i = 0; i < fnName.size(); ++i) {
		char c[3];

		c[0] = (fnName[i] & 0xf) + 'A';
		c[1] = (fnName[i] >> 4) + 'A';
		c[2] = '\0';

		mangledName += c;
	}

	return mangledName + std::string(fnName);
}

std::string BC2CXX::mangleOperatorName(const std::string_view &operatorName) {
	std::string mangledName = "_slkaotop_";

	for (size_t i = 0; i < operatorName.size(); ++i) {
		char c[3];

		c[0] = (operatorName[i] & 0xf) + 'A';
		c[1] = (operatorName[i] >> 4) + 'A';
		c[2] = '\0';

		mangledName += c;
	}

	return mangledName;
}

std::string BC2CXX::mangleFieldName(const std::string &fieldName) {
	return "_slkaot_" + fieldName;
}

std::shared_ptr<cxxast::Namespace> BC2CXX::completeModuleNamespace(CompileContext &compileContext, const peff::DynArray<IdRefEntry> &entries) {
	std::shared_ptr<cxxast::Namespace> ns = compileContext.rootNamespace;

	for (size_t i = 0; i < entries.size(); ++i) {
		const IdRefEntry &idRefEntry = entries.at(i);

		std::string name((std::string_view)idRefEntry.name);

		if (auto m = ns->getMember(name);
			m) {
			if (m->nodeKind != cxxast::NodeKind::Namespace) {
				std::terminate();
			}

			ns = std::static_pointer_cast<cxxast::Namespace>(m);
		} else {
			std::shared_ptr<cxxast::Namespace> newNamespace = std::make_shared<cxxast::Namespace>(std::move(name));

			ns->addPublicMember(newNamespace);

			ns = newNamespace;
		}
	}

	return ns;
}

std::shared_ptr<cxxast::Expr> BC2CXX::compileRef(CompileContext &compileContext, const peff::DynArray<IdRefEntry> &entries) {
	std::shared_ptr<cxxast::Expr> e = std::make_shared<cxxast::IdExpr>((std::string)(std::string_view)entries.front().name);

	for (size_t i = 0; i < entries.size() - 1; ++i) {
		const IdRefEntry &idRefEntry = entries.at(i);

		std::string name((std::string_view)idRefEntry.name);

		e = std::make_shared<cxxast::BinaryExpr>(
			cxxast::BinaryOp::Scope,
			e,
			std::make_shared<cxxast::IdExpr>(std::move(name)));
	}

	return e;
}

std::shared_ptr<cxxast::Expr> BC2CXX::compileValue(CompileContext &compileContext, const Value &value) {
	std::shared_ptr<cxxast::Expr> e;

	switch (value.valueType) {
		case ValueType::I8:
			e = std::make_shared<cxxast::CastExpr>(
				std::make_shared<cxxast::CharTypeName>(cxxast::SignKind::Signed),
				std::make_shared<cxxast::IntLiteralExpr>(value.getI8()));
			break;
		case ValueType::I16:
			e = std::make_shared<cxxast::CastExpr>(
				std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Signed, cxxast::IntModifierKind::Short),
				std::make_shared<cxxast::IntLiteralExpr>(value.getI16()));
			break;
		case ValueType::I32:
			e = std::make_shared<cxxast::CastExpr>(
				std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Signed, cxxast::IntModifierKind::Unspecified),
				std::make_shared<cxxast::IntLiteralExpr>(value.getI32()));
			break;
		case ValueType::I64:
			e = std::make_shared<cxxast::CastExpr>(
				std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Signed, cxxast::IntModifierKind::LongLong),
				std::make_shared<cxxast::IntLiteralExpr>(value.getI64()));
			break;
		case ValueType::U8:
			e = std::make_shared<cxxast::CastExpr>(
				std::make_shared<cxxast::CharTypeName>(cxxast::SignKind::Unsigned),
				std::make_shared<cxxast::IntLiteralExpr>(value.getU8()));
			break;
		case ValueType::U16:
			e = std::make_shared<cxxast::CastExpr>(
				std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unsigned, cxxast::IntModifierKind::Short),
				std::make_shared<cxxast::IntLiteralExpr>(value.getU16()));
			break;
		case ValueType::U32:
			e = std::make_shared<cxxast::CastExpr>(
				std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unsigned, cxxast::IntModifierKind::Unspecified),
				std::make_shared<cxxast::IntLiteralExpr>(value.getU32()));
			break;
		case ValueType::U64:
			e = std::make_shared<cxxast::CastExpr>(
				std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unsigned, cxxast::IntModifierKind::LongLong),
				std::make_shared<cxxast::IntLiteralExpr>(value.getU64()));
			break;
		case ValueType::F32:
			e = std::make_shared<cxxast::FloatLiteralExpr>(value.getF32());
			break;
		case ValueType::F64:
			e = std::make_shared<cxxast::DoubleLiteralExpr>(value.getF64());
			break;
		case ValueType::Bool:
			e = std::make_shared<cxxast::BoolLiteralExpr>(value.getBool());
			break;
		case ValueType::ObjectRef: {
			const ObjectRef &objectRef = value.getObjectRef();
			if (objectRef.kind != ObjectRefKind::InstanceRef)
				std::terminate();
			Object *object = objectRef.asInstance.instanceObject;
			if (object) {
				switch (object->getKind()) {
					case ObjectKind::String: {
						std::string name((std::string_view)((StringObject *)object)->data);

						e = std::make_shared<cxxast::StringLiteralExpr>(std::move(name));
						break;
					}
					case ObjectKind::Array: {
						std::vector<std::shared_ptr<cxxast::Expr>> args;

						for (size_t i = 0; i < ((ArrayObject *)object)->length; ++i) {
							ObjectRef elementRef = ObjectRef::makeArrayElementRef(((ArrayObject *)object), i);
							args.push_back(
								compileValue(compileContext,
									compileContext.runtime->readVarUnsafe(elementRef)));
						}

						e = std::make_shared<cxxast::InitializerListExpr>(
							compileType(compileContext, ((ArrayObject *)object)->elementType),
							std::move(args));
						break;
					}
				}
			} else {
				e = std::make_shared<cxxast::NullptrLiteralExpr>();
			}
			break;
		}
		case ValueType::RegRef: {
			uint32_t index = value.getRegIndex();

			if (auto it = compileContext.dynamicContents.vregNames.find(index); it != compileContext.dynamicContents.vregNames.end()) {
				e = std::make_shared<cxxast::IdExpr>(std::string(it->second));
			} else {
				std::terminate();
			}
			break;
		}
		default:
			// Invalid type name, terminate.
			std::terminate();
	}

	return e;
}

std::shared_ptr<cxxast::TypeName> BC2CXX::compileType(CompileContext &compileContext, const Type &type) {
	std::shared_ptr<cxxast::TypeName> tn;

	switch (type.typeId) {
		case TypeId::None:
			tn = std::make_shared<cxxast::VoidTypeName>();
			break;
		case TypeId::Value: {
			switch (type.getValueTypeExData()) {
				case ValueType::I8:
					tn = std::make_shared<cxxast::CharTypeName>(cxxast::SignKind::Signed);
					break;
				case ValueType::I16:
					tn = std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Signed, cxxast::IntModifierKind::Short);
					break;
				case ValueType::I32:
					tn = std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Signed, cxxast::IntModifierKind::Unspecified);
					break;
				case ValueType::I64:
					tn = std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Signed, cxxast::IntModifierKind::LongLong);
					break;
				case ValueType::U8:
					tn = std::make_shared<cxxast::CharTypeName>(cxxast::SignKind::Unsigned);
					break;
				case ValueType::U16:
					tn = std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unsigned, cxxast::IntModifierKind::Short);
					break;
				case ValueType::U32:
					tn = std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unsigned, cxxast::IntModifierKind::Unspecified);
					break;
				case ValueType::U64:
					tn = std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unsigned, cxxast::IntModifierKind::LongLong);
					break;
				case ValueType::F32:
					tn = std::make_shared<cxxast::FloatTypeName>();
					break;
				case ValueType::F64:
					tn = std::make_shared<cxxast::DoubleTypeName>();
					break;
				case ValueType::Bool:
					tn = std::make_shared<cxxast::BoolTypeName>();
					break;
				default:
					// Invalid type name, terminate.
					std::terminate();
			}
			break;
		}
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
		case TypeId::FnDelegate: {
			tn = genInstanceObjectTypeName();
			break;
		}
		case TypeId::Ref: {
			switch (compileContext.dynamicContents.compilationTarget) {
				case CompilationTarget::VarDef:
					tn = genObjectRefTypeName();
					break;
				default:
					tn = std::make_shared<cxxast::RefTypeName>(
						genObjectRefTypeName());
					tn->isConst = true;
			}
			break;
		}
		case TypeId::Any: {
			tn = genAnyTypeName();
			break;
		}
		default:
			std::terminate();
	}
	return tn;
}

std::shared_ptr<cxxast::Fn> BC2CXX::compileFnOverloading(CompileContext &compileContext, FnOverloadingObject *fnOverloadingObject) {
	if ((fnOverloadingObject->genericParams.size()) && (!fnOverloadingObject->mappedGenericArgs.size())) {
		return {};
	} else {
		std::shared_ptr<cxxast::Fn> fnOverloading = std::make_shared<cxxast::Fn>(mangleFnName((std::string_view)fnOverloadingObject->fnObject->name));

		fnOverloading->name += "4";

		for (size_t i = 0; i < fnOverloadingObject->paramTypes.size(); ++i) {
			if (i)
				fnOverloading->name += "3";
			fnOverloading->name += mangleTypeName(fnOverloadingObject->paramTypes.at(i));
			fnOverloading->signature.paramTypes.push_back(compileType(compileContext, fnOverloadingObject->paramTypes.at(i)));
		}

		if (fnOverloadingObject->overloadingFlags & OL_VARG) {
			fnOverloading->name += "9";
			fnOverloading->signature.paramTypes.push_back(
				std::make_shared<cxxast::PointerTypeName>(
					genAnyTypeName()));	 // varArgs
			fnOverloading->signature.paramTypes.push_back(
				genSizeTypeName());	 // nVarArgs
		}

		fnOverloading->returnType = compileType(compileContext, fnOverloadingObject->returnType);

		fnOverloading->properties = {};

		if (fnOverloadingObject->overloadingFlags & OL_VIRTUAL) {
			fnOverloading->properties.isVirtual = true;
		}

		runtimeFnToAstFnMap[fnOverloadingObject] = fnOverloading;

		return fnOverloading;
	}
}

std::shared_ptr<cxxast::Class> BC2CXX::compileClass(CompileContext &compileContext, ClassObject *moduleObject) {
	if (moduleObject->genericParams.size() && (!moduleObject->mappedGenericArgs.size())) {
		return {};
	} else {
		compileContext.pushDynamicContents();
		compileContext.dynamicContents.compilationTarget = CompilationTarget::Class;

		std::shared_ptr<cxxast::Class> cls = std::make_shared<cxxast::Class>(mangleClassName((std::string)(std::string_view)moduleObject->name, moduleObject->genericArgs));

		for (size_t i = 0; i < moduleObject->fieldRecords.size(); ++i) {
			const FieldRecord &fr = moduleObject->fieldRecords.at(i);

			std::shared_ptr<cxxast::Var> varObject;

			{
				std::string name((std::string_view)fr.name);
				cxxast::StorageClass storageClass;

				if (fr.accessModifier & ACCESS_STATIC)
					storageClass = cxxast::StorageClass::Static;
				else
					storageClass = cxxast::StorageClass::Unspecified;

				varObject = std::make_shared<cxxast::Var>(
					mangleFieldName(name),
					storageClass,
					compileType(compileContext, fr.type),
					compileValue(compileContext,
						compileContext.runtime->readVarUnsafe(
							ObjectRef::makeFieldRef(moduleObject, i))));
			}

			if (fr.accessModifier & ACCESS_PUB) {
				cls->addPublicMember(varObject);
			} else {
				cls->addProtectedMember(varObject);
			}
		}

		for (auto i = moduleObject->scope->members.begin(); i != moduleObject->scope->members.end(); ++i) {
			switch (i.value()->getKind()) {
				case ObjectKind::Class: {
					ClassObject *m = (ClassObject *)i.value();
					std::shared_ptr<cxxast::Class> compiledCls = compileClass(compileContext, m);
					if (compiledCls) {
						if (m->accessModifier & ACCESS_PUB)
							cls->addPublicMember(compiledCls);
						else
							cls->addProtectedMember(compiledCls);
					}
					break;
				}
				case ObjectKind::Fn: {
					FnObject *m = (FnObject *)i.value();

					for (auto j : m->overloadings) {
						std::shared_ptr<cxxast::Fn> compiledFn = compileFnOverloading(compileContext, j);

						if (compiledFn) {
							if (compiledFn->properties.isPublic) {
								cls->addPublicMember(compiledFn);
							} else {
								cls->addProtectedMember(compiledFn);
							}
						}
					}

					break;
				}
			}
		}

		compileContext.popDynamicContents();

		return cls;
	}
}

void BC2CXX::compileModule(CompileContext &compileContext, ModuleObject *moduleObject) {
	compileContext.pushDynamicContents();
	compileContext.dynamicContents.compilationTarget = CompilationTarget::Module;

	peff::DynArray<IdRefEntry> fullModuleName;
	if (!compileContext.runtime->getFullRef(peff::getDefaultAlloc(), moduleObject, fullModuleName)) {
		throw std::bad_alloc();
	}

	std::shared_ptr<cxxast::Namespace> ns = completeModuleNamespace(compileContext, fullModuleName);
	for (size_t i = 0; i < moduleObject->fieldRecords.size(); ++i) {
		const FieldRecord &fr = moduleObject->fieldRecords.at(i);

		std::shared_ptr<cxxast::Var> varObject;

		{
			std::string name((std::string_view)fr.name);
			cxxast::StorageClass storageClass;

			if (fr.accessModifier & ACCESS_STATIC)
				storageClass = cxxast::StorageClass::Static;
			else
				storageClass = cxxast::StorageClass::Unspecified;

			varObject = std::make_shared<cxxast::Var>(
				mangleFieldName(name),
				storageClass,
				compileType(compileContext, fr.type),
				compileValue(compileContext,
					compileContext.runtime->readVarUnsafe(
						ObjectRef::makeFieldRef(moduleObject, i))));
		}

		if (fr.accessModifier & ACCESS_PUB) {
			ns->addPublicMember(varObject);
		} else {
			ns->addProtectedMember(varObject);
		}
	}

	for (auto i = moduleObject->scope->members.begin(); i != moduleObject->scope->members.end(); ++i) {
		switch (i.value()->getKind()) {
			case ObjectKind::Module: {
				compileModule(compileContext, (ModuleObject *)i.value());
				break;
			}
			case ObjectKind::Class: {
				ClassObject *m = (ClassObject *)i.value();
				std::shared_ptr<cxxast::Class> compiledCls = compileClass(compileContext, m);
				if (compiledCls) {
					if (m->accessModifier & ACCESS_PUB)
						ns->addPublicMember(compiledCls);
					else
						ns->addProtectedMember(compiledCls);
				}
				break;
			}
		}
	}

	compileContext.popDynamicContents();
}

std::pair<std::shared_ptr<cxxast::IfndefDirective>, std::shared_ptr<cxxast::Namespace>> BC2CXX::compile(ModuleObject *moduleObject) {
	std::shared_ptr<cxxast::Namespace> rootNamespace = std::make_shared<cxxast::Namespace>("slkaot");
	CompileContext cc(moduleObject->associatedRuntime, rootNamespace);

	std::string headerPath;
	std::string includeGuardName;

	peff::DynArray<IdRefEntry> fullModuleName;
	if (!cc.runtime->getFullRef(peff::getDefaultAlloc(), moduleObject, fullModuleName)) {
		throw std::bad_alloc();
	}

	for (size_t i = 0; i < fullModuleName.size(); ++i) {
		IdRefEntry &idRefEntry = fullModuleName.at(i);

		std::string name((std::string_view)idRefEntry.name);

		if (!i) {
			includeGuardName += "_SLKAOT_";
			headerPath += "slkaot/";
		} else {
			includeGuardName += "_";
			headerPath += "/";
		}

		{
			std::string uppercaseName(name);

			for (size_t i = 0; i < name.size(); ++i) {
				char &c = uppercaseName[i];
				if (isalnum(c)) {
					c = toupper(c);
				} else {
					c = '_';
				}
			}

			includeGuardName += uppercaseName;
		}
		headerPath += name;
	}

	headerPath += ".h";
	includeGuardName += "_";

	compileModule(cc, moduleObject);

	std::shared_ptr<cxxast::IfndefDirective> includeGuardWrapper =
		std::make_shared<cxxast::IfndefDirective>(
			std::string(includeGuardName),
			std::vector<std::shared_ptr<cxxast::ASTNode>>{
				std::make_shared<cxxast::Directive>(
					"define",
					std::vector<std::shared_ptr<cxxast::Expr>>{ std::make_shared<cxxast::IdExpr>(std::string(includeGuardName)) }),
				std::make_shared<cxxast::IncludeDirective>(
					"slake/runtime.h",
					true),
				rootNamespace },
			std::vector<std::shared_ptr<cxxast::ASTNode>>{});

	rootNamespace->defPrecedingNodes.push_back(
		std::make_shared<cxxast::IncludeDirective>(
			std::string(headerPath),
			true));

	return { includeGuardWrapper, rootNamespace };
}
