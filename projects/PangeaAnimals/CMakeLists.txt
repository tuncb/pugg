# define minimum cmake version
cmake_minimum_required(VERSION 3.0.0)

PROJECT(PangeaAnimals)

include_directories("../pugg/include")
include_directories("../Zoo")

set(SOURCES plugin.cpp Cat.h Dog.h)

add_library(PangeaAnimals SHARED ${SOURCES})

