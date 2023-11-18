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

		struct FnOverloadingRegistry {
			Location loc;
			shared_ptr<TypeNameNode> returnType;

			GenericParamList genericParams;
			unordered_map<string, size_t> genericParamIndices;

			deque<Param> params;
			unordered_map<string, size_t> paramIndices;

			shared_ptr<BlockStmtNode> body;
			weak_ptr<FnNode> parent;
			AccessModifier access = 0;

			FnOverloadingRegistry(
				Location loc,
				shared_ptr<TypeNameNode> returnType,
				GenericParamList genericParams,
				deque<Param> params);

			inline bool operator<(const FnOverloadingRegistry &rhs) {
				if (params.size() < rhs.params.size())
					return true;
			}

			inline bool isAbstract() { return !body; }
		};

		class FnNode final : public MemberNode {
		public:
			deque<FnOverloadingRegistry> overloadingRegistries;
			string name;
			bool used = false;

			inline FnNode(
				string name)
				: name(name), MemberNode(ACCESS_PUB) {}
			virtual ~FnNode() = default;

			virtual inline Location getLocation() const override {
				throw std::logic_error("Please get locations from the overloading registries");
			}

			virtual inline NodeType getNodeType() const override { return AST_FN; }

			virtual Ref getName() const override { return Ref({ RefEntry({}, name, {}) }); }
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

		public:
			string name;
			deque<Ins> body;
			unordered_map<string, size_t> labels;

			GenericParamList genericParams;
			unordered_map<string, size_t> genericParamIndices;

			deque<Param> params;
			unordered_map<string, size_t> paramIndices;

			shared_ptr<TypeNameNode> returnType;

			inline CompiledFnNode(Location loc, string name) : _loc(loc), name(name) {}
			virtual ~CompiledFnNode() = default;

			virtual inline Location getLocation() const override {
				return _loc;
			}

			virtual inline NodeType getNodeType() const override { return AST_FN; }

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

			virtual Ref getName() const override { return Ref({ RefEntry({}, name, {}) }); }
		};

		class LabelRefNode final : public AstNode {
		public:
			string label;

			inline LabelRefNode(string label) : label(label) {}
			virtual ~LabelRefNode() = default;

			virtual inline Location getLocation() const override {
				throw std::logic_error("Should not get location of a label reference");
			}

			virtual inline NodeType getNodeType() const override { return AST_LABEL_REF; }
		};

		class RegRefNode final : public AstNode {
		public:
			RegId reg;

			inline RegRefNode(RegId reg) : reg(reg) {}
			virtual ~RegRefNode() = default;

			virtual inline Location getLocation() const override {
				throw std::logic_error("Should not get location of a label reference");
			}

			virtual inline NodeType getNodeType() const override { return AST_REG_REF; }
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

			virtual inline NodeType getNodeType() const override { return AST_LVAR_REF; }
		};

		class ArgRefNode final : public AstNode {
		public:
			uint32_t index;
			bool unwrapData;

			inline ArgRefNode(uint32_t index, bool unwrapData = false) : index(index), unwrapData(unwrapData) {}
			virtual ~ArgRefNode() = default;

			virtual inline Location getLocation() const override {
				throw std::logic_error("Should not get location of a label reference");
			}

			virtual inline NodeType getNodeType() const override { return AST_ARG_REF; }
		};
	}
}

#endif
