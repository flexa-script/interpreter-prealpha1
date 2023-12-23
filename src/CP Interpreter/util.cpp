#include <algorithm>

#include "util.h"


namespace axe {

	std::string tolower(std::string str) {
		std::string lowerStr = str;
		std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), [](unsigned char c) { return std::tolower(c); });
		return lowerStr;
	}

}
