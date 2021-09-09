# define minimum cmake version
cmake_minimum_required(VERSION 2.6.2)

PROJECT(PanthalassaAnimals)

set(SOURCES plugin.cpp Fish.h Whale.h)

add_library(PanthalassaAnimals SHARED ${SOURCES})
target_link_libraries(PanthalassaAnimals PUBLIC IZoo pugg)
