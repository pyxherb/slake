#include "compiler.h"

using namespace slake::slkc;

#define INSMAP_ENTRY(name, exprs)      \
	{                                  \
		Opcode::name, { #name, exprs } \
	}

InsMap Compiler::defaultInsMap = {
	INSMAP_ENTRY(NOP, {}),
	INSMAP_ENTRY(PUSH, { TypeId::ANY })
};

template <typename T>
static void _write(std::ostream &fs, const T &value) {
	fs.write((const char *)&value, sizeof(T));
}

template <typename T>
static void _write(std::ostream &fs, T &&value) {
	const T v = value;
	fs.write((const char *)&v, sizeof(T));
}

template <typename T>
static void _write(std::ostream &fs, const T *ptr, size_t size) {
	fs.write((const char *)ptr, size);
}

class SlakeErrorListener : public antlr4::BaseErrorListener {
	virtual void syntaxError(
		antlr4::Recognizer *recognizer,
		antlr4::Token *offendingSymbol,
		size_t line,
		size_t charPositionInLine,
		const std::string &msg,
		std::exception_ptr e) override {
		throw FatalCompilationError(
			Message(
				Location(line, charPositionInLine),
				MSG_ERROR,
				msg));
	}
};

void Compiler::compile(std::istream &is, std::ostream &os) {
	antlr4::ANTLRInputStream s(is);

	SlakeLexer lexer(&s);
	antlr4::CommonTokenStream tokens(&lexer);
	SlakeErrorListener errorListener;

	SlakeParser parser(&tokens);
	parser.removeErrorListeners();
	parser.addErrorListener(&errorListener);
	auto tree = parser.prog();

	slake::slkc::AstVisitor visitor(this);
	visitor.visit(tree);

	{
		slxfmt::ImgHeader ih = {};

		memcpy(ih.magic, slxfmt::IMH_MAGIC, sizeof(ih.magic));
		ih.fmtVer = 0;
		ih.nImports = (uint16_t)_targetModule->imports.size();

		if (_targetModule->moduleName.size())
			ih.flags |= slxfmt::IMH_MODNAME;

		os.write((char *)&ih, sizeof(ih));
	}

	if (_targetModule->moduleName.size())
		compileRef(os, _targetModule->moduleName);

	for (auto i : _targetModule->imports) {
		_write(os, (uint32_t)i.first.size());
		_write(os, i.first.data(), i.first.length());
		compileRef(os, i.second);
	}

	compileScope(is, os, _targetModule->scope);
}

void Compiler::compileScope(std::istream &is, std::ostream &os, shared_ptr<Scope> scope) {
	unordered_map<string, shared_ptr<VarNode>> vars;
	unordered_map<string, shared_ptr<FnNode>> funcs;
	unordered_map<string, shared_ptr<CompiledFnNode>> compiledFuncs;
	unordered_map<string, shared_ptr<ClassNode>> classes;
	unordered_map<string, shared_ptr<InterfaceNode>> interfaces;
	unordered_map<string, shared_ptr<TraitNode>> traits;

	for (auto i : scope->members) {
		switch (i.second->getNodeType()) {
			case AST_VAR:
				vars[i.first] = static_pointer_cast<VarNode>(i.second);
				break;
			case AST_FN:
				funcs[i.first] = static_pointer_cast<FnNode>(i.second);
				break;
			case AST_CLASS:
				classes[i.first] = static_pointer_cast<ClassNode>(i.second);
				break;
			case AST_INTERFACE:
				interfaces[i.first] = static_pointer_cast<InterfaceNode>(i.second);
				break;
			case AST_TRAIT:
				traits[i.first] = static_pointer_cast<TraitNode>(i.second);
				break;
			default:
				assert(false);
		}
	}

	_write<uint32_t>(os, vars.size());
	for (auto &i : vars) {
		slxfmt::VarDesc vad = {};

		if (i.second->access & ACCESS_PUB)
			vad.flags |= slxfmt::VAD_PUB;
		if (i.second->access & ACCESS_STATIC)
			vad.flags |= slxfmt::VAD_STATIC;
		if (i.second->access & ACCESS_FINAL)
			vad.flags |= slxfmt::VAD_FINAL;
		if (i.second->access & ACCESS_NATIVE)
			vad.flags |= slxfmt::VAD_NATIVE;
		if (i.second->initValue)
			vad.flags |= slxfmt::VAD_INIT;

		vad.lenName = (uint8_t)i.first.length();
		_write(os, vad);
		_write(os, i.first.data(), i.first.length());

		compileTypeName(os, i.second->type);

		if (i.second->initValue) {
			if (auto ce = evalConstExpr(i.second->initValue); ce)
				compileExpr(i.second->initValue);
			else
				throw FatalCompilationError(
					Message(
						i.second->initValue->getLocation(),
						MSG_ERROR,
						"Expecting a compiling-time expression"));
		}
	}

	for (auto &i : funcs) {
		for (auto &j : i.second->overloadingRegistries) {
			string mangledFnName = i.first;

			if (i.first != "new") {
				bool hasVarArg = j.paramIndices.count("...");

				for (size_t k = 0; k < j.params.size() - hasVarArg; ++k) {
					mangledFnName += "@" + to_string(j.params[k].type);
				}

				if (compiledFuncs.count(mangledFnName)) {
					throw FatalCompilationError(
						Message(
							i.second->getLocation(),
							MSG_ERROR,
							"Duplicated function overloading"));
				}
			}

			auto compiledFn = make_shared<CompiledFnNode>(j.loc, mangledFnName);
			compiledFn->returnType = j.returnType;
			compiledFn->params = j.params;
			compiledFn->paramIndices = j.paramIndices;
			compiledFn->genericParams = j.genericParams;
			compiledFn->genericParamIndices = j.genericParamIndices;
			compiledFn->access = j.access;

			context = Context();
			context.curFn = compiledFn;
			context.curScope = scope;

			if (j.body)
				compileStmt(j.body);

			compiledFuncs[mangledFnName] = compiledFn;
		}
	}

	_write(os, (uint32_t)compiledFuncs.size());
	for (auto &i : compiledFuncs) {
		slxfmt::FnDesc fnd = {};
		bool hasVarArg = false;

		if (i.second->access & ACCESS_PUB)
			fnd.flags |= slxfmt::FND_PUB;
		if (i.second->access & ACCESS_STATIC)
			fnd.flags |= slxfmt::FND_STATIC;
		if (i.second->access & ACCESS_NATIVE)
			fnd.flags |= slxfmt::FND_NATIVE;
		if (i.second->access & ACCESS_OVERRIDE)
			fnd.flags |= slxfmt::FND_OVERRIDE;
		if (i.second->access & ACCESS_FINAL)
			fnd.flags |= slxfmt::FND_FINAL;

		if (i.second->paramIndices.count("..."))
			hasVarArg = true;

		if (hasVarArg)
			fnd.flags |= slxfmt::FND_VARG;

		fnd.lenName = (uint16_t)i.first.length();
		fnd.lenBody = (uint32_t)i.second->body.size();
		fnd.nParams = (uint8_t)i.second->params.size() - hasVarArg;

		_write(os, fnd);
		_write(os, i.first.data(), i.first.length());

		compileTypeName(os, i.second->returnType);

		for (size_t j = 0; j < i.second->params.size() - hasVarArg; ++j)
			compileTypeName(os, i.second->params[j].type);

		for (auto &j : i.second->body) {
			slxfmt::InsHeader ih;
			ih.opcode = j.opcode;

			if (j.operands.size() > 3)
				throw FatalCompilationError(
					Message(
						i.second->getLocation(),
						MSG_ERROR,
						"Too many operands"));
			ih.nOperands = (uint8_t)j.operands.size();

			_write(os, ih);

			for (auto &k : j.operands) {
				if (k) {
					if (k->getNodeType() == AST_LABEL_REF) {
						auto &label = static_pointer_cast<LabelRefNode>(k)->label;
						if (!i.second->labels.count(label))
							throw FatalCompilationError(
								Message(
									i.second->getLocation(),
									MSG_ERROR,
									"Undefined label: " + static_pointer_cast<LabelRefNode>(k)->label));
						k = make_shared<U32LiteralExprNode>(i.second->getLocation(), i.second->labels.at(label));
					}
				}
				compileValue(os, k);
			}
		}
	}

	_write(os, (uint32_t)classes.size());
	for (auto &i : classes) {
		slxfmt::ClassTypeDesc ctd = {};

		if (i.second->access & ACCESS_PUB)
			ctd.flags |= slxfmt::CTD_PUB;
		if (i.second->access & ACCESS_FINAL)
			ctd.flags |= slxfmt::CTD_FINAL;
		if (i.second->parentClass)
			ctd.flags |= slxfmt::CTD_DERIVED;
		ctd.nImpls = (uint8_t)i.second->implInterfaces.size();
		ctd.lenName = (uint8_t)i.first.length();
		// Unimplemented yet, leave this field blank.
		// ctd.nGenericParams = (uint8_t)i.second->genericParams.size();
		ctd.nGenericParams = 0;

		_write(os, ctd);
		_write(os, i.first.data(), i.first.length());

		if (i.second->parentClass)
			compileRef(os, i.second->parentClass->ref);

		for (auto &j : i.second->implInterfaces)
			compileRef(os, j->ref);

		// Unimplemented yet, do not try to compile generic parameters.
		// for (auto &j : i.second->genericParams)
		//	compileGenericParam(os, j);

		compileScope(is, os, i.second->scope);
	}

	_write(os, (uint32_t)interfaces.size());
	for (auto &i : interfaces) {
		slxfmt::InterfaceTypeDesc ctd = {};

		if (i.second->access & ACCESS_PUB)
			ctd.flags |= slxfmt::ITD_PUB;

		ctd.nParents = (uint8_t)i.second->parentInterfaces.size();

		ctd.lenName = (uint8_t)i.first.length();

		_write(os, ctd);
		_write(os, i.first.data(), i.first.length());

		for (auto j : i.second->parentInterfaces) {
			compileRef(os, j->ref);
		}

		compileScope(is, os, i.second->scope);
	}

	_write(os, (uint32_t)traits.size());
	for (auto &i : traits) {
		slxfmt::TraitTypeDesc ctd = {};

		if (i.second->access & ACCESS_PUB)
			ctd.flags |= slxfmt::TTD_PUB;

		ctd.nParents = (uint8_t)i.second->parentTraits.size();

		ctd.lenName = (uint8_t)i.first.length();

		_write(os, ctd);
		_write(os, i.first.data(), i.first.length());

		for (auto j : i.second->parentTraits) {
			compileRef(os, j);
		}

		compileScope(is, os, i.second->scope);
	}
}

void Compiler::compileTypeName(std::ostream &fs, shared_ptr<TypeNameNode> typeName) {
	switch (typeName->getTypeId()) {
		case TYPE_I8: {
			_write(fs, slxfmt::Type::I8);
			break;
		}
		case TYPE_I16: {
			_write(fs, slxfmt::Type::I16);
			break;
		}
		case TYPE_I32: {
			_write(fs, slxfmt::Type::I32);
			break;
		}
		case TYPE_I64: {
			_write(fs, slxfmt::Type::I64);
			break;
		}
		case TYPE_U8: {
			_write(fs, slxfmt::Type::U8);
			break;
		}
		case TYPE_U16: {
			_write(fs, slxfmt::Type::U16);
			break;
		}
		case TYPE_U32: {
			_write(fs, slxfmt::Type::U32);
			break;
		}
		case TYPE_U64: {
			_write(fs, slxfmt::Type::U64);
			break;
		}
		case TYPE_F32: {
			_write(fs, slxfmt::Type::F32);
			break;
		}
		case TYPE_F64: {
			_write(fs, slxfmt::Type::F64);
			break;
		}
		case TYPE_BOOL: {
			_write(fs, slxfmt::Type::BOOL);
			break;
		}
		case TYPE_STRING: {
			_write(fs, slxfmt::Type::STRING);
			break;
		}
		case TYPE_VOID: {
			_write(fs, slxfmt::Type::NONE);
			break;
		}
		case TYPE_ANY: {
			_write(fs, slxfmt::Type::ANY);
			break;
		}
		case TYPE_ARRAY: {
			_write(fs, slxfmt::Type::ARRAY);
			compileTypeName(fs, static_pointer_cast<ArrayTypeNameNode>(typeName)->elementType);
			break;
		}
		case TYPE_MAP: {
			_write(fs, slxfmt::Type::MAP);
			compileTypeName(fs, static_pointer_cast<MapTypeNameNode>(typeName)->keyType);
			compileTypeName(fs, static_pointer_cast<MapTypeNameNode>(typeName)->valueType);
			break;
		}
		case TYPE_FN: {
			// stub
			break;
		}
		case TYPE_CUSTOM: {
			_write(fs, slxfmt::Type::OBJECT);

			auto dest = resolveCustomType(static_pointer_cast<CustomTypeNameNode>(typeName));

			compileRef(fs, getFullName(static_pointer_cast<MemberNode>(dest)));
			break;
		}
		default:
			assert(false);
	}
}

void Compiler::compileRef(std::ostream &fs, const Ref &ref) {
	for (size_t i = 0; i < ref.size(); ++i) {
		slxfmt::RefEntryDesc rsd = {};

		auto &entry = ref[i];

		if (i + 1 < ref.size())
			rsd.flags |= slxfmt::RSD_NEXT;

		rsd.lenName = entry.name.size();
		rsd.nGenericArgs = entry.genericArgs.size();
		_write(fs, rsd);
		_write(fs, entry.name.data(), entry.name.length());

		for (auto &j : entry.genericArgs)
			compileTypeName(fs, j);
	}
}

void Compiler::compileValue(std::ostream &fs, shared_ptr<AstNode> value) {
	slxfmt::ValueDesc vd = {};

	if (!value) {
		vd.type = slxfmt::Type::NONE;
		_write(fs, vd);
		return;
	}

	switch (value->getNodeType()) {
		case AST_TYPENAME:
			vd.type = slxfmt::Type::TYPENAME;
			_write(fs, vd);

			compileTypeName(fs, static_pointer_cast<TypeNameNode>(value));
			break;
		case AST_REG_REF: {
			auto v = static_pointer_cast<RegRefNode>(value);
			vd.type = slxfmt::Type::REG;
			_write(fs, vd);

			_write(fs, v->reg);
			break;
		}
		case AST_ARG_REF: {
			auto v = static_pointer_cast<ArgRefNode>(value);
			vd.type = v->unwrapData ? slxfmt::Type::ARG_VALUE : slxfmt::Type::ARG;
			_write(fs, vd);

			_write(fs, v->index);
			break;
		}
		case AST_LVAR_REF: {
			auto v = static_pointer_cast<LocalVarRefNode>(value);
			vd.type = v->unwrapData ? slxfmt::Type::LVAR_VALUE : slxfmt::Type::LVAR;
			_write(fs, vd);

			_write(fs, v->index);
			break;
		}
		case AST_EXPR: {
			shared_ptr<ExprNode> expr = static_pointer_cast<ExprNode>(value);
			switch (expr->getExprType()) {
				case EXPR_I8: {
					vd.type = slxfmt::Type::I8;
					_write(fs, vd);

					_write(fs, static_pointer_cast<I8LiteralExprNode>(expr)->data);
					break;
				}
				case EXPR_I16: {
					vd.type = slxfmt::Type::I16;
					_write(fs, vd);

					_write(fs, static_pointer_cast<I16LiteralExprNode>(expr)->data);
					break;
				}
				case EXPR_I32: {
					vd.type = slxfmt::Type::I32;
					_write(fs, vd);

					_write(fs, static_pointer_cast<I32LiteralExprNode>(expr)->data);
					break;
				}
				case EXPR_I64: {
					vd.type = slxfmt::Type::I64;
					_write(fs, vd);

					_write(fs, static_pointer_cast<I64LiteralExprNode>(expr)->data);
					break;
				}
				case EXPR_U8: {
					vd.type = slxfmt::Type::U8;
					_write(fs, vd);

					_write(fs, static_pointer_cast<U8LiteralExprNode>(expr)->data);
					break;
				}
				case EXPR_U16: {
					vd.type = slxfmt::Type::U16;
					_write(fs, vd);

					_write(fs, static_pointer_cast<U16LiteralExprNode>(expr)->data);
					break;
				}
				case EXPR_U32: {
					vd.type = slxfmt::Type::U32;
					_write(fs, vd);

					_write(fs, static_pointer_cast<U32LiteralExprNode>(expr)->data);
					break;
				}
				case EXPR_U64: {
					vd.type = slxfmt::Type::U64;
					_write(fs, vd);

					_write(fs, static_pointer_cast<U64LiteralExprNode>(expr)->data);
					break;
				}
				case EXPR_F32: {
					vd.type = slxfmt::Type::F32;
					_write(fs, vd);

					_write(fs, static_pointer_cast<F32LiteralExprNode>(expr)->data);
					break;
				}
				case EXPR_F64: {
					vd.type = slxfmt::Type::F64;
					_write(fs, vd);

					_write(fs, static_pointer_cast<F64LiteralExprNode>(expr)->data);
					break;
				}
				case EXPR_BOOL: {
					vd.type = slxfmt::Type::BOOL;
					_write(fs, vd);

					_write(fs, static_pointer_cast<BoolLiteralExprNode>(expr)->data);
					break;
				}
				case EXPR_STRING: {
					vd.type = slxfmt::Type::STRING;
					_write(fs, vd);

					auto &s = static_pointer_cast<StringLiteralExprNode>(expr)->data;

					_write(fs, (uint32_t)s.length());
					_write(fs, s.data(), s.size());
					break;
				}
				case EXPR_REF: {
					vd.type = slxfmt::Type::REF;
					_write(fs, vd);

					compileRef(fs, static_pointer_cast<RefExprNode>(expr)->ref);
					break;
				}
				default:
					assert(false);
			}
			break;
		}
		default:
			assert(false);
	}
}
