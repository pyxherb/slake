#ifndef _SLKC_COMPILER_AST_FN_H_
#define _SLKC_COMPILER_AST_FN_H_

#include "typename.h"
#include "generic.h"
#include "stmt.h"
#include "member.h"
#include <slake/access.h>

namespace slake {
	namespace slkc {
		class FnNode;

		class FnOverloadingNode final : public MemberNode {
		private:
			virtual shared_ptr<AstNode> doDuplicate() override;

		public:
			Location loc;
			shared_ptr<TypeNameNode> returnType;

			deque<Param> params;
			unordered_map<string, size_t> paramIndices;

			shared_ptr<BlockStmtNode> body;
			FnNode* owner;
			AccessModifier access = 0;

			inline FnOverloadingNode(const FnOverloadingNode &other) : MemberNode(other) {
				loc = other.loc;
				returnType = other.returnType;

				params = other.params;
				paramIndices = other.paramIndices;

				body = other.body;
				owner = other.owner;
				access = other.access;
			}

			FnOverloadingNode(
				Location loc,
				Compiler *compiler,
				shared_ptr<TypeNameNode> returnType,
				GenericParamNodeList genericParams,
				deque<Param> params,
				shared_ptr<Scope> scope = make_shared<Scope>());

			virtual inline NodeType getNodeType() const override { return NodeType::FnOverloading; }

			virtual RefEntry getName() const override { throw std::logic_error("Cannot get name of a function overloading"); }
			virtual Location getLocation() const override { return loc; }

			inline bool isAbstract() { return !body; }
			inline bool isVaridic() { return paramIndices.count("..."); }
		};

		class FnNode final : public MemberNode {
		private:
			virtual shared_ptr<AstNode> doDuplicate() override;

		public:
			deque<shared_ptr<FnOverloadingNode>> overloadingRegistries;
			string name;
			bool used = false;

			inline FnNode(const FnNode &other) : MemberNode(other) {
				overloadingRegistries.resize(other.overloadingRegistries.size());
				for (size_t i = 0; i < other.overloadingRegistries.size(); ++i) {
					overloadingRegistries[i] = other.overloadingRegistries[i]->duplicate<FnOverloadingNode>();
					overloadingRegistries[i]->owner = this;
				}
				name = other.name;
				used = false;
			}
			inline FnNode(
				Compiler *compiler,
				string name)
				: name(name), MemberNode(compiler, ACCESS_PUB) {}
			virtual ~FnNode() = default;

			virtual inline Location getLocation() const override {
				throw std::logic_error("Please get locations from the overloading registries");
			}

			virtual inline NodeType getNodeType() const override { return NodeType::Fn; }

			virtual RefEntry getName() const override { return RefEntry({}, name, genericArgs); }
		};

		struct Ins {
			Opcode opcode;
			deque<shared_ptr<AstNode>> operands;

			Ins() = default;
			inline Ins(
				Opcode opcode,
				deque<shared_ptr<AstNode>> operands = {})
				: opcode(opcode), operands(operands) {}
		};

		class CompiledFnNode final : public MemberNode {
		private:
			Location _loc;
			virtual shared_ptr<AstNode> doDuplicate() override;

		public:
			string name;
			deque<Ins> body;
			unordered_map<string, size_t> labels;

			GenericParamNodeList genericParams;
			unordered_map<string, size_t> genericParamIndices;

			deque<Param> params;
			unordered_map<string, size_t> paramIndices;

			shared_ptr<TypeNameNode> returnType;

			deque<slxfmt::SourceLocDesc> srcLocDescs;

