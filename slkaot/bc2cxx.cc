#include "bc2cxx.h"
#include "compiler.h"

using namespace slake;
using namespace slake::slkaot;
using namespace slake::slkaot::bc2cxx;

std::pair<size_t, size_t> BC2CXX::getLocalVarSizeAndAlignmentInfoOfType(const Type &type) {
	switch (type.typeId) {
		case TypeId::I8:
			return { sizeof(int8_t), sizeof(int8_t) };
		case TypeId::I16:
			return { sizeof(int16_t), sizeof(int16_t) };
		case TypeId::I32:
			return { sizeof(int32_t), sizeof(int32_t) };
		case TypeId::I64:
			return { sizeof(int64_t), sizeof(int64_t) };
		case TypeId::U8:
			return { sizeof(uint8_t), sizeof(uint8_t) };
		case TypeId::U16:
			return { sizeof(uint16_t), sizeof(uint16_t) };
		case TypeId::U32:
			return { sizeof(uint32_t), sizeof(uint32_t) };
		case TypeId::U64:
			return { sizeof(uint64_t), sizeof(uint64_t) };
		case TypeId::F32:
			return { sizeof(float), sizeof(float) };
		case TypeId::F64:
			return { sizeof(double), sizeof(double) };
		case TypeId::Bool:
			return { sizeof(bool), sizeof(bool) };
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
		case TypeId::FnDelegate:
			return { sizeof(Object *), sizeof(void *) };
		case TypeId::Ref:
			return { sizeof(EntityRef), sizeof(size_t) };
		case TypeId::Any:
			return { sizeof(Value), sizeof(size_t) };
		default:
			std::terminate();
	}
}

std::shared_ptr<cxxast::Expr> BC2CXX::genGetValueDataExpr(const Type &type, std::shared_ptr<cxxast::Expr> expr) {
	switch (type.typeId) {
		case TypeId::I8:
			return std::make_shared<cxxast::CallExpr>(
				std::make_shared<cxxast::BinaryExpr>(
					cxxast::BinaryOp::MemberAccess,
					expr,
					std::make_shared<cxxast::IdExpr>("getI8")),
				std::vector<std::shared_ptr<cxxast::Expr>>{});
		case TypeId::I16:
			return std::make_shared<cxxast::CallExpr>(
				std::make_shared<cxxast::BinaryExpr>(
					cxxast::BinaryOp::MemberAccess,
					expr,
					std::make_shared<cxxast::IdExpr>("getI16")),
				std::vector<std::shared_ptr<cxxast::Expr>>{});
		case TypeId::I32:
			return std::make_shared<cxxast::CallExpr>(
				std::make_shared<cxxast::BinaryExpr>(
					cxxast::BinaryOp::MemberAccess,
					expr,
					std::make_shared<cxxast::IdExpr>("getI32")),
				std::vector<std::shared_ptr<cxxast::Expr>>{});
		case TypeId::I64:
			return std::make_shared<cxxast::CallExpr>(
				std::make_shared<cxxast::BinaryExpr>(
					cxxast::BinaryOp::MemberAccess,
					expr,
					std::make_shared<cxxast::IdExpr>("getI64")),
				std::vector<std::shared_ptr<cxxast::Expr>>{});
		case TypeId::U8:
			return std::make_shared<cxxast::CallExpr>(
				std::make_shared<cxxast::BinaryExpr>(
					cxxast::BinaryOp::MemberAccess,
					expr,
					std::make_shared<cxxast::IdExpr>("getU8")),
				std::vector<std::shared_ptr<cxxast::Expr>>{});
		case TypeId::U16:
			return std::make_shared<cxxast::CallExpr>(
				std::make_shared<cxxast::BinaryExpr>(
					cxxast::BinaryOp::MemberAccess,
					expr,
					std::make_shared<cxxast::IdExpr>("getU16")),
				std::vector<std::shared_ptr<cxxast::Expr>>{});
		case TypeId::U32:
			return std::make_shared<cxxast::CallExpr>(
				std::make_shared<cxxast::BinaryExpr>(
					cxxast::BinaryOp::MemberAccess,
					expr,
					std::make_shared<cxxast::IdExpr>("getU32")),
				std::vector<std::shared_ptr<cxxast::Expr>>{});
		case TypeId::U64:
			return std::make_shared<cxxast::CallExpr>(
				std::make_shared<cxxast::BinaryExpr>(
					cxxast::BinaryOp::MemberAccess,
					expr,
					std::make_shared<cxxast::IdExpr>("getU64")),
				std::vector<std::shared_ptr<cxxast::Expr>>{});
		case TypeId::Bool:
			return std::make_shared<cxxast::CallExpr>(
				std::make_shared<cxxast::BinaryExpr>(
					cxxast::BinaryOp::MemberAccess,
					expr,
					std::make_shared<cxxast::IdExpr>("getBool")),
				std::vector<std::shared_ptr<cxxast::Expr>>{});
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
		case TypeId::FnDelegate:
			return std::make_shared<cxxast::BinaryExpr>(
				cxxast::BinaryOp::MemberAccess,
				std::make_shared<cxxast::BinaryExpr>(
					cxxast::BinaryOp::MemberAccess,
					std::make_shared<cxxast::CallExpr>(
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::MemberAccess,
							expr,
							std::make_shared<cxxast::IdExpr>("getEntityRef")),
						std::vector<std::shared_ptr<cxxast::Expr>>{}),
					std::make_shared<cxxast::IdExpr>("asObject")),
				std::make_shared<cxxast::IdExpr>("instanceObject"));
		case TypeId::Ref:
			return std::make_shared<cxxast::CallExpr>(
				std::make_shared<cxxast::BinaryExpr>(
					cxxast::BinaryOp::MemberAccess,
					expr,
					std::make_shared<cxxast::IdExpr>("getEntityRef")),
				std::vector<std::shared_ptr<cxxast::Expr>>{});
		case TypeId::Any:
			return expr;
		default:
			std::terminate();
	}
}

