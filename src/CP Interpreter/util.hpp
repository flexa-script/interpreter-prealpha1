#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <vector>
#include <list>


namespace axe {
	std::string tolower(std::string);
	std::vector<std::string> split_vector(std::string, char);
	std::list<std::string> split_list(std::string, char);
	bool contains(std::string, std::string);
	std::string join(std::vector<std::string>, const char* const);
	unsigned int hashcode(const std::string&);
}

#endif // UTIL_HPP
