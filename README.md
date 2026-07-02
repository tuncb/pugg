# Pugg

A lightweight, header-only C++17 framework for loading classes from shared libraries (.so/.dll) at runtime using an object-oriented plugin architecture.

[![CI](https://github.com/pbosetti/pugg/actions/workflows/cmake.yml/badge.svg)](https://github.com/pbosetti/pugg/actions/workflows/cmake.yml) ![C++17 badge](https://img.shields.io/badge/C%2B%2B-17-blue.svg) [![Apache 2.0](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)

> This is a fork of <tuncb/pugg> with some bug fixes, improvements and modernization.

## Credits

* [nuclex blog](http://blog.nuclex-games.com/tutorials/cxx/plugin-architecture/) for the original idea and base implementation.
* Sebastian (hackspider) for bug fixes.
* Szymon Janikowski for tutorial fixes.
* Alex Elzenaar for code improvements and Linux support.

## Features

* Runtime loading of shared libraries and automatic driver registration
* Version checking — drivers older than the required minimum are rejected
* Header-only — no compilation step for the framework itself
* Type-safe driver retrieval via `dynamic_cast`
* RAII ownership of loaded libraries and registered drivers
* Platform independent (Linux, macOS, Windows)

## Requirements

* C++17 compiler (GCC ≥ 7, Clang ≥ 5, MSVC ≥ 2017)
* CMake ≥ 3.14

## Installation

Copy the headers from `src/pugg/include` into your project, or integrate via CMake FetchContent:

```cmake
include(FetchContent)
FetchContent_Declare(
  pugg
  GIT_REPOSITORY https://github.com/pbosetti/pugg.git
  GIT_TAG        1.2.0
  GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(pugg)

target_link_libraries(MyProject PRIVATE pugg)
```

On Linux you must also link the dynamic loader:

```cmake
target_link_libraries(MyProject PRIVATE ${CMAKE_DL_LIBS})
```

## Tutorial

### 1. Define the interface

Declare a pure-virtual interface for the objects your plugins will provide. Add a `version` constant and a `server_name()` so Pugg can associate drivers with the right server.

```cpp
class Animal
{
public:
  virtual ~Animal() = default;
  virtual std::string kind() const = 0;
  virtual bool can_swim() const = 0;

  static const int version = 1;
  static std::string server_name() { return "AnimalServer"; }
};
```

### 2. Define the driver base class

A driver is a factory that creates instances of your interface. Derive from `pugg::Driver`:

```cpp
#include <pugg/Driver.h>

class AnimalDriver : public pugg::Driver
{
public:
  AnimalDriver(std::string name, int version)
      : pugg::Driver(Animal::server_name(), std::move(name), version) {}

  virtual Animal *create() = 0;
};
```

### 3. Implement a plugin

In a separate shared-library project, implement the interface and export the registration entry point:

```cpp
// Cat.h
class Cat : public Animal
{
public:
  std::string kind() const override { return "Cat"; }
  bool can_swim() const override { return false; }
};

class CatDriver : public AnimalDriver
{
public:
  CatDriver() : AnimalDriver("CatDriver", Animal::version) {}
  Animal *create() override { return new Cat(); }
};
```

```cpp
// plugin.cpp
#include <pugg/Kernel.h>
#include "Cat.h"

#ifdef _WIN32
#  define EXPORTIT __declspec(dllexport)
#else
#  define EXPORTIT
#endif

extern "C" EXPORTIT bool register_pugg_plugin(pugg::Kernel *kernel)
{
  return static_cast<bool>(kernel->add_driver(std::make_unique<CatDriver>()));
}
```

`register_pugg_plugin` is the fixed entry point Pugg looks for when loading a shared library. It must have the signature `bool register_pugg_plugin(pugg::Kernel *)` and return `true` only if all of its drivers were registered — `load_plugin()` treats a `false` return (or a thrown exception, tolerated for legacy plugins) as failure, rolls back any drivers the plugin did manage to register, and unloads the library. Use `std::make_unique` to transfer ownership cleanly; a raw `new` pointer is also accepted for backwards compatibility.

### 4. Load plugins in the host application

```cpp
#include <pugg/Kernel.h>
#include <Animal.h>

#include <iostream>
#include <memory>
#include <vector>

int main()
{
  pugg::Kernel kernel;

  // Register the server (template overload reads version from the class)
  kernel.add_server<Animal>();

  // Load plugins — returns false if the file is missing, has no entry point,
  // or its register_pugg_plugin() reported failure; kernel.last_error() explains why
  if (!kernel.load_plugin("libMyAnimals.so"))
  {
    std::cerr << kernel.last_error() << "\n";
    return 1;
  }

  // Retrieve a specific driver by name
  if (auto *driver = kernel.get_driver<AnimalDriver>(Animal::server_name(), "CatDriver"))
  {
    std::unique_ptr<Animal> cat{driver->create()};
    std::cout << cat->kind() << "\n";
  }

  // Or iterate over all registered drivers
  for (auto *driver : kernel.get_all_drivers<AnimalDriver>(Animal::server_name()))
  {
    std::unique_ptr<Animal> animal{driver->create()};
    std::cout << animal->kind()
              << " (swims: " << std::boolalpha << animal->can_swim() << ")\n";
  }
}
```

## API Reference

### `pugg::Driver`

Base class for all plugin drivers.

```cpp
namespace pugg {
class Driver
{
public:
  Driver(std::string server_name, std::string name, int version);
  virtual ~Driver() = default;

  const std::string &server_name() const;
  const std::string &name() const;
  int version() const;
};
}
```

### `pugg::AddDriverResult`

Outcome of `Kernel::add_driver()`, returned by value and convertible to `bool` (`true` on success).

```cpp
namespace pugg {
struct AddDriverResult
{
  enum class Reason
  {
    ok,               // The driver was registered.
    null_driver,      // A null driver pointer was passed.
    unknown_server,   // No server with the driver's server_name() is registered.
    version_mismatch, // The driver's version is below the server's minimum.
    duplicate_name    // A driver with the same name is already registered.
  };

  Reason reason{Reason::ok};

  explicit operator bool() const;
};
}
```

The `reason` field lets a host distinguish *why* registration was rejected — e.g. to report a version mismatch to the user — and `Kernel::last_error()` returns a human-readable message for the same failure.

### `pugg::Kernel`

Manages plugin loading, server registration, and driver lifecycle.

```cpp
namespace pugg {
class Kernel
{
public:
  // Register a server by name and minimum required driver version.
  void add_server(std::string name, int min_driver_version);

  // Template overload: reads server_name() and version from class T.
  template <class T>
  void add_server();

  // Register a driver. Takes ownership; the driver is destroyed if registration
  // fails because the server is unregistered, the driver's version is too old,
  // or a driver with the same name is already registered with that server.
  // Both unique_ptr and raw pointer (for backwards compatibility) are accepted.
  [[nodiscard]] AddDriverResult add_driver(std::unique_ptr<Driver> driver);
  [[nodiscard]] AddDriverResult add_driver(Driver *driver);  // wraps in unique_ptr

  // Load a shared library and call its register_pugg_plugin() entry point, which
  // must have signature `bool register_pugg_plugin(pugg::Kernel *)`. Returns false
  // if the file cannot be opened, the entry point is missing, or it returns false
  // (or throws) — in which case any drivers it registered are rolled back and the
  // library is unloaded. On success the library stays loaded until clear().
  [[nodiscard]] bool load_plugin(const std::filesystem::path &path);

  // Retrieve a driver by server name and driver name.
  // Returns nullptr if not found or if DriverType does not match the actual type.
  template <class DriverType>
  [[nodiscard]] DriverType *get_driver(const std::string &server_name,
                                       const std::string &name);

  // Retrieve all drivers registered under a server.
  // Drivers that do not dynamic_cast to DriverType are silently skipped.
  template <class DriverType>
  [[nodiscard]] std::vector<DriverType *> get_all_drivers(const std::string &server_name);

  // Human-readable description of the most recent add_driver() or load_plugin()
  // failure. Empty if the last operation succeeded.
  [[nodiscard]] const std::string &last_error() const;

  // Remove all drivers, keeping server registrations intact.
  void clear_drivers();

  // Remove all drivers, servers, and unload all plugins.
  void clear();
};
}
```

#### Ownership and lifetime

`Kernel` owns the `Driver` objects (via `unique_ptr`) and the loaded shared libraries (via RAII `DllHandle`). Destruction order is guaranteed: drivers are destroyed before the libraries that contain their vtables are unloaded.

#### Error handling

`add_driver()` rejects a duplicate driver name instead of silently replacing the existing entry — overwriting it would destroy a driver that a host may already hold a pointer to via `get_driver()`/`get_all_drivers()`. `load_plugin()` snapshots the drivers registered before calling into the plugin, so if the plugin's `register_pugg_plugin()` returns `false` or throws, only the drivers *it* added are rolled back before the library is unloaded — pre-existing registrations from other plugins are untouched. Call `last_error()` after a failed `add_driver()` or `load_plugin()` for a human-readable explanation.

#### Type safety

`get_driver` and `get_all_drivers` use `dynamic_cast` internally. Passing the wrong `DriverType` returns `nullptr` rather than invoking undefined behaviour.

## Building the examples and running tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Tests require [Catch2 v3](https://github.com/catchorg/Catch2). On Ubuntu/Debian: `sudo apt install catch2`. On macOS: `brew install catch2`. On Windows the CMakeLists falls back to FetchContent automatically.

## History

### 1.2.0
* `add_driver` now returns `AddDriverResult` (convertible to `bool`) instead of `bool`, distinguishing null-driver, unknown-server, version-mismatch, and duplicate-name failures
* `add_driver` rejects a duplicate driver name instead of silently replacing (and destroying) an already-registered driver
* `register_pugg_plugin` must now return `bool` (`true` only if all drivers registered); `load_plugin()` rolls back any drivers a failing plugin did register and unloads the library, and tolerates exceptions thrown by legacy plugins
* `load_plugin` takes a `std::filesystem::path` instead of `std::string`
* Added `Kernel::last_error()` for a human-readable explanation of the most recent `add_driver()`/`load_plugin()` failure

### 1.0.4
* Modernized to C++17 minimum (`target_compile_features` propagates the requirement to consumers)
* `add_driver` now takes `std::unique_ptr<Driver>` — no memory leak on registration failure; raw-pointer overload retained for backwards compatibility
* `get_driver` / `get_all_drivers` use `dynamic_cast` — wrong `DriverType` safely returns `nullptr` instead of undefined behaviour
* `Driver` string getters return `const std::string &` instead of by value
* `DllHandle` move-assignment now frees the existing handle before adopting a new one
* `[[nodiscard]]` on all functions whose return value must not be silently discarded
* Catch2 unit and integration tests (26 test cases)
* GitHub Actions CI matrix: Ubuntu / macOS / Windows × Debug / Release

### 1.0.3
* Fix `_plugins` and `_servers` declaration order to guarantee correct destruction sequence

### 1.0.2
* Added `Kernel::add_server<T>()` template overload

### 1.0.1
* Internal code improvements
* Removed Conan support

### 1.0.0
* Added `clear()` and `clear_drivers()` methods
* 1.0.0 release

### 0.60
* Linux support, bug fixes

### 0.55
* Switched to CMake build system

### 0.51
* Removed unnecessary try/catch in plugin loader

### 0.5
* New documentation, Boost license, name convention changes, removed wstring support

### 0.41
* Fixed memory leak when loading a DLL without the `register_pugg_plugin` symbol

### 0.4
* `std::wstring` filename support

### 0.3
* Name convention changes, documentation improvements

### 0.2
* Per-driver version control, removed per-server version templating
