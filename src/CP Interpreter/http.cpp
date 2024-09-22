#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#include "http.hpp"

#include "interpreter.hpp"
#include "semantic_analysis.hpp"
#include "vendor/axeutils.hpp"

#pragma comment(lib, "ws2_32.lib")

using namespace modules;
using namespace visitor;

HTTP::HTTP() {}

HTTP::~HTTP() = default;

void HTTP::register_functions(visitor::SemanticAnalyser* visitor) {
	visitor->builtin_functions["request"] = nullptr;
}

void HTTP::register_functions(visitor::Interpreter* visitor) {

	visitor->builtin_functions["request"] = [this, visitor]() {
		Value* config_value = visitor->builtin_arguments[0];
		if (parser::is_void(config_value->type)) {
			throw std::exception("Config is null");
		}
		cp_struct config_str = config_value->get_str();
		std::string hostname = config_str["hostname"]->get_s();
		std::string path = config_str["path"]->get_s();
		std::string method = config_str["method"]->get_s();
		std::string port = "80";
		std::string headers = "";
		std::string parameters = "";
		std::string data = config_str["data"]->get_s();

		// check mandatory parameters
		if (hostname.empty()) {
			throw std::runtime_error("Hostname must be informed.");
		}
		else if (method.empty()) {
			throw std::runtime_error("Method must be informed.");
		}

		// check if has path
		if (path.empty()) {
			path = "/";
		}

		// get port
		int param_port = config_str["port"]->get_i();
		if (param_port  != 0) {
			port = std::to_string(param_port);
		}

		// build parameters
		cp_struct str_parameters = config_str["parameters"]->get_str();
		for (auto parameter : str_parameters) {
			if (parameters.empty()) {
				parameters = "?";
			}
			else {
				parameters += "&";
			}
			parameters += parameter.first + "=" + parameter.second->get_s();
		}

		// build headers
		cp_struct str_headers = config_str["headers"]->get_str();
		for (auto header : str_headers) {
			headers += header.first + ": " + header.second->get_s() + "\r\n";
		}

		WSADATA wsa;
		SOCKET sock;
		struct sockaddr_in server;
		struct addrinfo* result = NULL;
		struct addrinfo hints;

		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
			throw std::runtime_error("Failed. Error Code: " + WSAGetLastError());
		}

		// set hints to DNS resolution
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET; // IPv4
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		// resolve DNS
		if (getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result) != 0) {
			WSACleanup();
			throw std::runtime_error("Failed to resolve hostname.");
		}

		// create socket
		sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (sock == INVALID_SOCKET) {
			freeaddrinfo(result);
			WSACleanup();
			throw std::runtime_error("Socket creation failed. Error: " + WSAGetLastError());
		}

		// connect to server
		if (connect(sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
			closesocket(sock);
			freeaddrinfo(result);
			WSACleanup();
			throw std::runtime_error("Connection failed. Error: " + WSAGetLastError());
		}

		// prepare HTTP request
		std::string request = method + " " + path + parameters + " HTTP/1.1\r\n";
		request += "Host: " + hostname + "\r\n";

		// adds headers to request
		if (!headers.empty()) {
			request += headers;
		}
		else {
			request += "\r\n";
		}

		// adds body to request
		if (!data.empty()) {
			request += "Content-Length: " + std::to_string(data.length()) + "\r\n";
			request += "\r\n";
			request += data;
		}
		else {
			request += "\r\n";
		}

		// send HTTP request
		send(sock, request.c_str(), request.length(), 0);

		// receive responce
		char buffer[8192];
		int bytes_received = recv(sock, buffer, sizeof(buffer), 0);

		closesocket(sock);
		freeaddrinfo(result);
		WSACleanup();

		std::string raw_response(buffer, bytes_received);
		std::vector<std::string> response_lines = axe::StringUtils::split(raw_response, "\r\n");
		bool is_body = false;
		std::string res_body;

		// create response struct
		cp_struct res_str;
		auto status = axe::StringUtils::split(response_lines[0], ' ');
		res_str["http_version"] = new Value(cp_string(status[0]));
		res_str["status"] = new Value(cp_int(stoll(status[1])));
		res_str["status_description"] = new Value(cp_string(status[2]));
		// prepare cp instructions
		cp_struct res_headers_str;
		res_headers_str["root"] = new Value(Type::T_VOID);
		res_headers_str["size"] = new Value(cp_int(0));
		auto headers_value = new Value(res_headers_str, "Dictionary", "cp");
		auto header_identifier = new ASTIdentifierNode(std::vector<Identifier>{ Identifier("headers_value") }, "cp", 0, 0);
		auto identifier_vector = std::vector<Identifier>{ Identifier("emplace") };
		auto fcall = new ASTFunctionCallNode("cp", identifier_vector, std::vector<ASTExprNode*>(), 0, 0);

		visitor->scopes["cp"].push_back(new InterpreterScope());
		auto curr_scope = visitor->scopes["cp"].back();
		(new ASTDeclarationNode("headers_value", Type::T_STRUCT, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "Dictionary", "cp", new ASTNullNode(0, 0), false, 0, 0))->accept(visitor);
		auto var = curr_scope->find_declared_variable("headers_value");
		var->set_value(headers_value);

		for (size_t i = 1; i < response_lines.size(); ++i) {
			auto& line = response_lines[i];
			if (is_body) {
				res_body += line;
			}
			else {
				if (line == "") {
					is_body = true;
					continue;
				}

				auto header = axe::StringUtils::split(line, ": ");
				auto parameters = std::vector<ASTExprNode*> {
					header_identifier,
					new ASTLiteralNode<cp_string>(header[0], 0, 0),
					new ASTLiteralNode<cp_string>(header[1], 0, 0)
				};
				fcall->parameters = parameters;
				fcall->accept(visitor);

			}
		}

		visitor->scopes["cp"].pop_back();

		res_str["headers"] = headers_value;
		res_str["data"] = new Value(cp_string(res_body));
		res_str["raw"] = new Value(cp_string(raw_response));

		visitor->current_expression_value = new Value(res_str, "HttpResponse", "cp");

		};

}
