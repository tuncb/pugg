//          Copyright Tunc Bahcecioglu 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <iostream>


#ifdef _WIN32
	#include <windows.h>
	#include <cwchar>
#endif

#ifdef WITH_STD_FILESYSTEM
#include <filesystem>
#else
#include <dirent.h> // some impls have broken impl of std::filesystem
#include <stdlib.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "Driver.h"
#include "Plugin.h"

namespace pugg {

class Kernel;

namespace detail {

template <class K, class V>
void delete_all_values(std::map<K,V>& map) {
	for (typename std::map<K,V>::iterator iter = map.begin(); iter != map.end(); ++iter) {
		delete iter->second;
	}
}
template <class V>
void delete_all_values(std::vector<V>& vec) {
	for (typename std::vector<V>::iterator iter = vec.begin(); iter != vec.end(); ++iter) {
		delete *iter;
	}
}

class Server
{
public:
	Server(std::string name, int min_driver_version) : _name(name), _min_driver_version(min_driver_version) {}
	~Server() {delete_all_values(_drivers);}

	std::string name() {return _name;}
	int min_driver_version() {return _min_driver_version;}
	std::map<std::string, Driver*>& drivers() {return _drivers;}

	void clear()
	{
		delete_all_values(_drivers);
	}
private:
	std::string _name;
	int _min_driver_version;
	std::map<std::string, Driver*> _drivers;
};
}

class Kernel
{
public:
	virtual ~Kernel()
	{
		pugg::detail::delete_all_values(_servers);
		pugg::detail::delete_all_values(_plugins);
	}

	void add_server(std::string name, int min_driver_version )
	{
		_servers[name] = new pugg::detail::Server(name, min_driver_version);
	}

	bool add_driver(pugg::Driver* driver)
	{
		if (! driver) return false;

		pugg::detail::Server* server = _get_server(driver->server_name());
		if (! server) return false;

		if (server->min_driver_version() > driver->version()) return false;

		server->drivers()[driver->name()] = driver;
		return true;
	}


	#ifdef WIN32
		static const constexpr wchar_t* platformDllExt = L".dll";
	#else
		static const constexpr char* platformDllExt = ".so";
	#endif

