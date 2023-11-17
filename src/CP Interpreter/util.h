#ifndef UTIL_H
#define UTIL_H

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>


std::string tolower(std::string str) {
	std::string lowerStr = str;
	std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), [](unsigned char c) { return std::tolower(c); });
	return lowerStr;
}

std::string replace(std::string str, char ch1, char ch2) {
	for (size_t i = 0; i < str.length(); i++) {
		if (str[i] == ch1) {
			str[i] = ch2;
		}
	}
	return str;
}

// Split a string.
std::vector<std::string> split(std::string string, char splitChar) {
	auto strings = std::vector<std::string>();
	std::string currentString = "";
	for (char currentChar : string) {
		if (currentChar == splitChar) {
			if (!currentString.empty()) {
				strings.push_back(currentString);
				currentString = "";
			}
			continue;
		}
		currentString += currentChar;
	}
	if (!currentString.empty()) {
		strings.push_back(currentString);
	}
	return strings;
}

std::vector<std::string> split(std::string string, std::vector<char> splitChars) {
	auto strings = std::vector<std::string>();
	std::string currentString = "";
	for (char currentChar : string) {
		bool splitted = false;
		for (char splitChar : splitChars) {
			if (currentChar == splitChar) {
				if (!currentString.empty()) {
					strings.push_back(currentString);
					currentString = "";
				}
				splitted = true;
			}
			if (splitted) continue;
		}
		if (splitted) continue;
		currentString += currentChar;
	}
	if (!currentString.empty()) {
		strings.push_back(currentString);
	}
	return strings;
}

// Remove spaces in the begin of a string.
std::string trimLeft(std::string string) {
	size_t begin = 0;
	for (size_t i = 0; i < string.size(); i++) {
		if (string[i] != ' ') {
			begin = i;
			break;
		}
	}
	return string.substr(begin, string.size());
}

// Remove spaces in the end of a string.
std::string trimRight(std::string string) {
	size_t end = 0;
	for (size_t i = string.size(); i > 0; i--) {
		if (string[i - 1] != ' ') {
			end = i;
			break;
		}
	}
	return string.substr(0, end);
}

// Remove spaces in the begin and end of a string.
std::string trim(std::string string) {
	return trimLeft(trimRight(string));
}

// Clamp a double value.
double clamp(double val, double min, double max) {
	if (val > max) return max;
	if (val < min) return min;
	return val;
}

// Clamp a float value.
float clamp(float val, float min, float max) {
	return clamp((double)val, (double)min, (double)max);
}

// Clamp a int value.
int clamp(int val, int min, int max) {
	return clamp((float)val, (float)min, (float)max);
}

#endif // UTIL_H
