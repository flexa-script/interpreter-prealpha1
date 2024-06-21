#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <vector>
#include <list>


namespace axe {
	class Util {
	public:
		static std::string& ltrim(std::string& s);
		static std::string& rtrim(std::string& s);
		static std::string& trim(std::string& s);
		static std::string tolower(std::string);
		static std::vector<std::string> split_vector(std::string, char);
		static std::list<std::string> split_list(std::string, char);
		static bool contains(std::string, std::string);
		static bool contains(std::vector<std::string>, std::string);
		static std::string join(std::vector<std::string>, const char* const);
		static long long hashcode(const std::string&);
		static std::string replace(std::string, const std::string, const std::string);
		static std::string get_current_path();
		static std::string normalize_path_sep(std::string);
	};
}

#endif // !UTIL_HPP
