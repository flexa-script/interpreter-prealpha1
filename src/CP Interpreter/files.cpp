#include "files.hpp"

using namespace modules;

Files::Files() {}

Files::~Files() {
	for (auto& val : files) {
		delete val;
	}
	files.clear();
}

void Files::register_functions(visitor::Interpreter* interpreter) {

	interpreter->builtin_functions["open"] = [this, interpreter]() {
		// initialize file struct values
		Value* cpfile = interpreter->builtin_arguments[0];
		cpfile->str->second["path"] = new Value(interpreter->builtin_arguments[1]);
		cpfile->str->second["mode"] = new Value(interpreter->builtin_arguments[2]);

		int parmode = interpreter->builtin_arguments[2]->i;
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

		auto rval = Value(parser::Type::T_BOOL);
		std::fstream* fs;
		try {
			fs = new std::fstream(interpreter->builtin_arguments[1]->s, mode);
			rval.b = fs->is_open();
		}
		catch (...) {
			rval.b = false;
		}
		if (rval.b) {
			// create a new file
			files.push_back(fs);
			cpfile->str->second[INSTANCE_ID_NAME] = new Value(parser::Type::T_INT);
			cpfile->str->second[INSTANCE_ID_NAME]->i = files.size() - 1;
		}
		interpreter->current_expression_value = rval;
	};

	interpreter->builtin_functions["read"] = [this, interpreter]() {
		Value* cpfile = interpreter->builtin_arguments[0];
		if (!parser::is_void(cpfile->curr_type)) {
			auto rval = Value(parser::Type::T_STRING);
			
			std::fstream* fs = files[cpfile->str->second[INSTANCE_ID_NAME]->i];

			fs->seekg(0);

			std::stringstream ss;
			std::string line;
			while (std::getline(*fs, line)) {
				ss << line << std::endl;
			}
			rval.s = ss.str();

			interpreter->current_expression_value = rval;
		}
	};

	interpreter->builtin_functions["read_line"] = [this, interpreter]() {
		Value* cpfile = interpreter->builtin_arguments[0];
		if (!parser::is_void(cpfile->curr_type)) {
			auto rval = Value(parser::Type::T_STRING);

			std::fstream* fs = files[cpfile->str->second[INSTANCE_ID_NAME]->i];

			std::string line;
			std::getline(*fs, line);
			rval.s = line;

			interpreter->current_expression_value = rval;
		}
	};

	interpreter->builtin_functions["read_all_bytes"] = [this, interpreter]() {
		Value* cpfile = interpreter->builtin_arguments[0];
		if (!parser::is_void(cpfile->curr_type)) {
			auto rval = Value(parser::Type::T_ARRAY);
			rval.set_arr_type(parser::Type::T_CHAR);

			std::fstream* fs = files[cpfile->str->second[INSTANCE_ID_NAME]->i];

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
			rval.arr = arr;

			delete[] buffer;

			interpreter->current_expression_value = rval;
		}
	};

	interpreter->builtin_functions["write"] = [this, interpreter]() {
		Value* cpfile = interpreter->builtin_arguments[0];
		if (!parser::is_void(cpfile->curr_type)) {
			std::fstream* fs = files[cpfile->str->second[INSTANCE_ID_NAME]->i];
			*fs << interpreter->builtin_arguments[1]->s;
		}
	};

	interpreter->builtin_functions["write_bytes"] = [this, interpreter]() {
		Value* cpfile = interpreter->builtin_arguments[0];
		if (!parser::is_void(cpfile->curr_type)) {
			std::fstream* fs = files[cpfile->str->second[INSTANCE_ID_NAME]->i];

			auto arr = interpreter->builtin_arguments[1]->arr;

			std::streamsize buffer_size = arr.size();

			char* buffer = new char[buffer_size];

			for (size_t i = 0; i < buffer_size; ++i) {
				buffer[i] = arr[i]->c;
			}

			fs->write(buffer, sizeof(buffer));
		}
	};

	interpreter->builtin_functions["is_open"] = [this, interpreter]() {
		Value* cpfile = interpreter->builtin_arguments[0];
		if (!parser::is_void(cpfile->curr_type)) {
			auto rval = Value(parser::Type::T_BOOL);
			rval.b = files[cpfile->str->second[INSTANCE_ID_NAME]->i]->is_open();
			interpreter->current_expression_value = rval;
		}
	};

	interpreter->builtin_functions["close"] = [this, interpreter]() {
		Value* cpfile = interpreter->builtin_arguments[0];
		if (!parser::is_void(cpfile->curr_type)) {
			if (files[cpfile->str->second[INSTANCE_ID_NAME]->i]) {
				files[cpfile->str->second[INSTANCE_ID_NAME]->i]->close();
				files[cpfile->str->second[INSTANCE_ID_NAME]->i]->~basic_fstream();
				files[cpfile->str->second[INSTANCE_ID_NAME]->i] = nullptr;
				cpfile->set_null();
			}
		}
	};

}
