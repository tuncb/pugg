#include <pugg/Kernel.h>

#include "Fish.h"
#include "Whale.h"

#ifdef _WIN32
#define EXPORTIT __declspec(dllexport)
#else
#define EXPORTIT __attribute__((visibility("default")))
#endif

extern "C" EXPORTIT bool register_pugg_plugin(pugg::Kernel *kernel)
{
  if (!kernel)
    return false;
  return kernel->add_driver(std::make_unique<FishDriver>()) &&
         kernel->add_driver(std::make_unique<WhaleDriver>());
}
