#include "Animal.h"
#include <pugg/Kernel.h>

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main()
{
  cout << "Zoo example" << endl;
  cout << "Zoo loads animals from plugins" << endl;
  cout << "Loading plugins..." << endl;

  pugg::Kernel kernel;
  kernel.add_server(Animal::server_name(), Animal::version);

#ifdef WIN32
#define DLL_PANTHALASSA_ANIMALS "PanthalassaAnimals.dll"
#define DLL_PANGEA_ANIMALS "PangeaAnimals.dll"
#else
#define DLL_PANTHALASSA_ANIMALS "libPanthalassaAnimals.so"
#define DLL_PANGEA_ANIMALS "libPangeaAnimals.so"
#endif

  kernel.load_plugin(DLL_PANTHALASSA_ANIMALS);
  kernel.load_plugin(DLL_PANGEA_ANIMALS);

  auto driver = kernel.get_driver<AnimalDriver>(Animal::server_name(), "DogDriver");
  cout << "driver = " << driver->name();
  vector<AnimalDriver *> drivers = kernel.get_all_drivers<AnimalDriver>(Animal::server_name());
  vector<std::unique_ptr<Animal>> animals;
  for (auto&& driver: drivers)
  {
    animals.push_back(std::unique_ptr<Animal>(driver->create()));
  }

  for (auto&& animal : animals)
  {
    cout << "Animal kind = " << animal->kind() << endl;
    cout << "Can Animal Swim = " << animal->can_swim() << endl;
  }

  cout << "Press ENTER to exit..." << endl;
  getchar();
}
