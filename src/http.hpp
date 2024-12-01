#ifndef HTTP_HPP
#define HTTP_HPP

#include "module.hpp"

namespace modules {
	class HTTP : public Module {
	public:
		HTTP();
		~HTTP();

		void register_functions(visitor::SemanticAnalyser* visitor) override;
		void register_functions(visitor::Interpreter* visitor) override;
		void register_functions(visitor::Compiler* visitor) override;
		void register_functions(VirtualMachine* vm) override;
	};
}

#endif // !HTTP_HPP
