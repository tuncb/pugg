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

#ifdef WIN32
using HandleType = HMODULE;
#else
using HandleType = void *;
#endif

namespace pugg
{

class Kernel;

namespace detail
{

typedef void fnRegisterPlugin(pugg::Kernel *);

struct DllHandle
{
  HandleType handle;
};

struct DllHandleDeleter
{
  void operator()(DllHandle *dllHandle)
  {
    if (dllHandle->handle)
    {
#ifdef WIN32
      FreeLibrary(dllHandle->handle);
#else
      dlclose(_handle);
#endif
    }
    delete dllHandle;
  }
};

fnRegisterPlugin *getRegisterFunc(const DllHandle &dllHandle)
{
#ifdef WIN32
  return reinterpret_cast<fnRegisterPlugin *>(GetProcAddress(dllHandle.handle, "register_pugg_plugin"));
#else
  return reinterpret_cast<fnRegisterPlugin *>(dlsym(dllHandle.handle, "register_pugg_plugin"));
#endif
}

auto loadDll(const std::string &filename) -> std::unique_ptr<DllHandle, DllHandleDeleter>
{
#ifdef WIN32
  return std::unique_ptr<DllHandle, DllHandleDeleter>(new DllHandle{LoadLibraryA(filename.c_str())});
#else
  return std::unique_ptr<DllHandle, DllHandleDeleter>(new DllHandle{dlopen(filename.c_str(), RTLD_NOW)});
#endif
}

class Plugin
{
public:
  Plugin() : _register_function(NULL)
  {
  }

  bool load(const std::string &filename)
  {
    auto dllHandle = loadDll(filename);
    if (!dllHandle->handle)
      return false;

    _register_function = getRegisterFunc(*dllHandle);
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
  std::unique_ptr<DllHandle, DllHandleDeleter> _dllHandle;
};

} // namespace detail
} // namespace pugg
