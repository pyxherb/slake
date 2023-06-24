#include "compile.h"
#include <slake/slxfmt.h>
#include <bcparse.hh>

using namespace Slake;
using namespace Slake::Assembler;

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

void Assembler::compile(std::ostream &fs) {
	{
		SlxFmt::ImgHeader ih = {};

		memcpy(ih.magic, SlxFmt::IMH_MAGIC, sizeof(ih.magic));
		ih.fmtVer = 0;
		ih.nImports = 0;

		fs.write((char *)&ih, sizeof(ih));
	}

	compileScope(fs, rootScope);
}

void Assembler::compileScope(std::ostream &fs, shared_ptr<Scope> scope) {
	_write(fs, (uint32_t)scope->vars.size());
	for (auto &i : scope->vars) {
		SlxFmt::VarDesc vad = {};

		if (i.second->access & ACCESS_PUB)
			vad.flags |= SlxFmt::VAD_PUB;
		if (i.second->access & ACCESS_STATIC)
			vad.flags |= SlxFmt::VAD_STATIC;
		if (i.second->access & ACCESS_FINAL)
			vad.flags |= SlxFmt::VAD_FINAL;
		if (i.second->access & ACCESS_NATIVE)
			vad.flags |= SlxFmt::VAD_NATIVE;
		if (i.second->initValue)
			vad.flags |= SlxFmt::VAD_INIT;

		vad.lenName = i.first.length();
		_write(fs, vad);
		_write(fs, i.first.data(), i.first.length());

		compileTypeName(fs, i.second->type);

		if (i.second->initValue)
			compileOperand(fs, i.second->initValue);
	}

	_write(fs, (uint32_t)scope->funcs.size());
	for (auto &i : scope->funcs) {
		SlxFmt::FnDesc fnd = {};

		if (i.second->access & ACCESS_PUB)
			fnd.flags |= SlxFmt::FND_PUB;
		if (i.second->access & ACCESS_STATIC)
			fnd.flags |= SlxFmt::FND_STATIC;
		if (i.second->access & ACCESS_FINAL)
			fnd.flags |= SlxFmt::FND_FINAL;
		if (i.second->access & ACCESS_NATIVE)
			fnd.flags |= SlxFmt::FND_NATIVE;
		if (i.second->access & ACCESS_OVERRIDE)
			fnd.flags |= SlxFmt::FND_OVERRIDE;

		if (i.second->params.isVariadic)
			fnd.flags |= SlxFmt::FND_VARG;

		fnd.lenName = (uint16_t)i.first.length();
		fnd.lenBody = (uint32_t)i.second->body.size();
		fnd.nParams = (uint8_t)i.second->params.size();

		_write(fs, fnd);
		_write(fs, i.first.data(), i.first.length());

		compileTypeName(fs, i.second->returnType);

		for (auto &j : i.second->params)
			compileTypeName(fs, j);

		for (auto &j : i.second->body) {
			SlxFmt::InsHeader ih;
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

	_write(fs, (uint32_t)(scope->classes.size() + scope->interfaces.size()));
	for (auto &i : scope->classes) {
		SlxFmt::ClassTypeDesc ctd = {};

		if (i.second->access & ACCESS_PUB)
			ctd.flags |= SlxFmt::CTD_PUB;
		if (i.second->access & ACCESS_FINAL)
			ctd.flags |= SlxFmt::CTD_FINAL;
		if (i.second->parent)
			ctd.flags |= SlxFmt::CTD_DERIVED;
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

	for (auto &i : scope->interfaces) {
		SlxFmt::ClassTypeDesc ctd = {};

		if (i.second->access & ACCESS_PUB)
			ctd.flags |= SlxFmt::CTD_PUB;
		if (i.second->access & ACCESS_FINAL)
			ctd.flags |= SlxFmt::CTD_FINAL;
		if (i.second->parent)
			ctd.flags |= SlxFmt::CTD_DERIVED;
		ctd.flags |= SlxFmt::CTD_INTERFACE;

		ctd.lenName = i.first.length();

		_write(fs, ctd);
		_write(fs, i.first.data(), i.first.length());

		if (i.second->parent)
			compileRef(fs, i.second->parent);

		compileScope(fs, i.second->scope);
	}

	_write(fs, (uint32_t)0);
}

void Assembler::compileOperand(std::ostream &fs, shared_ptr<Operand> operand) {
	SlxFmt::ValueDesc vd = {};
	switch (operand->getOperandType()) {
		case OperandType::NONE: {
			vd.type = SlxFmt::ValueType::NONE;
			_write(fs, vd);
			break;
		}
		case OperandType::I8: {
			vd.type = SlxFmt::ValueType::I8;
			_write(fs, vd);

			_write(fs, static_pointer_cast<I8Operand>(operand)->data);
			break;
		}
		case OperandType::I16: {
			vd.type = SlxFmt::ValueType::I16;
			_write(fs, vd);

			_write(fs, static_pointer_cast<I16Operand>(operand)->data);
			break;
		}
		case OperandType::I32: {
			vd.type = SlxFmt::ValueType::I32;
			_write(fs, vd);

			_write(fs, static_pointer_cast<I32Operand>(operand)->data);
			break;
		}
		case OperandType::I64: {
			vd.type = SlxFmt::ValueType::I64;
			_write(fs, vd);

			_write(fs, static_pointer_cast<I64Operand>(operand)->data);
			break;
		}
		case OperandType::U8: {
			vd.type = SlxFmt::ValueType::U8;
			_write(fs, vd);

			_write(fs, static_pointer_cast<U8Operand>(operand)->data);
			break;
		}
		case OperandType::U16: {
			vd.type = SlxFmt::ValueType::U16;
			_write(fs, vd);

			_write(fs, static_pointer_cast<U16Operand>(operand)->data);
			break;
		}
		case OperandType::U32: {
			vd.type = SlxFmt::ValueType::U32;
			_write(fs, vd);

			_write(fs, static_pointer_cast<U32Operand>(operand)->data);
			break;
		}
		case OperandType::U64: {
			vd.type = SlxFmt::ValueType::U64;
			_write(fs, vd);

			_write(fs, static_pointer_cast<U64Operand>(operand)->data);
			break;
		}
		case OperandType::F32: {
			vd.type = SlxFmt::ValueType::F32;
			_write(fs, vd);

			_write(fs, static_pointer_cast<F32Operand>(operand)->data);
			break;
		}
		case OperandType::F64: {
			vd.type = SlxFmt::ValueType::F64;
			_write(fs, vd);

			_write(fs, static_pointer_cast<F64Operand>(operand)->data);
			break;
		}
		case OperandType::BOOL: {
			vd.type = SlxFmt::ValueType::BOOL;
			_write(fs, vd);

			_write(fs, static_pointer_cast<BoolOperand>(operand)->data);
			break;
		}
		case OperandType::STRING: {
			vd.type = SlxFmt::ValueType::STRING;
			_write(fs, vd);

			auto &s = static_pointer_cast<StringOperand>(operand)->data;

			_write(fs, (uint32_t)s.length());
			_write(fs, s.data(), s.size());
			break;
		}
		case OperandType::REF: {
			vd.type = SlxFmt::ValueType::REF;
			_write(fs, vd);

			compileRef(fs, static_pointer_cast<RefOperand>(operand)->data);
			break;
		}
		default:
			assert(false);
	}
}

void Assembler::compileRef(std::ostream &fs, shared_ptr<Ref> ref) {
	for (size_t i = 0; i < ref->scopes.size(); ++i) {
		SlxFmt::RefScopeDesc rsd = {};

		auto &scope = ref->scopes[i];

		if (i + 1 < ref->scopes.size())
			rsd.flags |= SlxFmt::RSD_NEXT;

		rsd.lenName = scope->name.size();
		// rsd.nGenericArgs = scope->genericArgs.size();
		_write(fs, rsd);
		_write(fs, scope->name.data(), scope->name.length());
	}
}

void Assembler::compileTypeName(std::ostream &fs, shared_ptr<TypeName> typeName) {
	switch (typeName->type) {
		case TYPE_I8: {
			_write(fs, SlxFmt::ValueType::I8);
			break;
		}
		case TYPE_I16: {
			_write(fs, SlxFmt::ValueType::I16);
			break;
		}
		case TYPE_I32: {
			_write(fs, SlxFmt::ValueType::I32);
			break;
		}
		case TYPE_I64: {
			_write(fs, SlxFmt::ValueType::I64);
			break;
		}
		case TYPE_U8: {
			_write(fs, SlxFmt::ValueType::U8);
			break;
		}
		case TYPE_U16: {
			_write(fs, SlxFmt::ValueType::U16);
			break;
		}
		case TYPE_U32: {
			_write(fs, SlxFmt::ValueType::U32);
			break;
		}
		case TYPE_U64: {
			_write(fs, SlxFmt::ValueType::U64);
			break;
		}
		case TYPE_F32: {
			_write(fs, SlxFmt::ValueType::F32);
			break;
		}
		case TYPE_F64: {
			_write(fs, SlxFmt::ValueType::F64);
			break;
		}
		case TYPE_BOOL: {
			_write(fs, SlxFmt::ValueType::BOOL);
			break;
		}
		case TYPE_STRING: {
			_write(fs, SlxFmt::ValueType::STRING);
			break;
		}
		case TYPE_VOID: {
			_write(fs, SlxFmt::ValueType::NONE);
			break;
		}
		case TYPE_ANY: {
			_write(fs, SlxFmt::ValueType::ANY);
			break;
		}
		case TYPE_ARRAY: {
			_write(fs, SlxFmt::ValueType::ARRAY);
			compileTypeName(fs, static_pointer_cast<ArrayTypeName>(typeName)->elementType);
			break;
		}
		case TYPE_MAP: {
			_write(fs, SlxFmt::ValueType::MAP);
			compileTypeName(fs, static_pointer_cast<MapTypeName>(typeName)->keyType);
			compileTypeName(fs, static_pointer_cast<MapTypeName>(typeName)->valueType);
			break;
		}
		case TYPE_FN: {
			// stub
			break;
		}
		case TYPE_CUSTOM: {
			_write(fs, SlxFmt::ValueType::OBJECT);
			compileRef(fs, static_pointer_cast<CustomTypeName>(typeName)->ref);
			break;
		}
		default:
			assert(false);
	}
}
