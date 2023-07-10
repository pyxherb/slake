#include "compile.h"
#include <slake/slxfmt.h>
#include <bcparse.hh>
#include <cstring>

using namespace slake;
using namespace slake::bcc;

template <typename T>
static void _write(std::ostream &fs, const T &value) {
	fs.write((const char *)&value, sizeof(T));
}

template <typename T>
static void _write(std::ostream &fs, const T &&value) {
	const T v = value;
	fs.write((const char *)&v, sizeof(T));
}

template <typename T>
static void _write(std::ostream &fs, const T *ptr, size_t size) {
	fs.write((const char *)ptr, size);
}

void bcc::compile(std::ostream &fs) {
	{
		slxfmt::ImgHeader ih = {};

		memcpy(ih.magic, slxfmt::IMH_MAGIC, sizeof(ih.magic));
		ih.fmtVer = 0;
		ih.nImports = 0;

		fs.write((char *)&ih, sizeof(ih));
	}

	compileScope(fs, rootScope);
}

void bcc::compileScope(std::ostream &fs, shared_ptr<Scope> scope) {
	//
	// Compile variables.
	//
	_write(fs, (uint32_t)scope->vars.size());
	for (auto &i : scope->vars) {
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

		vad.lenName = i.first.length();
		_write(fs, vad);
		_write(fs, i.first.data(), i.first.length());

		compileTypeName(fs, i.second->type);

		if (i.second->initValue)
			compileOperand(fs, i.second->initValue);
	}

	//
	// Compile functions.
	//
	_write(fs, (uint32_t)scope->funcs.size());
	for (auto &i : scope->funcs) {
		slxfmt::FnDesc fnd = {};

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

		if (i.second->params.isVariadic)
			fnd.flags |= slxfmt::FND_VARG;

		fnd.lenName = (uint16_t)i.first.length();
		fnd.lenBody = (uint32_t)i.second->body.size();
		fnd.nParams = (uint8_t)i.second->params.size();

		_write(fs, fnd);
		_write(fs, i.first.data(), i.first.length());

		compileTypeName(fs, i.second->returnType);

		for (auto &j : i.second->params)
			compileTypeName(fs, j);

		for (auto &j : i.second->body) {
			slxfmt::InsHeader ih;
			ih.opcode = j->opcode;

			if (j->operands.size() > 3)
				throw parser::syntax_error(j->getLocation(), "Too many operands");
			ih.nOperands = (uint8_t)j->operands.size();

			_write(fs, ih);

			for (auto &k : j->operands) {
				if (k->getOperandType() == OperandType::LABEL) {
					auto &label = static_pointer_cast<LabelOperand>(k)->data;
					if (!i.second->labels.count(label))
						throw parser::syntax_error(k->getLocation(), "Label not defined: `" + label + "'");
					k = make_shared<I32Operand>(k->getLocation(), i.second->labels.at(label));
				}
				compileOperand(fs, k);
			}
		}
	}

	//
	// Compile classes.
	//
	_write(fs, (uint32_t)scope->classes.size());
	for (auto &i : scope->classes) {
		slxfmt::ClassTypeDesc ctd = {};

		if (i.second->access & ACCESS_PUB)
			ctd.flags |= slxfmt::CTD_PUB;
		if (i.second->access & ACCESS_FINAL)
			ctd.flags |= slxfmt::CTD_FINAL;
		if (i.second->parent)
			ctd.flags |= slxfmt::CTD_DERIVED;
		ctd.nImpls = i.second->impls.size();
		ctd.lenName = i.first.length();

		_write(fs, ctd);
		_write(fs, i.first.data(), i.first.length());

		if (i.second->parent)
			compileRef(fs, i.second->parent);
		for (auto &j : i.second->impls)
			compileRef(fs, j);

		compileScope(fs, i.second->scope);
	}

	//
	// Compile interfaces.
	//
	_write(fs, (uint32_t)scope->interfaces.size());
	for (auto &i : scope->interfaces) {
		slxfmt::InterfaceTypeDesc ctd = {};

		if (i.second->access & ACCESS_PUB)
			ctd.flags |= slxfmt::ITD_PUB;

		ctd.nParents = (uint8_t)i.second->parents.size();

		ctd.lenName = i.first.length();

		_write(fs, ctd);
		_write(fs, i.first.data(), i.first.length());

		for(auto j:i.second->parents) {
			compileRef(fs, j);
		}

		compileScope(fs, i.second->scope);
	}

	//
	// Compile traits
	//
	_write(fs, (uint32_t)scope->traits.size());
	for (auto &i : scope->traits) {
		slxfmt::TraitTypeDesc ctd = {};

		if (i.second->access & ACCESS_PUB)
			ctd.flags |= slxfmt::TTD_PUB;

		ctd.nParents = (uint8_t)i.second->parents.size();

		ctd.lenName = i.first.length();

		_write(fs, ctd);
		_write(fs, i.first.data(), i.first.length());

		for(auto j:i.second->parents) {
			compileRef(fs, j);
		}

		compileScope(fs, i.second->scope);
	}
	// Compile Structures
	_write(fs, (uint32_t)0);
}

