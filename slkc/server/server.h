#ifndef _SLKC_SERVER_SERVER_H_
#define _SLKC_SERVER_SERVER_H_

#include "../compiler/compiler.h"
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
			Fn,			   // Function
			Type,		   // Type
			GenericParam,  // Generic parameter
			Class,		   // Class
			Interface,	   // Interface
			Trait,		   // Trait
			Module,		   // Module
			Enum,		   // Enumeration
			EnumConst,	   // Enumeration constant
			File,		   // File
			Keyword		   // Keyword
		};

		struct CompletionItem {
			CompletionItemType type;

			string label;
			string details;
			string documentations;
			string insertText;

			bool deprecated;
		};

		struct Document {
			string uri;
			string languageId;
			string content;
			ClientMarkupType markupType;

			std::shared_ptr<Compiler> compiler;

			void getCompletionItems(
				Scope *scope,
				std::deque<CompletionItem> &completionItems,
				const std::set<NodeType> &targetNodeTypes);
			std::deque<CompletionItem> getCompletionItems(Location location);
		};

		enum class RequestType {
			DocumentOpen = 0,
			DocumentUpdate,
			DocumentClose,
			Completion
		};

		enum class ResponseType {
			DocumentOk = 0,
			DocumentError,
			Completion
		};

		struct DocumentErrorResponseBody {
			std::string uri;
			int code;
			string message;
		};

		struct CompletionResponseBody {
			std::string uri;
			std::deque<CompletionItem> completionItems;
		};

		class Server {
		public:
			httplib::Server server;

			unordered_map<string, std::shared_ptr<Document>> openedDocuments;

			Server();

			static Json::Value locationToJson(const Location &loc);
			static Json::Value compilerMessageToJson(const Message &msg);

			void start(uint16_t port);
		};
	}
}

#endif
