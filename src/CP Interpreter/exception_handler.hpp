#ifndef EXCEPTION_HANDLER_HPP
#define EXCEPTION_HANDLER_HPP

#include <string>

#include "visitor.hpp"

class ExceptionHandler {
public:
	static void throw_operation_err(const std::string op, parser::Type ltype, parser::Type rtype);
	static void throw_type_err(const std::string op, parser::Type ltype, parser::Type rtype);

};

#endif // !EXCEPTION_HANDLER_HPP
