#include <execinfo.h>

#include "util.hpp"
#include "log.hpp"

namespace Server {

Server::Logger::ptr g_logger = SERVER_LOG_NAME("system");

pid_t getThreadId(){
    return syscall(SYS_gettid);
};

uint32_t getFiberId(){
    return 0;
};

void Backtrace(std::vector<std::string>& bt, int size, int skip){
    void** array = (void**)malloc((sizeof(void*) * size));

    // Get at most size level of stack frame info 
    size_t s = ::backtrace(array, size);

    // store info into strings
    char** strings = backtrace_symbols(array, s);

    if (strings == NULL) {
        SERVER_LOG_ERROR(g_logger) << "backtrace_symbols error";
        return;
    }
    
    // skip stack frames
    for (size_t i = skip; i < s; ++i) {
        bt.push_back(strings[i]);
    }

    free(strings);
    free(array);
};

std::string BacktraceToString(int size, int skip, const std::string& prefix){
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);

    std::stringstream ss;
    for (size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl;
    }

    return ss.str();
};

}