			inline CompiledFnNode(const CompiledFnNode& other) {
				name = other.name;
				body = other.body;
				labels = other.labels;

				genericParams.resize(other.genericParams.size());
				for (size_t i = 0; i < other.genericParams.size(); ++i)
					genericParams[i] = other.genericParams[i]->duplicate<GenericParamNode>();
				genericParamIndices = other.genericParamIndices;

				params = other.params;
				paramIndices = other.paramIndices;

				returnType = other.returnType->duplicate<TypeNameNode>();

				srcLocDescs = other.srcLocDescs;
			}
			inline CompiledFnNode(Location loc, string name) : _loc(loc), name(name) {}
			virtual ~CompiledFnNode() = default;

			virtual inline Location getLocation() const override {
				return _loc;
			}

			virtual inline NodeType getNodeType() const override { return NodeType::Fn; }

			inline void insertIns(Ins ins) { body.push_back(ins); }
			inline void insertIns(
				Opcode opcode,
				shared_ptr<AstNode> op1) {
				insertIns({ opcode, { op1 } });
			}
			inline void insertIns(
				Opcode opcode,
				shared_ptr<AstNode> op1,
				shared_ptr<AstNode> op2) {
				insertIns({ opcode, { op1, op2 } });
			}
			inline void insertIns(
				Opcode opcode,
				shared_ptr<AstNode> op1,
				shared_ptr<AstNode> op2,
				shared_ptr<AstNode> op3) {
				insertIns({ opcode, { op1, op2, op3 } });
			}
			inline void insertIns(
				Opcode opcode,
				shared_ptr<AstNode> op1,
				shared_ptr<AstNode> op2,
				shared_ptr<AstNode> op3,
				shared_ptr<AstNode> op4) {
				insertIns({ opcode, { op1, op2, op3, op4 } });
			}
			inline void insertLabel(string name) { labels[name] = body.size(); }

			virtual RefEntry getName() const override { return RefEntry(_loc, name, genericArgs); }
		};

		class LabelRefNode final : public AstNode {
		public:
			string label;

			inline LabelRefNode(string label) : label(label) {}
			virtual ~LabelRefNode() = default;

			virtual inline Location getLocation() const override {
				throw std::logic_error("Should not get location of a label reference");
			}

			virtual inline NodeType getNodeType() const override { return NodeType::LabelRef; }
		};

		class RegRefNode final : public AstNode {
		public:
			uint32_t index;
			bool unwrapData;

			inline RegRefNode(uint32_t index, bool unwrapData = false) : index(index), unwrapData(unwrapData) {}
			virtual ~RegRefNode() = default;

			virtual inline Location getLocation() const override {
				throw std::logic_error("Should not get location of a register reference");
			}

			virtual inline NodeType getNodeType() const override { return NodeType::RegRef; }
		};

		class LocalVarRefNode final : public AstNode {
		public:
			uint32_t index;
			bool unwrapData;

			inline LocalVarRefNode(uint32_t index, bool unwrapData = false) : index(index), unwrapData(unwrapData) {}
			virtual ~LocalVarRefNode() = default;

			virtual inline Location getLocation() const override {
				throw std::logic_error("Should not get location of a label reference");
			}

			virtual inline NodeType getNodeType() const override { return NodeType::LocalVarRef; }
		};

		class ArgRefNode final : public AstNode {
		public:
			uint32_t index;
			bool unwrapData;

			inline ArgRefNode(uint32_t index, bool unwrapData = false) : index(index), unwrapData(unwrapData) {}
			virtual ~ArgRefNode() = default;

			virtual inline Location getLocation() const override {
				throw std::logic_error("Should not get location of a argument reference");
			}

			virtual inline NodeType getNodeType() const override { return NodeType::ArgRef; }
		};

		class GenericArgRefNode final : public AstNode {
		public:
			uint32_t index;

			inline GenericArgRefNode(uint32_t index) : index(index) {}
			virtual ~GenericArgRefNode() = default;

			virtual inline Location getLocation() const override {
				throw std::logic_error("Should not get location of a generic argument reference");
			}

			virtual inline NodeType getNodeType() const override { return NodeType::GenericArgRef; }
		};
	}
}

#endif
