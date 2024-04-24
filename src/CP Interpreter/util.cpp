#include <algorithm>
#include <sstream>
#include <iostream>
#include <iterator>

#include "util.hpp"


namespace axe {

	std::string tolower(std::string str) {
		std::string lowerStr = str;
		std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), [](unsigned char c) { return std::tolower(c); });
		return lowerStr;
	}

    std::vector<std::string> split_vector(std::string string, char sep) {
        std::vector<std::string> strings;
        std::istringstream stringstream(string);
        std::string currentString;
        while (std::getline(stringstream, currentString, sep)) {
            strings.push_back(currentString);
        }
        return strings;
    }

    std::list<std::string> split_list(std::string string, char sep) {
        auto v = split_vector(string, sep);
        return std::list<std::string>(v.begin(), v.end());
    }

    bool contains(std::string string, std::string cont) {
        return string.find(cont) != std::string::npos;
    }

    std::string join(std::vector<std::string> strings, const char* const delim) {
        std::string string;
        std::ostringstream imploded;
        std::copy(strings.begin(), strings.end(), std::ostream_iterator<std::string>(imploded, delim));
        string = imploded.str();
        if (!string.empty()) string.pop_back();
        return string;
    }

    unsigned int hashcode(const std::string& str) {
        int h = 0;
        for (size_t i = 0; i < str.size(); ++i){
            h = h * 31 + static_cast<unsigned int>(str[i]);
        }
        return h;
    }

}
