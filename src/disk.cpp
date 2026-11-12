/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include "disk.hpp"
#include "output_io.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cerrno>
int Disk::enable(const std::string &path) {
    fd_=open(path.c_str(),O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK,0600);
    if (fd_<0) { std::cerr << "Cannot open " << path << ": " << std::strerror(errno) << '\n'; return 1; }
    std::cerr << "Saving to disk: " << path << '\n'; return 0;
}
void Disk::disable() noexcept { if (fd_>=0) { close(fd_); fd_=-1; } }
void Disk::output(std::vector<unsigned char> *buffer, const std::atomic<bool> *cancel) {
    if (fd_>=0) OutputIO::writeAll(fd_,buffer->data(),buffer->size(),cancel);
}
