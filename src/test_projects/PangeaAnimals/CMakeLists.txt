cmake_minimum_required(VERSION 3.1...3.27)

set(SOURCES plugin.cpp Cat.h Dog.h)

add_library(PangeaAnimals SHARED ${SOURCES})

target_link_libraries(PangeaAnimals PRIVATE pugg)
target_link_libraries(PangeaAnimals PRIVATE animal)

