#include "util.h"
#include "CPUtil.h"


std::string CPUtil::loadSource(std::string path) {
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

std::string CPUtil::getLibName(size_t index, std::string path) {
	//auto splPath = axe::split(path, '\\');
	//std::string fileName = splPath.at(splPath.size() - 1);
	std::string fileName = path.substr(index, path.size());
	std::string libName = fileName.substr(0, fileName.length() - 3);
	//std::replace(libName.begin(), libName.end(), '/', '.');
	std::replace(libName.begin(), libName.end(), '\\', '.');
	return libName;
}
