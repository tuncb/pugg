# define minimum cmake version
cmake_minimum_required(VERSION 2.6.2)

PROJECT(PangeaAnimals)

set(SOURCES plugin.cpp Cat.h Dog.h)

add_library(PangeaAnimals SHARED ${SOURCES})
target_link_libraries(PangeaAnimals PUBLIC IZoo pugg)
