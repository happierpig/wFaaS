#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace util{

void readFileToBytes(const std::string& path, std::vector<uint8_t>& codeBytes);

}