std::shared_ptr<cxxast::Expr> BC2CXX::genDefaultValue(const Type &type) {
	std::shared_ptr<cxxast::Expr> e;

	switch (type.typeId) {
		case TypeId::I8:
			e = std::make_shared<cxxast::CastExpr>(
				std::make_shared<cxxast::CharTypeName>(cxxast::SignKind::Signed),
				std::make_shared<cxxast::IntLiteralExpr>(0));
			break;
		case TypeId::I16:
			e = std::make_shared<cxxast::CastExpr>(
				std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Signed, cxxast::IntModifierKind::Short),
				std::make_shared<cxxast::IntLiteralExpr>(0));
			break;
		case TypeId::I32:
			e = std::make_shared<cxxast::CastExpr>(
				std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Signed, cxxast::IntModifierKind::Unspecified),
				std::make_shared<cxxast::IntLiteralExpr>(0));
			break;
		case TypeId::I64:
			e = std::make_shared<cxxast::CastExpr>(
				std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Signed, cxxast::IntModifierKind::LongLong),
				std::make_shared<cxxast::IntLiteralExpr>(0));
			break;
		case TypeId::U8:
			e = std::make_shared<cxxast::CastExpr>(
				std::make_shared<cxxast::CharTypeName>(cxxast::SignKind::Unsigned),
				std::make_shared<cxxast::IntLiteralExpr>(0));
			break;
		case TypeId::U16:
			e = std::make_shared<cxxast::CastExpr>(
				std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unsigned, cxxast::IntModifierKind::Short),
				std::make_shared<cxxast::IntLiteralExpr>(0));
			break;
		case TypeId::U32:
			e = std::make_shared<cxxast::CastExpr>(
				std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unsigned, cxxast::IntModifierKind::Unspecified),
				std::make_shared<cxxast::IntLiteralExpr>(0));
			break;
		case TypeId::U64:
			e = std::make_shared<cxxast::CastExpr>(
				std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unsigned, cxxast::IntModifierKind::LongLong),
				std::make_shared<cxxast::IntLiteralExpr>(0));
			break;
		case TypeId::F32:
			e = std::make_shared<cxxast::FloatLiteralExpr>(0);
			break;
		case TypeId::F64:
			e = std::make_shared<cxxast::DoubleLiteralExpr>(0);
			break;
		case TypeId::Bool:
			e = std::make_shared<cxxast::BoolLiteralExpr>(0);
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
		case TypeId::FnDelegate:
			e = std::make_shared<cxxast::NullptrLiteralExpr>();
			break;
		case TypeId::Any:
			e = std::make_shared<cxxast::CallExpr>(
				genValueRefExpr(),
				std::vector<std::shared_ptr<cxxast::Expr>>{
					std::make_shared<cxxast::BinaryExpr>(
						cxxast::BinaryOp::Scope,
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::Scope,
							std::make_shared<cxxast::IdExpr>("slake"),
							std::make_shared<cxxast::IdExpr>("ValueType")),
						std::make_shared<cxxast::IdExpr>("Undefined")) });
			break;
		default:
			// Invalid type name, terminate.
			std::terminate();
	}

	return e;
}

