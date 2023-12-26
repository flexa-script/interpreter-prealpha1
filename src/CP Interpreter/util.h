#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>


namespace axe {
	std::string tolower(std::string);
	std::vector<std::string> split(std::string, char);
	bool contains(std::string, std::string);
	std::string join(std::vector<std::string>, const char* const);
}

#endif // UTIL_H
