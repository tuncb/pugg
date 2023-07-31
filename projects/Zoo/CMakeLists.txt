# define minimum cmake version
cmake_minimum_required(VERSION 3.0.0)

PROJECT(Zoo)

include_directories("../pugg/include")
set(SOURCES main.cpp Animal.h)

add_executable(Zoo ${SOURCES})
target_link_libraries(Zoo
    ${CMAKE_DL_LIBS}
)

