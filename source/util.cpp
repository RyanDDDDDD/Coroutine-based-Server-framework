#include "util.hpp"

namespace Server {
    pid_t GetThreadId(){
        return syscall(SYS_gettid);
    };

    uint32_t GetFiberId(){
        return 0;
    };
}