#include <pugg/Kernel.h>

#include "Cat.h"
#include "Dog.h"

#ifdef _WIN32
#define EXPORTIT __declspec(dllexport)
#else
#define EXPORTIT __attribute__((visibility("default")))
#endif

extern "C" EXPORTIT bool register_pugg_plugin(pugg::Kernel *kernel)
{
  if (!kernel)
    return false;
  return kernel->add_driver(std::make_unique<CatDriver>()) &&
         kernel->add_driver(std::make_unique<DogDriver>());
}
