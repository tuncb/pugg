#pragma once
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "Driver.h"
#include "Plugin.h"

/// @file Kernel.h
/// @brief The pugg::Kernel class, the central registry that loads plugins and owns drivers.

namespace pugg
{
namespace detail
{

/// @brief A registered server interface and the drivers implementing it.
struct Server
{
  std::string name;                                   ///< Server interface name.
  int min_driver_version{0};                           ///< Minimum driver version accepted by this server.
  std::map<std::string, std::unique_ptr<Driver>> drivers; ///< Registered drivers, keyed by driver name.
};

} // namespace detail

/// @brief Outcome of Kernel::add_driver(), convertible to bool (true on success).
///
/// The reason field allows hosts to distinguish why a registration was
/// rejected, e.g. to report a protocol version mismatch to the user.
struct AddDriverResult
{
  /// @brief Why the registration succeeded or failed.
  enum class Reason
  {
    ok,               ///< The driver was registered.
    null_driver,      ///< A null driver pointer was passed.
    unknown_server,   ///< No server with the driver's server_name() is registered.
    version_mismatch, ///< The driver's version is below the server's minimum.
    duplicate_name    ///< A driver with the same name is already registered.
  };

  Reason reason{Reason::ok}; ///< Outcome of the registration.

  /// @brief True if the driver was registered.
  explicit operator bool() const { return reason == Reason::ok; }
};

/// @brief Central registry for server interfaces, plugins, and the drivers they provide.
///
/// A Kernel declares which server interfaces it accepts (add_server()), loads plugin
/// shared libraries (load_plugin()) which in turn register Driver instances
/// (add_driver()), and exposes typed lookup of those drivers (get_driver(),
/// get_all_drivers()).
///
/// @note Declaration order of the member variables matters: loaded plugin libraries
/// (_plugins) must outlive the registered servers/drivers (_servers), since a Driver's
/// vtable lives inside its owning DLL. clear() and the destructor both rely on this
/// order to tear things down safely.
class Kernel
{
public:
  /// @brief Register a server interface that plugins can provide drivers for.
  /// @param name Name of the server interface.
  /// @param min_driver_version Minimum driver version accepted for this server;
  /// drivers below this version are rejected by add_driver().
  void add_server(std::string name, int min_driver_version)
  {
    detail::Server server;
    server.name = name;
    server.min_driver_version = min_driver_version;
    _servers[std::move(name)] = std::move(server);
  }

  /// @brief Register a server interface, taking its name and version from the type.
  /// @tparam T Server descriptor type providing a static `server_name()` and a static
  /// `version` member.
  template <class T>
  void add_server()
  {
    add_server(T::server_name(), T::version);
  }

  /// @brief Register a driver with its server.
  ///
  /// Takes ownership of @p driver. Registration fails (and @p driver is destroyed) if
  /// no matching server is registered, if the driver's version is below the
  /// server's minimum required version, or if a driver with the same name is
  /// already registered with that server.
  /// @param driver Driver instance to register; must not be null.
  /// @return An AddDriverResult that converts to true if the driver was
  /// registered; on failure its reason field tells why and last_error() holds
  /// a human-readable message.
  [[nodiscard]] AddDriverResult add_driver(std::unique_ptr<Driver> driver)
  {
    using Reason = AddDriverResult::Reason;

    if (!driver)
    {
      _last_error = "cannot register a null driver";
      return {Reason::null_driver};
    }

    auto server_iter = _servers.find(driver->server_name());
    if (server_iter == _servers.end())
    {
      _last_error = "no server named '" + driver->server_name() + "' is registered";
      return {Reason::unknown_server};
    }

    if (server_iter->second.min_driver_version > driver->version())
    {
      _last_error = "driver '" + driver->name() + "' has version " +
                    std::to_string(driver->version()) + ", but server '" +
                    server_iter->second.name + "' requires at least " +
                    std::to_string(server_iter->second.min_driver_version);
      return {Reason::version_mismatch};
    }

    auto [driver_iter, inserted] =
        server_iter->second.drivers.try_emplace(driver->name(), std::move(driver));
    if (!inserted)
    {
      // Replacing would destroy the old driver while hosts may still hold
      // pointers returned by get_driver()/get_all_drivers().
      _last_error = "a driver named '" + driver_iter->first +
                    "' is already registered with server '" +
                    server_iter->second.name + "'";
      return {Reason::duplicate_name};
    }
    return {};
  }

  /// @brief Raw-pointer overload of add_driver(), kept for backwards compatibility.
  ///
  /// Takes ownership of @p driver, deleting it if registration fails.
  /// @param driver Driver instance to register; must not be null.
  /// @return An AddDriverResult that converts to true if the driver was registered.
  [[nodiscard]] AddDriverResult add_driver(Driver *driver)
  {
    return add_driver(std::unique_ptr<Driver>(driver));
  }

