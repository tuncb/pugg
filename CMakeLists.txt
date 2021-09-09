# define minimum cmake version
cmake_minimum_required(VERSION 2.6.2)

PROJECT(pugg_examples)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/pugg)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/Zoo)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/PanthalassaAnimals)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/PangeaAnimals)
