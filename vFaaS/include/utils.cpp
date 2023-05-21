#include "utils.hpp"

std::string wasmFilePath = "/code/test.wasm";
const char* functionConfigPath = "/code/func.config";

namespace util{

void readFileToBytes(const std::string& path, std::vector<uint8_t>& codeBytes){
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) throw std::runtime_error("Couldn't open file " + path);
    struct stat statbuf;
    int staterr = fstat(fd, &statbuf);
    if (staterr < 0) throw std::runtime_error("Couldn't stat file " + path);
    size_t fsize = statbuf.st_size;
    posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    codeBytes.resize(fsize);
    readBytes(fd, codeBytes.data(), fsize);
    close(fd);
    return;
}

bool startWorker(int *pid, int *infd, int *outfd){
    char* command[] = {"./worker_exe", NULL};
    int p1[2], p2[2];
    if (!pid || !infd || !outfd) return false;
    if (pipe(p1) == -1) goto err_pipe1;
    if (pipe(p2) == -1) goto err_pipe2;
    if ((*pid = fork()) == -1) goto err_fork;
    if (*pid){
        /* Parent process. */
        *infd = p1[1];  // writeFd
        *outfd = p2[0]; // readFd
        close(p1[0]);
        close(p2[1]);
        return true;
    } else {
        /* Child process. */
        dup2(p1[0], 0); // redirect stdin
        dup2(p2[1], PIPE_WRITE_FD); // 10 is just an arbitrary file descriptor which avoids collision from stdout
        close(p1[1]);
        close(p2[0]);
        execv(command[0], command);
        /* Error occured. */
        fprintf(stderr, "error running %s: %s", command[0], strerror(errno));
        abort();
    }
err_fork:
    close(p2[1]);
    close(p2[0]);
err_pipe2:
    close(p1[1]);
    close(p1[0]);
err_pipe1:
    perror("[Host] Couldn't fork new process.");
    return false;
}

void readBytes(int fd, unsigned char* buffer, int bufferLength){
    int cpos = 0;
    while (cpos < bufferLength) {
        int rc = read(fd, buffer + cpos, bufferLength - cpos);
        if (rc < 0) {
            perror("[Host Worker Read] Couldn't Read from worker.");
            throw "[Host Worker Read] Couldn't Read from worker.";
        } else {
            cpos += rc;
        }
    }
}

void readFromJson(std::string& raw, uint8_t* ptr, int ptrLen){
    std::string decodedString = base64_decode(raw, false);
    int size = decodedString.size();
    if(size != ptrLen){
        std::cout << "[Json Util] Unmatched data length: " << raw << std::endl;
        throw "[Json Util] Unmatched data length.";
    }
    std::copy(decodedString.data(), decodedString.data() + size, ptr);
}

std::string writeToJson(const uint8_t* ptr, int len){
    return base64_encode(ptr, len, false);
}

PIPE_COMMAND readPIPECommand(int fd){
    PIPE_COMMAND tmp;
    readBytes(fd, (unsigned char*)(&tmp), sizeof(PIPE_COMMAND));
    return tmp;
}

void writePIPECommand(int fd, PIPE_COMMAND cmd){
    write(fd, &cmd, sizeof(PIPE_COMMAND));
}

}



/*
    Test Code for base64 encoding
*/

// #include "../include/base64.h"
// #include <iostream>
// class testClass{
//     public:
//     int x;
//     int y;
//     int z;
//     testClass(int _x, int _y, int _z){
//         this->x = _x;
//         this->y = _y;
//         this->z = _z;
//     }
//     void print(){
//         std::cout << x << ' ' << y << ' ' << z << std::endl;
//     }
// };
// int main(){
//     testClass testUnit(1,2,3);
//     unsigned char* ptr = (unsigned char*)(&testUnit);
//     std::string encodedBytes = base64_encode(ptr, sizeof(testClass), false);
//     std::cout << "Encoded: " << encodedBytes << std::endl;
//     std::string decodedBytes = base64_decode(encodedBytes, false);
//     std::cout << "Decoded: " << decodedBytes << std::endl;
//     std::cout << "Origin Size : " << sizeof(testClass) << std::endl << "Now Size : " << decodedBytes.size() << std::endl;
//     ptr = new unsigned char[sizeof(testClass)];
//     std::copy(decodedBytes.data(),decodedBytes.data()+decodedBytes.size(),ptr);
//     ((testClass*)(ptr))->print();
//     return 0;
// }