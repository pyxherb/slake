#include "server.h"
#include "../compiler/compiler.h"
#include <slake/util/stream.hh>

using namespace slake::slkc;

slake::slkc::Server::Server() {
	server.Post("/documentOpen", [this](const httplib::Request &request, httplib::Response &response) {
		puts("/documentOpen");

		Json::Value rootValue;

		Json::Reader reader;

		string uri;
		string languageId;
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
			std::lock_guard<mutex> docMutexGuard(doc->mutex);

			doc->uri = uri;
			doc->languageId = languageId;
			doc->compiler = make_shared<Compiler>();

			openedDocuments[uri] = doc;

			Json::Value responseValue;

			responseValue["type"] = (uint32_t)ResponseType::DocumentOk;

			Json::Value &responseBodyValue = responseValue["body"];

			responseBodyValue["uri"] = uri;

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

		string uri;
		string content;

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

		if (auto it = openedDocuments.find(uri); it != openedDocuments.end()) {
			std::shared_ptr<Document> doc = it->second;
			std::lock_guard<mutex> docMutexGuard(doc->mutex);

			openedDocuments[uri] = doc;

			doc->compiler->reset();
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

		string uri;

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

		if (auto it = openedDocuments.find(uri); it != openedDocuments.end()) {
			std::lock_guard<mutex> docMutexGuard(it->second->mutex);
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

		string uri;
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

		if (auto it = openedDocuments.find(uri); it != openedDocuments.end()) {
			std::shared_ptr<Document> doc = it->second;
			std::lock_guard<mutex> docMutexGuard(doc->mutex);

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
		return;

	badRequest:
		response.status = httplib::BadRequest_400;
		return;

	notFound:
		response.status = httplib::NotFound_404;
		return;
	});
	server.Post("/stop", [this](const httplib::Request &request, httplib::Response &response) {
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

void slake::slkc::Server::start(uint16_t port) {
	if (!server.listen("0.0.0.0", port))
		throw std::runtime_error("Error binding specified port");
}
