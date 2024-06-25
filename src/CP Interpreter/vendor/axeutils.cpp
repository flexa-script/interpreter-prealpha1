#include <Windows.h>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <iterator>
#include <filesystem>

#include "axeutils.hpp"


namespace axe {

	// StringUtils

	std::string StringUtils::ltrim(std::string s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
			return !std::isspace(ch);
			}));
		return s;
	}

	std::string StringUtils::rtrim(std::string s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
			}).base(), s.end());
		return s;
	}

	std::string StringUtils::trim(std::string s) {
		return ltrim(rtrim(s));
	}

	std::string StringUtils::replace(std::string str, const std::string from, const std::string to) {
		if (from.empty()) {
			return str;
		}
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length();
		}
		return str;
	}

	std::string StringUtils::tolower(std::string str) {
		std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
		return str;
	}

	bool StringUtils::contains(const std::string& string, const std::string& cont) {
		return string.find(cont) != std::string::npos;
	}

	std::string StringUtils::join(const std::vector<std::string>& strings, const char* const delim) {
		std::string string;
		std::ostringstream imploded;
		std::copy(strings.begin(), strings.end(), std::ostream_iterator<std::string>(imploded, delim));
		string = imploded.str();
		if (!string.empty()) string.pop_back();
		return string;
	}
	
	template<typename OutputIterator>
	void StringUtils::split(const std::string& str, char sep, OutputIterator result) {
		std::istringstream iss(str);
		std::string item;
		while (std::getline(iss, item, sep)) {
			*result++ = item;
		}
	}

	std::vector<std::string> StringUtils::split_vector(const std::string& str, char sep) {
		std::vector<std::string> result;
		split(str, sep, std::back_inserter(result));
		return result;
	}

	std::list<std::string> StringUtils::split_list(const std::string& str, char sep) {
		std::list<std::string> result;
		split(str, sep, std::back_inserter(result));
		return result;
	}

	bool StringUtils::contains(const std::vector<std::string>& c, const std::string& v) {
		return std::find(c.begin(), c.end(), v) != c.end();
	}

	long long StringUtils::hashcode(const std::string& str) {
		long long h = 0;
		for (size_t i = 0; i < str.size(); ++i) {
			h = h * 31 + static_cast<long long>(str[i]);
		}
		return h;
	}


	// CollectionUtils

	template<>
	bool CollectionUtils::contains(const std::string& str, const std::string& substr) {
		return str.find(substr) != std::string::npos;
	}

	template<typename Container, typename T>
	bool CollectionUtils::contains(const Container& c, const T& value) {
		return std::find(c.begin(), c.end(), value) != c.end();
	}


	// PathUtils

	std::string PathUtils::get_current_path() {
		HMODULE this_process_handle = GetModuleHandle(NULL);
		wchar_t this_process_path[MAX_PATH];

		GetModuleFileNameW(NULL, this_process_path, sizeof(this_process_path));

		std::filesystem::path path(this_process_path);

		return normalize_path_sep(path.remove_filename().generic_string());
	}

	std::string PathUtils::normalize_path_sep(const std::string& path) {
		std::string sep(std::string{ std::filesystem::path::preferred_separator });
		return StringUtils::replace(StringUtils::replace(path, "\\", sep), "/", sep);
	}

}
