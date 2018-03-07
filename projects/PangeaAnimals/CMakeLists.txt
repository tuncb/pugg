# define minimum cmake version
cmake_minimum_required(VERSION 2.6.2)

PROJECT(PangeaAnimals)

include_directories("../../include")
include_directories("../Zoo")

set(SOURCES plugin.cpp Cat.h Dog.h)

add_library(PangeaAnimals SHARED ${SOURCES})