  /// @brief Look up a registered driver by server and driver name, cast to a concrete type.
  /// @tparam DriverType Concrete Driver subclass to cast the result to.
  /// @param server_name Name of the server interface the driver implements.
  /// @param name Name of the driver to look up.
  /// @return Pointer to the driver if found and the dynamic_cast succeeds, nullptr otherwise.
  /// The returned pointer is owned by the Kernel and remains valid until clear(),
  /// clear_drivers(), or the Kernel's destruction.
  template <class DriverType>
  [[nodiscard]] DriverType *get_driver(const std::string &server_name, const std::string &name)
  {
    auto server_iter = _servers.find(server_name);
    if (server_iter == _servers.end())
      return nullptr;

    auto driver_iter = server_iter->second.drivers.find(name);
    if (driver_iter == server_iter->second.drivers.end())
      return nullptr;

    return dynamic_cast<DriverType *>(driver_iter->second.get());
  }

  /// @brief Look up all registered drivers for a server that match a concrete type.
  /// @tparam DriverType Concrete Driver subclass to filter and cast results to.
  /// @param server_name Name of the server interface to look up drivers for.
  /// @return Drivers registered for @p server_name whose dynamic_cast to DriverType
  /// succeeds; empty if the server is unknown or has no matching drivers. Returned
  /// pointers are owned by the Kernel and remain valid until clear(), clear_drivers(),
  /// or the Kernel's destruction.
  template <class DriverType>
  [[nodiscard]] std::vector<DriverType *> get_all_drivers(const std::string &server_name)
  {
    std::vector<DriverType *> drivers;

    auto server_iter = _servers.find(server_name);
    if (server_iter == _servers.end())
      return drivers;

    for (auto &[key, driver_ptr] : server_iter->second.drivers)
    {
      if (auto *typed = dynamic_cast<DriverType *>(driver_ptr.get()))
        drivers.push_back(typed);
    }
    return drivers;
  }

  /// @brief Load a plugin shared library and let it register its drivers with this Kernel.
  ///
  /// Loads @p filename and calls its exported `register_pugg_plugin` function, passing
  /// this Kernel so the plugin can call add_driver(). The registration function must
  /// have the signature `bool register_pugg_plugin(pugg::Kernel *)`, must not throw,
  /// and must return true only if all its drivers were registered. If it returns false
  /// (or throws, which is tolerated for legacy plugins), the library is unloaded and
  /// load_plugin() fails with last_error() explaining why.
  /// @param path Path to the plugin shared library (.so/.dylib/.dll).
  /// @return true if the library was loaded and successfully registered its drivers,
  /// false otherwise. On success the library is kept open for the lifetime of this
  /// Kernel (or until clear()).
  [[nodiscard]] bool load_plugin(const std::filesystem::path &path)
  {
    using fnRegisterPlugin = bool(pugg::Kernel *);

    _last_error.clear();

    const std::string filename = path.string();
    auto dllHandle = detail::DllHandle{detail::loadDll(filename)};
    if (!dllHandle.isValid())
    {
      _last_error = "cannot load library '" + filename + "'";
      return false;
    }

    auto *registerFunc = dllHandle.getFunction<fnRegisterPlugin>("register_pugg_plugin");
    if (!registerFunc)
    {
      _last_error = "'" + filename + "' does not export register_pugg_plugin";
      return false;
    }

    // Snapshot the registered drivers, so that a failed registration can be
    // rolled back before the library is unloaded.
    std::map<std::string, std::set<std::string>> preexisting;
    for (auto &[server_name, server] : _servers)
      for (auto &[driver_name, driver] : server.drivers)
        preexisting[server_name].insert(driver_name);

    bool registered = false;
    try
    {
      registered = registerFunc(this);
    }
    catch (...)
    {
      // Legacy plugins may throw on registration failure. The exception must
      // be fully consumed here, while the library is still loaded: letting it
      // propagate would unload the library during stack unwinding, destroying
      // the exception object's own code and type_info.
      registered = false;
    }

    if (!registered)
    {
      // Destroy any drivers this plugin managed to register before failing:
      // they must not outlive their code, which is about to be unloaded.
      for (auto &[server_name, server] : _servers)
      {
        auto known = preexisting.find(server_name);
        for (auto it = server.drivers.begin(); it != server.drivers.end();)
        {
          if (known != preexisting.end() && known->second.count(it->first) > 0)
            ++it;
          else
            it = server.drivers.erase(it);
        }
      }
      if (_last_error.empty())
        _last_error = "plugin registration failed for '" + filename + "'";
      return false; // dllHandle goes out of scope and the library is unloaded
    }

    _plugins.push_back(std::move(dllHandle));
    return true;
  }

  /// @brief Human-readable description of the most recent add_driver() or
  /// load_plugin() failure. Empty if the last operation succeeded.
  [[nodiscard]] const std::string &last_error() const { return _last_error; }

  /// @brief Destroy all registered drivers, keeping registered servers and loaded plugins.
  void clear_drivers()
  {
    for (auto &[name, server] : _servers)
      server.drivers.clear();
  }

  /// @brief Destroy all registered servers/drivers and unload all loaded plugins.
  void clear()
  {
    // Destroy Driver objects before unloading DLLs — member destruction
    // order (reverse of declaration) ensures _servers goes before _plugins,
    // but clear() must replicate that same sequence explicitly.
    _servers.clear();
    _plugins.clear();
  }

protected:
  // Declaration order matters: _plugins must outlive _servers so that Driver
  // vtables (which live in the DLLs) remain valid during Server/Driver destruction.
  std::vector<detail::DllHandle> _plugins;
  std::map<std::string, detail::Server> _servers;
  std::string _last_error;
};

} // namespace pugg
