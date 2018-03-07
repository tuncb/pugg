#pragma once
#include <Animal.h>

class Cat : public Animal
{
public:
    std::string kind() {return "Cat";}
    bool can_swim() {return false;}
};

class CatDriver : public AnimalDriver
{
public:
    CatDriver() : AnimalDriver("CatDriver", Cat::version) {}
    Animal* create() {return new Cat();}
};