cmake_minimum_required(VERSION 3.1...3.27)

set(SOURCES plugin.cpp Fish.h Whale.h)

add_library(PanthalassaAnimals SHARED ${SOURCES})

target_link_libraries(PanthalassaAnimals PRIVATE pugg)
target_link_libraries(PanthalassaAnimals PRIVATE animal)

