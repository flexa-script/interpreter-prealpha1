#include "exception_handler.hpp"


void ExceptionHandler::throw_type_err(const std::string op, Type ltype, Type rtype) {
	throw std::runtime_error("invalid types '" + type_str(ltype)
		+ "' and '" + type_str(rtype));
}

void ExceptionHandler::throw_operation_err(const std::string op, Type ltype, Type rtype) {
	throw std::runtime_error("invalid '" + op + "' operation ('" + type_str(ltype)
		+ "' and '" + type_str(rtype) + ")");
}
