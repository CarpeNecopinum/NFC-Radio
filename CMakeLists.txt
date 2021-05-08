cmake_minimum_required(VERSION 3.12)
project(radio)

set(CMAKE_CXX_STANDARD 20)

set(headers 
    src/NFCListener.hh
)

set(sources
    src/NFCListener.cc
    src/radio.cc
)

# add the executable
add_executable(radio ${headers} ${sources})
target_link_libraries(radio PRIVATE nfc freefare)
target_include_directories(radio PRIVATE ./src)