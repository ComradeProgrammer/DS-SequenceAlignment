#ifndef __UTIL_H__
#define __UTIL_H__
#include <sstream>
#include <string>
std::string charArrayToString(const char* data, const uint64_t len);
void readFastaFile(std::string file_name, std::string& out_res);
#endif
