# define minimum cmake version
cmake_minimum_required(VERSION 3.0.0)

PROJECT(pugg_examples)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(./projects/Zoo)
add_subdirectory(./projects/PanthalassaAnimals)
add_subdirectory(./projects/PangeaAnimals)
