#include "util.hpp"
#include "cputil.hpp"


std::string CPUtil::load_source(std::string path) {
	std::string source;

	// read the file
	std::ifstream file;
	file.open(path);

	if (!file) {
		std::cout << "Could not load file from \"" << path << "\"." << std::endl;
	}
	else {
		// convert whole program to std::string
		std::string line;
		while (std::getline(file, line)) {
			source.append(line + "\n");
		}
	}

	// skips the Byte Order Mark (BOM) that defines UTF-8 in some text files.
	if ((unsigned char)source[0] == 0xEF &&
		(unsigned char)source[1] == 0xBB &&
		(unsigned char)source[2] == 0xBF) {
		source = source.substr(3, source.size());
	}

	return source;
}

std::string CPUtil::get_lib_name(size_t index, std::string path) {
	std::string file_name = path.substr(index, path.size());
	std::string lib_name = file_name.substr(0, file_name.length() - 3);
	std::replace(lib_name.begin(), lib_name.end(), '\\', '.');
	return lib_name;
}
