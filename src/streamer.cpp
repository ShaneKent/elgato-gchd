/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <array>
#include <iostream>

#include <streamer.hpp>
#include "output_queue.hpp"

void Streamer::loop() {
    OutputQueue writer([&](std::vector<unsigned char> &buffer, const std::atomic<bool> *cancel) {
        disk.output(&buffer,cancel); fifo.output(&buffer,cancel); socket.output(&buffer,cancel);
    });
    std::cerr << "Streamer has been started.\n";
    try {
        while (process_->isActive()) {
            writer.check();
            std::vector<unsigned char> buffer;
            gchd_->stream(&buffer);
            if (!buffer.empty()) writer.push(std::move(buffer));
        }
        writer.finish();
    } catch (...) {
        try { writer.finish(); } catch (...) {}
        throw;
    }
    std::cerr << "Output: " << writer.written() << " bytes, " << writer.dropped() << " dropped buffers\n";
}

Streamer::Streamer(GCHD *gchd, Process *process) {
	gchd_ = gchd;
	process_ = process;
}
