#ifndef EXCEPTION_HANDLER_HPP
#define EXCEPTION_HANDLER_HPP

#include <string>

#include "visitor.hpp"

class ExceptionHandler {
public:
	static void throw_operation_err(const std::string op, parser::Type ltype, parser::Type rtype);
	static void throw_operation_type_err(const std::string op, parser::Type ltype, parser::Type rtype);
	static void throw_declaration_type_err(const std::string& identifier, parser::Type ltype, parser::Type rtype);
	static void throw_return_type_err(const std::string& identifier, parser::Type ltype, parser::Type rtype);
	static void throw_mismatched_type_err(parser::Type ltype, parser::Type rtype);
	static void throw_condition_type_err();
	static void throw_struct_type_err(const std::string& type_name, parser::Type type);
	static void throw_struct_member_err(const std::string& type_name, const std::string& variable);
};

#endif // !EXCEPTION_HANDLER_HPP
