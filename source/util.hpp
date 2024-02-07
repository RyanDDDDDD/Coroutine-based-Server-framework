#ifndef __SERVER_TUIL_HPP__
#define __SERVER_TUIL_HPP__

#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <cstdint>

namespace Server {
    pid_t getThreadId();

    uint32_t getFiberId();
}

#endif