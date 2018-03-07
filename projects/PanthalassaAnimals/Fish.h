#pragma once
#include <Animal.h>

class Fish : public Animal
{
public:
    std::string kind() {return "Fish";}
    bool can_swim() {return true;}
};

class FishDriver : public AnimalDriver
{
public:
    FishDriver() : AnimalDriver("FishDriver", Fish::version) {}
    Animal* create() {return new Fish();}
};