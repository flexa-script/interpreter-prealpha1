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
	};
}

#endif // !HTTP_HPP