void bcc::compileOperand(std::ostream &fs, shared_ptr<Operand> operand) {
	slxfmt::ValueDesc vd = {};
	switch (operand->getOperandType()) {
		case OperandType::NONE: {
			vd.type = slxfmt::ValueType::NONE;
			_write(fs, vd);
			break;
		}
		case OperandType::I8: {
			vd.type = slxfmt::ValueType::I8;
			_write(fs, vd);

			_write(fs, static_pointer_cast<I8Operand>(operand)->data);
			break;
		}
		case OperandType::I16: {
			vd.type = slxfmt::ValueType::I16;
			_write(fs, vd);

			_write(fs, static_pointer_cast<I16Operand>(operand)->data);
			break;
		}
		case OperandType::I32: {
			vd.type = slxfmt::ValueType::I32;
			_write(fs, vd);

			_write(fs, static_pointer_cast<I32Operand>(operand)->data);
			break;
		}
		case OperandType::I64: {
			vd.type = slxfmt::ValueType::I64;
			_write(fs, vd);

			_write(fs, static_pointer_cast<I64Operand>(operand)->data);
			break;
		}
		case OperandType::U8: {
			vd.type = slxfmt::ValueType::U8;
			_write(fs, vd);

			_write(fs, static_pointer_cast<U8Operand>(operand)->data);
			break;
		}
		case OperandType::U16: {
			vd.type = slxfmt::ValueType::U16;
			_write(fs, vd);

			_write(fs, static_pointer_cast<U16Operand>(operand)->data);
			break;
		}
		case OperandType::U32: {
			vd.type = slxfmt::ValueType::U32;
			_write(fs, vd);

			_write(fs, static_pointer_cast<U32Operand>(operand)->data);
			break;
		}
		case OperandType::U64: {
			vd.type = slxfmt::ValueType::U64;
			_write(fs, vd);

			_write(fs, static_pointer_cast<U64Operand>(operand)->data);
			break;
		}
		case OperandType::F32: {
			vd.type = slxfmt::ValueType::F32;
			_write(fs, vd);

			_write(fs, static_pointer_cast<F32Operand>(operand)->data);
			break;
		}
		case OperandType::F64: {
			vd.type = slxfmt::ValueType::F64;
			_write(fs, vd);

			_write(fs, static_pointer_cast<F64Operand>(operand)->data);
			break;
		}
		case OperandType::BOOL: {
			vd.type = slxfmt::ValueType::BOOL;
			_write(fs, vd);

			_write(fs, static_pointer_cast<BoolOperand>(operand)->data);
			break;
		}
		case OperandType::STRING: {
			vd.type = slxfmt::ValueType::STRING;
			_write(fs, vd);

			auto &s = static_pointer_cast<StringOperand>(operand)->data;

			_write(fs, (uint32_t)s.length());
			_write(fs, s.data(), s.size());
			break;
		}
		case OperandType::REF: {
			vd.type = slxfmt::ValueType::REF;
			_write(fs, vd);

			compileRef(fs, static_pointer_cast<RefOperand>(operand)->data);
			break;
		}
		default:
			assert(false);
	}
}

void bcc::compileRef(std::ostream &fs, shared_ptr<Ref> ref) {
	for (size_t i = 0; i < ref->scopes.size(); ++i) {
		slxfmt::RefScopeDesc rsd = {};

		auto &scope = ref->scopes[i];

		if (i + 1 < ref->scopes.size())
			rsd.flags |= slxfmt::RSD_NEXT;

		rsd.lenName = scope->name.size();
		// rsd.nGenericArgs = scope->genericArgs.size();
		_write(fs, rsd);
		_write(fs, scope->name.data(), scope->name.length());
	}
}

void bcc::compileTypeName(std::ostream &fs, shared_ptr<TypeName> typeName) {
	switch (typeName->type) {
		case TYPE_I8: {
			_write(fs, slxfmt::ValueType::I8);
			break;
		}
		case TYPE_I16: {
			_write(fs, slxfmt::ValueType::I16);
			break;
		}
		case TYPE_I32: {
			_write(fs, slxfmt::ValueType::I32);
			break;
		}
		case TYPE_I64: {
			_write(fs, slxfmt::ValueType::I64);
			break;
		}
		case TYPE_U8: {
			_write(fs, slxfmt::ValueType::U8);
			break;
		}
		case TYPE_U16: {
			_write(fs, slxfmt::ValueType::U16);
			break;
		}
		case TYPE_U32: {
			_write(fs, slxfmt::ValueType::U32);
			break;
		}
		case TYPE_U64: {
			_write(fs, slxfmt::ValueType::U64);
			break;
		}
		case TYPE_F32: {
			_write(fs, slxfmt::ValueType::F32);
			break;
		}
		case TYPE_F64: {
			_write(fs, slxfmt::ValueType::F64);
			break;
		}
		case TYPE_BOOL: {
			_write(fs, slxfmt::ValueType::BOOL);
			break;
		}
		case TYPE_STRING: {
			_write(fs, slxfmt::ValueType::STRING);
			break;
		}
		case TYPE_VOID: {
			_write(fs, slxfmt::ValueType::NONE);
			break;
		}
		case TYPE_ANY: {
			_write(fs, slxfmt::ValueType::ANY);
			break;
		}
		case TYPE_ARRAY: {
			_write(fs, slxfmt::ValueType::ARRAY);
			compileTypeName(fs, static_pointer_cast<ArrayTypeName>(typeName)->elementType);
			break;
		}
		case TYPE_MAP: {
			_write(fs, slxfmt::ValueType::MAP);
			compileTypeName(fs, static_pointer_cast<MapTypeName>(typeName)->keyType);
			compileTypeName(fs, static_pointer_cast<MapTypeName>(typeName)->valueType);
			break;
		}
		case TYPE_FN: {
			// stub
			break;
		}
		case TYPE_CUSTOM: {
			_write(fs, slxfmt::ValueType::OBJECT);
			compileRef(fs, static_pointer_cast<CustomTypeName>(typeName)->ref);
			break;
		}
		default:
			assert(false);
	}
}
