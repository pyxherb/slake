#ifndef _SLKAOT_CXXAST_H_
#define _SLKAOT_CXXAST_H_

#include <slake/runtime.h>
#include <string>
#include <map>
#include <unordered_map>

namespace slake {
	namespace slkaot {
		namespace cxxast {
			class AbstractModule;

			enum class NodeKind : uint8_t {
				FnOverloading,
				Fn,
				Struct,
				Class,
				Namespace
			};

			class ASTNode : std::enable_shared_from_this<ASTNode> {
			public:
				NodeKind nodeKind;

				ASTNode(NodeKind nodeKind);
				virtual ~ASTNode();
			};

			class AbstractMember : public ASTNode {
			public:
				std::weak_ptr<AbstractModule> parent;
				std::string name;

				AbstractMember(NodeKind nodeKind, std::string &&name);
				virtual ~AbstractMember();
			};

			class AbstractModule : public AbstractMember {
			public:
				std::string name;
				std::unordered_map<std::string, std::shared_ptr<AbstractMember>> publicMembers;
				std::unordered_map<std::string, std::shared_ptr<AbstractMember>> protectedMembers;

				AbstractModule(NodeKind nodeKind, std::string &&name);

				void addPublicMember(std::string &&name, std::shared_ptr<AbstractMember> memberNode);
				void addProtectedMember(std::string &&name, std::shared_ptr<AbstractMember> memberNode);
				std::shared_ptr<AbstractMember> getMember();
				void removeMember(const std::string &name);
			};

			using ParamList = std::vector<Type>;

			struct ParamListComparator {
				bool operator()(const ParamList &lhs, const ParamList &rhs);
			};

			struct FnOverloadingProperties {
				bool isStatic: 1;
				bool isOverriden: 1;
				bool isVirtual: 1;
			};

			class FnOverloading : public ASTNode {
			public:
				ParamList paramTypes;
				FnOverloadingProperties properties;

				FnOverloading();
				virtual ~FnOverloading();
			};

			class Fn : public AbstractMember {
			public:
				std::map<ParamList, std::shared_ptr<FnOverloading>, ParamListComparator> overloadings;

				Fn(std::string &&name);
				virtual ~Fn();
			};

			class Struct : public AbstractModule {
			public:
				Struct(std::string &&name);
				virtual ~Struct();
			};

			class Class : public AbstractModule {
			public:
				Class(std::string &&name);
				virtual ~Class();
			};

			class Namespace : public AbstractModule {
			public:
				Namespace(std::string &&name);
				virtual ~Namespace();
			};
		}
	}
}

#endif
