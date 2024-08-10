#ifndef _SLKC_SERVER_SERVER_H_
#define _SLKC_SERVER_SERVER_H_

#include "../compiler/compiler.h"
#include <mutex>
#include <httplib.h>
#include <json/json.h>

namespace slake {
	namespace slkc {
		enum class ClientMarkupType {
			PlainText = 0,
			Markdown
		};

		enum class CompletionItemType {
			Var = 0,	   // Variable
			LocalVar,	   // Local variable
			Param,		   // Parameter
			Fn,			   // Function
			Type,		   // Type
			GenericParam,  // Generic parameter
			Class,		   // Class
			Interface,	   // Interface
			Module,		   // Module
			Enum,		   // Enumeration
			EnumConst,	   // Enumeration constant
			File,		   // File
			Keyword		   // Keyword
		};

		struct CompletionItem {
			CompletionItemType type;

			std::string label;
			std::string details;
			std::string documentations;
			std::string insertText;

			bool deprecated;
		};

		struct SemanticToken {
			SourcePosition position;
			unsigned int length;
			SemanticType type;
			std::set<SemanticTokenModifier> modifiers;
		};

		struct Document {
			std::string uri;
			std::string languageId;
			std::string content;
			ClientMarkupType markupType = ClientMarkupType::PlainText;
			std::mutex mutex;

			std::shared_ptr<Compiler> compiler;

			void _walkForCompletion(
				Scope *scope,
				std::unordered_map<std::string, MemberNode *> &membersOut,
				std::set<Scope*> &walkedScopes,
				bool isTopLevelRef,
				bool isStatic);
			void _walkForCompletion(
				AstNode *m,
				std::unordered_map<std::string, MemberNode *> &membersOut,
				std::set<Scope *> &walkedScopes,
				bool isStatic);
			std::unordered_map<std::string, MemberNode *> _walkForCompletion(Scope *scope, bool isTopLevelRef, bool isStatic);

			CompletionItemType _toCompletionItemType(NodeType nodeType);
			void _getCompletionItems(
				std::unordered_map<std::string, MemberNode *> &membersOut,
				std::deque<CompletionItem> &completionItems,
				const std::set<NodeType> &targetNodeTypes);
			std::deque<CompletionItem> getCompletionItems(SourcePosition location);

			void _getImportCompletionItems(
				std::string path,
				std::deque<CompletionItem> &completionItems);

			Json::Value extractTypeName(std::shared_ptr<TypeNameNode> typeName);
			Json::Value extractDeclaration(std::shared_ptr<VarNode> m);
			Json::Value extractDeclaration(std::shared_ptr<ParamNode> m);
			Json::Value extractDeclaration(std::shared_ptr<LocalVarNode> m);
			Json::Value extractDeclaration(std::shared_ptr<FnOverloadingNode> m);
			Json::Value extractDeclaration(std::shared_ptr<GenericParamNode> m);
			Json::Value extractDeclaration(std::shared_ptr<ClassNode> m);
			Json::Value extractDeclaration(std::shared_ptr<InterfaceNode> m);
			Json::Value extractDeclaration(std::shared_ptr<ModuleNode> m);
		};

		enum class DeclarationKind {
			Property = 0,
			Var,
			Param,
			LocalVar,
			FnOverloading,
			GenericParam,
			Class,
			Interface,
			Module
		};

		enum class HoverResponseKind {
			None = 0,
			Declaration
		};

		enum class RequestType {
			DocumentOpen = 0,
			DocumentUpdate,
			DocumentClose,
			Completion,
			SemanticTokens,
			Hover
		};

		enum class ResponseType {
			DocumentOk = 0,
			DocumentError,
			Completion,
			SemanticTokens,
			Hover
		};

		class Server {
		public:
			httplib::Server server;

			std::unordered_map<std::string, std::shared_ptr<Document>> openedDocuments;
			std::deque<std::string> modulePaths;

			Server();

			static bool jsonToLocation(const Json::Value &value, SourceLocation &locationOut);
			static bool jsonToPosition(const Json::Value &value, SourcePosition &positionOut);
			static Json::Value locationToJson(const SourceLocation &loc);
			static Json::Value positionToJson(const SourcePosition &pos);
			static Json::Value compilerMessageToJson(const Message &msg);
			static Json::Value completionItemToJson(const CompletionItem &item);
			static Json::Value semanticTokenToJson(const SemanticToken &loc);

			void start(uint16_t port);
		};
	}
}

#endif
