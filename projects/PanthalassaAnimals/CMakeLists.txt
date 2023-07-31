# define minimum cmake version
cmake_minimum_required(VERSION 3.0.0)

PROJECT(PanthalassaAnimals)

include_directories("../pugg/include")
include_directories("../Zoo")

set(SOURCES plugin.cpp Fish.h Whale.h)

add_library(PanthalassaAnimals SHARED ${SOURCES})

