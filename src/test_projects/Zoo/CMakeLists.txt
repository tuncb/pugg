cmake_minimum_required(VERSION 3.1...3.27)

add_executable(Zoo main.cpp)

target_link_libraries(Zoo PRIVATE ${CMAKE_DL_LIBS})
target_link_libraries(Zoo PRIVATE pugg)
target_link_libraries(Zoo PRIVATE animal)

