cmake_minimum_required(VERSION 3.15)
project(program)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

include_directories(include)

add_executable(program
    src/GameManager.cpp
    src/escaper.cpp
    src/pursuer.cpp
    src/main.cpp
)

target_link_libraries(program PRIVATE Threads::Threads)