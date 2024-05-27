#include <Windows.h>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <iterator>
#include <filesystem>

#include "util.hpp"


namespace axe {

    std::string Util::replace(std::string str, const std::string from, const std::string to) {
        std::string replaced = str;
        if (from.empty()){
            return replaced;
        }
        size_t start_pos = 0;
        while ((start_pos = replaced.find(from, start_pos)) != std::string::npos) {
            replaced.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        return replaced;
    }

	std::string Util::tolower(std::string str) {
		std::string lowerStr = str;
		std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), [](unsigned char c) { return std::tolower(c); });
		return lowerStr;
	}

    std::vector<std::string> Util::split_vector(std::string string, char sep) {
        std::vector<std::string> strings;
        std::istringstream stringstream(string);
        std::string currentString;
        while (std::getline(stringstream, currentString, sep)) {
            strings.push_back(currentString);
        }
        return strings;
    }

    std::list<std::string> Util::split_list(std::string string, char sep) {
        auto v = split_vector(string, sep);
        return std::list<std::string>(v.begin(), v.end());
    }

    bool Util::contains(std::string string, std::string cont) {
        return string.find(cont) != std::string::npos;
    }

    bool Util::contains(std::vector<std::string> c, std::string v) {
        return std::find(c.begin(), c.end(), v) != c.end();
    }

    std::string Util::join(std::vector<std::string> strings, const char* const delim) {
        std::string string;
        std::ostringstream imploded;
        std::copy(strings.begin(), strings.end(), std::ostream_iterator<std::string>(imploded, delim));
        string = imploded.str();
        if (!string.empty()) string.pop_back();
        return string;
    }

    unsigned int Util::hashcode(const std::string& str) {
        unsigned int h = 0;
        for (size_t i = 0; i < str.size(); ++i){
            h = h * 31 + static_cast<unsigned int>(str[i]);
        }
        return h;
    }

    std::string Util::get_current_path() {
        HMODULE this_process_handle = GetModuleHandle(NULL);
        wchar_t this_process_path[MAX_PATH];

        GetModuleFileNameW(NULL, this_process_path, sizeof(this_process_path));

        std::filesystem::path path(this_process_path);

        return path.remove_filename().generic_string();
    }

    std::string Util::normalize_path_sep(std::string path) {
        std::string sep(std::string{ std::filesystem::path::preferred_separator });
        return replace(replace(path, "\\", sep), "/", sep);
    }

}
