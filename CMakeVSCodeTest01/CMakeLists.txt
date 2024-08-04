cmake_minimum_required(VERSION 3.5.0)
project(CMakeTest VERSION 0.1.0 LANGUAGES C CXX)

add_executable(CMakeTest hello.cpp)

include(CTest)
enable_testing()

