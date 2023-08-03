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

namespace detail
{


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
  dlclose(handle);
}

template <typename FuncType> auto getFunction(HandleType handle, const std::string &name) -> FuncType *
{
  return reinterpret_cast<FuncType *>(dlsym(handle, name.c_str()));
}

auto loadDll(const std::string &filename) -> HandleType
{
  return dlopen(filename.c_str(), RTLD_NOW);
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

  auto isValid() const -> bool
  {
    return _handle != nullptr;
  }

  template <typename FuncType> auto getFunction(const std::string &name) const -> FuncType *
  {
    return ::pugg::detail::getFunction<FuncType>(_handle, name);
  }

  auto handle() const -> HandleType
  {
    return _handle;
  }

private:
  HandleType _handle;
};

} // namespace detail
} // namespace pugg
