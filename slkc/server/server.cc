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
	server.Get("/completion", [this](const httplib::Request &request, httplib::Response &response) {

	});
	server.Post("/stop", [this](const httplib::Request &request, httplib::Response &response) {
		server.stop();
		response.status = httplib::OK_200;
		response.body = "OK";
	});
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

void slake::slkc::Server::start(uint16_t port) {
	if (!server.listen("0.0.0.0", port))
		throw std::runtime_error("Error binding specified port");
}
