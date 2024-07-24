#include <filesystem>

#include "vendor/axeutils.hpp"
#include "cputil.hpp"

std::string CPUtil::load_source(const std::string& path) {
	std::string source;

	std::ifstream file;
	file.open(path);

	if (!file) {
		std::cout << "Could not load file from \"" << path << "\"." << std::endl;
	}
	else {
		std::string line;
		while (std::getline(file, line)) {
			source.append(line + "\n");
		}
	}

	if ((unsigned char)source[0] == 0xEF &&
		(unsigned char)source[1] == 0xBB &&
		(unsigned char)source[2] == 0xBF) {
		source = source.substr(3, source.size());
	}

	return source;
}

std::string CPUtil::get_lib_name(const std::string& libpath) {
	std::string file_name = libpath;
	std::string lib_name = file_name.substr(0, file_name.length() - 3);
	std::replace(lib_name.begin(), lib_name.end(), (char)std::filesystem::path::preferred_separator, '.');
	return lib_name;
}

std::string CPUtil::get_prog_name(const std::string& progpath) {
	auto norm_path = axe::PathUtils::normalize_path_sep(progpath);
	auto index = norm_path.rfind(std::filesystem::path::preferred_separator) + 1;
	std::string file_name = progpath.substr(index, progpath.size());
	std::string lib_name = file_name.substr(0, file_name.length() - 3);
	std::replace(lib_name.begin(), lib_name.end(), '\\', '.');
	return lib_name;
}
