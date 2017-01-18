#ifndef file_system_h
#define file_system_h

#include <string>
#include <iostream>
#include <fstream>
#include <cstring>
#include "IO_system.h"
bool init(std::string filename);
void save(const std::string filename);
bool create(const char* name);
void directory();
int read(int index, char *mem, int count);
int write(int index, char *mem, int count);
int lseek(int index, int position);
int open(const char *name);
bool close(int index);
bool destroy(const char* name);

#endif