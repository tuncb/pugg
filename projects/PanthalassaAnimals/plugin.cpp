#include <pugg/Kernel.h>

#include "Fish.h"
#include "Whale.h"

#ifdef _WIN32
#  define EXPORTIT __declspec( dllexport )
#else
#  define EXPORTIT
#endif

extern "C" EXPORTIT void register_pugg_plugin(pugg::Kernel* kernel)
{
	kernel->add_driver(new FishDriver());
	kernel->add_driver(new WhaleDriver());
}
