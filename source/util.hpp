#ifndef __SERVER_TUIL_HPP__
#define __SERVER_TUIL_HPP__

#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <cstdint>
#include <vector>
#include <string>

namespace Server {
// Get thread id from kernel
pid_t getThreadId();

// Get coroutine id from kernel
uint32_t getFiberId();

// Get stack frames
void Backtrace(std::vector<std::string>& bt, int size, int skip = 1);

// Get info of stack frames
std::string BacktraceToString(int size, int skip = 2, const std::string& prefix = "");

}

#endif