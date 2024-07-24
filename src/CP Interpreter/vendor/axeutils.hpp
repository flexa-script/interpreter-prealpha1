#ifndef AXEUTILS_HPP
#define AXEUTILS_HPP

#include <string>
#include <vector>
#include <list>

namespace axe {
	class StringUtils {
	public:
		static std::string ltrim(std::string s);
		static std::string rtrim(std::string s);
		static std::string trim(std::string s);
		static std::string replace(std::string str, const std::string& from, const std::string& to);
		static void replace_inline(std::string& str, const std::string& from, const std::string& to);
		static void replace_first(std::string& str, const std::string& from, const std::string& to);
		static std::string tolower(std::string str);
		static bool contains(const std::string& string, const std::string& cont);

		static std::string join(const std::vector<std::string>& strings, const char* const delim);
		static std::vector<std::string> split_vector(const std::string& str, char sep);
		static std::list<std::string> split_list(const std::string& str, char sep);
		static bool contains(const std::vector<std::string>& c, const std::string& v);

		static long long hashcode(const std::string& str);

	private:
		template<typename OutputIterator>
		static void split(const std::string& str, char sep, OutputIterator result);
	};

	class CollectionUtils {
	public:
		template<typename Container, typename T>
		static bool contains(const Container& c, const T& value);
	};

	class PathUtils {
	public:
		static std::string get_current_path();
		static std::string normalize_path_sep(const std::string& path);
	};
}

#endif // !AXEUTILS_HPP
