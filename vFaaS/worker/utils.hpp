#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

typedef int PIPE_COMMAND;
#define PIPE_COMMAND_INPUT 1
#define PIPE_COMMAND_RETURN 2

namespace util{
/*
    Read webassembly binary code from file and send it into Vector.
*/
void readFileToBytes(const std::string& path, std::vector<uint8_t>& codeBytes);

/*
    Tool function used to exec a new process. Bidirectional communication is established by infd and outfd
*/
bool startWorker(int *pid, int *infd, int *outfd);

/*
    Tool function to guanrantee the size of reading bytes.
*/
void readBytes(int fd, unsigned char* buffer, int bufferLength);

}