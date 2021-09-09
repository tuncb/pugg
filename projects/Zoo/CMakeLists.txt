# define minimum cmake version
cmake_minimum_required(VERSION 2.6.2)

PROJECT(Zoo)

include_directories("../pugg/include")
set(SOURCES main.cpp )

add_library(IZoo INTERFACE)
target_include_directories(IZoo INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(Zoo ${SOURCES})
target_link_libraries(Zoo PRIVATE
    ${CMAKE_DL_LIBS}
    IZoo
)