	void discover_backends(
	#ifdef WIN32
		std::wstring
	#else
		std::string
	#endif
	backendsDirPath
	){
		#ifndef WITH_STD_FILESYSTEM

		#ifdef WIN32
		auto openDirF = _wopendir;
		using DIR_T = _WDIR;
		using StrT = std::wstring;
		auto readDirFunc = _wreaddir;
		auto slenFunc = std::wcslen;
		auto &debugMsgSteam = std::wcerr;
		auto statF = wstat;
		auto closeDirF = _wclosedir;
		#else
		auto openDirF = opendir;
		using DIR_T = DIR;
		using StrT = std::string;
		auto readDirFunc = readdir;
		auto slenFunc = std::strlen;
		auto &debugMsgSteam = std::cerr;
		auto statF = stat;
		auto closeDirF = closedir;
		#endif
		DIR_T *dir = openDirF(backendsDirPath.data());

		#else
		std::filesystem::directory_iterator dir(backendsDirPath.data());
		#endif

		auto backendPath = backendsDirPath +
		#ifdef WIN32
		L"/"
		#else
		"/"
		#endif
		;
		auto backendsPrefixLen = backendPath.size();
		backendPath.reserve(PATH_MAX);
		if(dir) {
			#ifndef WITH_STD_FILESYSTEM
			StrT resolved;
			resolved.reserve(PATH_MAX);

			for(auto *ent = readDirFunc(dir);ent;ent = readDirFunc(dir)){
			#else
			for(std::filesystem::directory_entry &ent: dir){
			#endif
				#ifdef WITH_STD_FILESYSTEM
				try{
					auto filename = ent.path.c_str();
					auto backendPath = filesystem::canonical(ent.path);
					std::cerr << "Backend library candidate path: " << backendPath << std::endl;
				#else
				auto filename = ent->d_name;
				struct stat path_stat;
				resolved.resize(PATH_MAX);
				backendPath.resize(backendsPrefixLen);
				backendPath += filename;

				debugMsgSteam << "Backend library candidate path: " << backendPath << std::endl;

				if(
					#ifdef WIN32
					GetFullPathNameW(backendPath.data(), resolved.size(), resolved.data(), nullptr)
					#else
					realpath(backendPath.data(), const_cast<char *>(resolved.data()))
					#endif
				){
					auto len = slenFunc(resolved.data());
					resolved.resize(len);
				#endif
					debugMsgSteam << "Backend library candidate resolved path: " << resolved << std::endl;

					#ifdef WITH_STD_FILESYSTEM
					#else
					statF(resolved.data(), &path_stat);
					debugMsgSteam << ent->d_name << "(" << backendPath << ") -> " << resolved << " : " << path_stat.st_mode << std::endl;
					if(!S_ISREG(path_stat.st_mode)){
						debugMsgSteam << "Backend library is not a usual file: mode is " << path_stat.st_mode << std::endl;
						continue;
					}
					auto extensionPos = resolved.rfind(platformDllExt);
					auto expectedExtensionPos = len - (slenFunc(platformDllExt));
					if(extensionPos == expectedExtensionPos){
						this->load_plugin(resolved);
					}
					#endif

				#ifdef WITH_STD_FILESYSTEM
				}catch(std::exception &ex){
					auto err = ex.what();
				#else
				}else{
					#ifdef WIN32
					auto err = GetLastError();
					#else
					auto err = strerror(errno);
					#endif
				#endif
					debugMsgSteam << "Error resolving " << filename << " : ";
					debugMsgSteam << err << std::endl;
				}
			}
			closeDirF(dir);
		}
	}

	template <class DriverType>
	DriverType* get_driver(const std::string& server_name, const std::string& name)
	{
		pugg::detail::Server* server = _get_server(server_name);
		if (! server) return nullptr;

		std::map<std::string,pugg::Driver*>::iterator driver_iter = server->drivers().find(name);
		if (driver_iter == server->drivers().end()) return nullptr;
		return static_cast<DriverType*>(driver_iter->second);
	}

	template <class DriverType>
	std::vector<DriverType*> get_all_drivers(const std::string& server_name)
	{
		std::vector<DriverType*> drivers;

		pugg::detail::Server* server = _get_server(server_name);
		if (! server) return drivers;

		for (std::map<std::string, pugg::Driver*>::iterator iter = server->drivers().begin(); iter != server->drivers().end(); ++iter) {
			drivers.push_back(static_cast<DriverType*>(iter->second));
		}
		return drivers;
	}


	bool load_plugin(const std::string& filename)
	{
		pugg::detail::Plugin* plugin = new pugg::detail::Plugin();
		if (plugin->load(filename)) {
			plugin->register_plugin(this);
			_plugins.push_back(plugin);
			return true;
		} else {
			delete plugin;
			return false;
		}
	}

	#if WIN32
	bool load_plugin(const std::wstring& filename)
	{
		pugg::detail::Plugin* plugin = new pugg::detail::Plugin();
		if (plugin->load(filename)) {
			plugin->register_plugin(this);
			_plugins.push_back(plugin);
			return true;
		} else {
			delete plugin;
			return false;
		}
	}
	#endif

	void clear_drivers()
	{
		for (std::map<std::string,pugg::detail::Server*>::iterator iter = _servers.begin(); iter != _servers.end(); ++iter) {
			iter->second->clear();
		}
	}

	void clear()
	{
		pugg::detail::delete_all_values(_servers);
		pugg::detail::delete_all_values(_plugins);
	}

protected:
	std::map<std::string,pugg::detail::Server*> _servers;
	std::vector<pugg::detail::Plugin*> _plugins;

	pugg::detail::Server* _get_server(const std::string& name)
	{
		std::map<std::string,pugg::detail::Server*>::iterator server_iter = _servers.find(name);
		if (server_iter == _servers.end())
			return nullptr;
		else
			return server_iter->second;
	}
};

}





