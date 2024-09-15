#include "files.hpp"

#include "interpreter.hpp"
#include "semantic_analysis.hpp"

using namespace modules;
using namespace visitor;

Files::Files() {}

Files::~Files() {
	for (auto& val : files) {
		delete val;
	}
	files.clear();
}

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
		Value* cpfile = visitor->builtin_arguments[0];
		cp_struct str = cp_struct();
		str["path"] = new Value(visitor->builtin_arguments[1]);
		str["mode"] = new Value(visitor->builtin_arguments[2]);

		int parmode = visitor->builtin_arguments[2]->get_i();

		auto rval = new Value(parser::Type::T_BOOL);
		std::fstream* fs;
		try {
			fs = new std::fstream(visitor->builtin_arguments[1]->get_s(), parmode);
			rval->set(cp_bool(fs->is_open()));
		}
		catch (...) {
			rval->set(cp_bool(false));
		}
		if (rval->b) {
			// create a new file
			files.push_back(fs);
			str[INSTANCE_ID_NAME] = new Value(parser::Type::T_INT);
			str[INSTANCE_ID_NAME]->set(cp_int(files.size() - 1));
		}

		cpfile->set(str, "File", "cp");

		visitor->current_expression_value = rval;
	};

	visitor->builtin_functions["read"] = [this, visitor]() {
		Value* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			auto rval = new Value(parser::Type::T_STRING);
			
			std::fstream* fs = files[cpfile->get_str()[INSTANCE_ID_NAME]->get_i()];

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
		Value* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			auto rval = new Value(parser::Type::T_STRING);

			std::fstream* fs = files[cpfile->get_str()[INSTANCE_ID_NAME]->get_i()];

			std::string line;
			std::getline(*fs, line);
			rval->set(line);

			visitor->current_expression_value = rval;
		}
	};

	visitor->builtin_functions["read_all_bytes"] = [this, visitor]() {
		Value* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			auto rval = new Value(parser::Type::T_ARRAY);
			rval->set_arr_type(parser::Type::T_CHAR);

			std::fstream* fs = files[cpfile->get_str()[INSTANCE_ID_NAME]->get_i()];

			fs->seekg(0);

			// find file size
			fs->seekg(0, std::ios::end);
			std::streamsize buffer_size = fs->tellg();
			fs->seekg(0, std::ios::beg);

			// buffer to store read data
			char* buffer = new char[buffer_size];

			Value** arr = new Value*[buffer_size];

			// read all bytes
			if (fs->read(buffer, buffer_size)) {
				for (size_t i = 0; i < buffer_size; ++i) {
					Value* val = new Value(parser::Type::T_CHAR);
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
		Value* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			std::fstream* fs = files[cpfile->get_str()[INSTANCE_ID_NAME]->get_i()];
			*fs << visitor->builtin_arguments[1]->get_s();
		}
	};

	visitor->builtin_functions["write_bytes"] = [this, visitor]() {
		Value* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			std::fstream* fs = files[cpfile->get_str()[INSTANCE_ID_NAME]->get_i()];

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
		Value* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			auto rval = new Value(parser::Type::T_BOOL);
			rval->set(cp_bool(files[cpfile->get_str()[INSTANCE_ID_NAME]->get_i()]->is_open()));
			visitor->current_expression_value = rval;
		}
	};

	visitor->builtin_functions["close"] = [this, visitor]() {
		Value* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			if (files[cpfile->get_str()[INSTANCE_ID_NAME]->get_i()]) {
				files[cpfile->get_str()[INSTANCE_ID_NAME]->get_i()]->close();
				files[cpfile->get_str()[INSTANCE_ID_NAME]->get_i()]->~basic_fstream();
				files[cpfile->get_str()[INSTANCE_ID_NAME]->get_i()] = nullptr;
				cpfile->set_null();
			}
		}
	};

}
