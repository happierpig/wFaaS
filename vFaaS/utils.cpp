#include "utils.hpp"

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
    int rc = read(fd, codeBytes.data(), fsize);
    if (rc < 0 || rc != fsize) {
        perror("Couldn't read file");
        throw std::runtime_error("Couldn't read file " + path);
    }else std::cout << "Read " << rc << " Bytes." << std::endl;
    close(fd);
    return;
}

}