bool BC2CXX::_isSimpleIdExpr(std::shared_ptr<cxxast::Expr> expr) {
	switch (expr->exprKind) {
		case cxxast::ExprKind::IntLiteral:
		case cxxast::ExprKind::UIntLiteral:
		case cxxast::ExprKind::LongLiteral:
		case cxxast::ExprKind::ULongLiteral:
		case cxxast::ExprKind::CharLiteral:
		case cxxast::ExprKind::StringLiteral:
		case cxxast::ExprKind::FloatLiteral:
		case cxxast::ExprKind::DoubleLiteral:
		case cxxast::ExprKind::BoolLiteral:
		case cxxast::ExprKind::NullptrLiteral:
		case cxxast::ExprKind::InitializerList:
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
		case ValueType::EntityRef: {
			const EntityRef &entityRef = value.getEntityRef();
			if (entityRef.kind != ObjectRefKind::ObjectRef)
				std::terminate();
			Object *object = entityRef.asObject.instanceObject;
			if (object) {
				compileContext.mappedObjects.insert(object);
				e = std::make_shared<cxxast::BinaryExpr>(cxxast::BinaryOp::PtrAccess,
					std::make_shared<cxxast::CastExpr>(
						std::make_shared<cxxast::PointerTypeName>(
							std::make_shared<cxxast::CustomTypeName>(
								false,
								std::make_shared<cxxast::BinaryExpr>(
									cxxast::BinaryOp::Scope,
									_getAbsRef(compileContext.rootNamespace),
									std::make_shared<cxxast::IdExpr>("MappedObjects")))),
						genMappedObjectsRef()),
					std::make_shared<cxxast::IdExpr>(mangleConstantObjectName(object)));
			} else {
				e = std::make_shared<cxxast::NullptrLiteralExpr>();
			}
			break;
		}
		case ValueType::RegRef: {
			uint32_t index = value.getRegIndex();

			if (auto it = compileContext.vregInfo.find(index); it != compileContext.vregInfo.end()) {
				if (compileContext.isGenerator) {
					e = std::make_shared<cxxast::BinaryExpr>(
						cxxast::BinaryOp::PtrAccess,
						std::make_shared<cxxast::IdExpr>(mangleParamName(0)),
						std::make_shared<cxxast::IdExpr>(std::string(it->second.vregVarName)));
				} else {
					e = std::make_shared<cxxast::IdExpr>(std::string(it->second.vregVarName));
				}
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

std::shared_ptr<cxxast::Expr> BC2CXX::compileValueAsAny(CompileContext &compileContext, const Value &value) {
	std::shared_ptr<cxxast::Expr> e;

	switch (value.valueType) {
		case ValueType::I8:
			e = std::make_shared<cxxast::CallExpr>(
				genValueRefExpr(),
				std::vector<std::shared_ptr<cxxast::Expr>>{
					std::make_shared<cxxast::CastExpr>(
						std::make_shared<cxxast::CharTypeName>(cxxast::SignKind::Signed),
						std::make_shared<cxxast::IntLiteralExpr>(value.getI8())) });
			break;
		case ValueType::I16:
			e = std::make_shared<cxxast::CallExpr>(
				genValueRefExpr(),
				std::vector<std::shared_ptr<cxxast::Expr>>{ std::make_shared<cxxast::CastExpr>(
					std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Signed, cxxast::IntModifierKind::Short),
					std::make_shared<cxxast::IntLiteralExpr>(value.getI16())) });
			break;
		case ValueType::I32:
			e = std::make_shared<cxxast::CallExpr>(
				genValueRefExpr(),
				std::vector<std::shared_ptr<cxxast::Expr>>{ std::make_shared<cxxast::CastExpr>(
					std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Signed, cxxast::IntModifierKind::Unspecified),
					std::make_shared<cxxast::IntLiteralExpr>(value.getI32())) });
			break;
		case ValueType::I64:
			e = std::make_shared<cxxast::CallExpr>(
				genValueRefExpr(),
				std::vector<std::shared_ptr<cxxast::Expr>>{ std::make_shared<cxxast::CastExpr>(
					std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Signed, cxxast::IntModifierKind::LongLong),
					std::make_shared<cxxast::IntLiteralExpr>(value.getI64())) });
			break;
		case ValueType::U8:
			e = std::make_shared<cxxast::CallExpr>(
				genValueRefExpr(),
				std::vector<std::shared_ptr<cxxast::Expr>>{ std::make_shared<cxxast::CastExpr>(
					std::make_shared<cxxast::CharTypeName>(cxxast::SignKind::Unsigned),
					std::make_shared<cxxast::IntLiteralExpr>(value.getU8())) });
			break;
		case ValueType::U16:
			e = std::make_shared<cxxast::CallExpr>(
				genValueRefExpr(),
				std::vector<std::shared_ptr<cxxast::Expr>>{ std::make_shared<cxxast::CastExpr>(
					std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unsigned, cxxast::IntModifierKind::Short),
					std::make_shared<cxxast::IntLiteralExpr>(value.getU16())) });
			break;
		case ValueType::U32:
			e = std::make_shared<cxxast::CallExpr>(
				genValueRefExpr(),
				std::vector<std::shared_ptr<cxxast::Expr>>{ std::make_shared<cxxast::CastExpr>(
					std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unsigned, cxxast::IntModifierKind::Unspecified),
					std::make_shared<cxxast::IntLiteralExpr>(value.getU32())) });
			break;
		case ValueType::U64:
			e = std::make_shared<cxxast::CallExpr>(
				genValueRefExpr(),
				std::vector<std::shared_ptr<cxxast::Expr>>{ std::make_shared<cxxast::CastExpr>(
					std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unsigned, cxxast::IntModifierKind::LongLong),
					std::make_shared<cxxast::IntLiteralExpr>(value.getU64())) });
			break;
		case ValueType::F32:
			e = std::make_shared<cxxast::CallExpr>(
				genValueRefExpr(),
				std::vector<std::shared_ptr<cxxast::Expr>>{
					std::make_shared<cxxast::FloatLiteralExpr>(value.getF32()) });
			break;
		case ValueType::F64:
			e = std::make_shared<cxxast::CallExpr>(
				genValueRefExpr(),
				std::vector<std::shared_ptr<cxxast::Expr>>{
					std::make_shared<cxxast::DoubleLiteralExpr>(value.getF64()) });
			break;
		case ValueType::Bool:
			e = std::make_shared<cxxast::CallExpr>(
				genValueRefExpr(),
				std::vector<std::shared_ptr<cxxast::Expr>>{
					std::make_shared<cxxast::BoolLiteralExpr>(value.getBool()) });
			break;
		case ValueType::EntityRef: {
			const EntityRef &entityRef = value.getEntityRef();
			if (entityRef.kind != ObjectRefKind::ObjectRef)
				std::terminate();
			Object *object = entityRef.asObject.instanceObject;
			if (object) {
				compileContext.mappedObjects.insert(object);
				e = std::make_shared<cxxast::BinaryExpr>(cxxast::BinaryOp::PtrAccess,
					std::make_shared<cxxast::CastExpr>(
						std::make_shared<cxxast::PointerTypeName>(
							std::make_shared<cxxast::CustomTypeName>(
								false,
								std::make_shared<cxxast::BinaryExpr>(
									cxxast::BinaryOp::Scope,
									_getAbsRef(compileContext.rootNamespace),
									std::make_shared<cxxast::IdExpr>("MappedObjects")))),
						genMappedObjectsRef()),
					std::make_shared<cxxast::IdExpr>(mangleConstantObjectName(object)));
			} else {
				e = std::make_shared<cxxast::NullptrLiteralExpr>();
			}

			e = std::make_shared<cxxast::CallExpr>(
				genValueRefExpr(),
				std::vector<std::shared_ptr<cxxast::Expr>>{
					std::make_shared<cxxast::CallExpr>(
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::Scope,
							std::make_shared<cxxast::BinaryExpr>(
								cxxast::BinaryOp::Scope,
								std::make_shared<cxxast::IdExpr>("slake"),
								std::make_shared<cxxast::IdExpr>("EntityRef")),
							std::make_shared<cxxast::IdExpr>("makeObjectRef")),
						std::vector<std::shared_ptr<cxxast::Expr>>{
							e }) });
			break;
		}
		case ValueType::RegRef: {
			uint32_t index = value.getRegIndex();

			if (auto it = compileContext.vregInfo.find(index); it != compileContext.vregInfo.end()) {
				if (compileContext.isGenerator) {
					e = std::make_shared<cxxast::BinaryExpr>(
						cxxast::BinaryOp::PtrAccess,
						std::make_shared<cxxast::IdExpr>(mangleParamName(0)),
						std::make_shared<cxxast::IdExpr>(std::string(it->second.vregVarName)));
				} else {
					e = std::make_shared<cxxast::IdExpr>(std::string(it->second.vregVarName));
				}
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
		case TypeId::Void:
			tn = std::make_shared<cxxast::VoidTypeName>();
			break;
		case TypeId::I8:
			tn = std::make_shared<cxxast::CharTypeName>(cxxast::SignKind::Signed);
			break;
		case TypeId::I16:
			tn = std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Signed, cxxast::IntModifierKind::Short);
			break;
		case TypeId::I32:
			tn = std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Signed, cxxast::IntModifierKind::Unspecified);
			break;
		case TypeId::I64:
			tn = std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Signed, cxxast::IntModifierKind::LongLong);
			break;
		case TypeId::U8:
			tn = std::make_shared<cxxast::CharTypeName>(cxxast::SignKind::Unsigned);
			break;
		case TypeId::U16:
			tn = std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unsigned, cxxast::IntModifierKind::Short);
			break;
		case TypeId::U32:
			tn = std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unsigned, cxxast::IntModifierKind::Unspecified);
			break;
		case TypeId::U64:
			tn = std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unsigned, cxxast::IntModifierKind::LongLong);
			break;
		case TypeId::F32:
			tn = std::make_shared<cxxast::FloatTypeName>();
			break;
		case TypeId::F64:
			tn = std::make_shared<cxxast::DoubleTypeName>();
			break;
		case TypeId::Bool:
			tn = std::make_shared<cxxast::BoolTypeName>();
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
		case TypeId::FnDelegate: {
			tn = genInstanceObjectTypeName();
			break;
		}
		case TypeId::Ref: {
			tn = genObjectRefTypeName();
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

std::shared_ptr<cxxast::TypeName> BC2CXX::compileParamType(CompileContext &compileContext, const Type &type) {
	std::shared_ptr<cxxast::TypeName> tn;

	switch (type.typeId) {
		case TypeId::Void:
			return compileType(compileContext, type);
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64:
		case TypeId::F32:
		case TypeId::F64:
		case TypeId::Bool:
			return compileType(compileContext, type);
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
		case TypeId::FnDelegate:
			return std::make_shared<cxxast::RefTypeName>(compileType(compileContext, type));
		case TypeId::Ref:
			return compileType(compileContext, type);
		case TypeId::Any:
			return compileType(compileContext, type);
		default:;
	}

	std::terminate();
}

std::shared_ptr<cxxast::Fn> BC2CXX::compileFnOverloading(CompileContext &compileContext, FnOverloadingObject *fnOverloadingObject) {
	if (auto p = getMappedAstNode(fnOverloadingObject); p)
		return std::static_pointer_cast<cxxast::Fn>(p);
	if ((fnOverloadingObject->genericParams.size()) && (!fnOverloadingObject->mappedGenericArgs.size())) {
		return {};
	} else {
		std::shared_ptr<cxxast::Fn> fnOverloading = std::make_shared<cxxast::Fn>(mangleFnName((std::string_view)fnOverloadingObject->fnObject->name));

		if (fnOverloadingObject->overloadingFlags & OL_VARG) {
			fnOverloading->name += "9";
			fnOverloading->signature.paramTypes.push_back(
				std::make_shared<cxxast::PointerTypeName>(
					genAnyTypeName()));	 // varArgs
			fnOverloading->signature.paramTypes.push_back(
				genSizeTypeName());	 // nVarArgs
		}

		fnOverloading->returnType = genInternalExceptionPtrTypeName();

		fnOverloading->properties = {};

		fnOverloading->rtOverloading = fnOverloadingObject;

		if (fnOverloadingObject->overloadingFlags & OL_VIRTUAL) {
			fnOverloading->properties.isVirtual = true;
		}

		registerRuntimeEntityToAstNodeRegistry(fnOverloadingObject, fnOverloading);
		compileContext.mappedObjects.insert(fnOverloadingObject);

		switch (fnOverloadingObject->overloadingKind) {
			case FnOverloadingKind::Regular: {
				// DO NOT set parameter types, we deferred for generator functions.
				recompilableFns.insert(fnOverloading);
				break;
			}
			default:
				for (size_t i = 0; i < fnOverloadingObject->paramTypes.size(); ++i) {
					fnOverloading->name = mangleTypeName(fnOverloadingObject->paramTypes.at(i)) + fnOverloading->name;
					fnOverloading->signature.paramTypes.push_back(compileParamType(compileContext, fnOverloadingObject->paramTypes.at(i)));
				}
		}

		return fnOverloading;
	}
}

std::shared_ptr<cxxast::Class> BC2CXX::compileClass(CompileContext &compileContext, ClassObject *moduleObject) {
	if (auto p = getMappedAstNode(moduleObject); p)
		return std::static_pointer_cast<cxxast::Class>(p);
	if (moduleObject->genericParams.size() && (!moduleObject->mappedGenericArgs.size())) {
		return {};
	} else {
		peff::DynArray<IdRefEntry> fullRef(peff::getDefaultAlloc());
		if (!compileContext.runtime->getFullRef(peff::getDefaultAlloc(), moduleObject, fullRef))
			throw std::bad_alloc();
		std::string className = mangleClassName(mangleRef(fullRef));
		std::shared_ptr<cxxast::Class> cls = std::make_shared<cxxast::Class>(std::move(className));

		registerRuntimeEntityToAstNodeRegistry(moduleObject, cls);
		compileContext.mappedObjects.insert(moduleObject);

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
							EntityRef::makeFieldRef(moduleObject, i))));
			}

			registerRuntimeEntityToAstNodeRegistry(EntityRef::makeFieldRef(moduleObject, i), varObject);

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
							compileContext.rootNamespace->addPublicMember(compiledCls);
						else
							compileContext.rootNamespace->addProtectedMember(compiledCls);
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

		return cls;
	}
}

