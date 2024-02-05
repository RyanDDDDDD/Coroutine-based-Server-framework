# Server Framework

## Development Environment

    Ubuntu 11.4.0
    Clang 14.0.0
    Cmake 3.22.1

## External Dependency

[Boost](https://www.boost.org/)  \
[Catch2](https://github.com/catchorg/Catch2) \
[yaml-cpp](https://github.com/jbeder/yaml-cpp)


## Project Structure

    bin --- binaries
    build --- intermediate files
    cmake --- cmake function folder
    lib --- library output folder
    tests --- testing code
    source --- source code
    CMakeLists.txt
    Makefile

## Modules
### Logger Module

[**Log4j**](https:github.com/apache/logging-log4j2) is a java-based logging framework, we will refer to its implementation to create a similar framework.

    Logger (Define Logger class)
        |
        |  
        |  
    Appender (Logger output) ----- Formatter (format Log)
        |
        |----------
        |         |
    Console     Files

When we try to log info, we would pass an event into Logger, the Logger would use Appender to output event to console or files;

The appender contains a formatter, which contains a set of formatter items, these items could be config by our pre-defined pattern(support default pattern if we don't set a specific pattern)

------

The message format we would use (refer to [Log4j TTCC](https://en.wikipedia.org/wiki/Log4j#cite_note-28))

    %m --- message body
    %p --- priority level
    %r --- number of milliseconds elapsed since the logger created
    %c --- name of logger
    %t --- thread id
    %n --- newline char
    %d --- time stamp
    %f --- file name
    %l --- line number
    %T --- Tab
    %F --- Coroutine Id

### Configuration Module

Configuration Module is used to store all system configuration.

We use **Yaml** as the configuration file to setup our program.
 

### Coroutine Module

### Coroutine Schedule Module

### IO Coroutine Schedule Module

## Testing

We would use Catch2 for unit testing, and a high-resolution timer for performance testing