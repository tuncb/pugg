//          Copyright Tunc Bahcecioglu 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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
#include <stdexcept>

namespace pugg {

class Kernel;

namespace detail {

typedef void fnRegisterPlugin(pugg::Kernel*);

class DllLoader
{
public:
	~DllLoader()
	{
		this->free();
	}

	bool load(std::string filename)
	{
#ifdef WIN32
		_handle = LoadLibraryA(filename.c_str());
#else
		_handle = dlopen(filename.c_str(), RTLD_NOW);
		if(!_handle){
			throw std::runtime_error(dlerror());
		}
#endif
		return (_handle != nullptr);
	}


#ifdef WIN32
	bool load(std::wstring filename)
	{
		_handle = LoadLibraryW(filename.c_str());
		if(!_handle){
			auto errorCode = GetLastError();
			LPVOID lpMsgBuf;
			std::string msg;
			constexpr auto flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
			auto sz = FormatMessageA(
				flags,
				nullptr,
				errorCode,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				nullptr,
				0,
				nullptr
			);
			msg.reserve(sz);
			msg.resize(sz);
			FormatMessageA(
				flags,
				nullptr,
				errorCode,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				msg.data(),
				0,
				nullptr
			);
			throw std::runtime_error(msg);
		}
		return (_handle != nullptr);
	}
#endif

	fnRegisterPlugin* register_function()
	{
#ifdef WIN32
		return reinterpret_cast<fnRegisterPlugin*>(GetProcAddress(_handle, "register_pugg_plugin"));
#else
		return reinterpret_cast<fnRegisterPlugin*>(dlsym(_handle, "register_pugg_plugin"));
#endif
	}
	void free()
	{
#ifdef WIN32
		if (_handle) { FreeLibrary(_handle); }
#else
		if (_handle) { dlclose(_handle); }
#endif
	}
private:
#ifdef WIN32
	HMODULE _handle;
#else
	void* _handle;
#endif
};

class Plugin
{
public:
	Plugin() : _register_function(nullptr) {}

	bool load(const std::string& filename)
	{
		if (! _dll_loader.load(filename)) return false;
		_register_function = _dll_loader.register_function();

		if (_register_function) {
			return true;
		} else {
			_dll_loader.free();
			return false;
		}
	}

	#if WIN32
	bool load(const std::wstring& filename)
	{
		if (! _dll_loader.load(filename)) return false;
		_register_function = _dll_loader.register_function();

		if (_register_function) {
			return true;
		} else {
			_dll_loader.free();
			return false;
		}
	}
	#endif

	void register_plugin(pugg::Kernel* kernel)
	{
		_register_function(kernel);
	}
private:

	fnRegisterPlugin* _register_function;
	DllLoader _dll_loader;
};



}
}
