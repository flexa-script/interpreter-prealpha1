#ifndef CPSOURCE_CPP
#define CPSOURCE_CPP

#include <string>


class CPSource {
public:
	std::string name;
	std::string source;
	CPSource(std::string name, std::string source) : name(name), source(source) {}
};

#endif // !CPSOURCE_CPP
