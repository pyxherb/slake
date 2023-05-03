#include <slkparse.hh>

#include "misc.hh"

using namespace Slake;
using namespace Compiler;

void State::compile(std::shared_ptr<Scope> scope, std::fstream &fs, bool isTopLevel) {
	this->scope = scope;  // Set up scope of the state.

	if (isTopLevel) {
		SlxFmt::ImgHeader ih = { 0 };
		std::memcpy(ih.magic, SlxFmt::IMH_MAGIC, sizeof(ih.magic));
		ih.nImports = (std::uint8_t)scope->imports.size();
		ih.fmtVer = 0;
		fs.write((char *)&ih, sizeof(ih));

		for (auto i : scope->imports) {
			_writeValue((std::uint32_t)(i.first.length()), fs);
			_writeValue(*(i.first.c_str()), (std::streamsize)i.first.length(), fs);
			writeValue(i.second, fs);
		}
	}

	//
	// Write value descriptors (VAD).
	//
	{
		for (auto &i : scope->vars) {
			SlxFmt::VarDesc vad = { 0 };
			vad.lenName = (std::uint8_t)i.first.length();

			if (i.second->accessModifier & ~(ACCESS_PUB | ACCESS_FINAL | ACCESS_STATIC | ACCESS_NATIVE))
				throw parser::syntax_error(i.second->getLocation(), "Invalid modifier combination");
			if (i.second->isPublic())
				vad.flags |= SlxFmt::VAD_PUB;
			if (i.second->isFinal())
				vad.flags |= SlxFmt::VAD_FINAL;
			if (i.second->isStatic())
				vad.flags |= SlxFmt::VAD_STATIC;
			if (i.second->isNative())
				vad.flags |= SlxFmt::VAD_NATIVE;

			if (i.second->initValue)
				vad.flags |= SlxFmt::VAD_INIT;

			fs.write((char *)&vad, sizeof(vad));
			fs.write(i.first.c_str(), i.first.length());

			compileTypeName(fs, i.second->typeName);

			if (i.second->initValue) {
				auto e = evalConstExpr(i.second->initValue);
				if (!e)
					throw parser::syntax_error(i.second->initValue->getLocation(), "Expression cannot be evaluated in compile-time");
				writeValue(e, fs);
			}
		}
		{
			SlxFmt::VarDesc vad = { 0 };
			fs.write((char *)&vad, sizeof(vad));
		}
	}

	// Compile and write functions.
	{
		for (auto &i : scope->fnDefs) {
			if (i.second->accessModifier & ~(ACCESS_PUB | ACCESS_FINAL | ACCESS_STATIC | ACCESS_OVERRIDE | ACCESS_NATIVE))
				throw parser::syntax_error(i.second->getLocation(), "Invalid modifier combination");
			if (isTopLevel)
				if (i.second->accessModifier & (ACCESS_FINAL | ACCESS_STATIC | ACCESS_OVERRIDE | ACCESS_NATIVE))
					throw parser::syntax_error(i.second->getLocation(), "Invalid modifier combination");

			currentFn = i.first;					   // Set up current function name of the state.
			fnDefs[i.first] = std::make_shared<Fn>();  // Create a new empty function.

			// Compile the function if is a definition.
			if (i.second->execBlock) {
				auto ctxt = context;
				for (auto &j : *(i.second->params))
					context.lvars[j->name] = LocalVar((std::uint32_t)(context.lvars.size()), j->typeName, true);

				for (auto &i : std::static_pointer_cast<CodeBlock>(i.second->execBlock)->ins)
					compileStmt(i);

				// Check if current branch has any return statement.
				if (!context.returned) {
					if (i.second->returnTypeName->kind != TypeNameKind::NONE)
						throw parser::syntax_error(location(i.second->execBlock->getLocation().end), "Must return a value");
					fnDefs[i.first]->insertIns(Ins(Opcode::RET, { std::make_shared<NullLiteralExpr>(i.second->getLocation()) }));
				}
				context = ctxt;
			}

			// Write the function descriptor (FND).
			{
				SlxFmt::FnDesc fnd = { 0 };
				if (i.second->isPublic())
					fnd.flags |= SlxFmt::FND_PUB;
				if (i.second->isFinal())
					fnd.flags |= SlxFmt::FND_FINAL;
				if (i.second->isStatic())
					fnd.flags |= SlxFmt::FND_STATIC;
				if (i.second->isOverride())
					fnd.flags |= SlxFmt::FND_OVERRIDE;
				if (i.second->isNative())
					fnd.flags |= SlxFmt::FND_NATIVE;
				fnd.lenName = i.first.length();
				fnd.lenBody = fnDefs[currentFn]->body.size();
				fnd.nParams = i.second->params->size();
				_writeValue(fnd, fs);
				_writeValue(*(i.first.c_str()), i.first.length(), fs);
			}

			for (auto j : i.second->genericParams) {
				_writeValue((std::uint32_t)j.size(), fs);
				_writeValue(*(j.c_str()), (std::streamsize)j.size(), fs);
			}

			compileTypeName(fs, i.second->returnTypeName);

			for (auto j : *(i.second->params))
				compileTypeName(fs, j->typeName);

			// Write for each instructions.
			for (auto &k : fnDefs[currentFn]->body) {
				SlxFmt::InsHeader ih(k.opcode, (std::uint8_t)k.operands.size());
				_writeValue(ih, fs);
				for (auto &l : k.operands) {
					if (l->getExprKind() == ExprKind::LABEL) {
						writeValue(
							std::make_shared<UIntLiteralExpr>(l->getLocation(),
								fnDefs[currentFn]->labels[std::static_pointer_cast<LabelExpr>(l)->label]),
							fs);
					} else
						writeValue(l, fs);
				}
			}
		}
		SlxFmt::FnDesc fnd = { 0 };
		_writeValue(fnd, fs);
	}

	// Write CTD for each class/trait.
	{
		for (auto i : scope->types) {
			switch (i.second->getKind()) {
				case Type::Kind::CLASS: {
					auto t = std::static_pointer_cast<ClassType>(i.second);
					SlxFmt::ClassTypeDesc ctd = { 0 };

					if (i.second->accessModifier & ~(ACCESS_PUB | ACCESS_FINAL))
						throw parser::syntax_error(i.second->getLocation(), "Invalid modifier combination");

					if (i.second->isPublic())
						ctd.flags |= SlxFmt::CTD_PUB;
					if (i.second->isFinal())
						ctd.flags |= SlxFmt::CTD_FINAL;

					ctd.lenName = (std::uint8_t)i.first.length();
					ctd.nImpls = (std::uint8_t)t->impls->impls.size();
					ctd.nGenericParams = (std::uint8_t)t->genericParams.size();
					_writeValue(ctd, fs);
					_writeValue(*(i.first.c_str()), (std::streamsize)i.first.size(), fs);

					// Write reference to the parent class which is derived by it.
					if (t->parent) {
						ctd.flags |= SlxFmt::CTD_DERIVED;
						if (t->parent->kind != TypeNameKind::CUSTOM)
							throw parser::syntax_error(i.second->getLocation(), "Invalid parent type");
						auto tn = std::static_pointer_cast<CustomTypeName>(t->parent);
						compileTypeName(fs, tn);
					}

					// Write references to implemented interfaces.
					for (auto &j : t->impls->impls) {
						if (j->kind != TypeNameKind::CUSTOM)
							throw parser::syntax_error(i.second->getLocation(), "Invalid parent type");
						auto tn = std::static_pointer_cast<CustomTypeName>(j);
						compileTypeName(fs, tn);
					}

					compile(std::static_pointer_cast<ClassType>(i.second)->scope, fs, false);
					break;
				}
				case Type::Kind::TRAIT: {
					auto t = std::static_pointer_cast<TraitType>(i.second);
					SlxFmt::ClassTypeDesc ctd = { 0 };

					if (t->accessModifier & ~ACCESS_PUB)
						throw parser::syntax_error(i.second->getLocation(), "Invalid modifier combination");

					ctd.flags |= SlxFmt::CTD_TRAIT | (t->isPublic() ? SlxFmt::CTD_PUB : 0);
					ctd.lenName = (std::uint8_t)i.first.length();
					ctd.nGenericParams = (std::uint8_t)t->genericParams.size();

					_writeValue(ctd, fs);
					_writeValue(*(i.first.c_str()), (std::streamsize)i.first.size(), fs);

					if (t->parent) {
						ctd.flags |= SlxFmt::CTD_DERIVED;
						if (t->parent->kind != TypeNameKind::CUSTOM)
							throw parser::syntax_error(i.second->getLocation(), "Invalid parent type");
						auto tn = std::static_pointer_cast<CustomTypeName>(t->parent);
						writeValue(tn->typeRef, fs);
					}

					compile(std::static_pointer_cast<TraitType>(i.second)->scope, fs, false);
					break;
				}
			}
		}
		SlxFmt::ClassTypeDesc ctd = { 0 };
		_writeValue(ctd, fs);
	}

	// Write STD for each structure.
	{
		for (auto i : scope->types) {
			if (i.second->getKind() != Type::Kind::STRUCT)
				continue;
			auto t = std::static_pointer_cast<StructType>(i.second);
			SlxFmt::StructTypeDesc std = { 0 };
			std.nMembers = t->vars.size();
			std.lenName = (std::uint8_t)i.first.size();
			fs.write((char *)&std, sizeof(std));
			_writeValue(*(i.first.c_str()), (std::streamsize)i.first.size(), fs);

			for (auto j : t->varIndices) {
				SlxFmt::StructMemberDesc smd = {};
				auto item = t->vars[j.second];
				if (item->typeName->kind == TypeNameKind::CUSTOM)
					throw parser::syntax_error(item->typeName->getLocation(), "Non-literal types are not acceptable");
				smd.type = _tnKind2vtMap.at(item->typeName->kind);
				smd.lenName = j.first.size();
				_writeValue(smd, fs);
				_writeValue(*(j.first.c_str()), (std::streamsize)j.first.size(), fs);
			}
		}
		SlxFmt::StructTypeDesc std = { 0 };
		fs.write((char *)&std, sizeof(std));
	}
}
