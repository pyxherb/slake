#include "server.h"
#include "../compiler/compiler.h"
#include <slake/util/stream.hh>

using namespace slake::slkc;

slake::slkc::Server::Server() {
	server.Post("/documentOpen", [this](const httplib::Request &request, httplib::Response &response) {
		puts("/documentOpen");

		Json::Value rootValue;

		Json::Reader reader;

		std::string uri;
		std::string content;
		std::string languageId;
		ClientMarkupType clientMarkupType;

		if (!reader.parse(request.body, rootValue)) {
			response.body = "Error parsing request";
			goto badRequest;
		}

		if (!rootValue.isMember("uri")) {
			response.body = "Missing document URI";
			goto badRequest;
		}
		if (!rootValue["uri"].isString()) {
			response.body = "Invalid document URI";
			goto badRequest;
		}
		uri = rootValue["uri"].asString();

		if (!rootValue.isMember("content")) {
			response.body = "Missing content";
			goto badRequest;
		}
		if (!rootValue["content"].isString()) {
			response.body = "Invalid content";
			goto badRequest;
		}
		content = rootValue["content"].asString();

		if (!rootValue.isMember("languageId")) {
			response.body = "Missing language ID";
			goto badRequest;
		}
		if (!rootValue["languageId"].isString()) {
			response.body = "Invalid language ID";
			goto badRequest;
		}
		languageId = rootValue["languageId"].asString();

		if (!rootValue.isMember("markupType")) {
			response.body = "Missing markup type";
			goto badRequest;
		}
		if (!rootValue["markupType"].isUInt()) {
			response.body = "Invalid markup type";
			goto badRequest;
		}
		clientMarkupType = (ClientMarkupType)rootValue["markupType"].asUInt();

		switch (clientMarkupType) {
			case ClientMarkupType::PlainText:
			case ClientMarkupType::Markdown:
				break;
			default:
				response.body = "Invalid markup kind";
				goto badRequest;
		}

		{
			std::shared_ptr<Document> doc = std::make_shared<Document>();
			std::lock_guard<std::mutex> docMutexGuard(doc->mutex);

			doc->uri = uri;
			doc->languageId = languageId;
			doc->compiler = std::make_shared<Compiler>();

			openedDocuments[uri] = doc;

			doc->compiler->modulePaths = modulePaths;

			try {
				std::stringstream ss(content);
				util::PseudoOutputStream pseudoOs;
				doc->compiler->compile(ss, pseudoOs);
			} catch (FatalCompilationError e) {
				doc->compiler->messages.push_back(e.message);
			}

			Json::Value responseValue;

			responseValue["type"] = (uint32_t)ResponseType::DocumentOk;

			Json::Value &responseBodyValue = responseValue["body"],
						&compilerMessagesValue = responseBodyValue["compilerMessages"];

			responseBodyValue["uri"] = uri;
			for (auto &i : doc->compiler->messages)
				compilerMessagesValue.insert(Json::Value::ArrayIndex(0), compilerMessageToJson(i));

			doc->compiler->messages.clear();

			response.body = responseValue.toStyledString();
		}
		return;

	badRequest:
		response.status = httplib::BadRequest_400;
	});
	server.Put("/documentUpdate", [this](const httplib::Request &request, httplib::Response &response) {
		puts("/documentUpdate");

		Json::Value rootValue;

		Json::Reader reader;

		std::string uri;
		std::string content;

		if (!reader.parse(request.body, rootValue)) {
			response.body = "Error parsing request";
			goto badRequest;
		}

		if (!rootValue.isMember("uri")) {
			response.body = "Missing document URI";
			goto badRequest;
		}
		if (!rootValue["uri"].isString()) {
			response.body = "Invalid document URI";
			goto badRequest;
		}
		uri = rootValue["uri"].asString();

		if (!rootValue.isMember("content")) {
			response.body = "Missing content";
			goto badRequest;
		}
		if (!rootValue["content"].isString()) {
			response.body = "Invalid content";
			goto badRequest;
		}
		content = rootValue["content"].asString();

		{
			if (auto it = openedDocuments.find(uri); it != openedDocuments.end()) {
				std::lock_guard<std::mutex> docMutexGuard(it->second->mutex);
				std::shared_ptr<Document> doc = it->second;

				doc->compiler->reset();

				doc->compiler->modulePaths = modulePaths;

				try {
					std::stringstream ss(content);
					util::PseudoOutputStream pseudoOs;
					doc->compiler->compile(ss, pseudoOs);
				} catch (FatalCompilationError e) {
					doc->compiler->messages.push_back(e.message);
				}

				Json::Value responseValue;

				responseValue["type"] = (uint32_t)ResponseType::DocumentOk;

				Json::Value &responseBodyValue = responseValue["body"],
							&compilerMessagesValue = responseBodyValue["compilerMessages"];

				responseBodyValue["uri"] = uri;
				for (auto &i : doc->compiler->messages)
					compilerMessagesValue.insert(Json::Value::ArrayIndex(0), compilerMessageToJson(i));

				doc->compiler->messages.clear();

				response.body = responseValue.toStyledString();
			} else {
				response.body = "No such opened file";
				goto notFound;
			}
		}
		return;

	badRequest:
		response.status = httplib::BadRequest_400;
		return;

	notFound:
		response.status = httplib::NotFound_404;
		return;
	});
	server.Post("/documentClose", [this](const httplib::Request &request, httplib::Response &response) {
		puts("/documentClose");

		Json::Value rootValue;

		Json::Reader reader;

		std::string uri;

		if (!reader.parse(request.body, rootValue)) {
			response.body = "Error parsing request";
			goto badRequest;
		}

		if (!rootValue.isMember("uri")) {
			response.body = "Missing document URI";
			goto badRequest;
		}
		if (!rootValue["uri"].isString()) {
			response.body = "Invalid document URI";
			goto badRequest;
		}
		uri = rootValue["uri"].asString();

		{
			if (auto it = openedDocuments.find(uri); it != openedDocuments.end()) {
				std::lock_guard<std::mutex> docMutexGuard(it->second->mutex);
				openedDocuments.erase(it);

				Json::Value responseValue;

				responseValue["type"] = (uint32_t)ResponseType::DocumentOk;

				Json::Value &responseBodyValue = responseValue["body"];

				responseBodyValue["uri"] = uri;

				response.body = responseValue.toStyledString();
			} else {
				response.body = "No such opened file";
				goto notFound;
			}
		}
		return;

	badRequest:
		response.status = httplib::BadRequest_400;
		return;

	notFound:
		response.status = httplib::NotFound_404;
		return;
	});
	server.Post("/completion", [this](const httplib::Request &request, httplib::Response &response) {
		puts("/completion");

		Json::Value rootValue;

		Json::Reader reader;

		std::string uri;
		Location loc;

		if (!reader.parse(request.body, rootValue)) {
			response.body = "Error parsing request";
			goto badRequest;
		}

		if (!rootValue.isMember("uri")) {
			response.body = "Missing document URI";
			goto badRequest;
		}
		if (!rootValue["uri"].isString()) {
			response.body = "Invalid document URI";
			goto badRequest;
		}
		uri = rootValue["uri"].asString();

		if (!rootValue.isMember("location")) {
			response.body = "Missing document location";
			goto badRequest;
		}
		if (!jsonToLocation(rootValue["location"], loc)) {
			response.body = "Invalid document location";
			goto badRequest;
		}

		{
			if (auto it = openedDocuments.find(uri); it != openedDocuments.end()) {
				std::lock_guard<std::mutex> docMutexGuard(it->second->mutex);
				std::shared_ptr<Document> doc = it->second;

				Json::Value responseValue;

				responseValue["type"] = (uint32_t)ResponseType::Completion;

				Json::Value &responseBodyValue = responseValue["body"],
							&completionItemsValue = responseBodyValue["completionItems"];

				auto completionItems = doc->getCompletionItems(loc);

				responseBodyValue["uri"] = uri;
				for (auto &i : completionItems)
					completionItemsValue.insert(Json::Value::ArrayIndex(0), completionItemToJson(i));

				doc->compiler->messages.clear();

				response.body = responseValue.toStyledString();
			} else {
				response.body = "No such opened file";
				goto notFound;
			}
		}
		return;

	badRequest:
		response.status = httplib::BadRequest_400;
		return;

	notFound:
		response.status = httplib::NotFound_404;
		return;
	});
	server.Post("/semanticTokens", [this](const httplib::Request &request, httplib::Response &response) {
		puts("/semanticTokens");

		Json::Value rootValue;

		Json::Reader reader;

		std::string uri;
		Location loc;

		if (!reader.parse(request.body, rootValue)) {
			response.body = "Error parsing request";
			goto badRequest;
		}

		if (!rootValue.isMember("uri")) {
			response.body = "Missing document URI";
			goto badRequest;
		}
		if (!rootValue["uri"].isString()) {
			response.body = "Invalid document URI";
			goto badRequest;
		}
		uri = rootValue["uri"].asString();

		{
			if (auto it = openedDocuments.find(uri); it != openedDocuments.end()) {
				std::lock_guard<std::mutex> docMutexGuard(it->second->mutex);
				std::shared_ptr<Document> doc = it->second;

				Json::Value responseValue;

				responseValue["type"] = (uint32_t)ResponseType::SemanticTokens;

				Json::Value &responseBodyValue = responseValue["body"],
							&semanticTokensValue = responseBodyValue["semanticTokens"];

				responseBodyValue["uri"] = uri;

				const size_t nTokens = doc->compiler->lexer->tokens.size();
				for (size_t i = 0; i < nTokens; ++i) {
					const Token &token = doc->compiler->lexer->tokens[i];
					const TokenInfo &tokenInfo = doc->compiler->tokenInfos[i];

					SemanticToken semanticToken = {};

					semanticToken.location = token.beginLocation;
					semanticToken.length = (unsigned int)token.text.size();
					semanticToken.type = tokenInfo.semanticType;
					semanticToken.modifiers = tokenInfo.semanticModifiers;

					semanticTokensValue.append(semanticTokenToJson(semanticToken));
				}

				response.body = responseValue.toStyledString();
			} else {
				response.body = "No such opened file";
				goto notFound;
			}
		}
		return;

	badRequest:
		response.status = httplib::BadRequest_400;
		return;

	notFound:
		response.status = httplib::NotFound_404;
		return;
	});
	server.Post("/hover", [this](const httplib::Request &request, httplib::Response &response) {
		puts("/hover");

		Json::Value rootValue;

		Json::Reader reader;

		std::string uri;
		Location loc;

		if (!reader.parse(request.body, rootValue)) {
			response.body = "Error parsing request";
			goto badRequest;
		}

		if (!rootValue.isMember("uri")) {
			response.body = "Missing document URI";
			goto badRequest;
		}
		if (!rootValue["uri"].isString()) {
			response.body = "Invalid document URI";
			goto badRequest;
		}
		uri = rootValue["uri"].asString();

		if (!rootValue.isMember("location")) {
			response.body = "Missing document location";
			goto badRequest;
		}
		if (!jsonToLocation(rootValue["location"], loc)) {
			response.body = "Invalid document location";
			goto badRequest;
		}

		{
			if (auto it = openedDocuments.find(uri); it != openedDocuments.end()) {
				std::lock_guard<std::mutex> docMutexGuard(it->second->mutex);
				std::shared_ptr<Document> doc = it->second;

				Json::Value responseValue;

				responseValue["type"] = (uint32_t)ResponseType::Hover;

				Json::Value &responseBodyValue = responseValue["body"];

				size_t idxToken = doc->compiler->lexer->getTokenByLocation(loc);

				if (idxToken == SIZE_MAX)
					goto badRequest;

				TokenInfo &tokenInfo = doc->compiler->tokenInfos[idxToken];

				responseBodyValue["uri"] = uri;

				if (tokenInfo.semanticInfo.correspondingMember) {
					auto &member = tokenInfo.semanticInfo.correspondingMember;

					switch (member->getNodeType()) {
						case NodeType::Var:
							responseBodyValue["content"] = std::to_string(doc->compiler->getFullName((VarNode *)member.get()), doc->compiler.get());
							break;
						case NodeType::Param:
							responseBodyValue["content"] = "(Parameter)";
							break;
						case NodeType::LocalVar:
							responseBodyValue["content"] = "(Local variable)";
							break;
						case NodeType::FnOverloading: {
							auto m = std::static_pointer_cast<FnOverloadingNode>(member);

							std::string fullName;

							fullName += std::to_string(m->returnType ? m->returnType : std::make_shared<VoidTypeNameNode>(Location(), SIZE_MAX), doc->compiler.get());
							fullName += " ";
							fullName += std::to_string(doc->compiler->getFullName(m->owner), doc->compiler.get());

							fullName += "(";

							for (size_t i = 0; i < m->params.size(); ++i) {
								if (i)
									fullName += ",";
								if (m->params[i]->name == "...") {
									fullName += "...";
								} else {
									fullName += std::to_string(m->params[i]->type, doc->compiler.get());
									fullName += " ";
									fullName += m->params[i]->name;
								}
							}

							fullName += ")";

							responseBodyValue["content"] = fullName;
							break;
						}
						case NodeType::GenericParam:
							responseBodyValue["content"] = "(Generic parameter) " + ((GenericParamNode *)member.get())->name;
							break;
						case NodeType::Class:
							responseBodyValue["content"] = std::to_string(doc->compiler->getFullName((ClassNode *)member.get()), doc->compiler.get());
							break;
						case NodeType::Interface:
							responseBodyValue["content"] = std::to_string(doc->compiler->getFullName((InterfaceNode *)member.get()), doc->compiler.get());
							break;
						case NodeType::Trait:
							responseBodyValue["content"] = std::to_string(doc->compiler->getFullName((TraitNode *)member.get()), doc->compiler.get());
							break;
						case NodeType::Module:
							responseBodyValue["content"] = std::to_string(doc->compiler->getFullName((ModuleNode *)member.get()), doc->compiler.get());
							break;
					}
				}

				response.body = responseValue.toStyledString();
			} else {
				response.body = "No such opened file";
				goto notFound;
			}
		}
		return;

	badRequest:
		response.status = httplib::BadRequest_400;
		return;

	notFound:
		response.status = httplib::NotFound_404;
		return;
	});
	server.Get("/stop", [this](const httplib::Request &request, httplib::Response &response) {
		server.stop();
		response.status = httplib::OK_200;
		response.body = "OK";
	});
}

