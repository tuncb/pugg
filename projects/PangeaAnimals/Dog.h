#pragma once

#include <Animal.h>
#include <pugg/Driver.h>

class Dog : public Animal
{
public:
  std::string kind()
  {
    return "Dog";
  }
  bool can_swim()
  {
    return true;
  }
};

class DogDriver : public AnimalDriver
{
public:
  DogDriver() : AnimalDriver("DogDriver", Dog::version)
  {
  }
  Animal *create()
  {
    return new Dog();
  }
};
