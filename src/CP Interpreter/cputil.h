#ifndef CPUTIL_H
#define CPUTIL_H

#include <iostream>
#include <fstream>
#include <regex>
#include <iomanip>


class CPUtil {
public:
	static std::string loadSource(std::string);
	static std::string getLibName(size_t, std::string);
};

#endif //CPUTIL_H