bool slake::slkc::Server::jsonToLocation(const Json::Value &value, Location &locationOut) {
	if (!value.isMember("line"))
		return false;
	if (!value["line"].isUInt())
		return false;
	locationOut.line = value["line"].asUInt();

	if (!value.isMember("column"))
		return false;
	if (!value["column"].isUInt())
		return false;
	locationOut.column = value["column"].asUInt();

	return true;
}

Json::Value slake::slkc::Server::locationToJson(const Location &loc) {
	Json::Value value;

	value["line"] = loc.line;
	value["column"] = loc.column;

	return value;
}

Json::Value slake::slkc::Server::compilerMessageToJson(const Message &msg) {
	Json::Value value;

	value["location"] = locationToJson(msg.loc);
	value["type"] = (int)msg.type;
	value["message"] = msg.msg;

	return value;
}

Json::Value slake::slkc::Server::completionItemToJson(const CompletionItem &item) {
	Json::Value value;

	value["type"] = (int)item.type;

	value["label"] = item.label;
	value["details"] = item.details;
	value["documentations"] = item.documentations;
	value["insertText"] = item.insertText;

	value["deprecated"] = item.deprecated;

	return value;
}

Json::Value slake::slkc::Server::semanticTokenToJson(const SemanticToken &item) {
	Json::Value value;

	value["type"] = (int)item.type;

	auto &modifiersValue = value["modifiers"];
	for (auto i : item.modifiers) {
		Json::Value modifierValue = (int)i;

		modifiersValue.insert(Json::Value::ArrayIndex(0), modifierValue);
	}

	value["location"] = locationToJson(item.location);
	value["length"] = item.length;

	return value;
}

void slake::slkc::Server::start(uint16_t port) {
	if (!server.listen("0.0.0.0", port))
		throw std::runtime_error("Error binding specified port");
}
