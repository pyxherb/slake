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
		SourcePosition pos;

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

		if (!rootValue.isMember("position")) {
			response.body = "Missing document position";
			goto badRequest;
		}
		if (!jsonToPosition(rootValue["position"], pos)) {
			response.body = "Invalid document position";
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

				auto completionItems = doc->getCompletionItems(pos);

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
					Token *token = doc->compiler->lexer->tokens[i].get();
					const TokenInfo &tokenInfo = doc->compiler->tokenInfos[i];

					SemanticToken semanticToken = {};

					semanticToken.position = token->location.beginPosition;
					semanticToken.length = (unsigned int)token->text.size();
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
		SourcePosition pos;

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

		if (!rootValue.isMember("position")) {
			response.body = "Missing document position";
			goto badRequest;
		}
		if (!jsonToPosition(rootValue["position"], pos)) {
			response.body = "Invalid document position";
			goto badRequest;
		}

		{
			if (auto it = openedDocuments.find(uri); it != openedDocuments.end()) {
				std::lock_guard<std::mutex> docMutexGuard(it->second->mutex);
				std::shared_ptr<Document> doc = it->second;

				Json::Value responseValue;

				responseValue["type"] = (uint32_t)ResponseType::Hover;

				Json::Value &responseBodyValue = responseValue["body"];

				size_t idxToken = doc->compiler->lexer->getTokenByPosition(pos);

				if (idxToken == SIZE_MAX)
					goto badRequest;

				TokenInfo &tokenInfo = doc->compiler->tokenInfos[idxToken];

				responseBodyValue["uri"] = uri;

				responseBodyValue["responseKind"] = (uint32_t)HoverResponseKind::None;

				if (tokenInfo.semanticInfo.correspondingMember) {
					auto &member = tokenInfo.semanticInfo.correspondingMember;

					responseBodyValue["responseKind"] = (uint32_t)HoverResponseKind::Declaration;

					auto &contentsValue = responseBodyValue["contents"];

					switch (member->getNodeType()) {
						case NodeType::Var: {
							auto m = std::static_pointer_cast<VarNode>(member);

							contentsValue = doc->extractDeclaration(m);
							break;
						}
						case NodeType::Param: {
							auto m = std::static_pointer_cast<ParamNode>(member);

							contentsValue = doc->extractDeclaration(m);
							break;
						}
						case NodeType::LocalVar: {
							auto m = std::static_pointer_cast<LocalVarNode>(member);

							contentsValue = doc->extractDeclaration(m);
							break;
						}
						case NodeType::FnOverloadingValue: {
							auto m = std::static_pointer_cast<FnOverloadingNode>(member);

							contentsValue = doc->extractDeclaration(m);
							break;
						}
						case NodeType::GenericParam: {
							auto m = std::static_pointer_cast<GenericParamNode>(member);

							contentsValue = doc->extractDeclaration(m);
							break;
						}
						case NodeType::Class: {
							auto m = std::static_pointer_cast<ClassNode>(member);

							contentsValue = doc->extractDeclaration(m);
							break;
						}
						case NodeType::Interface: {
							auto m = std::static_pointer_cast<InterfaceNode>(member);

							contentsValue = doc->extractDeclaration(m);
							break;
						}
						case NodeType::Module: {
							auto m = std::static_pointer_cast<ModuleNode>(member);

							contentsValue = doc->extractDeclaration(m);
							break;
						}
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

bool slake::slkc::Server::jsonToPosition(const Json::Value &value, SourcePosition &positionOut) {
	if (!value.isMember("line"))
		return false;
	if (!value["line"].isUInt())
		return false;
	positionOut.line = value["line"].asUInt();

	if (!value.isMember("column"))
		return false;
	if (!value["column"].isUInt())
		return false;
	positionOut.column = value["column"].asUInt();

	return true;
}

Json::Value slake::slkc::Server::positionToJson(const SourcePosition &pos) {
	Json::Value value;

	value["line"] = pos.line;
	value["column"] = pos.column;

	return value;
}

bool slake::slkc::Server::jsonToLocation(const Json::Value &value, SourceLocation &locationOut) {
	if (!value.isMember("begin"))
		return false;
	if(!jsonToPosition(value["begin"], locationOut.beginPosition))
		return false;

	if (!value.isMember("end"))
		return false;
	if(!jsonToPosition(value["end"], locationOut.beginPosition))
		return false;

	return true;
}

Json::Value slake::slkc::Server::locationToJson(const SourceLocation &pos) {
	Json::Value value;

	value["begin"] = positionToJson(pos.beginPosition);
	value["end"] = positionToJson(pos.endPosition);

	return value;
}

Json::Value slake::slkc::Server::compilerMessageToJson(const Message &msg) {
	Json::Value value;

	value["location"] = locationToJson(msg.sourceLocation);
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

	value["position"] = positionToJson(item.position);
	value["length"] = item.length;

	return value;
}

void slake::slkc::Server::start(uint16_t port) {
	if (!server.listen("0.0.0.0", port))
		throw std::runtime_error("Error binding specified port");
}
