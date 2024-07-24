#ifndef CPUTIL_H
#define CPUTIL_H

#include <iostream>
#include <fstream>
#include <regex>
#include <iomanip>

class CPUtil {
public:
	static std::string load_source(const std::string& path);
	static std::string get_lib_name(const std::string& libpath);
	static std::string get_prog_name(const std::string& progpath);
};

#endif // !CPUTIL_H
