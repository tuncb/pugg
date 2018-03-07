#include <iostream>
#include <pugg/Kernel.h>

int main()
{
  pugg::Kernel kernel;
  kernel.load_plugin("donotexists.dll");
}