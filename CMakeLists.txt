cmake_minimum_required(VERSION 3.16)
project(xlog)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG master
)
FetchContent_MakeAvailable(fmt)

find_package(Boost REQUIRED COMPONENTS log)
find_package(Threads REQUIRED)

add_library(xlog SHARED xlog.cpp)
target_link_libraries(xlog Threads::Threads Boost::log fmt::fmt)

add_library(xlog-static STATIC xlog.cpp)
target_link_libraries(xlog-static Threads::Threads Boost::log fmt::fmt)
