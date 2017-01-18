#ifndef str_operations_h
#define str_operations_h

#include <vector>
#include <string>
#include <sstream>
#include <iostream>

std::vector<std::string> tokenize(std::string input);
void printVector(std::vector<std::string> v);
std::string toLower(std::string s);

#endif