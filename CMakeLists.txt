cmake_minimum_required(VERSION 3.1...3.27)

project(
  puggSuite
  VERSION 1.0.2
  LANGUAGES CXX)

add_subdirectory(./src/pugg)

if( CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
# We are building as stand-alone project
  add_subdirectory(./src/test_projects/Animal)
  add_subdirectory(./src/test_projects/Zoo)
  add_subdirectory(./src/test_projects/PanthalassaAnimals)
  add_subdirectory(./src/test_projects/PangeaAnimals)
endif()

