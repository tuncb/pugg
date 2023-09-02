#pragma once
#include <map>
#include <string>
#include <vector>

#include "Driver.h"
#include "Plugin.h"

namespace pugg
{
namespace detail
{

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

    auto server_iter = _servers.find(driver->server_name());

    if (server_iter == _servers.end())
      return false;

    if (server_iter->second.min_driver_version > driver->version())
      return false;

    server_iter->second.drivers[driver->name()] = std::unique_ptr<pugg::Driver>(driver);
    return true;
  }

  template <class DriverType> DriverType *get_driver(const std::string &server_name, const std::string &name)
  {
    auto server_iter = _servers.find(server_name);
    if (server_iter == _servers.end())
      return nullptr;

    auto driver_iter = server_iter->second.drivers.find(name);
    if (driver_iter == server_iter->second.drivers.end())
      return nullptr;
    return static_cast<DriverType *>(driver_iter->second.get());
  }

  template <class DriverType> std::vector<DriverType *> get_all_drivers(const std::string &server_name)
  {
    std::vector<DriverType *> drivers;

    auto server_iter = _servers.find(server_name);
    if (server_iter == _servers.end())
      return drivers;

    for (auto iter = server_iter->second.drivers.begin(); iter != server_iter->second.drivers.end(); ++iter)
    {
      drivers.push_back(static_cast<DriverType *>(iter->second.get()));
    }
    return drivers;
  }

  bool load_plugin(const std::string &filename)
  {
    typedef void fnRegisterPlugin(pugg::Kernel *);

    using namespace pugg::detail;
    auto dllHandle = DllHandle{loadDll(filename)};
    if (!dllHandle.isValid())
      return false;

    auto registerFunc = dllHandle.getFunction<fnRegisterPlugin>("register_pugg_plugin");
    if (registerFunc)
    {
      registerFunc(this);
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
};

} // namespace pugg
