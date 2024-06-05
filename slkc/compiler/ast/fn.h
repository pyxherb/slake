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

		class ParamNode : public AstNode {
		private:
			virtual std::shared_ptr<AstNode> doDuplicate() override;

		public:
			Location loc;

			std::shared_ptr<TypeNameNode> type;
			std::string name;

			// The original type will be saved during generic instantiation.
			std::shared_ptr<TypeNameNode> originalType;

			size_t idxNameToken = SIZE_MAX,
				   idxColonToken = SIZE_MAX;

			inline ParamNode(const ParamNode &other) : AstNode(other) {
				loc = other.loc;

				if (other.type)
					type = other.type->duplicate<TypeNameNode>();
				name = other.name;

				if (originalType)
					originalType = other.originalType->duplicate<TypeNameNode>();

				idxNameToken = other.idxNameToken;
				idxColonToken = other.idxColonToken;
			}
			inline ParamNode(Location loc) : loc(loc) {}
			virtual ~ParamNode() = default;

			virtual inline Location getLocation() const override { return loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Param; }
		};

		class FnOverloadingNode final : public MemberNode {
		private:
			virtual std::shared_ptr<AstNode> doDuplicate() override;

		public:
			Location loc;

			std::shared_ptr<TypeNameNode> returnType;

			/// @brief Actual parameters. DO NOT forget to update the parameter indices after you updated the parameters.
			std::deque<std::shared_ptr<ParamNode>> params;
			/// @brief Parameter indices. Note that it needs to be updated manually after you updated the parameters.
			std::unordered_map<std::string, size_t> paramIndices;

			std::shared_ptr<BlockStmtNode> body;
			FnNode *owner = nullptr;
			AccessModifier access = 0;

			bool isAsync = false,
				 isVirtual = false;

			size_t idxNameToken = SIZE_MAX,
				   idxParamLParentheseToken = SIZE_MAX,
				   idxParamRParentheseToken = SIZE_MAX,
				   idxAsyncModifierToken = SIZE_MAX,
				   idxReturnTypeColonToken = SIZE_MAX,
				   idxVirtualModifierToken = SIZE_MAX;
			std::deque<size_t> idxParamCommaTokens;

			inline FnOverloadingNode(const FnOverloadingNode &other) : MemberNode(other) {
				loc = other.loc;

				if (other.returnType)
					returnType = other.returnType->duplicate<TypeNameNode>();

				params.resize(other.params.size());
				for (size_t i = 0; i < other.params.size(); ++i) {
					params[i] = other.params[i]->duplicate<ParamNode>();
				}
				paramIndices = other.paramIndices;

				body = other.body;
				owner = other.owner;
				access = other.access;

				isAsync = other.isAsync;
				isVirtual = other.isVirtual;

				idxNameToken = other.idxNameToken;
				idxParamLParentheseToken = other.idxParamLParentheseToken;
				idxParamRParentheseToken = other.idxParamRParentheseToken;
				idxAsyncModifierToken = other.idxAsyncModifierToken;
				idxReturnTypeColonToken = other.idxReturnTypeColonToken;
				idxVirtualModifierToken = other.idxVirtualModifierToken;
				idxParamCommaTokens = other.idxParamCommaTokens;
			}

			FnOverloadingNode(
				Location loc,
				Compiler *compiler,
				std::shared_ptr<Scope> scope);

			virtual inline NodeType getNodeType() const override { return NodeType::FnOverloadingValue; }

			virtual IdRefEntry getName() const override;
			virtual Location getLocation() const override { return loc; }

			inline bool isAbstract() { return !body; }
			inline bool isVaridic() { return paramIndices.count("..."); }

			/// @brief Update parameter indices.
			void updateParamIndices();
		};

		class FnNode final : public MemberNode {
		private:
			virtual std::shared_ptr<AstNode> doDuplicate() override;

		public:
			FnNode *parentFn = nullptr;

			std::deque<std::shared_ptr<FnOverloadingNode>> overloadingRegistries;
			std::string name;

			inline FnNode(const FnNode &other) : MemberNode(other) {
				// We don't copy parentFn because it has to be linked to a inherited function dynamically LOL

				overloadingRegistries.resize(other.overloadingRegistries.size());
				for (size_t i = 0; i < other.overloadingRegistries.size(); ++i) {
					overloadingRegistries[i] = other.overloadingRegistries[i]->duplicate<FnOverloadingNode>();
					overloadingRegistries[i]->owner = this;
				}
				name = other.name;
			}
			inline FnNode(
				Compiler *compiler,
				std::string name)
				: name(name), MemberNode(compiler, ACCESS_PUB) {}
			virtual ~FnNode() = default;

			virtual inline Location getLocation() const override {
				throw std::logic_error("Please get locations from the overloading registries");
			}

			virtual inline NodeType getNodeType() const override { return NodeType::Fn; }

			virtual IdRefEntry getName() const override {
				throw std::logic_error("Please get name from overloading registries");
			}
		};

		struct Ins {
			Opcode opcode;
			std::deque<std::shared_ptr<AstNode>> operands;

			Ins() = default;
			inline Ins(
				Opcode opcode,
				std::deque<std::shared_ptr<AstNode>> operands = {})
				: opcode(opcode), operands(operands) {}
		};

		class CompiledFnNode final : public MemberNode {
		private:
			Location _loc;
			virtual std::shared_ptr<AstNode> doDuplicate() override;

		public:
			std::string name;
			std::deque<Ins> body;
			std::unordered_map<std::string, uint32_t> labels;

			GenericParamNodeList genericParams;
			std::unordered_map<std::string, size_t> genericParamIndices;

			std::deque<std::shared_ptr<ParamNode>> params;
			std::unordered_map<std::string, size_t> paramIndices;

			std::shared_ptr<TypeNameNode> returnType;

			std::deque<slxfmt::SourceLocDesc> srcLocDescs;

			bool isAsync = false;

			inline CompiledFnNode(const CompiledFnNode &other) {
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
			inline CompiledFnNode(Location loc, std::string name) : _loc(loc), name(name) {}
			virtual ~CompiledFnNode() = default;

			virtual inline Location getLocation() const override {
				return _loc;
			}

			virtual inline NodeType getNodeType() const override { return NodeType::Fn; }

			inline void insertIns(Ins ins) { body.push_back(ins); }
			inline void insertIns(
				Opcode opcode,
				std::shared_ptr<AstNode> op1) {
				insertIns({ opcode, { op1 } });
			}
			inline void insertIns(
				Opcode opcode,
				std::shared_ptr<AstNode> op1,
				std::shared_ptr<AstNode> op2) {
				insertIns({ opcode, { op1, op2 } });
			}
			inline void insertIns(
				Opcode opcode,
				std::shared_ptr<AstNode> op1,
				std::shared_ptr<AstNode> op2,
				std::shared_ptr<AstNode> op3) {
				insertIns({ opcode, { op1, op2, op3 } });
			}
			inline void insertIns(
				Opcode opcode,
				std::shared_ptr<AstNode> op1,
				std::shared_ptr<AstNode> op2,
				std::shared_ptr<AstNode> op3,
				std::shared_ptr<AstNode> op4) {
				insertIns({ opcode, { op1, op2, op3, op4 } });
			}
			inline void insertLabel(std::string name) { labels[name] = (uint32_t)body.size(); }

			virtual IdRefEntry getName() const override {
				if (genericArgs.size())
					return IdRefEntry(_loc, SIZE_MAX, name, genericArgs);
				return IdRefEntry(_loc, SIZE_MAX, name, getPlaceholderGenericArgs());
			}
		};

		class LabelRefNode final : public AstNode {
		public:
			std::string label;

			inline LabelRefNode(std::string label) : label(label) {}
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
	}
}

#endif
