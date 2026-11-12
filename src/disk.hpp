#pragma once
#include <atomic>
#include <string>
#include <vector>
class Disk {
    int fd_=-1;
public:
    int enable(const std::string &path);
    void disable() noexcept;
    void output(std::vector<unsigned char> *buffer, const std::atomic<bool> *cancel=nullptr);
    ~Disk() { disable(); }
};