void BC2CXX::compileModule(CompileContext &compileContext, ModuleObject *moduleObject) {
	if (auto p = getMappedAstNode(moduleObject); p)
		return;

	peff::DynArray<IdRefEntry> fullModuleName(peff::getDefaultAlloc());
	if (!compileContext.runtime->getFullRef(peff::getDefaultAlloc(), moduleObject, fullModuleName)) {
		throw std::bad_alloc();
	}

	std::shared_ptr<cxxast::Namespace> ns = completeModuleNamespace(compileContext, fullModuleName);

	registerRuntimeEntityToAstNodeRegistry(moduleObject, ns);
	compileContext.mappedObjects.insert(moduleObject);

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
						EntityRef::makeFieldRef(moduleObject, i))));
		}

		registerRuntimeEntityToAstNodeRegistry(EntityRef::makeFieldRef(moduleObject, i), varObject);

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
						compileContext.rootNamespace->addPublicMember(compiledCls);
					else
						compileContext.rootNamespace->addProtectedMember(compiledCls);
				}
				break;
			}
			case ObjectKind::Fn: {
				FnObject *m = (FnObject *)i.value();

				for (auto j : m->overloadings) {
					std::shared_ptr<cxxast::Fn> compiledFn = compileFnOverloading(compileContext, j);

					if (compiledFn) {
						if (compiledFn->properties.isPublic) {
							ns->addPublicMember(compiledFn);
						} else {
							ns->addProtectedMember(compiledFn);
						}
					}
				}

				break;
			}
		}
	}
}

