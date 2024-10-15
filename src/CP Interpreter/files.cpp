#include "files.hpp"

#include "interpreter.hpp"
#include "semantic_analysis.hpp"

using namespace modules;
using namespace visitor;

Files::Files() {}

Files::~Files() = default;

void Files::register_functions(visitor::SemanticAnalyser* visitor) {
	visitor->builtin_functions["open"] = nullptr;
	visitor->builtin_functions["read"] = nullptr;
	visitor->builtin_functions["read_line"] = nullptr;
	visitor->builtin_functions["read_all_bytes"] = nullptr;
	visitor->builtin_functions["write"] = nullptr;
	visitor->builtin_functions["write_bytes"] = nullptr;
	visitor->builtin_functions["is_open"] = nullptr;
	visitor->builtin_functions["close"] = nullptr;
}

void Files::register_functions(visitor::Interpreter* visitor) {

	visitor->builtin_functions["open"] = [this, visitor]() {
		// initialize file struct values
		RuntimeValue* cpfile = new RuntimeValue(parser::Type::T_STRUCT);

		cp_struct str = cp_struct();
		str["path"] = new RuntimeValue(visitor->builtin_arguments[0]);
		str["mode"] = new RuntimeValue(visitor->builtin_arguments[1]);

		int parmode = visitor->builtin_arguments[1]->get_i();

		std::fstream* fs = nullptr;
		try {
			fs = new std::fstream(visitor->builtin_arguments[0]->get_s(), parmode);
			str[INSTANCE_ID_NAME] = new RuntimeValue(cp_int(fs));
			cpfile->set(str, "File", "cp");
			visitor->current_expression_value = cpfile;
		}
		catch (std::exception ex) {
			throw std::runtime_error(ex.what());
		}
	};

	visitor->builtin_functions["read"] = [this, visitor]() {
		RuntimeValue* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			auto rval = new RuntimeValue(parser::Type::T_STRING);
			
			std::fstream* fs = ((std::fstream*)cpfile->get_str()[INSTANCE_ID_NAME]->get_i());

			fs->seekg(0);

			std::stringstream ss;
			std::string line;
			while (std::getline(*fs, line)) {
				ss << line << std::endl;
			}
			rval->set(ss.str());

			visitor->current_expression_value = rval;
		}
	};

	visitor->builtin_functions["read_line"] = [this, visitor]() {
		RuntimeValue* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			auto rval = new RuntimeValue(parser::Type::T_STRING);

			std::fstream* fs = ((std::fstream*)cpfile->get_str()[INSTANCE_ID_NAME]->get_i());

			std::string line;
			std::getline(*fs, line);
			rval->set(line);

			visitor->current_expression_value = rval;
		}
	};

	visitor->builtin_functions["read_all_bytes"] = [this, visitor]() {
		RuntimeValue* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			auto rval = new RuntimeValue(parser::Type::T_ARRAY);
			rval->set_arr_type(parser::Type::T_CHAR);

			std::fstream* fs = ((std::fstream*)cpfile->get_str()[INSTANCE_ID_NAME]->get_i());

			fs->seekg(0);

			// find file size
			fs->seekg(0, std::ios::end);
			std::streamsize buffer_size = fs->tellg();
			fs->seekg(0, std::ios::beg);

			// buffer to store readed data
			char* buffer = new char[buffer_size];

			RuntimeValue** arr = new RuntimeValue*[buffer_size];

			// read all bytes
			if (fs->read(buffer, buffer_size)) {
				for (size_t i = 0; i < buffer_size; ++i) {
					RuntimeValue* val = new RuntimeValue(parser::Type::T_CHAR);
					val->set(buffer[i]);
					arr[i] = val;
				}
			}
			rval->set(cp_array(arr, buffer_size), Type::T_CHAR, std::vector<ASTExprNode*>());

			delete[] buffer;

			visitor->current_expression_value = rval;
		}
	};

	visitor->builtin_functions["write"] = [this, visitor]() {
		RuntimeValue* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			std::fstream* fs = ((std::fstream*)cpfile->get_str()[INSTANCE_ID_NAME]->get_i());
			*fs << visitor->builtin_arguments[1]->get_s();
		}
	};

	visitor->builtin_functions["write_bytes"] = [this, visitor]() {
		RuntimeValue* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			std::fstream* fs = ((std::fstream*)cpfile->get_str()[INSTANCE_ID_NAME]->get_i());

			auto arr = visitor->builtin_arguments[1]->get_arr();

			std::streamsize buffer_size = arr.second;

			char* buffer = new char[buffer_size];

			for (size_t i = 0; i < buffer_size; ++i) {
				buffer[i] = arr.first[i]->get_c();
			}

			fs->write(buffer, sizeof(buffer));
		}
	};

	visitor->builtin_functions["is_open"] = [this, visitor]() {
		RuntimeValue* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			auto rval = new RuntimeValue(parser::Type::T_BOOL);
			rval->set(cp_bool(((std::fstream*)cpfile->get_str()[INSTANCE_ID_NAME]->get_i())->is_open()));
			visitor->current_expression_value = rval;
		}
	};

	visitor->builtin_functions["close"] = [this, visitor]() {
		RuntimeValue* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			if (((std::fstream*)cpfile->get_str()[INSTANCE_ID_NAME]->get_i())) {
				((std::fstream*)cpfile->get_str()[INSTANCE_ID_NAME]->get_i())->close();
				((std::fstream*)cpfile->get_str()[INSTANCE_ID_NAME]->get_i())->~basic_fstream();
				cpfile->set_null();
			}
		}
	};

}
