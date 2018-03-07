#pragma once
#include <pugg/Driver.h>
#include <string>

class Animal
{
public:
    Animal() {}
    virtual ~Animal() {}

    virtual std::string kind() = 0;
    virtual bool can_swim() = 0;

    static const int version = 1;
    static const std::string server_name() {return "AnimalServer";}
};

class AnimalDriver : public pugg::Driver
{
public:
    AnimalDriver(std::string name, int version) : pugg::Driver(Animal::server_name(),name,version) {}
    virtual Animal* create() = 0;
};
