# define minimum cmake version
cmake_minimum_required(VERSION 2.6.2)

PROJECT(Zoo)

include_directories("../../include")
set(SOURCES main.cpp Animal.h)

add_executable(Zoo ${SOURCES})
target_link_libraries(Zoo
    ${CMAKE_DL_LIBS}
)

