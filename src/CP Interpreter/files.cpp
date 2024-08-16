#include "files.hpp"

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
		std::get<2>(cpfile->str)["path"] = new Value(visitor->builtin_arguments[1]);
		std::get<2>(cpfile->str)["mode"] = new Value(visitor->builtin_arguments[2]);

		int parmode = visitor->builtin_arguments[2]->i;
		int mode = 0;
		if (parmode > 10) {
			if (parmode >= 10 && parmode < 20) {
			}
			else if (parmode >= 20 && parmode < 30) {
				mode = std::fstream::binary;
			}
			parmode %= 10;
		}

		switch (parmode)
		{
		case 1:
			mode |= std::fstream::in;
			break;
		case 2:
			mode |= std::fstream::out;
			break;
		case 3:
			mode |= std::fstream::app;
			break;
		default:
			break;
		}

		auto rval = new Value(parser::Type::T_BOOL);
		std::fstream* fs;
		try {
			fs = new std::fstream(visitor->builtin_arguments[1]->s, mode);
			rval->b = fs->is_open();
		}
		catch (...) {
			rval->b = false;
		}
		if (rval->b) {
			// create a new file
			files.push_back(fs);
			std::get<2>(cpfile->str)[INSTANCE_ID_NAME] = new Value(parser::Type::T_INT);
			std::get<2>(cpfile->str)[INSTANCE_ID_NAME]->i = files.size() - 1;
		}
		visitor->current_expression_value = rval;
	};

	visitor->builtin_functions["read"] = [this, visitor]() {
		Value* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			auto rval = new Value(parser::Type::T_STRING);
			
			std::fstream* fs = files[std::get<2>(cpfile->str)[INSTANCE_ID_NAME]->i];

			fs->seekg(0);

			std::stringstream ss;
			std::string line;
			while (std::getline(*fs, line)) {
				ss << line << std::endl;
			}
			rval->s = ss.str();

			visitor->current_expression_value = rval;
		}
	};

	visitor->builtin_functions["read_line"] = [this, visitor]() {
		Value* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			auto rval = new Value(parser::Type::T_STRING);

			std::fstream* fs = files[std::get<2>(cpfile->str)[INSTANCE_ID_NAME]->i];

			std::string line;
			std::getline(*fs, line);
			rval->s = line;

			visitor->current_expression_value = rval;
		}
	};

	visitor->builtin_functions["read_all_bytes"] = [this, visitor]() {
		Value* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			auto rval = new Value(parser::Type::T_ARRAY);
			rval->set_arr_type(parser::Type::T_CHAR);

			std::fstream* fs = files[std::get<2>(cpfile->str)[INSTANCE_ID_NAME]->i];

			fs->seekg(0);

			// find file size
			fs->seekg(0, std::ios::end);
			std::streamsize buffer_size = fs->tellg();
			fs->seekg(0, std::ios::beg);

			// buffer to store read data
			char* buffer = new char[buffer_size];

			cp_array arr;

			// read all bytes
			if (fs->read(buffer, buffer_size)) {
				for (size_t i = 0; i < buffer_size; ++i) {
					Value* val = new Value(parser::Type::T_CHAR);
					val->set(buffer[i]);
					arr.push_back(val);
				}
			}
			rval->arr = arr;

			delete[] buffer;

			visitor->current_expression_value = rval;
		}
	};

	visitor->builtin_functions["write"] = [this, visitor]() {
		Value* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			std::fstream* fs = files[std::get<2>(cpfile->str)[INSTANCE_ID_NAME]->i];
			*fs << visitor->builtin_arguments[1]->s;
		}
	};

	visitor->builtin_functions["write_bytes"] = [this, visitor]() {
		Value* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			std::fstream* fs = files[std::get<2>(cpfile->str)[INSTANCE_ID_NAME]->i];

			auto arr = visitor->builtin_arguments[1]->arr;

			std::streamsize buffer_size = arr.size();

			char* buffer = new char[buffer_size];

			for (size_t i = 0; i < buffer_size; ++i) {
				buffer[i] = arr[i]->c;
			}

			fs->write(buffer, sizeof(buffer));
		}
	};

	visitor->builtin_functions["is_open"] = [this, visitor]() {
		Value* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			auto rval = new Value(parser::Type::T_BOOL);
			rval->b = files[std::get<2>(cpfile->str)[INSTANCE_ID_NAME]->i]->is_open();
			visitor->current_expression_value = rval;
		}
	};

	visitor->builtin_functions["close"] = [this, visitor]() {
		Value* cpfile = visitor->builtin_arguments[0];
		if (!parser::is_void(cpfile->type)) {
			if (files[std::get<2>(cpfile->str)[INSTANCE_ID_NAME]->i]) {
				files[std::get<2>(cpfile->str)[INSTANCE_ID_NAME]->i]->close();
				files[std::get<2>(cpfile->str)[INSTANCE_ID_NAME]->i]->~basic_fstream();
				files[std::get<2>(cpfile->str)[INSTANCE_ID_NAME]->i] = nullptr;
				cpfile->set_null();
			}
		}
	};

}
