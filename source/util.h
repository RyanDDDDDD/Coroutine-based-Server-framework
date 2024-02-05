#ifndef __SERVER_TUIL_H__
#define __SERVER_TUIL_H__

#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <cstdint>

namespace Server {
    pid_t GetThreadId();

    uint32_t GetFiberId();
}

#endif