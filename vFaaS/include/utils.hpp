#ifndef _UTILS_H_
#define _UTILS_H_

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <ctime>
#include <sys/time.h>


#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "base64.h"

typedef int PIPE_COMMAND;
#define PIPE_COMMAND_INPUT 1
#define PIPE_COMMAND_RETURN 2
#define PIPE_COMMAND_STATE_READ 3
#define PIPE_COMMAND_STATE_WRITE 4
#define PIPE_COMMAND_STATE_FOUND 5
#define PIPE_COMMAND_STATE_NOT_FOUND 6
#define PIPE_COMMAND_READY 7

#define PIPE_WRITE_FD 10713

extern std::string wasmFilePath;
extern const char* functionConfigPath;

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

/*
    Tool function used to translate data in json
*/
void readFromJson(std::string& raw, uint8_t* ptr, int ptrLen);

std::string writeToJson(const uint8_t* ptr, int len);

PIPE_COMMAND readPIPECommand(int fd);

void writePIPECommand(int fd, PIPE_COMMAND cmd);
}

#endif