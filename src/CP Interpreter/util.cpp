#include <algorithm>
#include <sstream>
#include <iostream>
#include <iterator>

#include "util.h"


namespace axe {

	std::string tolower(std::string str) {
		std::string lowerStr = str;
		std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), [](unsigned char c) { return std::tolower(c); });
		return lowerStr;
	}

    std::vector<std::string> split(std::string string, char sep) {
        std::vector<std::string> strings;
        std::istringstream stringstream(string);
        std::string currentString;
        while (std::getline(stringstream, currentString, sep)) {
            strings.push_back(currentString);
        }
        return strings;
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

}
