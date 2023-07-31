#pragma once
#include <map>
#include <string>
#include <vector>

#include "Driver.h"
#include "Plugin.h"

namespace pugg
{

class Kernel;

namespace detail
{

typedef void fnRegisterPlugin(pugg::Kernel *);

auto registerDll(pugg::Kernel *kernel, const DllHandle &dllHandle) -> bool
{
  if (!dllHandle.isValid())
    return false;

  auto func = dllHandle.getFunction<fnRegisterPlugin>("register_pugg_plugin");
  if (func == nullptr)
    return false;

  func(kernel);
  return true;
}

struct Server
{
  std::string name;
  int min_driver_version;
  std::map<std::string, std::unique_ptr<Driver>> drivers;
};
} // namespace detail

class Kernel
{
public:
  void add_server(std::string name, int min_driver_version)
  {
    _servers[name] = pugg::detail::Server{name, min_driver_version};
  }

  bool add_driver(pugg::Driver *driver)
  {
    if (!driver)
      return false;

    pugg::detail::Server *server = _get_server(driver->server_name());
    if (!server)
      return NULL;

    if (server->min_driver_version > driver->version())
      return false;

    server->drivers[driver->name()] = std::unique_ptr<pugg::Driver>(driver);
    return true;
  }

  template <class DriverType> DriverType *get_driver(const std::string &server_name, const std::string &name)
  {
    pugg::detail::Server *server = _get_server(server_name);
    if (!server)
      return NULL;

    std::map<std::string, pugg::Driver *>::iterator driver_iter = server->drivers().find(name);
    if (driver_iter == server->drivers().end())
      return NULL;
    return static_cast<DriverType *>(driver_iter->second);
  }

  template <class DriverType> std::vector<DriverType *> get_all_drivers(const std::string &server_name)
  {
    std::vector<DriverType *> drivers;

    pugg::detail::Server *server = _get_server(server_name);
    if (!server)
      return drivers;

    for (auto iter = server->drivers.begin(); iter != server->drivers.end(); ++iter)
    {
      drivers.push_back(static_cast<DriverType *>(iter->second.get()));
    }
    return drivers;
  }

  bool load_plugin(const std::string &filename)
  {
    using namespace pugg::detail;
    auto dllHandle = DllHandle{loadDll(filename)};
    ;
    if (registerDll(this, dllHandle))
    {
      _plugins.push_back(std::move(dllHandle));
      return true;
    }

    return false;
  }

  void clear_drivers()
  {
    for (auto iter = _servers.begin(); iter != _servers.end(); ++iter)
    {
      iter->second.drivers.clear();
    }
  }

  void clear()
  {
    _servers.clear();
    _plugins.clear();
  }

protected:
  std::map<std::string, pugg::detail::Server> _servers;
  std::vector<pugg::detail::DllHandle> _plugins;

  pugg::detail::Server *_get_server(const std::string &name)
  {
    auto server_iter = _servers.find(name);
    if (server_iter == _servers.end())
      return NULL;
    else
      return &server_iter->second;
  }
};

} // namespace pugg
