cmake_minimum_required(VERSION 3.17)
project(cpprest_test)

set(CMAKE_CXX_STANDARD 17)

find_package(cpprestsdk REQUIRED)

add_executable(cpprest_test main.cpp)
target_link_libraries(${PROJECT_NAME}  PRIVATE cpprestsdk::cpprest)