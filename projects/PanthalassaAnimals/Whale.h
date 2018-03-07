#pragma once
#include <Animal.h>

class Whale : public Animal
{
public:
    std::string kind() {return "Whale";}
    bool can_swim() {return true;}
};

class WhaleDriver : public AnimalDriver
{
public:
    WhaleDriver() : AnimalDriver("WhaleDriver", Whale::version) {}
    Animal* create() {return new Whale();}
};