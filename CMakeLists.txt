cmake_minimum_required(VERSION 3.15)
project(program)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -O3")
# Шукаємо MPI
find_package(MPI REQUIRED)
find_package(Threads REQUIRED)

include_directories(include)

add_executable(program
    src/GameManager.cpp
    src/escaper.cpp
    src/pursuer.cpp
    src/main.cpp
)

# Лінкуємо MPI
target_link_libraries(program PRIVATE MPI::MPI_CXX Threads::Threads)



# cmake_minimum_required(VERSION 3.15)
# project(program)

# set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find_package(Threads REQUIRED)

# include_directories(include)

# add_executable(program
#     src/GameManager.cpp
#     src/escaper.cpp
#     src/pursuer.cpp
#     src/main.cpp
# )

# target_link_libraries(program PRIVATE Threads::Threads)