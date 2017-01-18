#include "str_operations.h"

std::vector<std::string> tokenize(std::string input) {
	std::string token;
	std::vector<std::string> tokens;
	std::istringstream split(input);

	while (std::getline(split, token, ' ')) {
		tokens.push_back(token);
	}

	return tokens;
}

void printVector(std::vector<std::string> v) {
	for (int i = 0; i < v.size(); i++) {
		std::cout << v[i] << std::endl;
	}
}

std::string toLower(std::string s) {
	std::string ret = "";
	for (int i = 0; i < s.length(); i++)
	{
		ret += tolower(s[i]);
	}

	return ret;

}
