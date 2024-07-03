#include "exception_handler.hpp"


void ExceptionHandler::throw_operation_type_err(const std::string op, Type ltype, Type rtype) {
	throw std::runtime_error("invalid types '" + type_str(ltype)
		+ "' and '" + type_str(rtype));
}

void ExceptionHandler::throw_operation_err(const std::string op, Type ltype, Type rtype) {
	throw std::runtime_error("invalid '" + op + "' operation ('" + type_str(ltype)
		+ "' and '" + type_str(rtype) + ")");
}

void ExceptionHandler::throw_type_err(const std::string& identifier, parser::Type ltype, parser::Type rtype) {
	throw std::runtime_error("invalid type '" + type_str(ltype)
		+ "' trying to assign '" + identifier
		+ "' variable of '" + type_str(rtype) + "' type");
}