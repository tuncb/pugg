#pragma once

#include <string>

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <memory>

namespace pugg
{

class Kernel;

namespace detail
{
typedef void fnRegisterPlugin(pugg::Kernel *);

#ifdef WIN32
using HandleType = HMODULE;

auto freeDll(HandleType handle)
{
  FreeLibrary(handle);
}

template <typename FuncType> auto getFunction(HandleType handle, const std::string &name) -> FuncType *
{
  return reinterpret_cast<FuncType *>(GetProcAddress(handle, name.c_str()));
}

auto loadDll(const std::string &filename) -> HandleType
{
  return LoadLibraryA(filename.c_str());
}

#else
using HandleType = void *;

auto freeDll(HandleType handle)
{
  dlclose(_handle);
}

template <typename FuncType> auto getFunction(HandleType handle, const std::string &name) -> FuncType *
{
  return reinterpret_cast<FuncType *>(dlsym(dllHandle.handle, name.c_str());
}

auto loadDll(const std::string &filename) -> HandleType
{
  return DllHandle{dlopen(filename.c_str(), RTLD_NOW)};
}

#endif

class DllHandle
{
public:
  DllHandle(HandleType handle) : _handle(handle)
  {
  }

  DllHandle(const DllHandle &) = delete;
  DllHandle &operator=(DllHandle const &) = delete;

  DllHandle(DllHandle &&other)
  {
    _handle = other._handle;
    other._handle = nullptr;
  }
  DllHandle &operator=(DllHandle &&other)
  {
    if (this != &other)
    {
      _handle = other._handle;
      other._handle = nullptr;
    }

    return *this;
  }

  ~DllHandle()
  {
    if (_handle)
      freeDll(_handle);
  }

  auto handle() -> HandleType
  {
    return _handle;
  }

private:
  HandleType _handle;
};

class Plugin
{
public:
  Plugin() : _register_function(nullptr), _dllHandle({nullptr})
  {
  }

  bool load(const std::string &filename)
  {
    auto dllHandle = DllHandle{loadDll(filename)};
    if (!dllHandle.handle())
      return false;

    _register_function = getFunction<fnRegisterPlugin>(_dllHandle.handle(), "register_pugg_plugin");
    if (!_register_function)
      return false;

    _dllHandle = std::move(dllHandle);
    return true;
  }

  void register_plugin(pugg::Kernel *kernel)
  {
    _register_function(kernel);
  }

private:
  fnRegisterPlugin *_register_function;
  DllHandle _dllHandle;
};

} // namespace detail
} // namespace pugg
