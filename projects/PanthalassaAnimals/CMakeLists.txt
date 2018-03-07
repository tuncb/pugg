# define minimum cmake version
cmake_minimum_required(VERSION 2.6.2)

PROJECT(PanthalassaAnimals)

include_directories("../../include")
include_directories("../Zoo")

set(SOURCES plugin.cpp Fish.h Whale.h)

add_library(PanthalassaAnimals SHARED ${SOURCES})