std::shared_ptr<cxxast::Namespace> BC2CXX::compile(ModuleObject *moduleObject) {
	std::shared_ptr<cxxast::Namespace> rootNamespace = std::make_shared<cxxast::Namespace>("slkaot");
	CompileContext cc(moduleObject->associatedRuntime, rootNamespace);

	std::string headerPath;
	std::string includeGuardName;

	peff::DynArray<IdRefEntry> fullModuleName(peff::getDefaultAlloc());
	if (!cc.runtime->getFullRef(peff::getDefaultAlloc(), moduleObject, fullModuleName)) {
		throw std::bad_alloc();
	}

	bool useGeneratedHeaderPath = includeName.empty();
	bool isUserIncludeSystem = false;

	for (size_t i = 0; i < fullModuleName.size(); ++i) {
		IdRefEntry &idRefEntry = fullModuleName.at(i);

		std::string name((std::string_view)idRefEntry.name);

		if (!i) {
			includeGuardName += "_SLKAOT_";
			if (useGeneratedHeaderPath) {
				headerPath += "slkaot/";
			}
		} else {
			includeGuardName += "_";
			if (useGeneratedHeaderPath) {
				headerPath += "/";
			}
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
		if (useGeneratedHeaderPath) {
			headerPath += name;
		}
	}

	if (useGeneratedHeaderPath) {
		headerPath += ".h";
		isUserIncludeSystem = true;
	} else {
		if (includeName[0] == '+') {
			isUserIncludeSystem = true;
			headerPath = includeName.substr(1);
		} else {
			headerPath = includeName;
		}
	}
	includeGuardName += "_";

	compileModule(cc, moduleObject);

	for (auto i : recompilableFns) {
		recompileFnOverloading(cc, i);
	}

	std::shared_ptr<cxxast::Struct> mappedObjectsStruct = std::make_shared<cxxast::Struct>("MappedObjects");

	rootNamespace->addPublicMember(
		mappedObjectsStruct);

	for (auto i : cc.mappedObjects) {
		mappedObjectsStruct->addPublicMember(std::make_shared<cxxast::Var>(
			mangleConstantObjectName(i.get()),
			cxxast::StorageClass::Unspecified,
			genInstanceObjectTypeName(),
			std::shared_ptr<cxxast::Expr>{}));
	}

	rootNamespace->declPrecedingNodes.push_back(
		std::make_shared<cxxast::Directive>(
			"ifndef",
			std::vector<std::shared_ptr<cxxast::Expr>>{
				std::make_shared<cxxast::IdExpr>(std::string(includeGuardName)) }));
	rootNamespace->declPrecedingNodes.push_back(
		std::make_shared<cxxast::Directive>(
			"define",
			std::vector<std::shared_ptr<cxxast::Expr>>{ std::make_shared<cxxast::IdExpr>(std::string(includeGuardName)) }));
	rootNamespace->declPrecedingNodes.push_back(
		std::make_shared<cxxast::IncludeDirective>(
			"slake/runtime.h",
			true));
	rootNamespace->declPrecedingNodes.push_back(
		std::make_shared<cxxast::IncludeDirective>(
			"slake/aot/context.h",
			true));
	rootNamespace->declTrailingNodes.push_back(
		std::make_shared<cxxast::Directive>(
			"endif",
			std::vector<std::shared_ptr<cxxast::Expr>>{}));

	rootNamespace->defPrecedingNodes.push_back(
		std::make_shared<cxxast::IncludeDirective>(
			std::string(headerPath),
			isUserIncludeSystem));

	return rootNamespace;
}
