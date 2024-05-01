#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <vector>
#include <list>


namespace axe {
	static class Util {
	public:
		static std::string tolower(std::string);
		static std::vector<std::string> split_vector(std::string, char);
		static std::list<std::string> split_list(std::string, char);
		static bool contains(std::string, std::string);
		static std::string join(std::vector<std::string>, const char* const);
		static unsigned int hashcode(const std::string&);
	};
}

#endif // UTIL_HPP
