#ifndef UTIL_H
#define UTIL_H

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>


namespace axe {

	std::string tolower(std::string str);

	std::string replace(std::string str, char ch1, char ch2);

	// Split a string.
	std::vector<std::string> split(std::string string, char splitChar);

	std::vector<std::string> split(std::string string, std::vector<char> splitChars);

	// Remove spaces in the begin of a string.
	std::string trimLeft(std::string string);

	// Remove spaces in the end of a string.
	std::string trimRight(std::string string);

	// Remove spaces in the begin and end of a string.
	std::string trim(std::string string);

	// Clamp a double value.
	double clamp(double val, double min, double max);

	// Clamp a float value.
	float clamp(float val, float min, float max);

	// Clamp a int value.
	int clamp(int val, int min, int max);

}

#endif // UTIL_H
