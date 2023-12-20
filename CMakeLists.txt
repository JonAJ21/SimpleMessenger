cmake_minimum_required(VERSION 3.2)
project(SimpleMessenger)

set(CMAKE_CXX_STANDART 14)


# AMQP-CPP
find_package(amqpcpp)
if (NOT ${amqpcpp_FOUND})
    message(FATAL_ERROR "AMQP-CPP Libraries not found.")
endif()
include_directories(amqpcpp)

add_library(amqp-cpp amqp-cpp.cpp amqp-cpp.h)
target_link_libraries(amqp-cpp amqpcpp)

# Poco
find_package(Poco REQUIRED COMPONENTS Net Foundation)
if (NOT ${Poco_FOUND})
    message(FATAL_ERROR "Poco C++ Libraries not found.")
endif()
include_directories(${Poco_INCLUDE_DIRS})


# SimplePocoHandler
add_library(poco_simple_handler SimplePocoHandler.cpp SimplePocoHandler.h)
target_link_libraries(poco_simple_handler ${Poco_LIBRARIES})


# PROGS
set(PROGS server client)

foreach(item ${PROGS})
    add_executable(${item} "${item}.cpp")
    target_link_libraries(${item} amqp-cpp poco_simple_handler)
endforeach(item ${PROGS})

