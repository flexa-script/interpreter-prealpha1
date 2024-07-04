#include "exception_handler.hpp"


void ExceptionHandler::throw_operation_type_err(const std::string op, Type ltype, Type rtype) {
	throw std::runtime_error("invalid types '" + type_str(ltype)
		+ "' and '" + type_str(rtype));
}

void ExceptionHandler::throw_operation_err(const std::string op, Type ltype, Type rtype) {
	throw std::runtime_error("invalid '" + op + "' operation ('" + type_str(ltype)
		+ "' and '" + type_str(rtype) + ")");
}

void ExceptionHandler::throw_declaration_type_err(const std::string& identifier, parser::Type ltype, parser::Type rtype) {
	throw std::runtime_error("found " + type_str(ltype)
		+ " in definition of '" + identifier
		+ "', expected " + type_str(rtype) + " type");
}

void ExceptionHandler::throw_return_type_err(const std::string& identifier, parser::Type ltype, parser::Type rtype) {
	throw std::runtime_error("invalid " + type_str(ltype)
		+ " return type for '" + identifier
		+ "' function with " + type_str(rtype)
		+ " return type");
}
