#include "../bc2cxx.h"

using namespace slake;
using namespace slake::slkaot;
using namespace slake::slkaot::bc2cxx;

void BC2CXX::_dumpAstNode(std::ostream &os, std::shared_ptr<cxxast::ASTNode> astNode, ASTDumpMode dumpMode, size_t indentLevel) {
	switch (dumpMode) {
	case ASTDumpMode::Header: {
		for (size_t i = 0; i < astNode->declPrecedingNodes.size(); ++i) {
			_dumpAstNode(os, astNode->declPrecedingNodes[i], dumpMode, indentLevel);
		}
		break;
	}
	case ASTDumpMode::Source: {
		for (size_t i = 0; i < astNode->defPrecedingNodes.size(); ++i) {
			_dumpAstNode(os, astNode->defPrecedingNodes[i], dumpMode, indentLevel);
		}
		break;
	}
	default:
		std::terminate();
	}

	switch (astNode->nodeKind) {
	case cxxast::NodeKind::Directive: {
		std::shared_ptr<cxxast::Directive> directive = std::static_pointer_cast<cxxast::Directive>(astNode);
		os << "#" << directive->name << " ";
		for (size_t i = 0; i < directive->args.size(); ++i) {
			_dumpAstNode(os, directive->args[i], dumpMode, 0);
			os << " ";
		}
		os << "\n";
		break;
	}
	case cxxast::NodeKind::IncludeDirective: {
		std::shared_ptr<cxxast::IncludeDirective> directive = std::static_pointer_cast<cxxast::IncludeDirective>(astNode);
		os << "#include ";

		if (directive->isSystem) {
			os << "<" << directive->name << ">";
		} else {
			os << "\"" << directive->name << "\"";
		}
		os << "\n";
		break;
	}
	case cxxast::NodeKind::IfDirective: {
		std::shared_ptr<cxxast::IfDirective> directive = std::static_pointer_cast<cxxast::IfDirective>(astNode);
		os << "#if", _dumpAstNode(os, directive->condition, dumpMode, 0), os << "\n";
		_dumpAstNode(os, directive->trueBranch, dumpMode, 0), os << "\n";
		os << "#else\n";
		_dumpAstNode(os, directive->elseBranch, dumpMode, 0), os << "\n";
		os << "#endif\n";
		break;
	}
	case cxxast::NodeKind::IfdefDirective: {
		std::shared_ptr<cxxast::IfdefDirective> directive = std::static_pointer_cast<cxxast::IfdefDirective>(astNode);
		os << "#ifdef " << directive->name << "\n";
		_dumpAstNode(os, directive->trueBranch, dumpMode, 0), os << "\n";
		os << "#else\n";
		_dumpAstNode(os, directive->elseBranch, dumpMode, 0), os << "\n";
		os << "#endif\n";
		break;
	}
	case cxxast::NodeKind::IfndefDirective: {
		std::shared_ptr<cxxast::IfndefDirective> directive = std::static_pointer_cast<cxxast::IfndefDirective>(astNode);
		os << "#ifndef " << directive->name << "\n";
		for (size_t i = 0; i < directive->trueBranch.size(); ++i) {
			if (i)
				os << "\n";
			_dumpAstNode(os, directive->trueBranch[i], dumpMode, 0);
		}
		os << "#else\n";
		for (size_t i = 0; i < directive->elseBranch.size(); ++i) {
			if (i)
				os << "\n";
			_dumpAstNode(os, directive->elseBranch[i], dumpMode, 0);
		}
		os << "#endif\n";
		break;
	}
	case cxxast::NodeKind::Fn: {
		std::shared_ptr<cxxast::Fn> overloading = std::static_pointer_cast<cxxast::Fn>(astNode);

		if (!overloading->body.size()) {
			if (dumpMode != ASTDumpMode::Header)
				break;
		}

		if (overloading->signature.genericParams.size()) {
			if (dumpMode == ASTDumpMode::Header)
				break;

			os << std::string(indentLevel, '\t');
			os << "template <";
			for (size_t i = 0; i < overloading->signature.genericParams.size(); ++i) {
				cxxast::GenericParam &genericParam = overloading->signature.genericParams.at(i);

				if (i)
					os << ", ";

				os << "typename ";
				os << genericParam.name;
			}
			os << ">\n";
		}

		if (dumpMode == ASTDumpMode::Header)
			os << std::string(indentLevel, '\t');

		if ((dumpMode == ASTDumpMode::Header) &&
			(overloading->properties.isVirtual))
			os << "virtual ";
		if ((dumpMode == ASTDumpMode::Header) &&
			(overloading->properties.isStatic))
			os << "static ";
		_dumpAstNode(os, overloading->returnType, dumpMode, 0);
		if (dumpMode == ASTDumpMode::Source) {
			os << " ";
			_dumpAstNode(os, _getAbsRef(overloading), dumpMode, 0);
			os << "(";
		} else {
			os << " " << overloading->name << "(";
		}
		for (size_t i = 0; i < overloading->signature.paramTypes.size(); ++i) {
			if (i)
				os << ", ";
			_dumpAstNode(os, overloading->signature.paramTypes[i], dumpMode, 0);
			os << " param" << i;
		}
		os << ")";
		if ((dumpMode == ASTDumpMode::Header) &&
			(overloading->properties.isOverriden))
			os << " override";
		if (overloading->signature.genericParams.size()) {
			if (dumpMode == ASTDumpMode::Header) {
				os << " {\n";

				os << std::string(indentLevel, '\t');
				os << "}\n";
			}
		} else {
			if (dumpMode == ASTDumpMode::Header) {
				os << ";\n";
			} else {
				os << " {\n";

				for (size_t i = 0; i < overloading->body.size(); ++i) {
					_dumpAstNode(os, overloading->body[i], dumpMode, indentLevel + 1);
				}

				os << "}\n";
			}
		}

		break;
	}
	case cxxast::NodeKind::Struct: {
		std::shared_ptr<cxxast::Struct> structNode = std::static_pointer_cast<cxxast::Struct>(astNode);

		if (dumpMode == ASTDumpMode::Header) {
			os << std::string(indentLevel, '\t');
			os << "struct " << structNode->name << " {\n";

			for (auto &i : structNode->publicMembers) {
				_dumpAstNode(os, i.second, dumpMode, indentLevel + 1);
			}

			if (structNode->protectedMembers.size()) {
				os << std::string(indentLevel, '\t');
				os << "protected:\n";

				for (auto &i : structNode->protectedMembers) {
					_dumpAstNode(os, i.second, dumpMode, indentLevel + 1);
				}
			}
			os << "\n";

			os << std::string(indentLevel, '\t');
			os << "};\n";
		} else {
			for (auto &i : structNode->publicMembers) {
				_dumpAstNode(os, i.second, dumpMode, indentLevel + 1);
			}
			for (auto &i : structNode->protectedMembers) {
				_dumpAstNode(os, i.second, dumpMode, indentLevel + 1);
			}
		}

		break;
	}
	case cxxast::NodeKind::Class: {
		std::shared_ptr<cxxast::Struct> classNode = std::static_pointer_cast<cxxast::Struct>(astNode);

		if (dumpMode == ASTDumpMode::Header) {
			if (classNode->genericParams.size()) {
				os << std::string(indentLevel, '\t');
				os << "template <";
				for (size_t i = 0; i < classNode->genericParams.size(); ++i) {
					cxxast::GenericParam &genericParam = classNode->genericParams.at(i);

					if (i)
						os << ", ";

					os << "typename ";
					os << genericParam.name;
				}
				os << ">\n";
			}
			os << std::string(indentLevel, '\t');
			os << "class " << classNode->name << " {\n";

			os << std::string(indentLevel, '\t');
			os << "protected:\n";

			for (auto &i : classNode->protectedMembers) {
				_dumpAstNode(os, i.second, dumpMode, indentLevel + 1);
			}

			os << "\n";

			os << std::string(indentLevel, '\t');
			os << "public:\n";

			for (auto &i : classNode->publicMembers) {
				_dumpAstNode(os, i.second, dumpMode, indentLevel + 1);
			}
			os << "\n";

			os << std::string(indentLevel, '\t');
			os << "};\n";
		} else {
			if (!classNode->genericParams.size()) {
				for (auto &i : classNode->protectedMembers) {
					_dumpAstNode(os, i.second, dumpMode, indentLevel);
				}
				for (auto &i : classNode->publicMembers) {
					_dumpAstNode(os, i.second, dumpMode, indentLevel);
				}
			}
		}

		break;
	}
	case cxxast::NodeKind::Namespace: {
		std::shared_ptr<cxxast::Namespace> namespaceNode = std::static_pointer_cast<cxxast::Namespace>(astNode);

		if (dumpMode == ASTDumpMode::Header) {
			if (namespaceNode->name.empty()) {
				for (auto &i : namespaceNode->publicMembers) {
					_dumpAstNode(os, i.second, dumpMode, indentLevel);
				}
				os << "\n";
			} else {
				os << std::string(indentLevel, '\t');
				os << "namespace " << namespaceNode->name << " {\n";

				for (auto &i : namespaceNode->publicMembers) {
					_dumpAstNode(os, i.second, dumpMode, indentLevel + 1);
				}
				for (auto &i : namespaceNode->protectedMembers) {
					_dumpAstNode(os, i.second, dumpMode, indentLevel + 1);
				}
				os << "\n";

				os << std::string(indentLevel, '\t');
				os << "}\n";
			}
		} else {
			for (auto &i : namespaceNode->publicMembers) {
				_dumpAstNode(os, i.second, dumpMode, indentLevel);
			}
			for (auto &i : namespaceNode->protectedMembers) {
				_dumpAstNode(os, i.second, dumpMode, indentLevel);
			}
		}

		break;
	}
	case cxxast::NodeKind::Var: {
		std::shared_ptr<cxxast::Var> varNode = std::static_pointer_cast<cxxast::Var>(astNode);

		if (dumpMode == ASTDumpMode::Header) {
			os << std::string(indentLevel, '\t');

			switch (varNode->storageClass) {
			case cxxast::StorageClass::Unspecified:
				break;
			case cxxast::StorageClass::Static:
				os << "static ";
				break;
			case cxxast::StorageClass::Extern:
				os << "extern ";
				break;
			case cxxast::StorageClass::Mutable:
				os << "mutable ";
				break;
			}
			_dumpAstNode(os, varNode->type, dumpMode, 0);
			os << " " << varNode->name << ";\n";
		} else {
			switch (varNode->storageClass) {
			case cxxast::StorageClass::Unspecified: {
				std::shared_ptr<cxxast::AbstractModule> parent = varNode->parent.lock();

				switch (parent->nodeKind) {
				case cxxast::NodeKind::Class:
				case cxxast::NodeKind::Struct:
					break;
				default:
					std::terminate();
				}
				break;
			}
			case cxxast::StorageClass::Extern: {
				_dumpAstNode(os, varNode->type, dumpMode, 0);
				os << " ";
				_dumpAstNode(os, _getAbsRef(varNode), dumpMode, 0);
				if (varNode->initialValue) {
					os << " = ";
					_dumpAstNode(os, varNode->initialValue, dumpMode, 0);
				}
				os << ";\n";
				break;
			}
			case cxxast::StorageClass::Static: {
				std::shared_ptr<cxxast::AbstractModule> parent = varNode->parent.lock();

				switch (parent->nodeKind) {
				case cxxast::NodeKind::Class:
				case cxxast::NodeKind::Struct: {
					_dumpAstNode(os, varNode->type, dumpMode, 0);
					os << " ";
					_dumpAstNode(os, _getAbsRef(varNode), dumpMode, 0);
					if (varNode->initialValue) {
						os << " = ";
						_dumpAstNode(os, varNode->initialValue, dumpMode, 0);
					}
					os << ";\n";
					break;
				}
				default:
					std::terminate();
				}
				break;
			}
			case cxxast::StorageClass::Mutable:
				break;
			}
		}

		break;
	}
	case cxxast::NodeKind::TypeName: {
		std::shared_ptr<cxxast::TypeName> tn = std::static_pointer_cast<cxxast::TypeName>(astNode);

		if (tn->isConst)
			os << "const ";
		if (tn->isVolatile)
			os << "volatile ";

		switch (tn->typeNameKind) {
		case cxxast::TypeNameKind::Void:
			os << "void";
			break;
		case cxxast::TypeNameKind::Int: {
			std::shared_ptr<cxxast::IntTypeName> i = std::static_pointer_cast<cxxast::IntTypeName>(tn);

			switch (i->signKind) {
			case cxxast::SignKind::Unspecified:
				break;
			case cxxast::SignKind::Signed:
				os << "signed ";
				break;
			case cxxast::SignKind::Unsigned:
				os << "unsigned ";
				break;
			}

			switch (i->modifierKind) {
			case cxxast::IntModifierKind::Unspecified:
				os << "int";
				break;
			case cxxast::IntModifierKind::Short:
				os << "short";
				break;
			case cxxast::IntModifierKind::Long:
				os << "long";
				break;
			case cxxast::IntModifierKind::LongLong:
				os << "long long";
				break;
			}
			break;
		}
		case cxxast::TypeNameKind::Bool:
			os << "bool";
			break;
		case cxxast::TypeNameKind::Char:
			os << "char";
			break;
		case cxxast::TypeNameKind::Float:
			os << "float";
			break;
		case cxxast::TypeNameKind::Double:
			os << "double";
			break;
		case cxxast::TypeNameKind::Pointer: {
			std::shared_ptr<cxxast::PointerTypeName> ptn = std::static_pointer_cast<cxxast::PointerTypeName>(tn);

			_dumpAstNode(os, ptn->pointedType, dumpMode, indentLevel);
			os << "*";

			break;
		}
		case cxxast::TypeNameKind::FnPointer: {
			// Function pointer's behavior depends on the context, do nothing.
			break;
		}
		case cxxast::TypeNameKind::Array: {
			std::shared_ptr<cxxast::ArrayTypeName> ptn = std::static_pointer_cast<cxxast::ArrayTypeName>(tn);

			_dumpAstNode(os, ptn->elementType, dumpMode, 0);
			os << "[]";

			break;
		}
		case cxxast::TypeNameKind::Ref: {
			std::shared_ptr<cxxast::RefTypeName> ptn = std::static_pointer_cast<cxxast::RefTypeName>(tn);

			_dumpAstNode(os, ptn->refType, dumpMode, 0);
			os << " &";

			break;
		}
		case cxxast::TypeNameKind::Rvalue: {
			std::shared_ptr<cxxast::RvalueTypeName> ptn = std::static_pointer_cast<cxxast::RvalueTypeName>(tn);

			_dumpAstNode(os, ptn->refType, dumpMode, 0);
			os << " &&";

			break;
		}
		case cxxast::TypeNameKind::Custom: {
			std::shared_ptr<cxxast::CustomTypeName> ptn = std::static_pointer_cast<cxxast::CustomTypeName>(tn);

			_dumpAstNode(os, ptn->refTarget, dumpMode, 0);

			break;
		}
		}
		break;
	}
	case cxxast::NodeKind::Stmt: {
		std::shared_ptr<cxxast::Stmt> stmt = std::static_pointer_cast<cxxast::Stmt>(astNode);
		os << std::string(indentLevel, '\t');

		switch (stmt->stmtKind) {
		case cxxast::StmtKind::Expr:
			_dumpAstNode(os, std::static_pointer_cast<cxxast::ExprStmt>(stmt)->expr, dumpMode, 0);
			os << ";";
			break;
		case cxxast::StmtKind::LocalVarDef: {
			std::shared_ptr<cxxast::LocalVarDefStmt> s = std::static_pointer_cast<cxxast::LocalVarDefStmt>(stmt);
			_dumpAstNode(os, s->type, dumpMode, 0);
			os << " ";

			for (size_t i = 0; i < s->varDefPairs.size(); ++i) {
				os << s->varDefPairs[i].name;
				if (s->varDefPairs[i].initialValue) {
					os << " = ";
					_dumpAstNode(os, s->varDefPairs[i].initialValue, dumpMode, 0);
				}
			}

			os << ";";
			break;
		}
		case cxxast::StmtKind::If: {
			std::shared_ptr<cxxast::IfStmt> s = std::static_pointer_cast<cxxast::IfStmt>(stmt);
			os << "if (";

			_dumpAstNode(os, s->condition, dumpMode, 0);

			os << ")\n";

			_dumpAstNode(os, s->trueBranch, dumpMode, indentLevel + 1);

			if (s->elseBranch) {
				os << std::string(indentLevel, '\t');
				os << "else\n";
				_dumpAstNode(os, s->elseBranch, dumpMode, indentLevel + 1);
			}
			break;
		}
		case cxxast::StmtKind::For: {
			std::shared_ptr<cxxast::ForStmt> s = std::static_pointer_cast<cxxast::ForStmt>(stmt);
			os << "for (";

			if (s->varDefs) {
				_dumpAstNode(os, s->varDefs, dumpMode, 0);
			}

			os << "; ";

			if (s->condition) {
				_dumpAstNode(os, s->condition, dumpMode, 0);
			}

			os << "; ";

			if (s->endExpr) {
				_dumpAstNode(os, s->endExpr, dumpMode, 0);
			}

			os << ")\n";

			_dumpAstNode(os, s->body, dumpMode, indentLevel + 1);
			break;
		}
		case cxxast::StmtKind::While: {
			std::shared_ptr<cxxast::ForStmt> s = std::static_pointer_cast<cxxast::ForStmt>(stmt);
			os << "while (";
			_dumpAstNode(os, s->condition, dumpMode, 0);
			os << ")\n";

			_dumpAstNode(os, s->body, dumpMode, indentLevel + 1);
			break;
		}
		case cxxast::StmtKind::Break:
			os << "break;";
			break;
		case cxxast::StmtKind::Continue:
			os << "continue;";
			break;
		case cxxast::StmtKind::Return: {
			std::shared_ptr<cxxast::ReturnStmt> s = std::static_pointer_cast<cxxast::ReturnStmt>(stmt);
			os << "return ";

			if (s->value)
				_dumpAstNode(os, s->value, dumpMode, indentLevel + 1);

			os << ";";
			break;
		}
		}

		os << "\n";
		break;
	}
	case cxxast::NodeKind::Expr: {
		std::shared_ptr<cxxast::Expr> expr = std::static_pointer_cast<cxxast::Expr>(astNode);

		switch (expr->exprKind) {
		case cxxast::ExprKind::IntLiteral: {
			std::shared_ptr<cxxast::IntLiteralExpr> e = std::static_pointer_cast<cxxast::IntLiteralExpr>(expr);

			os << e->data;
			break;
		}
		case cxxast::ExprKind::LongLiteral: {
			std::shared_ptr<cxxast::LongLiteralExpr> e = std::static_pointer_cast<cxxast::LongLiteralExpr>(expr);

			os << e->data;
			break;
		}
		case cxxast::ExprKind::UIntLiteral: {
			std::shared_ptr<cxxast::UIntLiteralExpr> e = std::static_pointer_cast<cxxast::UIntLiteralExpr>(expr);

			os << e->data << "U";
			break;
		}
		case cxxast::ExprKind::ULongLiteral: {
			std::shared_ptr<cxxast::ULongLiteralExpr> e = std::static_pointer_cast<cxxast::ULongLiteralExpr>(expr);

			os << e->data << "ULL";
			break;
		}
		case cxxast::ExprKind::CharLiteral: {
			std::shared_ptr<cxxast::CharLiteralExpr> e = std::static_pointer_cast<cxxast::CharLiteralExpr>(expr);

			char c[3];

			c[0] = (e->data & 0xf) + '0';
			c[1] = (e->data >> 4) + '0';
			c[2] = '\0';

			os << "'\\" << c << "'";
			break;
		}
		case cxxast::ExprKind::StringLiteral: {
			std::shared_ptr<cxxast::StringLiteralExpr> e = std::static_pointer_cast<cxxast::StringLiteralExpr>(expr);

			os << "\"";

			for (size_t i = 0; i < e->data.size(); ++i) {
				char c[3];

				if ((c[0] = (e->data[i] & 0xf)) > 9) {
					c[0] = c[0] - 10 + 'a';
				}
				if ((c[1] = (e->data[i] >> 4)) > 9) {
					c[1] = c[1] - 10 + 'a';
				}
				c[2] = '\0';

				os << "\\" << c;
			}

			os << "\"";
			break;
		}
		case cxxast::ExprKind::FloatLiteral: {
			std::shared_ptr<cxxast::FloatLiteralExpr> e = std::static_pointer_cast<cxxast::FloatLiteralExpr>(expr);

			std::stringstream backupFmtStream(nullptr);
			backupFmtStream.copyfmt(os);

			os << std::hexfloat << e->data;

			os.copyfmt(backupFmtStream);
			break;
		}
		case cxxast::ExprKind::DoubleLiteral: {
			std::shared_ptr<cxxast::DoubleLiteralExpr> e = std::static_pointer_cast<cxxast::DoubleLiteralExpr>(expr);

			std::stringstream backupFmtStream(nullptr);
			backupFmtStream.copyfmt(os);

			os << std::hexfloat << e->data;

			os.copyfmt(backupFmtStream);
			break;
		}
		case cxxast::ExprKind::BoolLiteral: {
			std::shared_ptr<cxxast::BoolLiteralExpr> e = std::static_pointer_cast<cxxast::BoolLiteralExpr>(expr);

			if (e->data)
				os << "true";
			else
				os << "false";
			break;
		}
		case cxxast::ExprKind::NullptrLiteral: {
			os << "nullptr";
			break;
		}
		case cxxast::ExprKind::Id: {
			std::shared_ptr<cxxast::IdExpr> e = std::static_pointer_cast<cxxast::IdExpr>(expr);

			os << e->name;
			break;
		}
		case cxxast::ExprKind::InitializerList: {
			std::shared_ptr<cxxast::InitializerListExpr> e = std::static_pointer_cast<cxxast::InitializerListExpr>(expr);

			if (e->type) {
				_dumpAstNode(os, e->type, dumpMode, 0);
			}

			os << "{ ";

			for (size_t i = 0; i < e->args.size(); ++i) {
				if (i)
					os << ", ";
				_dumpAstNode(os, e->args[i], dumpMode, 0);
			}

			os << " }";
			break;
		}
		case cxxast::ExprKind::Unary: {
			std::shared_ptr<cxxast::UnaryExpr> e = std::static_pointer_cast<cxxast::UnaryExpr>(expr);

			os << "(";
			switch (e->op) {
			case cxxast::UnaryOp::IncForward:
				os << "++";
				_dumpAstNode(os, e->operand, dumpMode, 0);
				break;
			case cxxast::UnaryOp::IncBackward:
				_dumpAstNode(os, e->operand, dumpMode, 0);
				os << "++";
				break;
			case cxxast::UnaryOp::DecForward:
				os << "--";
				_dumpAstNode(os, e->operand, dumpMode, 0);
				break;
			case cxxast::UnaryOp::DecBackward:
				_dumpAstNode(os, e->operand, dumpMode, 0);
				os << "--";
				break;
			case cxxast::UnaryOp::Plus:
				os << "+";
				_dumpAstNode(os, e->operand, dumpMode, 0);
				break;
			case cxxast::UnaryOp::Negate:
				os << "-";
				_dumpAstNode(os, e->operand, dumpMode, 0);
				break;
			case cxxast::UnaryOp::Not:
				os << "~";
				_dumpAstNode(os, e->operand, dumpMode, 0);
				break;
			case cxxast::UnaryOp::LNot:
				os << "!";
				_dumpAstNode(os, e->operand, dumpMode, 0);
				break;
			case cxxast::UnaryOp::Dereference:
				os << "*";
				_dumpAstNode(os, e->operand, dumpMode, 0);
				break;
			case cxxast::UnaryOp::AddressOf:
				os << "&";
				_dumpAstNode(os, e->operand, dumpMode, 0);
				break;
			}
			os << ")";
			break;
		}
		case cxxast::ExprKind::Binary: {
			std::shared_ptr<cxxast::BinaryExpr> e = std::static_pointer_cast<cxxast::BinaryExpr>(expr);

			switch (e->op) {
			case cxxast::BinaryOp::Add:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";

				os << " + ";

				os << "(";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << ")";
				break;
			case cxxast::BinaryOp::Sub:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";

				os << " - ";

				os << "(";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << ")";
				break;
			case cxxast::BinaryOp::Mul:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";

				os << " * ";

				os << "(";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << ")";
				break;
			case cxxast::BinaryOp::Div:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";

				os << " / ";

				os << "(";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << ")";
				break;
			case cxxast::BinaryOp::And:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";

				os << " & ";

				os << "(";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << ")";
				break;
			case cxxast::BinaryOp::Or:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";

				os << " | ";

				os << "(";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << ")";
				break;
			case cxxast::BinaryOp::Xor:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";

				os << " ^ ";

				os << "(";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << ")";
				break;
			case cxxast::BinaryOp::LAnd:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";

				os << " && ";

				os << "(";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << ")";
				break;
			case cxxast::BinaryOp::LOr:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";

				os << " || ";

				os << "(";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << ")";
				break;
			case cxxast::BinaryOp::Shl:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";

				os << " << ";

				os << "(";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << ")";
				break;
			case cxxast::BinaryOp::Shr:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";

				os << " >> ";

				os << "(";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << ")";
				break;
			case cxxast::BinaryOp::Eq:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";

				os << " == ";

				os << "(";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << ")";
				break;
			case cxxast::BinaryOp::Neq:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";

				os << " != ";

				os << "(";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << ")";
				break;
			case cxxast::BinaryOp::Lt:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";

				os << " < ";

				os << "(";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << ")";
				break;
			case cxxast::BinaryOp::Gt:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";

				os << " > ";

				os << "(";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << ")";
				break;
			case cxxast::BinaryOp::LtEq:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";

				os << " <= ";

				os << "(";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << ")";
				break;
			case cxxast::BinaryOp::GtEq:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";

				os << " >= ";

				os << "(";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << ")";
				break;
			case cxxast::BinaryOp::Cmp:
				// TODO: Implement it.
				break;
			case cxxast::BinaryOp::Subscript:
				os << "(";
				_dumpAstNode(os, e->lhs, dumpMode, 0);
				os << ")";
				os << "[";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				os << "]";
				break;
			case cxxast::BinaryOp::Scope:
				if (!_isSimpleIdExpr(e->lhs)) {
					os << "(";
					_dumpAstNode(os, e->lhs, dumpMode, 0);
					os << ")";
				} else {
					_dumpAstNode(os, e->lhs, dumpMode, 0);
				}
				os << "::";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				break;
			case cxxast::BinaryOp::MemberAccess:
				if (!_isSimpleIdExpr(e->lhs)) {
					os << "(";
					_dumpAstNode(os, e->lhs, dumpMode, 0);
					os << ")";
				} else {
					_dumpAstNode(os, e->lhs, dumpMode, 0);
				}
				os << ".";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				break;
			case cxxast::BinaryOp::PtrAccess:
				if (!_isSimpleIdExpr(e->lhs)) {
					os << "(";
					_dumpAstNode(os, e->lhs, dumpMode, 0);
					os << ")";
				} else {
					_dumpAstNode(os, e->lhs, dumpMode, 0);
				}
				os << "->";
				_dumpAstNode(os, e->rhs, dumpMode, 0);
				break;
			}
			break;
		}
		case cxxast::ExprKind::Call: {
			std::shared_ptr<cxxast::CallExpr> e = std::static_pointer_cast<cxxast::CallExpr>(expr);

			if (!_isSimpleIdExpr(e->callee)) {
				os << "(";
				_dumpAstNode(os, e->callee, dumpMode, 0);
				os << ")";
			} else {
				_dumpAstNode(os, e->callee, dumpMode, 0);
			}

			os << "(";

			for (size_t i = 0; i < e->args.size(); ++i) {
				if (i)
					os << ", ";
				_dumpAstNode(os, e->args[i], dumpMode, 0);
			}

			os << ")";

			break;
		}
		case cxxast::ExprKind::Cast: {
			std::shared_ptr<cxxast::CastExpr> e = std::static_pointer_cast<cxxast::CastExpr>(expr);

			os << "(";
			_dumpAstNode(os, e->destType, dumpMode, 0);
			os << ")";

			os << "(";
			_dumpAstNode(os, e->source, dumpMode, 0);
			os << ")";

			break;
		}
		case cxxast::ExprKind::New: {
			std::shared_ptr<cxxast::NewExpr> e = std::static_pointer_cast<cxxast::NewExpr>(expr);

			os << "new ";
			_dumpAstNode(os, e->type, dumpMode, 0);

			os << "(";

			for (size_t i = 0; i < e->args.size(); ++i) {
				if (i)
					os << ", ";
				_dumpAstNode(os, e->args[i], dumpMode, 0);
			}

			os << ")";

			break;
		}
		case cxxast::ExprKind::Conditional: {
			std::shared_ptr<cxxast::ConditionalExpr> e = std::static_pointer_cast<cxxast::ConditionalExpr>(expr);

			os << "(";
			_dumpAstNode(os, e->condition, dumpMode, 0);
			os << ")";

			os << " ? ";

			os << "(";
			_dumpAstNode(os, e->trueExpr, dumpMode, 0);
			os << ")";

			os << " : ";

			os << "(";
			_dumpAstNode(os, e->falseExpr, dumpMode, 0);
			os << ")";

			break;
		}
		case cxxast::ExprKind::TypeSizeof: {
			std::shared_ptr<cxxast::TypeSizeofExpr> e = std::static_pointer_cast<cxxast::TypeSizeofExpr>(expr);

			os << "sizeof(";
			_dumpAstNode(os, e->target, dumpMode, 0);
			os << ")";

			break;
		}
		case cxxast::ExprKind::ExprSizeof: {
			std::shared_ptr<cxxast::ExprSizeofExpr> e = std::static_pointer_cast<cxxast::ExprSizeofExpr>(expr);

			os << "sizeof(";
			_dumpAstNode(os, e->target, dumpMode, 0);
			os << ")";

			break;
		}
		case cxxast::ExprKind::This:
			os << "this";
			break;
		}
	}
	}

	switch (dumpMode) {
	case ASTDumpMode::Header: {
		for (size_t i = 0; i < astNode->declTrailingNodes.size(); ++i) {
			_dumpAstNode(os, astNode->declTrailingNodes[i], dumpMode, indentLevel);
		}
		break;
	}
	case ASTDumpMode::Source: {
		for (size_t i = 0; i < astNode->defTrailingNodes.size(); ++i) {
			_dumpAstNode(os, astNode->defTrailingNodes[i], dumpMode, indentLevel);
		}
		break;
	}
	default:
		std::terminate();
	}
}

void BC2CXX::dumpAstNode(std::ostream &os, std::shared_ptr<cxxast::ASTNode> astNode, ASTDumpMode dumpMode) {
	_dumpAstNode(os, astNode, dumpMode, 0);
}
